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

/******************************************************************************
 *  Kernel 0
 *****************************************************************************/
template<
    typename kType,
    typename vType,
    typename oType,
    typename voType,
    typename BinaryPredicate,
    typename BinaryFunction >
__kernel void perBlockScanByKey(
    global kType *keys, //input keys
    global vType *vals, //input values
    global voType *output, // offsetValues
    global int *output2, //offsetKeys
    const uint vecSize,
    local kType *ldsKeys,
    local oType *ldsVals,
    global BinaryPredicate *binaryPred,
    global BinaryFunction *binaryFunct,
    global kType *keyBuffer,
    global oType *valBuffer)
{
    size_t gloId = get_global_id( 0 );
    size_t groId = get_group_id( 0 );
    size_t locId = get_local_id( 0 );
    size_t wgSize = get_local_size( 0 );
    //printf("gid=%i, lTid=%i, gTid=%i\n", groId, locId, gloId);

    // if exclusive, load gloId=0 w/ init, and all others shifted-1
    kType key;
    oType val;
    //vType inVal = vals[gloId];
    //val = (oType) (*unaryOp)(inVal);
    
    if(gloId < vecSize){
      key = keys[ gloId ];
      val = vals[ gloId ];
      ldsKeys[ locId ] = key;
      ldsVals[ locId ] = val;
    }
    
    // Computes a scan within a workgroup
    // updates vals in lds but not keys
    oType sum = val;
    for( size_t offset = 1; offset < wgSize; offset *= 2 )
    {
        barrier( CLK_LOCAL_MEM_FENCE );
        kType key2 = ldsKeys[locId - offset];
        kType prevKey = ldsKeys[locId - 1];
        if (locId >= offset && (*binaryPred)(key, key2) && (*binaryPred)(key,prevKey))
        {
            oType y = ldsVals[ locId - offset ];
            sum = (*binaryFunct)( sum, y );
        }		
        else //This has to be optimized
        {
            if (offset == 1 && gloId < vecSize) //Only when you compare neighbours
            {
                    output2[ gloId ] = 1;
            }
                
        }
        barrier( CLK_LOCAL_MEM_FENCE );
        ldsVals[ locId ] = sum;
    }
    barrier( CLK_LOCAL_MEM_FENCE ); // needed for large data types
    //  Abort threads that are passed the end of the input vector
    if (gloId >= vecSize) return;

    // Each work item writes out its calculated scan result, relative to the beginning
    // of each work group
    output[ gloId ] = sum;
    
    if (locId == 0)
    {
        keyBuffer[ groId ] = ldsKeys[ wgSize-1 ];
        valBuffer[ groId ] = ldsVals[ wgSize-1 ];
    }
}


/******************************************************************************
 *  Kernel 1
 *****************************************************************************/
template<
    typename kType,
    typename oType,
    typename BinaryPredicate,
    typename BinaryFunction >
__kernel void intraBlockInclusiveScanByKey(
    global kType *keySumArray,
    global oType *preSumArray,
    global oType *postSumArray,
    const uint vecSize,
    local kType *ldsKeys,
    local oType *ldsVals,
    const uint workPerThread,
    global BinaryPredicate *binaryPred,
    global BinaryFunction  *binaryFunct )
{
    size_t groId = get_group_id( 0 );
    size_t gloId = get_global_id( 0 );
    size_t locId = get_local_id( 0 );
    size_t wgSize = get_local_size( 0 );
    uint mapId  = gloId * workPerThread;
    
    // do offset of zero manually
    uint offset;
    kType key;
    oType workSum;

    if (mapId < vecSize)
    {
        kType prevKey;

        // accumulate zeroth value manually
        offset = 0;
        key = keySumArray[ mapId+offset ];
        workSum = preSumArray[ mapId+offset ];
        postSumArray[ mapId+offset ] = workSum;

        //  Serial accumulation
        for( offset = offset+1; offset < workPerThread; offset += 1 )
        {
            prevKey = key;
            key = keySumArray[ mapId+offset ];
            if (mapId+offset<vecSize )
            {
                oType y = preSumArray[ mapId+offset ];
                if ( (*binaryPred)(key, prevKey ) )
                {
                    workSum = (*binaryFunct)( workSum, y );
                }
                else
                {
                    workSum = y;
                }
                postSumArray[ mapId+offset ] = workSum;
            }
        }
    }
    barrier( CLK_LOCAL_MEM_FENCE );
   
    oType scanSum = workSum;
    offset = 1;
    // load LDS with register sums
    ldsVals[ locId ] = workSum;
    ldsKeys[ locId ] = key;
    // scan in lds
    for( offset = offset*1; offset < wgSize; offset *= 2 )
    {
        barrier( CLK_LOCAL_MEM_FENCE );
        if (mapId < vecSize)
        {
            if (locId >= offset  )
            {
                oType y = ldsVals[ locId - offset ];
                kType key1 = ldsKeys[ locId ];
                kType key2 = ldsKeys[ locId-offset ];
                if ( (*binaryPred)( key1, key2 ) )
                {
                   scanSum = (*binaryFunct)( scanSum, y );
                }
                else
                   scanSum = ldsVals[ locId ];
             }
      
        }
        barrier( CLK_LOCAL_MEM_FENCE );
        ldsVals[ locId ] = scanSum;
    } // for offset
    barrier( CLK_LOCAL_MEM_FENCE );
    
    // write final scan from pre-scan and lds scan
    for( offset = 0; offset < workPerThread; offset += 1 )
    {
        barrier( CLK_GLOBAL_MEM_FENCE );

        if (mapId < vecSize && locId > 0)
        {
            oType y = postSumArray[ mapId+offset ];
            kType key1 = keySumArray[ mapId+offset ]; // change me
            kType key2 = ldsKeys[ locId-1 ];
            if ( (*binaryPred)( key1, key2 ) )
            {
                oType y2 = ldsVals[locId-1];
                y = (*binaryFunct)( y, y2 );
            }
            postSumArray[ mapId+offset ] = y;
        } // thread in bounds
    } // for 
    
} // end kernel


/******************************************************************************
 *  Kernel 2
 *****************************************************************************/
template<
    typename kType,
    typename oType,
    typename BinaryPredicate,
    typename BinaryFunction >
__kernel void perBlockAdditionByKey(
    global kType *keySumArray, //InputBuffer
    global oType *postSumArray, //InputBuffer
    global kType *keys, //Input keys
    global int *output2, //offsetArray
    global oType *output, //offsetValArray
    const uint vecSize,
    global BinaryPredicate *binaryPred,
    global BinaryFunction *binaryFunct)
{
    size_t gloId = get_global_id( 0 );
    size_t groId = get_group_id( 0 );
    size_t locId = get_local_id( 0 );

    //  Abort threads that are passed the end of the input vector
    if( gloId >= vecSize )
        return;
        
    oType scanResult = output[ gloId ];

    // accumulate prefix
    kType key1 = keySumArray[ groId-1 ];
    kType key2 = keys[ gloId ];
    if (groId > 0 && (*binaryPred)( key1, key2 ) )
    {
        oType postBlockSum = postSumArray[ groId-1 ];
        oType newResult = (*binaryFunct)( scanResult, postBlockSum );
        output[ gloId ] = newResult;
        output2[ gloId ] = 0;
    }
}

/******************************************************************************
 *  Kernel 3
 *****************************************************************************/
 template<
    typename kType,
    typename koType,
    typename voType >
__kernel void keyValueMapping(
    global kType *keys,
    global koType *keys_output,
    global voType *vals_output,
    global int *offsetArray,
    global koType *offsetValArray,
    const uint vecSize,
    const int numSections)
{
    
    size_t gloId = get_global_id( 0 );

    //  Abort threads that are passed the end of the input vector
    if( gloId >= vecSize )
        return;
    

    if(offsetArray[ gloId ] != 0)
    {
        //int x = offsetArray [ gloId ] -1;
        //int xkeys = keys[ gloId-1 ];
        //int xvals = offsetValArray [ gloId-1 ]; 

        keys_output[ offsetArray [ gloId ] -1 ] = keys[ gloId-1 ];
        vals_output[ offsetArray [ gloId ] -1 ] = offsetValArray [ gloId-1 ];
    }

  if( gloId == (vecSize-1) )
  {
    //printf("ossfetarr=%d\n", offsetArray [ gloId ]);
    //printf("keys=%d\n", keys[ gloId ]);
    //printf("offsetval=%d\n",offsetValArray [ gloId ]);
        keys_output[ numSections - 1 ] = keys[ gloId ]; //Copying the last key directly. Works either ways
        vals_output[ numSections - 1 ] = offsetValArray [ gloId ];
      
  }

    
}