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
#pragma OPENCL EXTENSION cl_amd_printf : enable
//#define USE_AMD_HSA 1

#if USE_AMD_HSA

/******************************************************************************
 *  HSA Kernel
 *****************************************************************************/
template< typename iType, typename oType, typename initType, typename BinaryFunction >
kernel void HSA_Scan(
    global oType    *output,
    global iType    *input,
    initType        init,
    const uint      numElements,
    const uint      numIterations,
    local oType     *lds,
    global BinaryFunction* binaryOp,
    global oType    *intermediateScanArray,
    global int      *dev2host,
    global int      *host2dev,
    int             exclusive)
{
    size_t gloId = get_global_id( 0 );
    size_t groId = get_group_id( 0 );
    size_t locId = get_local_id( 0 );
    size_t wgSize = get_local_size( 0 );

    // report P1 completion
    intermediateScanArray[ groId ] = input[ groId ];
    dev2host[ groId ] = 1;



    // wait for P2 completion
    for (size_t i = 0; i < 10000; i++ )
    {
        mem_fence(CLK_GLOBAL_MEM_FENCE);
        //printf("DEV: interScan[%i]=%i ( %i, %i )", groId, intermediateScanArray[groId], dev2host[groId], host2dev[groId]);
        if ( host2dev[ groId] == 2 )
        { // host reported P2 completion
            // report P3 completion
            dev2host[ groId ] = 3;
            break;
        }
    }
}






/******************************************************************************
 *  Not Using HSA
 *****************************************************************************/
#else

#define NUM_ITER 16
#define MIN(X,Y) X<Y?X:Y;
#define MAX(X,Y) X>Y?X:Y;
/******************************************************************************
 *  Kernel 2
 *****************************************************************************/
template< typename iPtrType, typename iIterType, typename BinaryFunction >
kernel void perBlockAddition( 
                global iPtrType* output_ptr,
                iIterType    output_iter, 
                global iPtrType* postSumArray_ptr,
                const uint vecSize,
    global BinaryFunction* binaryOp )
{
    // BinaryFunction bf = *binaryOp;
    
// 1 thread per element
#if 1
    size_t gloId = get_global_id( 0 );
    size_t groId = get_group_id( 0 );
    size_t locId = get_local_id( 0 );

    //  Abort threads that are passed the end of the input vector
    if( gloId >= vecSize )
        return;
        
    output_iter.init( output_ptr );

    iPtrType scanResult = output_iter[ gloId ];

    // accumulate prefix
    if (groId > 0)
    {
        iPtrType postBlockSum = postSumArray_ptr[ groId-1 ];
        iPtrType newResult = (*binaryOp)( scanResult, postBlockSum );
        output_iter[ gloId ] = newResult;
    }
#endif

// section increments
#if 0
    size_t wgSize  = get_local_size( 0 );
    size_t wgIdx   = get_group_id( 0 );
    size_t locId   = get_local_id( 0 );
    size_t secSize = wgSize / NUM_ITER; // threads per section (256 element)
    
    size_t secIdx  = NUM_ITER*wgIdx+locId/secSize;
    size_t elemIdx = secIdx*wgSize+locId%secSize;
    Type postBlockSum;
    if (secIdx==0) return;
    postBlockSum = postSumArray[ secIdx-1 ];
    
    size_t maxElemIdx = MIN( vecSize, elemIdx+secSize*NUM_ITER );
    for ( ; elemIdx<maxElemIdx; elemIdx+=secSize)
    {
        Type scanResult   = output[ elemIdx ];
        Type newResult    = (*binaryOp)( scanResult, postBlockSum );
        output[ elemIdx ] = newResult;
    }

#endif

// work-group increments
#if 0
    size_t wgSize  = get_local_size( 0 );
    size_t wgIdx   = get_group_id( 0 );
    size_t locId   = get_local_id( 0 );
    size_t secSize = wgSize;
    
    size_t secIdx  = NUM_ITER*wgIdx; // to be incremented
    size_t elemIdx = secIdx*secSize+locId;
    //Type postBlockSum;
    //if (secIdx==0) return;
    
    size_t maxSecIdx = secIdx+NUM_ITER;
    size_t maxElemIdx = MIN( vecSize, elemIdx+secSize*NUM_ITER);
    // secIdx = MAX( 1, secIdx);
    if (wgIdx==0) // skip
    {
        secIdx++;
        elemIdx+=secSize;
    }
    for ( ; elemIdx<maxElemIdx; secIdx++, elemIdx+=secSize)
    {
        Type postBlockSum = postSumArray[ secIdx-1 ];
        Type scanResult   = output[ elemIdx ];
        Type newResult    = (*binaryOp)( scanResult, postBlockSum );
        output[ elemIdx ] = newResult;
    }
#endif

// threads increment
#if 0

    size_t wgSize  = get_local_size( 0 );
    size_t wgIdx   = get_group_id( 0 );
    size_t locId   = get_local_id( 0 );
    size_t threadsPerSect = wgSize / NUM_ITER;
    
    size_t secIdx  = NUM_ITER*wgIdx;
    size_t elemIdx = secIdx*wgSize+locId*NUM_ITER;

    if (elemIdx < vecSize && secIdx > 0)
    {
        Type scanResult   = output[ elemIdx ];
        Type postBlockSum = postSumArray[ secIdx-1 ];
        Type newResult    = (*binaryOp)( scanResult, postBlockSum );
        output[ elemIdx ] = newResult;
    }
    //secIdx++;
    elemIdx++; //=wgSize;

    for (size_t i = 1; i < NUM_ITER-1; i++)
    {
      Type scanResult   = output[ elemIdx ];
      Type postBlockSum = postSumArray[ secIdx-1 ];
      Type newResult    = (*binaryOp)( scanResult, postBlockSum );
      output[ elemIdx ] = newResult;
      
      // secIdx++;
      elemIdx++; //=wgSize;
    }

    if (elemIdx < vecSize /*&& groId > 0*/)
    {
        Type scanResult = output[ elemIdx ];
        Type postBlockSum = postSumArray[ secIdx-1 ];
        Type newResult = (*binaryOp)( scanResult, postBlockSum );
        output[ elemIdx ] = newResult;
    }

#endif
}


/******************************************************************************
 *  Kernel 1
 *****************************************************************************/
template< typename iPtrType, typename initType, typename BinaryFunction >
kernel void intraBlockInclusiveScan(
                global iPtrType* postSumArray,
                global iPtrType* preSumArray, 
                initType identity,
                const uint vecSize,
                local iPtrType* lds,
                const uint workPerThread,
                global BinaryFunction* binaryOp
                )
{
    size_t gloId = get_global_id( 0 );
    size_t locId = get_local_id( 0 );
    size_t wgSize = get_local_size( 0 );
    uint mapId  = gloId * workPerThread;

    // do offset of zero manually
    uint offset;
    iPtrType workSum;
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
                iPtrType y = preSumArray[mapId+offset];
                workSum = (*binaryOp)( workSum, y );
                postSumArray[ mapId + offset ] = workSum;
            }
        }
    }
    barrier( CLK_LOCAL_MEM_FENCE );
    iPtrType scanSum = workSum;
    lds[ locId ] = workSum;
    offset = 1;
  // scan in lds
    for( offset = offset*1; offset < wgSize; offset *= 2 )
    {
        barrier( CLK_LOCAL_MEM_FENCE );
        if (mapId < vecSize)
        {
            if (locId >= offset)
            {
                iPtrType y = lds[ locId - offset ];
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
            iPtrType y = postSumArray[ mapId + offset ];
            iPtrType y2 = lds[locId-1];
            y = (*binaryOp)( y, y2 );
            postSumArray[ mapId + offset ] = y;
        } // thread in bounds

    } // for 

} // end kernel


/******************************************************************************
 *  Kernel 0
 *****************************************************************************/
template< typename iPtrType, typename iIterType, typename oPtrType, typename oIterType, typename initType, typename BinaryFunction >
kernel void perBlockInclusiveScan(
                global oPtrType* output_ptr,
                oIterType    output_iter, 
                global iPtrType* input_ptr,
                iIterType    input_iter, 
                initType identity,
                const uint vecSize,
                local oPtrType* lds,
                global BinaryFunction* binaryOp,
                global oPtrType* scanBuffer,
                int exclusive) // do exclusive scan ?
{
    size_t gloId = get_global_id( 0 );
    size_t groId = get_group_id( 0 );
    size_t locId = get_local_id( 0 );
    size_t wgSize = get_local_size( 0 );

    
    output_iter.init( output_ptr );
    input_iter.init( input_ptr );

    // if exclusive, load gloId=0 w/ identity, and all others shifted-1
    oPtrType val;
    if (gloId < vecSize){
       if (exclusive)
       {
          if (gloId > 0)
          { // thread>0
              val = input_iter[gloId-1];
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
          val = input_iter[gloId];
          lds[ locId ] = val;
       }
    }

    //  Computes a scan within a workgroup
    oPtrType sum = val;

    for( size_t offset = 1; offset < wgSize; offset *= 2 )
    {
        barrier( CLK_LOCAL_MEM_FENCE );
        if (locId >= offset)
        {
            oPtrType y = lds[ locId - offset ];
            sum = (*binaryOp)( sum, y );
        }
        barrier( CLK_LOCAL_MEM_FENCE );
        lds[ locId ] = sum;
    }
    barrier( CLK_LOCAL_MEM_FENCE );
  
    //  Abort threads that are passed the end of the input vector
    if (gloId >= vecSize) return; 
   
    //  Each work item writes out its calculated scan result, relative to the beginning
    //  of each work group
    output_iter[ gloId ] = sum;
    if (locId == 0)
    {
        // last work-group can be wrong b/c ignored
        scanBuffer[ groId ] = lds[ wgSize-1 ];
    }
}

// not using HSA
#endif
