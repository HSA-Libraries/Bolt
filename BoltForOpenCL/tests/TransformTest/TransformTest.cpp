// TransformTest.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "boltCL/transform.h"
#include "boltCL/functional.h"

#include <iostream>
#include <algorithm>  // for testing against STL functions.




template<typename InputIterator>
int checkResults(std::string msg, InputIterator first1 , InputIterator end1 , InputIterator first2)
{
	int errCnt = 0;
	static const int maxErrCnt = 20;
	int sz = end1-first1 ;
	for (int i=0; i<sz ; i++) {
		if (first1 [i] != first2 [i]) {
			errCnt++;
			if (errCnt < maxErrCnt) {
				std::cout << "MISMATCH " << msg << " STL= " << first1[i] << "  BOLT=" << first2[i] << std::endl;
			} else if (errCnt == maxErrCnt) {
				std::cout << "Max error count reached; no more mismatches will be printed...\n";
			}
		};
	};

	if (errCnt==0) {
		std::cout << " PASSED  " << msg << " Correct on all " << sz << " elements." << std::endl;
	} else {
		std::cout << "*FAILED  " << msg << "mismatch on " << errCnt << " / " << sz << " elements." << std::endl;
	};

	return errCnt;
};



std::string boltCode1 = BOLT_FUNCTOR(
	float op_sum(float x, float y) { return x + y ; };
);



// Test of a body operator which is constructed with a template argument
std::string myplusStr = BOLT_FUNCTOR(
	template<typename T>
struct myplus  
{
	T operator()(const T &lhs, const T &rhs) const {return lhs + rhs;}
};
);
CREATE_TYPENAME(myplus<float>);
CREATE_TYPENAME(myplus<int>);
CREATE_TYPENAME(myplus<double>);



void simpleTransform1(int aSize)
{
	std::string fName = __FUNCTION__ ;
	fName += ":";

	std::vector<float> A(aSize), B(aSize);

	for (int i=0; i<aSize; i++) {
		A[i] = float(i);
		B[i] = 10000.0f + (float)i;
	}


	{
		// Test1: Test case where a user creates a templatized functor "myplus<float>" and passes that to customize the transformation:
		std::vector<float> Z0(aSize), Z1(aSize);
		std::transform(A.begin(), A.end(), B.begin(), Z0.begin(), myplus<float>());

		boltcl::transform(A.begin(), A.end(), B.begin(), Z1.begin(), myplus<float>(), myplusStr);
		checkResults(fName + "myplus<float>", Z0.begin(), Z0.end(), Z1.begin());
	}

	{
		//Test2:  Use a  templatized function from the provided boltcl functional header.  "boltcl::plus<float>"
		std::vector<float> Z0(aSize), Z1(aSize);
		std::transform(A.begin(), A.end(), B.begin(), Z0.begin(), boltcl::plus<float>());

		boltcl::transform(A.begin(), A.end(), B.begin(), Z1.begin(), boltcl::plus<float>(), boltcl::oclcode::plus);
		checkResults(fName + "boltcl::plus<float>", Z0.begin(), Z0.end(), Z1.begin());
	}


#if 0
	{
		// Test4 : try use of a simple binary function "op_sum" created by user.
		// This doesn't work - OpenCL generates an error that "op_sum isn't a type name.  Maybe need to create an function signature rather than "op_sum".
		std::vector<float> Z0(aSize), Z1(aSize);
		std::transform(A.begin(), A.end(), B.begin(), Z0.begin(), op_sum);

		boltcl::transform(A.begin(), A.end(), B.begin(), Z1.begin(), op_sum, boltCode1, "op_sum");
		checkResults(fName + "Inline Binary Function", Z0.begin(), Z0.end(), Z1.begin());
	}
#endif


};




//---
// SimpleTransform2 Demonstrates:
// How to create a C++ functor object and use this to customize the transform library call.

std::string boltCode = BOLT_FUNCTOR(
struct SaxpyFunctor
{
	float _a;
	SaxpyFunctor(float a) : _a(a) {};

	float operator() (const float &xx, const float &yy) 
	{
		return _a * xx + yy;
	};
};
);  // end BOLT_FUNCTOR

// CREATE_TYPENAME is a bolt macro which adds a TypeName trait for the specified class
// The Bolt code uses this typename in the contruction of the kernel code.
CREATE_TYPENAME (SaxpyFunctor);
CREATE_TYPENAME (float);


void transformSaxpy(int aSize)
{
	std::string fName = __FUNCTION__ ;
	fName += ":";

	std::vector<float> A(aSize), B(aSize), Z1(aSize), Z0(aSize);

	for (int i=0; i<aSize; i++) {
		A[i] = float(i);
		B[i] = 10000.0f + (float)i;
	}

	SaxpyFunctor sb(10.0);

	
	std::transform(A.begin(), A.end(), B.begin(), Z0.begin(), sb);
	boltcl::transform(A.begin(), A.end(), B.begin(), Z1.begin(), sb, boltCode );  

	checkResults(fName, Z0.begin(), Z0.end(), Z1.begin());

};





int _tmain(int argc, _TCHAR* argv[])
{
	simpleTransform1(256);

	transformSaxpy(256);
	transformSaxpy(1024);

	return 0;
}

