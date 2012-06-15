// TransformTest.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "clbolt/reduce.h"
#include "clbolt/functional.h"

#include <iostream>
#include <algorithm>  // for testing against STL functions.
#include <numeric>

#include <thread>


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

	checkResult("simpleReduce2", stlReduce, boltReduce);
	//printf ("Sum: stl=%d,  bolt=%d %d\n", stlReduce, boltReduce, boltReduce2);
};




int _tmain(int argc, _TCHAR* argv[])
{
	simpleReduce1(256);

	return 0;
}

