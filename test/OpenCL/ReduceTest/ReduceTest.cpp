// TransformTest.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "clbolt/reduce.h"
#include "clbolt/functional.h"
#include "clbolt/control.h"

#include <iostream>
#include <algorithm>  // for testing against STL functions.
#include <numeric>

#include <thread>

#include "myocl.h"


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

	int boltReduce = clbolt::reduce(A.begin(), A.end(), 0, clbolt::plus<int>());
	//int boltReduce2 = clbolt::reduce(A.begin(), A.end());  // same as above...

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

	// Create an OCL context, device, queue.
	MyOclContext ocl = initOcl(CL_DEVICE_TYPE_GPU, deviceIndex);
	clbolt::control c(ocl._queue);  // construct control structure from the queue.
	//printContext(c.context());

	c.debug(clbolt::control::debug::Compile + clbolt::control::debug::SaveCompilerTemps);

	int stlReduce = std::accumulate(A.begin(), A.end(), 0);
	int boltReduce = 0;

	__int64 start = StartProfile();
	for (int i=0; i<numIters; i++) {
		boltReduce = clbolt::reduce(c, A.begin(), A.end());
	}
	EndProfile(start, numIters, "TestControl");

	checkResult("simpleReduce_TestControl", stlReduce, boltReduce);
};


int _tmain(int argc, _TCHAR* argv[])
{


	simpleReduce_TestControl(100, 1, 0);

	simpleReduce1(256);
	simpleReduce1(1024);

	int numIters = 100;
	simpleReduce_TestControl(100, 1, 0);
	simpleReduce_TestControl(1024000, numIters, 0);
	simpleReduce_TestControl(1024000, numIters, 1); // may fail on systems with only one GPU installed.

	return 0;
}

