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
//#pragma OPENCL EXTENSION cl_amd_printf : enable


/******************************************************************************
 *  Kernel 2
 *****************************************************************************/
template< typename iPtrType, typename iIterType, typename BinaryFunction >
kernel void perBlockAddition( 
                global iPtrType* output_ptr,
                iIterType    output_iter, 
                global iPtrType* postSumArray_ptr,
                const uint vecSize,
                global BinaryFunction* binaryOp
                )
{
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
}


/******************************************************************************
 *  Kernel 1
 *****************************************************************************/
template< typename iPtrType, typename BinaryFunction >
kernel void intraBlockInclusiveScan(
                global iPtrType* postSumArray,
                global iPtrType* preSumArray, 
                iPtrType identity,
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
    iPtrType scanSum;
    offset = 1;
    // load LDS with register sums
    if (mapId < vecSize)
    {
        lds[ locId ] = workSum;
        barrier( CLK_LOCAL_MEM_FENCE );
    
        if (locId >= offset)
        { // thread > 0
            iPtrType y = lds[ locId - offset ];
            iPtrType y2 = lds[ locId ];
            scanSum = (*binaryOp)( y2, y );
            lds[ locId ] = scanSum;
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
                iPtrType y = lds[ locId - offset ];
                scanSum = (*binaryOp)( scanSum, y );
                lds[ locId ] = scanSum;
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
template< typename iPtrType, typename iIterType, typename BinaryFunction >
kernel void perBlockInclusiveScan(
                global iPtrType* output_ptr,
                iIterType    output_iter, 
                global iPtrType* input_ptr,
                iIterType    input_iter, 
                iPtrType identity,
                const uint vecSize,
                local iPtrType* lds,
                global BinaryFunction* binaryOp,
                global iPtrType* scanBuffer,
                int exclusive) // do exclusive scan ?
{
    size_t gloId = get_global_id( 0 );
    size_t groId = get_group_id( 0 );
    size_t locId = get_local_id( 0 );
    size_t wgSize = get_local_size( 0 );

    //  Abort threads that are passed the end of the input vector
    if (gloId >= vecSize) return; // on SI this doesn't mess-up barriers

    output_iter.init( output_ptr );
    input_iter.init( input_ptr );

    // if exclusive, load gloId=0 w/ identity, and all others shifted-1
    iPtrType val;
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

    //  Computes a scan within a workgroup
    iPtrType sum = val;
    for( size_t offset = 1; offset < wgSize; offset *= 2 )
    {
        barrier( CLK_LOCAL_MEM_FENCE );
        if (locId >= offset)
        {
            iPtrType y = lds[ locId - offset ];
            sum = (*binaryOp)( sum, y );
        }
        barrier( CLK_LOCAL_MEM_FENCE );
        lds[ locId ] = sum;
    }

    //  Each work item writes out its calculated scan result, relative to the beginning
    //  of each work group
    output_iter[ gloId ] = sum;
    barrier( CLK_LOCAL_MEM_FENCE ); // needed for large data types
    if (locId == 0)
    {
        // last work-group can be wrong b/c ignored
        scanBuffer[ groId ] = lds[ wgSize-1 ];
    }
}

