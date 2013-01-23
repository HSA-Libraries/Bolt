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

template <typename iType, typename oType, typename binary_function>
kernel
void transformTemplate (global iType* A,
			global iType* B,
			global oType* Z,
			const uint length,
			global binary_function *userFunctor)
{
	int gx = get_global_id (0);
	if (gx >= length)
		return;

	iType aa = A[gx];
	iType bb = B[gx];
	Z[gx] = (*userFunctor)(aa, bb);
}


template <typename iType, typename oType, typename binary_function>
kernel
void transformNoBoundsCheckTemplate (global iType* A,
			global iType* B,
			global oType* Z,
			const uint length,
			global binary_function *userFunctor)
{
	int gx = get_global_id (0);  // * _BOLT_UNROLL; 

	iType aa0 = A[gx+0];
	//iType aa1 = A[gx+1];

	iType bb0 = B[gx+0];
	//iType bb1 = B[gx+1];

	Z[gx+0] = (*userFunctor)(aa0, bb0);
	//Z[gx+1] = (*userFunctor)(aa1, bb1);
}


template <typename iType, typename oType, typename unary_function>
kernel
void unaryTransformTemplate(global iType* A,
			global oType* Z,
			const uint length,
			global unary_function *userFunctor)
{
	int gx = get_global_id (0);
	if (gx >= length)
		return;

	iType aa = A[gx];
	Z[gx] = (*userFunctor)(aa); 
}

template <typename iType, typename oType, typename unary_function>
kernel
void unaryTransformNoBoundsCheckTemplate(global iType* A,
			global oType* Z,
			const uint length,
			global unary_function *userFunctor)
{
	int gx = get_global_id (0);  //  *_BOLT_UNROLL;

	iType aa0 = A[gx+0];
	//iType aa1 = A[gx+1];

	oType z0 = (*userFunctor)(aa0); 
	//oType z1 = (*userFunctor)(aa1); 

	Z[gx+0] = z0;
	//Z[gx+1] = z1;
}

#define BURST_SIZE 16

template <typename iType, typename oType, typename unary_function>
kernel
void unaryTransformA (
    global iType* input,
    global oType* output,
    const uint numElements,
    const uint numElementsPerThread,
    global unary_function *userFunctor )
{
	// global pointers
    // __global const iType  *inputBase =  &input[get_global_id(0)*numElementsPerThread];
    // __global oType *const outputBase = &output[get_global_id(0)*numElementsPerThread];

    __private iType  inReg[BURST_SIZE];
    //__private oType outReg[BURST_SIZE];
    //__private unary_function f = *userFunctor;

    // for each burst
    for (int offset = 0; offset < numElementsPerThread; offset+=BURST_SIZE)
    {
        // load burst
        for( int i = 0; i < BURST_SIZE; i++)
        {
            inReg[i]=input[get_global_id(0)*numElementsPerThread+offset+i];
        }
        // compute burst
        //for( int j = 0; j < BURST_SIZE; j++)
        //{
        //    inReg[j]=(*userFunctor)(inReg[j]);
        //}
        // write burst
        for( int k = 0; k < BURST_SIZE; k++)
        {
            output[get_global_id(0)*numElementsPerThread+offset+k]=inReg[k];
        }

    }
    // output[get_global_id(0)] = inReg[BURST_SIZE-1];
}