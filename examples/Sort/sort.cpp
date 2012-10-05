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


#include <bolt/cl/sort.h>
#include "myocl.h"

#include <vector>
#include <numeric>

template <typename T>
struct MyType {
    T a;

    bool operator() (const MyType& lhs, const MyType& rhs) {
        return (lhs.a > rhs.a);
    }
    bool operator < (const MyType& other) const {
        return (a < other.a);
    }
    bool operator > (const MyType& other) const {
        return (a > other.a);
    }
    MyType(const MyType &other)
        : a(other.a) { }
    MyType()
        : a(0) { }
    MyType(T& _in)
        : a(_in) { }
};

BOLT_CREATE_TYPENAME(MyType<int>);
BOLT_CREATE_TYPENAME(bolt::cl::greater< MyType<int> >);
BOLT_CREATE_CLCODE(MyType<int>, "template <typename T> struct MyType { T a; \n\nbool operator() (const MyType& lhs, const MyType& rhs) { return (lhs.a > rhs.a); } \n\nbool operator < (const MyType& other) const { return (a < other.a); }\n\n bool operator > (const MyType& other) const { return (a > other.a);} \n\n };");


int main()
{
	//Usage with basic vector implementation.
	int length = 256;
	std::vector<int> input(1024);
	std::generate(input.begin(), input.end(), rand);
	bolt::cl::sort( input.begin(), input.end(), bolt::cl::greater<int>());

    //Usage with Array types.
	int a[8] = {2, 9, 3, 7, 5, 6, 3, 8};
	bolt::cl::sort( a, a+8, bolt::cl::greater<int>());


	std::vector<int> boltInput(length);
	std::generate(boltInput.begin(), boltInput.end(), rand);
	// sort using the bolt::cl::control and device_vector
	MyOclContext ocl = initOcl(CL_DEVICE_TYPE_GPU, 0);
	bolt::cl::control c(ocl._queue);  // construct control structure from the queue.
    //Create a device_vector
	bolt::cl::device_vector<int> dvInput( boltInput.begin(), boltInput.end(), CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE, c);
	bolt::cl::sort(c, dvInput.begin(), dvInput.end());

	// Usage with user defined data types.
	typedef MyType<int> mytype;
	std::vector<mytype> myTypeBoltInput1(length);
	std::vector<mytype> myTypeBoltInput2(length);
    for (int i=0;i<length;i++)
    {
        myTypeBoltInput1[i].a= (int)(i +2);
        myTypeBoltInput2[i].a= (int)(i +2);
    }
    bolt::cl::sort(c, myTypeBoltInput1.begin(), myTypeBoltInput1.end(),bolt::cl::greater<mytype>());
    //OR
    bolt::cl::sort(c, myTypeBoltInput1.begin(), myTypeBoltInput1.end(),mytype());
	return 0;
}