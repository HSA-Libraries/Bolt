// TransformTest.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "boltCL/transform.h"

#include <iostream>
#include <algorithm>  // for testing against STL functions.


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

CREATE_TYPENAME (SaxpyFunctor);
CREATE_TYPENAME (float);


void simpleTransform2(int aSize)
{


	std::vector<float> A(aSize), B(aSize), Z1(aSize), Z0(aSize);

	for (int i=0; i<aSize; i++) {
		A[i] = float(i);
		B[i] = 10000.0f + (float)i;
	}

	SaxpyFunctor sb(10.0);

	std::cout << std::endl  ;
	std::cout << "info: running simpleTransform2 test" << std::endl;

	std::transform(A.begin(), A.end(), B.begin(), Z0.begin(), sb);
	bolt::transform2(A.begin(), A.end(), B.begin(), Z1.begin(), sb, boltCode );  

	int warningCnt = 0;
	for (unsigned i=0; i<Z0.size(); i++) {
		if (Z0[i] != Z1[i]) {
			if (++warningCnt < 20) {
				std::cout << "MISMATCH! Element:" << i << "STL=" << Z0[i] << " , BOLT=" << Z1[i] << std::endl;
			}
		}
		//std::cout << i << ":" << Z0[i] << " , " << Z1[i] << std::endl;
	}

	std::cout << "Detected " << warningCnt << " errors / aSize=" << aSize << std::endl;

};





int _tmain(int argc, _TCHAR* argv[])
{
	simpleTransform2(256);
	simpleTransform2(1024);

	return 0;
}

