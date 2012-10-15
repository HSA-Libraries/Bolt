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

// #ifdef cl_amd_printf
    // #pragma OPENCL EXTENSION cl_amd_printf : enable
// #endif

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

    //  TODO:  verify; is there a memory conflict if all threads read from the same address?
    Type postBlockSum = postSumArray[ groId ];

    // printf( "output[%d] = [%d]\n", gloId, scanResult );
    // printf( "postSumArray[%d] = [%d]\n", groId, postBlockSum );
    scanResult = (*binaryOp)( scanResult, postBlockSum );
    output[ gloId ] = scanResult;
}

template< typename iType, typename BinaryFunction >
kernel void intraBlockExclusiveScan(
                global iType* postSumArray,
                global iType* preSumArray,
                const uint vecSize,
                local volatile iType* lds,
                const uint workPerThread,
                global BinaryFunction* binaryOp    // Functor operation to apply on each step
                )
{
    size_t gloId = get_global_id( 0 );
    size_t groId = get_group_id( 0 );
    size_t locId = get_local_id( 0 );
    size_t wgSize = get_local_size( 0 );
    uint mapId  = gloId * workPerThread;

    //    Initialize the padding to 0, for when the scan algorithm looks left.
    //    Then bump the LDS pointer past the padding
    lds[ locId ] = 0;
    local volatile iType* pLDS = lds + ( wgSize / 2 );

    //  Abort threads that are passed the end of the input vector
    //  TODO:  I'm returning early for threads past the input vector size; not safe for barriers in kernel if wg != wavefront
    if( gloId >= vecSize )
        return;

    //	A inclusive sequential scan, reducing values for very large arrays
    iType workSum = 0;
    for( uint offset = 0; offset < workPerThread; offset += 1 )
    {
        iType y = preSumArray[ mapId + offset ];
        // printf( "preSumArray[%d] = [%g]\n", mapId + offset, y );
        workSum = (*binaryOp)( workSum, y );
        postSumArray[ mapId + offset ] = workSum;
    }
    barrier( CLK_LOCAL_MEM_FENCE );
    pLDS[ locId ] = workSum;

    //	This loop essentially computes an exclusive scan within a tile, writing 0 out for first element.
    iType scanSum = workSum;
    for( size_t offset = 1; offset < wgSize; offset *= 2 )
    {
        barrier( CLK_LOCAL_MEM_FENCE );
        
        //  Make sure that the index is a signed quantity, as we expect to generate negative indices
        int index = locId - offset;

        iType y = pLDS[ index ];
        scanSum = (*binaryOp)( scanSum, y );
        pLDS[ locId ] = scanSum;
    }

    //	Write out the values of the per-tile scan
    scanSum -= workSum;
    for( uint offset = 0; offset < workPerThread; offset += 1 )
    {
        iType y = postSumArray[ mapId + offset ];
        y = (*binaryOp)( y, scanSum );
        y -= preSumArray[ mapId + offset ];
        postSumArray[ mapId + offset ] = y;
        //printf( "postSumArray[%d] = [%g]\n", mapId + offset, postSumArray[ mapId + offset ] );
    }

}

template< typename iType, typename T, typename BinaryFunction >
kernel void perBlockInclusiveScan(
                global iType* output,
                global iType* input,
                const T init,
                const uint vecSize,
                local volatile iType* lds,
                global BinaryFunction* binaryOp,    // Functor operation to apply on each step
                global iType* scanBuffer )            // Passed to 2nd kernel; the of each block
{
    size_t gloId = get_global_id( 0 );
    size_t groId = get_group_id( 0 );
    size_t locId = get_local_id( 0 );
    size_t wgSize = get_local_size( 0 );

    //    Initialize the padding to 0, for when the scan algorithm looks left.
    //    Then bump the LDS pointer past the padding
    lds[ locId ] = 0;
    local volatile iType* pLDS = lds + ( wgSize / 2 );

    //  Abort threads that are passed the end of the input vector
    //  TODO:  I'm returning early for threads past the input vector size; not safe for barriers in kernel if wg != wavefront
    if( gloId >= vecSize )
        return;

    // Initialize the scan with init only on the first thread
    iType val = (gloId == 0) ? init: 0;
    
    val = val + input[ gloId ];
    
    pLDS[ locId ] = val;

    //  This loop essentially computes a scan within a workgroup
    //  No communication between workgroups
    iType sum = val;
    for( size_t offset = 1; offset < wgSize; offset *= 2 )
    {
        barrier( CLK_LOCAL_MEM_FENCE );
        
        //  Make sure that the index is a signed quantity, as we expect to generate negative indices
        int index = locId - offset;

        iType y = pLDS[ index ];
        sum = (*binaryOp)( sum, y );
        pLDS[ locId ] = sum;
    }

    //  Each work item writes out its calculated scan result, relative to the beginning
    //  of each work group
    output[ gloId ] = sum;
    // printf( "Output Array work-item[%d], sum[%d]\n", gloId, sum );

    barrier( CLK_LOCAL_MEM_FENCE );

    //  TODO:  verify; is there a memory conflict if all threads write to the same address?
    scanBuffer[ groId ] = pLDS[ wgSize - 1 ];
    // printf( "Block Sum Array work-item[%d], sum[%g]\n", gloId, pLDS[ wgSize - 1 ] );
    
    //	Take the very last thread in a tile, and save its value into a buffer for further processing
    // if( locId == (wgSize-1) )
    // {
        // scanBuffer[ groId ] = pLDS[ locId ];
    // }
}

