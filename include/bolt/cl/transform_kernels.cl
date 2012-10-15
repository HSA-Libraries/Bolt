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
			const int length,
			global binary_function *userFunctor)
{
	int gx = get_global_id (0);

	if (gx >= length)
		return;
	else
	{
		iType aa = A[gx];
		iType bb = B[gx];
		Z[gx] = (*userFunctor)(aa, bb);
	}
}


template <typename iType, typename oType, typename binary_function>
kernel
void transformNoBoundsCheckTemplate (global iType* A,
			global iType* B,
			global oType* Z,
			const int length,
			global binary_function *userFunctor)
{
	int gx = get_global_id (0);

	iType aa = A[gx];
	iType bb = B[gx];
	Z[gx] = (*userFunctor)(aa, bb);
}


template <typename iType, typename oType, typename unary_function>
kernel
void unaryTransformTemplate(global iType* A,
			global oType* Z,
			const int length,
			global unary_function *userFunctor)
{
	int gx = get_global_id (0);

	if (gx >= length)
		return;
	else
	{ 
		iType aa = A[gx];
		Z[gx] = (*userFunctor)(aa); 
	}
}
