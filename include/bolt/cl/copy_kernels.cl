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


// No Boundary Check Kernel
template < typename iType, typename oType >
__kernel
void copyNoBoundsCheck(
    global iType *src,
    global oType *dst,
	const uint length )
{
    uint i = get_global_id(0);
    dst[i] = src[i];
};


// Yes Boundary Generate Kernel
template < typename iType, typename oType >
__kernel
void copyBoundsCheck(
    global iType *src,
    global oType *dst,
	const uint length )
{
    uint i = get_global_id(0);

    if (i < length) {
        dst[i] = src[i];
    }
};

// ideal num threads
template < typename iType, typename oType >
__kernel
void copyA(
    global iType *src,
    global oType *dst,
	  const uint numElements )
{
    for (
        unsigned int i = get_global_id(0);
        i < numElements;
        i += get_global_size( 0 ) )
    {
        dst[i] = src[i];
    }
};

// ideal num threads\
// bursts
//#define BURST 16
template < typename iType, typename oType >
__kernel
void copyB(
    global iType *src,
    global oType *dst,
	  const uint numElements )
{
    __private iType tmp[BURST];
    for (
        unsigned int i = get_global_id(0)*BURST;
        i < numElements;
        i += get_global_size( 0 )*BURST )
    {
        
        for ( unsigned int j = 0; j < BURST; j++)
        {
            dst[i+j] = src[i+j];
        }
        //for ( unsigned int j = 0; j < BURST; j++)
        //{
        //    dst[i+j] = tmp[j];
        //}
    }
};

template < typename iType, typename oType >
__kernel
void copyC(
    global iType *src,
    global oType *dst,
	  const uint numElements )
{
    unsigned int offset = get_global_id(0)*BURST;
    __private iType tmp[BURST];
    
    for ( unsigned int i = 0; i < BURST; i++)
    {
        tmp[i] = src[offset+i];
    }
    for ( unsigned int j = 0; j < BURST; j++)
    {
        dst[offset+j] = tmp[j];
    }
    
};

#define ITER 16

__attribute__((reqd_work_group_size(256,1,1)))
__kernel
void copyDInstantiated(
    global const float4 * restrict src,
    global float4 * restrict dst,
	  const uint numElements )
{
    //unsigned int offset = get_global_id(0)*BURST;
    __global const float4 *srcPtr = &src[get_global_id(0)*BURST];
    __global float4 *dstPtr = &dst[get_global_id(0)*BURST];
    __private float4 tmp[BURST];



#if BURST >  0
                tmp[ 0] = srcPtr[ 0];
#endif                            
#if BURST >  1                    
                tmp[ 1] = srcPtr[ 1];
#endif                            
#if BURST >  2                    
                tmp[ 2] = srcPtr[ 2];
#endif                            
#if BURST >  3                    
                tmp[ 3] = srcPtr[ 3];
#endif                            
#if BURST >  4                    
                tmp[ 4] = srcPtr[ 4];
#endif                            
#if BURST >  5                    
                tmp[ 5] = srcPtr[ 5];
#endif                            
#if BURST >  6                    
                tmp[ 6] = srcPtr[ 6];
#endif                            
#if BURST >  7                    
                tmp[ 7] = srcPtr[ 7];
#endif                            
#if BURST >  8                    
                tmp[ 8] = srcPtr[ 8];
#endif                            
#if BURST >  9                    
                tmp[ 9] = srcPtr[ 9];
#endif                            
#if BURST > 10                    
                tmp[10] = srcPtr[10];
#endif                            
#if BURST > 11                    
                tmp[11] = srcPtr[11];
#endif                            
#if BURST > 12                    
                tmp[12] = srcPtr[12];
#endif                            
#if BURST > 13                    
                tmp[13] = srcPtr[13];
#endif                            
#if BURST > 14                    
                tmp[14] = srcPtr[14];
#endif                            
#if BURST > 15                    
                tmp[15] = srcPtr[15];
#endif


#if BURST >  0
                dstPtr[ 0] = tmp[ 0];
#endif                            
#if BURST >  1                    
                dstPtr[ 1] = tmp[ 1];
#endif                            
#if BURST >  2                    
                dstPtr[ 2] = tmp[ 2];
#endif                            
#if BURST >  3                    
                dstPtr[ 3] = tmp[ 3];
#endif                            
#if BURST >  4                    
                dstPtr[ 4] = tmp[ 4];
#endif                            
#if BURST >  5                    
                dstPtr[ 5] = tmp[ 5];
#endif                            
#if BURST >  6                    
                dstPtr[ 6] = tmp[ 6];
#endif                            
#if BURST >  7                    
                dstPtr[ 7] = tmp[ 7];
#endif                            
#if BURST >  8                    
                dstPtr[ 8] = tmp[ 8];
#endif                            
#if BURST >  9                    
                dstPtr[ 9] = tmp[ 9];
#endif                            
#if BURST > 10                    
                dstPtr[10] = tmp[10];
#endif                            
#if BURST > 11                    
                dstPtr[11] = tmp[11];
#endif                            
#if BURST > 12                    
                dstPtr[12] = tmp[12];
#endif                            
#if BURST > 13                    
                dstPtr[13] = tmp[13];
#endif                            
#if BURST > 14                    
                dstPtr[14] = tmp[14];
#endif                            
#if BURST > 15                    
                dstPtr[15] = tmp[15];
#endif


};