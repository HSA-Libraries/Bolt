/***************************************************************************
*   Copyright 2012 - 2013 Advanced Micro Devices, Inc.                                     
*                                                                                    
*   Licensed under the Apache License, Version 2.0 (the "License");   
*   you may not use this file except in compliance with the License.                 
*   You may obtain a copy of the License at                                          
*                                                                                    
*       http://www.apache.org/licenses/LICENSE-2.0                      
*                                                                                    
*   Unless required by applicable law or agreed to in writing, software              
*   distributed under the License is distributed on an "AS IS" BASIS,              
*   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.         
*   See the License for the specific language governing permissions and              
*   limitations under the License.                                                   
***************************************************************************/
// #pragma OPENCL EXTENSION cl_amd_printf : enable

/******************************************************************************
 *  Kernel 0
 *****************************************************************************/

//__attribute__((reqd_work_group_size(KERNEL0WORKGROUPSIZE,1,1)))
template< typename iValueType, typename iIterType, typename initType, typename oValueType,
        typename UnaryFunction, typename BinaryFunction >
__kernel void perBlockTransformScan(
                global iValueType* input_ptr,
                iIterType input_iter,
                initType identity,
                const uint vecSize,
                local oValueType* lds,
                global UnaryFunction* unaryOp,
                global BinaryFunction* binaryOp,
                global oValueType* scanBuffer,
                global oValueType* scanBuffer1,
                int exclusive) // do exclusive scan ?
{
    // 2 thread per element
    size_t gloId = get_global_id( 0 );
    size_t groId = get_group_id( 0 );
    size_t locId = get_local_id( 0 );
    size_t wgSize = get_local_size( 0 )  ;

    wgSize *=2;
    input_iter.init( input_ptr );
    size_t offset = 1;

    oValueType val;

   // load input into shared memory
    if (exclusive)
    {
       if (gloId > 0 && groId*wgSize+locId-1 < vecSize){
          iValueType inVal = input_iter[groId*wgSize+locId-1];
          val = (oValueType) (*unaryOp)(inVal);
          lds[locId] = val;
       }
       else{ 
          val = identity;
          lds[locId] = val;
       }
       if(groId*wgSize +locId+ (wgSize/2) -1 < vecSize){
          iValueType inVal = input_iter[groId*wgSize +locId+ (wgSize/2) -1];
          val = (oValueType) (*unaryOp)(inVal);
          lds[locId+(wgSize/2)] = val;
       }
     
    }
    else
    {
     if(groId*wgSize+locId < vecSize){
          iValueType inVal = input_iter[groId*wgSize+locId];
          val = (oValueType) (*unaryOp)(inVal);
          lds[locId] = val;
     }
     if(groId*wgSize +locId+ (wgSize/2) < vecSize){
        iValueType inVal = input_iter[ groId*wgSize +locId+ (wgSize/2)];
        val = (oValueType) (*unaryOp)(inVal);
        lds[locId+(wgSize/2)] = val;
     }
    }
  
    for (size_t start = wgSize>>1; start > 0; start >>= 1) 
    {
       barrier( CLK_LOCAL_MEM_FENCE );
       if (locId < start)
       {
          size_t temp1 = offset*(2*locId+1)-1;
          size_t temp2 = offset*(2*locId+2)-1;
          oValueType y = lds[temp2];
          oValueType y1 =lds[temp1];
          lds[temp2] = (*binaryOp)(y, y1);
       }
       offset *= 2;
    }
    barrier( CLK_LOCAL_MEM_FENCE );
    if (locId == 0)
    {
        scanBuffer[ groId ] = lds[wgSize -1];
        scanBuffer1[ groId ] = lds[wgSize/2 -1];
    }
}


/******************************************************************************
 *  Kernel 1
 *****************************************************************************/
//__attribute__((reqd_work_group_size(KERNEL1WORKGROUPSIZE,1,1)))
template< typename Type, typename BinaryFunction >
__kernel void intraBlockInclusiveScan(
                global Type* postSumArray,
                global Type* preSumArray, 
                const uint vecSize,
                local Type* lds,
                const uint workPerThread,
                global BinaryFunction* binaryOp
                )
{
    size_t groId = get_group_id( 0 );
    size_t gloId = get_global_id( 0 );
    size_t locId = get_local_id( 0 );
    size_t wgSize = get_local_size( 0 );
    uint mapId  = gloId * workPerThread;

    // do offset of zero manually
    uint offset;
    Type workSum;
    if (mapId < vecSize)
    {
        // accumulate zeroth value manually
        offset = 0;
        workSum = preSumArray[mapId+offset];
        postSumArray[ mapId + offset ] = workSum;

        //  Serial accumulation
        for( offset = offset+1; offset < workPerThread; offset += 1 )
        {
            if (mapId+offset<vecSize)
            {
                Type y = preSumArray[mapId+offset];
                workSum = (*binaryOp)( workSum, y );
                postSumArray[ mapId + offset ] = workSum;
            }
        }
    }
    barrier( CLK_LOCAL_MEM_FENCE );
    Type scanSum = workSum;
    lds[ locId ] = workSum;
    offset = 1;
    // load LDS with register sums
    // scan in lds
    for( offset = offset*1; offset < wgSize; offset *= 2 )
    {
        barrier( CLK_LOCAL_MEM_FENCE );
        if (mapId < vecSize)
        {
            if (locId >= offset)
            {
                Type y = lds[ locId - offset ];
                scanSum = (*binaryOp)( scanSum, y );
            }
        }
        barrier( CLK_LOCAL_MEM_FENCE );
        lds[ locId ] = scanSum;
    } // for offset
    barrier( CLK_LOCAL_MEM_FENCE );
    
    // write final scan from pre-scan and lds scan
    for( offset = 0; offset < workPerThread; offset += 1 )
    {
        barrier( CLK_GLOBAL_MEM_FENCE );

        if (mapId < vecSize && locId > 0)
        {
            Type y = postSumArray[ mapId + offset ];
            Type y2 = lds[locId-1];
            y = (*binaryOp)( y, y2 );
            postSumArray[ mapId + offset ] = y;
        } // thread in bounds
    } // for 
} // end kernel


/******************************************************************************
 *  Kernel 2
 *****************************************************************************/
//__attribute__((reqd_work_group_size(KERNEL2WORKGROUPSIZE,1,1)))
template< typename iValueType, typename iIterType, typename oValueType, typename oIterType, typename UnaryFunction, 
           typename initType, typename BinaryFunction >
__kernel void perBlockAddition( 
                global oValueType* output_ptr,
                oIterType output_iter,
                global iValueType* input_ptr,
                iIterType input_iter,
                global oValueType* postSumArray,
                global oValueType* preSumArray1,
                local oValueType* lds,
                const uint vecSize,
                global UnaryFunction* unaryOp,
                global BinaryFunction* binaryOp,
                int exclusive,
                initType identity)
{
   // 1 thread per element
    size_t gloId = get_global_id( 0 );
    size_t groId = get_group_id( 0 );
    size_t locId = get_local_id( 0 );
    size_t wgSize = get_local_size( 0 );
    output_iter.init( output_ptr );
    input_iter.init( input_ptr );

  // if exclusive, load gloId=0 w/ identity, and all others shifted-1
    oValueType val;
    if (gloId < vecSize){
       if (exclusive)
       {
          if (gloId > 0)
          { // thread>0
              iValueType inVal = input_iter[gloId-1];
              val = (oValueType) (*unaryOp)(inVal);
              lds[ locId ] = val;
          }
          else
          { // thread=0
              val = identity;
              lds[ locId ] = val;
          }
       }
       else
       {
          iValueType inVal = input_iter[gloId];
          val = (oValueType) (*unaryOp)(inVal);
          lds[ locId ] = val;
       }
    }
  
    //  Computes a scan within a workgroup
    oValueType sum = lds[ locId ];

    for( size_t offset = 1; offset < wgSize; offset *= 2 )
    {
        barrier( CLK_LOCAL_MEM_FENCE );
        if (locId >= offset)
        {
            oValueType y = lds[ locId - offset ];
            sum = (*binaryOp)( sum, y );
        }
        barrier( CLK_LOCAL_MEM_FENCE );
        lds[ locId ] = sum;
    }
    barrier( CLK_LOCAL_MEM_FENCE );
  //  Abort threads that are passed the end of the input vector
    if (gloId >= vecSize) return; 

    oValueType scanResult = sum;
    oValueType postBlockSum, newResult;
    // accumulate prefix
    oValueType y, y1;
    if(groId > 0) {
        if(groId % 2 == 0)
            postBlockSum = postSumArray[ groId/2 -1 ];
        else if(groId == 1)
            postBlockSum = preSumArray1[0];
        else {
             y = postSumArray[ groId/2 -1 ];
             y1 = preSumArray1[groId/2];
             postBlockSum = (*binaryOp)(y, y1);
        }
        newResult = (*binaryOp)( scanResult, postBlockSum );
    }
    else {
      newResult = scanResult;
    }
    output_iter[ gloId ] = newResult;
}
