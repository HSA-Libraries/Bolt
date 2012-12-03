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
template< typename Type, typename BinaryFunction >
kernel void perBlockAddition( 
                global Type* output,
                global Type* postSumArray,
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
        
    Type scanResult = output[ gloId ];

    // accumulate prefix
    if (groId > 0)
    {
        Type postBlockSum = postSumArray[ groId-1 ];
        Type newResult = (*binaryOp)( scanResult, postBlockSum );
        output[ gloId ] = newResult;
    }
}


/******************************************************************************
 *  Kernel 1
 *****************************************************************************/
template< typename iType, typename BinaryFunction >
kernel void intraBlockInclusiveScan(
                global iType* postSumArray,
                global iType* preSumArray, 
                iType identity,
                const uint vecSize,
                local iType* lds,
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
    iType workSum;
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
                iType y = preSumArray[mapId+offset];
                workSum = (*binaryOp)( workSum, y );
                postSumArray[ mapId + offset ] = workSum;
            }
        }
    }
    barrier( CLK_LOCAL_MEM_FENCE );
    iType scanSum;
    offset = 1;
    // load LDS with register sums
    if (mapId < vecSize)
    {
        lds[ locId ] = workSum;
        barrier( CLK_LOCAL_MEM_FENCE );
    
        if (locId >= offset)
        { // thread > 0
            iType y = lds[ locId - offset ];
            iType y2 = lds[ locId ];
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
                iType y = lds[ locId - offset ];
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
            iType y = postSumArray[ mapId + offset ];
            iType y2 = lds[locId-1];
            y = (*binaryOp)( y, y2 );
            postSumArray[ mapId + offset ] = y;
        } // thread in bounds
    } // for 
} // end kernel


/******************************************************************************
 *  Kernel 0
 *****************************************************************************/
template< typename iType, typename UnaryFunction, typename BinaryFunction >
kernel void perBlockTransformScan(
                global iType* output,
                global iType* input,
                iType identity,
                const uint vecSize,
                local iType* lds,
                global UnaryFunction* unaryOp,
                global BinaryFunction* binaryOp,
                global iType* scanBuffer,
                int exclusive) // do exclusive scan ?
{
    size_t gloId = get_global_id( 0 );
    size_t groId = get_group_id( 0 );
    size_t locId = get_local_id( 0 );
    size_t wgSize = get_local_size( 0 );

    //  Abort threads that are passed the end of the input vector
    if (gloId >= vecSize) return; // on SI this doesn't mess-up barriers

    // if exclusive, load gloId=0 w/ identity, and all others shifted-1
    iType val;
    if (exclusive)
    {
        if (gloId > 0)
        { // thread>0
            val = input[gloId-1];
            val = (*unaryOp)(val);
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
        val = input[gloId];
        val = (*unaryOp)(val);
        lds[ locId ] = val;
    }

    //  Computes a scan within a workgroup
    iType sum = val;
    for( size_t offset = 1; offset < wgSize; offset *= 2 )
    {
        barrier( CLK_LOCAL_MEM_FENCE );
        if (locId >= offset)
        {
            iType y = lds[ locId - offset ];
            sum = (*binaryOp)( sum, y );
        }
        barrier( CLK_LOCAL_MEM_FENCE );
        lds[ locId ] = sum;
    }

    //  Each work item writes out its calculated scan result, relative to the beginning
    //  of each work group
    output[ gloId ] = sum;
    barrier( CLK_LOCAL_MEM_FENCE ); // needed for large data types
    if (locId == 0)
    {
        // last work-group can be wrong b/c ignored
        scanBuffer[ groId ] = lds[ wgSize-1 ];
    }
}

