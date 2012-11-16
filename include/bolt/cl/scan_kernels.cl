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
    size_t waveNum = (locId / 64);
    size_t numWaves = get_local_size( 0 ) / 64;

    //  Abort threads that are passed the end of the input vector
    if( gloId >= vecSize )
        return;
        
    Type scanResult = output[ gloId ];

    //  TODO:  verify; is there a memory conflict if all threads read from the same address?
    Type postBlockSum = postSumArray[ groId /* *numWaves+waveNum*/ ];
    Type newResult = (*binaryOp)( scanResult, postBlockSum );

    //printf( "k0[%d]=%d; k1[%d]=%d; k2[%d]=%d\n",
    //    gloId, scanResult,
    //    groId*numWaves+waveNum, postBlockSum,
    //    gloId, newResult );
    // printf( "postSumArray[%d] = [%d]\n", groId, postBlockSum );
    
    
    output[ gloId ] = newResult;
}






















/******************************************************************************
 *  Kernel 1
 *****************************************************************************/
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
    
    //if( mapId >= vecSize )
    //    return;
    iType workSum = 0;
if (mapId < vecSize)
{

    //  Reducing workPerThread values per workitem into just 1 value per workitem; one wavefront execution
    
    for( uint offset = 0; offset < workPerThread; offset += 1 )
    {
        iType y = ((mapId + offset) < vecSize) ? preSumArray[ mapId + offset ] : 0;
        // printf( "preSumArray[%d] = [%g]\n", mapId + offset, y );
        workSum = (*binaryOp)( workSum, y );
        postSumArray[ mapId + offset ] = workSum;
    }
}
    barrier( CLK_LOCAL_MEM_FENCE );
    iType scanSum;
if (mapId < vecSize)
{
    pLDS[ locId ] = workSum;

    //	This loop computes the inclusive scan of the reduced block sum array
    scanSum = workSum;
}
    for( size_t offset = 1; offset < wgSize; offset *= 2 )
    {
        barrier( CLK_LOCAL_MEM_FENCE );
if (mapId < vecSize)
{
        //  Make sure that the index is a signed quantity, as we expect to generate negative indices
        int index = locId - offset;

        iType y = pLDS[ index ];
        scanSum = (*binaryOp)( scanSum, y );
        pLDS[ locId ] = scanSum;
}
    }
    barrier( CLK_LOCAL_MEM_FENCE );
    //	This subtraction converts the inclusive scan to the exclusive scan
if (mapId < vecSize)
{
    scanSum -= workSum;
}
    //  This adjusts the 1 value per workitem back into workPerThread values per workitem
    for( uint offset = 0; offset < workPerThread; offset += 1 )
    {
        barrier( CLK_GLOBAL_MEM_FENCE );

if (mapId < vecSize)
{
        iType y = postSumArray[ mapId + offset ];
        y = (*binaryOp)( y, scanSum );
        y -= preSumArray[ mapId + offset ];
        postSumArray[ mapId + offset ] = y;
        // printf( "postSumArray[%d] = [%g]\n", mapId + offset, postSumArray[ mapId + offset ] );
}
    }

}













/******************************************************************************
 *  Kernel 0
 *****************************************************************************/
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
    size_t waveNum = (locId / 64);
    size_t numWaves = get_local_size( 0 ) / 64;

    //    Initialize the padding to 0, for when the scan algorithm looks left.
    //    Then bump the LDS pointer past the padding
    lds[ locId ] = 0;
    local volatile iType* pLDS = lds + ( wgSize / 2 );

    //  Abort threads that are passed the end of the input vector
    //  TODO:  I'm returning early for threads past the input vector size; not safe for barriers in kernel if wg != wavefront
    //if( gloId >= vecSize )
    //    return;
    iType val;
if (gloId < vecSize)
{

    // Initialize the scan with init only on the first thread
    val = (gloId == 0) ? init: 0;
    
    val = val + input[ gloId ];
    
    pLDS[ locId ] = val;
}
    //  This loop essentially computes a scan within a workgroup
    //  No communication between workgroups
    iType sum = val;
    for( size_t offset = 1; offset < wgSize; offset *= 2 )
    {
        barrier( CLK_LOCAL_MEM_FENCE );
        
        //  Make sure that the index is a signed quantity, as we expect to generate negative indices
if (gloId < vecSize)
{
        int index = locId - offset;

        iType y = pLDS[ index ];
        sum = (*binaryOp)( sum, y );
        pLDS[ locId ] = sum;
}
    }
    barrier( CLK_LOCAL_MEM_FENCE );
    //  Each work item writes out its calculated scan result, relative to the beginning
    //  of each work group
if (gloId < vecSize)
{
    output[ gloId ] = sum;
}
    //printf( "Output Array work-item[%d], sum[%d]\n", gloId, sum );

    barrier( CLK_LOCAL_MEM_FENCE );
    
    //	Take a single thread, and save the last valid value in a workgroup to the block sum array
    //if( locId == 0 )
    //{
    //    uint lastValidWorkItem = min( wgSize, vecSize - ( get_group_id( 0 ) * wgSize ) );        
    //    scanBuffer[ groId ] = pLDS[ lastValidWorkItem-1 ];
    //    printf( "scanBuffer[%d] = pLDS[%d] = %g\n", gloId, lastValidWorkItem - 1, pLDS[ lastValidWorkItem - 1 ] );
    //}
    
    //  Verify; is it still performant to have every work item write to the same output array index?
if (gloId < vecSize)
{
    size_t lastValidWorkItem = min( wgSize, vecSize - ( groId * wgSize ) );        
    scanBuffer[ groId ] = pLDS[ lastValidWorkItem-1 ];
}
}

