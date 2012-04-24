// TransformTest.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "boltCL/transform.h"
#include "boltCL/functional.h"

#include <iostream>
#include <algorithm>  // for testing against STL functions.

#include <thread>




template<typename InputIterator1, typename InputIterator2>
int checkResults(std::string msg, InputIterator1 first1 , InputIterator1 end1 , InputIterator2 first2)
{
	int errCnt = 0;
	static const int maxErrCnt = 20;
	size_t sz = end1-first1 ;
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


void singleThreadReduction(const std::vector<float> &A, const std::vector<float> &B, std::vector<float> *Zbolt, int aSize) 
{
	boltcl::transform(A.begin(), A.end(), B.begin(), Zbolt->begin(), boltcl::multiplies<float>(), boltcl::oclcode::multiplies);
};


void multiThreadReductions(int aSize, int iters)
{
	std::string fName = __FUNCTION__ ;
	fName += ":";

	std::vector<float> A(aSize), B(aSize);
	for (int i=0; i<aSize; i++) {
		A[i] = float(i);
		B[i] = 10000.0f + (float)i;
	}


	{
		std::vector<float> Z0(aSize), Z1(aSize);
		std::transform(A.begin(), A.end(), B.begin(), Z0.begin(), boltcl::minus<float>()); // golden answer:

		// show we only compile it once:
		for (int i=0; i<iters; i++) {
			boltcl::transform(A.begin(), A.end(), B.begin(), Z1.begin(), boltcl::minus<float>(), boltcl::oclcode::minus);
			checkResults(fName + "MultiIteration - boltcl::minus<float>", Z0.begin(), Z0.end(), Z1.begin());
		};
	}

	// Now try multiple threads:
	// FIXME - multi-thread doesn't work until we fix up the kernel to be per-thread.
	if (0) {
		static const int threadCount = 4;
		std::vector<float> Z0(aSize);
		std::transform(A.begin(), A.end(), B.begin(), Z0.begin(), boltcl::multiplies<float>()); // golden answer:

		std::vector<float> ZBolt [threadCount];
		for (int i=0; i< threadCount; i++){
			ZBolt[i].resize(aSize);
		};

		std::thread t0(singleThreadReduction, A, B, &ZBolt[0], aSize);
		std::thread t1(singleThreadReduction,  A, B, &ZBolt[1], aSize);
		std::thread t2(singleThreadReduction,  A, B, &ZBolt[2], aSize);
		std::thread t3(singleThreadReduction, A, B,  &ZBolt[3], aSize);


		t0.join();
		t1.join();
		t2.join();
		t3.join();

		for (int i=0; i< threadCount; i++){
			checkResults(fName + "MultiThread", Z0.begin(), Z0.end(), ZBolt[i].begin());
		};
	}
};


void oclTransform(int aSize)
{
	std::vector<float> A(aSize), B(aSize);
	for (int i=0; i<aSize; i++) {
		A[i] = float(i);
		B[i] = 1000.0f + (float)i;
	}
	std::vector<float> Z0(aSize);
	std::transform(A.begin(), A.end(), B.begin(), Z0.begin(), boltcl::plus<float>()); // golden answer:

	int bufSize = aSize * sizeof(float);
	cl::Buffer bufferA(CL_MEM_READ_ONLY|CL_MEM_USE_HOST_PTR, bufSize, A.data());
	//cl::Buffer bufferA(begin(A), end(A), true);
	cl::Buffer bufferB(CL_MEM_READ_ONLY|CL_MEM_USE_HOST_PTR, bufSize, B.data());
	cl::Buffer bufferZ(CL_MEM_WRITE_ONLY, sizeof(float) * aSize);

	boltcl::transform<float>(bufferA, bufferB, bufferZ, boltcl::plus<float>(), boltcl::oclcode::plus);

	float * zMapped = static_cast<float*> (cl::CommandQueue::getDefault().enqueueMapBuffer(bufferZ, true, CL_MAP_READ | CL_MAP_WRITE, 0/*offset*/, bufSize));

	std::string fName = __FUNCTION__ ;
	fName += ":";

	checkResults(fName + "oclBuffers", Z0.begin(), Z0.end(), zMapped);
};



int _tmain(int argc, _TCHAR* argv[])
{
	simpleTransform1(256);

	transformSaxpy(256);
	transformSaxpy(1024);

	multiThreadReductions(1024, 10);

	oclTransform(1024);

	return 0;
}

