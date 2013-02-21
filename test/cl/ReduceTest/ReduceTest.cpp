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

// TransformTest.cpp : Defines the entry point for the console application.
//
#define OCL_CONTEXT_BUG_WORKAROUND 1

#include "stdafx.h"
#include <bolt/cl/iterator/counting_iterator.h>
#include <bolt/cl/reduce.h>
#include <bolt/cl/functional.h>
#include <bolt/cl/control.h>

#include <iostream>
#include <algorithm>  // for testing against STL functions.
#include <numeric>

#include "common/myocl.h"


extern void testDeviceVector();
extern void testTBB();
// Super-easy windows profiling interface.
// Move to timing infrastructure when that becomes available.
__int64 StartProfile() {
    __int64 begin;
    QueryPerformanceCounter((LARGE_INTEGER*)(&begin));
    return begin;
};

void EndProfile(__int64 start, int numTests, std::string msg) {
    __int64 end, freq;
    QueryPerformanceCounter((LARGE_INTEGER*)(&end));
    QueryPerformanceFrequency((LARGE_INTEGER*)(&freq));
    double duration = (end - start)/(double)(freq);
    printf("%s %6.2fs, numTests=%d %6.2fms/test\n", msg.c_str(), duration, numTests, duration*1000.0/numTests);
};



template<typename T>
void printCheckMessage(bool err, std::string msg, T  stlResult, T boltResult)
{
    if (err) {
        std::cout << "*ERROR ";
    } else {
        std::cout << "PASSED ";
    }

    std::cout << msg << "  STL=" << stlResult << " BOLT=" << boltResult << std::endl;
};

template<typename T>
bool checkResult(std::string msg, T  stlResult, T boltResult)
{
    bool err =  (stlResult != boltResult);
    printCheckMessage(err, msg, stlResult, boltResult);

    return err;
};


// For comparing floating point values:
template<typename T>
bool checkResult(std::string msg, T  stlResult, T boltResult, double errorThresh)
{
    bool err;
    if ((errorThresh != 0.0) && stlResult) {
        double ratio = (double)(boltResult) / (double)(stlResult) - 1.0;
        err = abs(ratio) > errorThresh;
    } else {
        // Avoid div-by-zero, check for exact match.
        err = (stlResult != boltResult);
    }

    printCheckMessage(err, msg, stlResult, boltResult);
    return err;
};



// Simple test case for bolt::reduce:
// Sum together specified numbers, compare against STL::accumulate function.
// Demonstrates:
//    * use of bolt with STL::vector iterators
//    * use of bolt with default plus 
//    * use of bolt with explicit plus argument
void simpleReduce1(int aSize)
{
    std::vector<int> A(aSize);

    for (int i=0; i < aSize; i++) {
        A[i] = i;
    };

    int stlReduce = std::accumulate(A.begin(), A.end(), 0);

    int boltReduce = bolt::cl::reduce(A.begin(), A.end(), 0, bolt::cl::plus<int>());
    //int boltReduce2 = bolt::cl::reduce(A.begin(), A.end());  // same as above...

    checkResult("simpleReduce1", stlReduce, boltReduce);
    //printf ("Sum: stl=%d,  bolt=%d %d\n", stlReduce, boltReduce, boltReduce2);
};


// Demonstrates use of bolt::control structure to control execution of routine.
void simpleReduce_TestControl(int aSize, int numIters, int deviceIndex)
{
    std::vector<int> A(aSize);

    for (int i=0; i < aSize; i++) {
        A[i] = i;
    };

    //  Tests need to be a bit more sophisticated before hardcoding which device to use in a system (my default is device 1); 
    //  this should be configurable on the command line
    // Create an OCL context, device, queue.


	// FIXME - temporarily disable use of new control queue here:
#if OCL_CONTEXT_BUG_WORKAROUND
	::cl::Context myContext = bolt::cl::control::getDefault( ).context( );
    bolt::cl::control c( getQueueFromContext(myContext, CL_DEVICE_TYPE_GPU, 0 )); 
#else
	MyOclContext ocl = initOcl(CL_DEVICE_TYPE_GPU, deviceIndex);
    bolt::cl::control c(ocl._queue);  // construct control structure from the queue.
#endif


    //printContext(c.context());

    c.debug(bolt::cl::control::debug::Compile + bolt::cl::control::debug::SaveCompilerTemps);

    int stlReduce = std::accumulate(A.begin(), A.end(), 0);
    int boltReduce = 0;

    char testTag[2000];
    sprintf_s(testTag, 2000, "simpleReduce_TestControl sz=%d iters=%d, device=%s", aSize, numIters, 
        c.device( ).getInfo<CL_DEVICE_NAME>( ).c_str( ) );

    __int64 start = StartProfile();
    for (int i=0; i<numIters; i++) {
        boltReduce = bolt::cl::reduce( c, A.begin(), A.end(), 0);
    }
    EndProfile(start, numIters, testTag);

    checkResult(testTag, stlReduce, boltReduce);
};


// Demonstrates use of bolt::control structure to control execution of routine.
void simpleReduce_TestSerial(int aSize)
{
    std::vector<int> A(aSize);

    for (int i=0; i < aSize; i++) {
        A[i] = i;
    };


    bolt::cl::control c;  // construct control structure from the queue.
    c.forceRunMode(bolt::cl::control::SerialCpu);

    int stlReduce = std::accumulate(A.begin(), A.end(), 0);
    int boltReduce = 0;

    boltReduce = bolt::cl::reduce(c, A.begin(), A.end(), 0);


    checkResult("TestSerial", stlReduce, boltReduce);
};


void simpleReduce_countingiterator(float start,int size)
{

    bolt::cl::counting_iterator<float> first(start);
    bolt::cl::counting_iterator<float> last = first +  size;

    std::vector<float> a(size);

    for (int i=0; i < size; i++) {
        a[i] = i+start;
    };
    
    float stlReduce = std::accumulate(a.begin(), a.end(), 0.0F);
    float boltReduce = bolt::cl::reduce(first, last, 0.0F);

    checkResult("TestSerial", stlReduce, boltReduce);
};


#if 0
// Disable test since the buffer interface is moving to device_vector.
void reduce_TestBuffer() {
    
    int a[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    cl::Buffer A(CL_MEM_USE_HOST_PTR, sizeof(int) * 10, a); // create a buffer from a.

    int sum = bolt::cl::reduce2<int>(A, 0, bolt::cl::plus<int>()); // note type of date in the buffer ("int") explicitly specified.
};
#endif


int _tmain(int argc, _TCHAR* argv[])
{
#if defined( ENABLE_TBB )
    testTBB( );
#endif

    testDeviceVector();

    int numIters = 100;
    simpleReduce_TestControl(1024000, numIters, 0);

    simpleReduce_TestControl(100, 1, 0);

    simpleReduce1(256);
    simpleReduce1(1024);

    
    simpleReduce_TestControl(100, 1, 0);

    simpleReduce_TestSerial(1000);

    simpleReduce_countingiterator(20.05F,10);
    
    //simpleReduce_TestControl(1024000, numIters, 1); // may fail on systems with only one GPU installed.

    return 0;
}

