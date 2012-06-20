// TransformTest.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "clbolt/transform_reduce.h"
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



// Simple test case for clbolt::transform_reduce:
// Perform a sum-of-squares
// Demonstrates:
//  * Use of transform_reduce function - takes two separate functors, one for transform and one for reduce.
//  * Performs a useful operation - squares each element, then adds them together
//  * Use of transform_reduce bolt is more efficient than two separate function calls due to fusion - 
//        After transform is applied, the result is immediately reduced without being written to memory.
//        Note the STL version uses two function calls - transform followed by accumulate.
//  * Operates directly on host buffers.
std::string squareMeCode = BOLT_CODE_STRING(
	template<typename T>
struct SquareMe {
	T operator() (const T &x ) const { return x*x; } ;
};
);
BOLT_CREATE_TYPENAME(SquareMe<int>);
BOLT_CREATE_TYPENAME(SquareMe<float>);



template<typename T>
void sumOfSquares(int aSize)
{
	std::vector<T> A(aSize), Z(aSize);

	for (int i=0; i < aSize; i++) {
		A[i] = i+1;
	};


	// For STL, perform the operation in two steps - transform then reduction:
	std::transform(A.begin(), A.end(), Z.begin(), SquareMe<T>());
	int stlReduce = std::accumulate(Z.begin(), Z.end(), 0);

	int boltReduce = clbolt::transform_reduce(A.begin(), A.end(), SquareMe<T>(), 0, 
                                              clbolt::plus<int>(), squareMeCode);

	checkResult(__FUNCTION__, stlReduce, boltReduce);
};




int _tmain(int argc, _TCHAR* argv[])
{
	sumOfSquares<int>(256);

	return 0;
}

