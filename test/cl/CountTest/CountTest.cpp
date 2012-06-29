// TransformTest.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <vector>
#include <algorithm>

#include <bolt/cl/count.h>

//Count with a vector input
void testCount1(int aSize)
{
	std::vector<int> A(aSize);
	for (int i=0; i < aSize; i++) {
		A[i] = i+1;
	};

	bolt::cl::count (A.begin(), A.end(), 37);
};


// Count with an array input:
void testCount2()
{
	const int aSize = 13;
	int A[aSize] = {0, 10, 42, 55, 13, 13, 42, 19, 42, 11, 42, 99, 13};

	size_t count42 = bolt::cl::count (A, A+aSize, 42);
	size_t count13 = bolt::cl::count (A, A+aSize, 13);

	bolt::cl::control::getDefault().debug(bolt::cl::control::debug::Compile);

	std::cout << "Count42=" << count42 << std::endl;
	std::cout << "Count13=" << count13 << std::endl;
	std::cout << "Count7=" << bolt::cl::count (A, A+aSize, 7) << std::endl;
	std::cout << "Count10=" << bolt::cl::count (A, A+aSize, 10) << std::endl;
};



// This breaks the BOLT_CODE_STRING macro - need to move to a #include file or replicate the code.
std::string InRange_CodeString = 
BOLT_CODE_STRING(
template<typename T>
// Functor for range checking.
struct InRange {
	InRange (T low, T high) {
		_low=low;
		_high=high;
	};

	bool operator() (const T& value) { 
		//printf("Val=%4.1f, Range:%4.1f ... %4.1f\n", value, _low, _high); 
		return (value >= _low) && (value <= _high) ; 
	};

	T _low;
	T _high;
};
);
BOLT_CREATE_TYPENAME(InRange<float>);
//BOLT_CREATE_CLCODE(InRange<float>, InRange_CodeString);



void testCountIf(int aSize) 
{
	std::vector<float> A(aSize);
	for (int i=0; i < aSize; i++) {
		A[i] = static_cast<float> (i+1);
	};

	std::cout << "STD Count7..15=" << std::count_if (A.begin(), A.end(), InRange<float>(7,15)) << std::endl;
	std::cout << "BOLT Count7..15=" << bolt::cl::count_if (A.begin(), A.end(), InRange<float>(7,15), InRange_CodeString) << std::endl;
}

int _tmain(int argc, _TCHAR* argv[])
{
	testCount1(100);

	testCount2();

	testCountIf(1024);

	return 0;
}

