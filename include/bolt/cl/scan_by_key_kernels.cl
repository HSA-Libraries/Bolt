/***************************************************************************
*   Copyright 2012 Advanced Micro Devices, Inc.                                     
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
    typename BinaryPredicate,
    typename BinaryFunction >
__kernel void perBlockTransformScan(
    global kType* keys,
    global vType* vals,
    global oType* output, // input
    oType init,
    const uint vecSize,
    local oType* ldsKeys,
    local oType* ldsVals,
    global UnaryFunction* binaryPred,
    global BinaryFunction* binaryFunct,
    global kType* keyBuffer,
    global oType* valBuffer,
    int exclusive) // do exclusive scan ?
{
    size_t gloId = get_global_id( 0 );
    size_t groId = get_group_id( 0 );
    size_t locId = get_local_id( 0 );
    size_t wgSize = get_local_size( 0 );
    //printf("gid=%i, lTid=%i, gTid=%i\n", groId, locId, gloId);

    //  Abort threads that are passed the end of the input vector
    if (gloId >= vecSize) return; // on SI this doesn't mess-up barriers

    // if exclusive, load gloId=0 w/ init, and all others shifted-1
    kType key;
    oType val;
    if (exclusive)
    {
        if (gloId > 0)
        { // thread>0
            //vType inVal = vals[gloId-1];
            //val = (oType) (*unaryOp)(inVal);
            ldsVals[ locId ] = vals[gloId-1];
            key = keys[gloId-1];
            ldsKeys[ locId ] = key;
        }
        else
        { // thread=0
            val = init;
            ldsVals[ locId ] = val;
            // key stays null, this thread should never try to compare it's key
            // nor should any thread compare it's key to ldsKey[ 0 ]
            // I could put another key into lds just for whatevs
        }
    }
    else
    {
        //vType inVal = vals[gloId];
        //val = (oType) (*unaryOp)(inVal);
        ldsVals[ locId ] = vals[gloId];
    }

    //  Computes a scan within a workgroup
    oType sum = val;
    for( size_t offset = 1; offset < wgSize; offset *= 2 )
    {
        barrier( CLK_LOCAL_MEM_FENCE );
        if (locId >= offset)
        {
            oType y = ldsVals[ locId - offset ];
            sum = (*binaryFunct)( sum, y );
        }
        barrier( CLK_LOCAL_MEM_FENCE );
        ldsVals[ locId ] = sum;
    }

    //  Each work item writes out its calculated scan result, relative to the beginning
    //  of each work group
    output[ gloId ] = sum;
    barrier( CLK_LOCAL_MEM_FENCE ); // needed for large data types
    if (locId == 0)
    {
        // last work-group can be wrong b/c ignored
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
__kernel void intraBlockInclusiveScan(
    global kType* keySumArray,
    global oType* preSumArray,
    global oType* postSumArray,
    const uint vecSize,
    local Type* ldsKeys,
    local Type* ldsVals,
    const uint workPerThread,
    global BinaryPredicate* binaryPred,
    global BinaryFunction* binaryFunct )
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
                workSum = (*binaryFunct)( workSum, y );
                postSumArray[ mapId + offset ] = workSum;
            }
        }
    }
    barrier( CLK_LOCAL_MEM_FENCE );
    Type scanSum;
    offset = 1;
    // load LDS with register sums
    if (mapId < vecSize)
    {
        ldsVals[ locId ] = workSum;
        barrier( CLK_LOCAL_MEM_FENCE );
    
        if (locId >= offset)
        { // thread > 0
            Type y = ldsVals[ locId - offset ];
            Type y2 = ldsVals[ locId ];
            scanSum = (*binaryFunct)( y2, y );
            ldsVals[ locId ] = scanSum;
        } else { // thread 0
            scanSum = workSum;
        }  
    }
    // scan in lds
    for( offset = offset*2; offset < wgSize; offset *= 2 )
    {
        barrier( CLK_LOCAL_MEM_FENCE );
        if (mapId < vecSize)
        {
            if (locId >= offset)
            {
                Type y = ldsVals[ locId - offset ];
                scanSum = (*binaryFunct)( scanSum, y );
                ldsVals[ locId ] = scanSum;
            }
        }
    } // for offset
    barrier( CLK_LOCAL_MEM_FENCE );
    
    // write final scan from pre-scan and lds scan
    for( offset = 0; offset < workPerThread; offset += 1 )
    {
        barrier( CLK_GLOBAL_MEM_FENCE );

        if (mapId < vecSize && locId > 0)
        {
            Type y = postSumArray[ mapId + offset ];
            Type y2 = ldsVals[locId-1];
            y = (*binaryFunct)( y, y2 );
            postSumArray[ mapId + offset ] = y;
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
__kernel void perBlockAddition(
    global kType *keySumArray,
    global oType *postSumArray,
    global kType *keys,
    global oType *output,
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
    if (groId > 0)
    {
        oType postBlockSum = postSumArray[ groId-1 ];
        oType newResult = (*binaryFunct)( scanResult, postBlockSum );
        output[ gloId ] = newResult;
    }
}
