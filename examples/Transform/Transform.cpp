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

#include <bolt/cl/transform.h>
#include <iostream>
#include <algorithm>  // for testing against STL functions.

BOLT_FUNCTOR(Functor,
struct Functor
{
	float _a;
	Functor(float a) : _a(a) {};
	float operator() (const float &xx, const float &yy)
	{
		return _a * xx + log(yy) + sqrt(xx);
	};
};
);

void transform(int aSize)
{
	std::vector<float> A(aSize), B(aSize), Z1(aSize), Z0(aSize);
	std::vector<float> backup(aSize);

	for (int i=0; i<aSize; i++) {
		A[i] = float(i);
		B[i] = 10000.0f + (float)i;
	}
	backup = B;
	Functor func(10.0);
	std::transform(A.begin(), A.end(), B.begin(), Z0.begin(), func);
	bolt::cl::transform(A.begin(), A.end(), B.begin(), Z1.begin(), func);
	for (int i=0; i<aSize; i++)
	{
		std::cout << "10.0 * " << A[i] << " + log(" << B[i] <<") + sqrt(" << A[i] <<")  =  " << Z1[i] << "\n";
		std::cout << "10.0 * " << A[i] << " + log(" << B[i] <<") + sqrt(" << A[i] <<")  =  " << Z0[i] << "\n";
	}
    return;
};

int main()
{
	transform(32);
    getchar();
	return 0;
}