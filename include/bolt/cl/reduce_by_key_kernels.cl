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
	typename voType,
    typename BinaryPredicate,
    typename BinaryFunction >
__kernel void perBlockScanByKey(
    global kType *keys,
    global vType *vals,
    global oType *output, // input
    global voType *output2,
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

    //  Abort threads that are passed the end of the input vector
    //if (gloId >= vecSize) return; // on SI this doesn't mess-up barriers

    // if exclusive, load gloId=0 w/ init, and all others shifted-1
    kType key;
    oType val;
    //vType inVal = vals[gloId];
    //val = (oType) (*unaryOp)(inVal);
    key = keys[ gloId ];
    val = vals[ gloId ];
    ldsKeys[ locId ] = key;
    ldsVals[ locId ] = val;

    // Computes a scan within a workgroup
    // updates vals in lds but not keys
    oType sum = val;
    for( size_t offset = 1; offset < wgSize; offset *= 2 )
    {
        barrier( CLK_LOCAL_MEM_FENCE );
        kType key2 = ldsKeys[ locId - offset];
        if (locId >= offset && (*binaryPred)( key, key2 )  )
        {
            oType y = ldsVals[ locId - offset ];
            sum = (*binaryFunct)( sum, y );
        }
        barrier( CLK_LOCAL_MEM_FENCE );
        ldsVals[ locId ] = sum;
    }

    // Each work item writes out its calculated scan result, relative to the beginning
    // of each work group
    output[ gloId ] = sum;
    barrier( CLK_LOCAL_MEM_FENCE ); // needed for large data types
    if (locId == 0)
    {
        keyBuffer[ groId ] = ldsKeys[ wgSize-1 ];
        valBuffer[ groId ] = ldsVals[ wgSize-1 ];
    }
}


