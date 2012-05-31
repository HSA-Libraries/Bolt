#include "stdafx.h"
#include <stdio.h>

#include <numeric>
#include <limits>
#include <bolt/functional.h>
#include <bolt/scan.h>

#include <list>	// For debugging purposes, to prove that we can reject lists

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

// Simple test case for bolt::inclusive_scan:
// Sum together specified numbers, compare against STL::partial_sum function.
// Demonstrates:
//    * use of bolt with STL::array iterators
//    * use of bolt with default plus 
//    * use of bolt with explicit plus argument
void simpleScanArray( )
{
	const unsigned int arraySize = 64;

	std::array< int, arraySize > stdA, boltA;
	std::array< int, arraySize > stdB, boltB;

	for (int i=0; i < arraySize; i++) {
//		stdA[i] = i;
//		boltA[i] = 1;
		stdA[i] = 1;
		boltA[i] = 1;
	};

	//Out-of-place
	std::partial_sum( stdA.begin( ), stdA.end( ), stdB.begin( ) );
	bolt::inclusive_scan( boltA.begin( ), boltA.end( ), boltB.begin( ) );

	//In-place
	std::partial_sum( stdB.begin( ), stdB.end( ), stdB.begin( ) );
	bolt::inclusive_scan( boltB.begin( ), boltB.end( ), boltB.begin( ) );

	// Binary operator
	//bolt::inclusive_scan( boltA.begin( ), boltA.end(), boltA.begin( ), bolt::plus<int>( ) );

	// Invalid calls
	//bolt::inclusive_scan( boltA.rbegin( ), boltA.rend( ) );  // reverse iterators should not be supported

	//printf ("Sum: stl=%d,  bolt=%d %d %d\n", stlScan, boltScan, boltScan2, boltScan3 );
};


//// Simple test case for bolt::inclusive_scan:
//// Sum together specified numbers, compare against STL::partial_sum function.
//// Demonstrates:
////    * use of bolt with STL::vector iterators
////    * use of bolt with default plus 
////    * use of bolt with explicit plus argument
//void simpleScan1(const int vSize)
//{
//	std::vector<int> V(vSize);
//	std::vector<bool> B(vSize);
//	std::list<int> L;
//
//	for (int i=0; i < vSize; i++) {
//		V[i] = i;
//		B[i] = false;
//	};
//
//	//	Don't need to test a big list, as it's slow to destroy itself in debug build and interface should reject anyway
//	L.push_back( 0 );
//	L.push_back( 1 );
//	L.push_back( 2 );
//
//
//	int stlScan = std::partial_sum(V.begin(), V.end(), 0);
//
//	int boltScan = bolt::inclusive_scan(V.begin(), V.end(), 0, bolt::plus<int>());
//	int boltScan2 = bolt::inclusive_scan(V.begin(), V.end());  // same as above...
//	int boltScan3 = bolt::inclusive_scan(V.rbegin(), V.rend());  // reverse iterators should not be supported
//	int boltScanList = bolt::inclusive_scan(L.begin(), L.end());  // bi-directional iterators should not be supported
//	int boltScanBool = bolt::inclusive_scan(B.begin(), B.end());  // std::vector<bool> iterators should not be supported
//
//	printf ("Sum: stl=%d,  bolt=%d %d %d %d %d\n", stlScan, boltScan, boltScan2, boltScan3, boltScanList, boltScanBool );
//};
//
//
//
//// Demonstrates:
////   * Call bolt:inclusive_scan to compute maximum and minimum - same call works for multiple functions
////   * Pass regular C-style array rather than iterator to bolt::inclusive_scan function call.
////   * Define a user-specified functor object  - see 'scanner', used to compute max
////   * Bolt lamda will work once C++AMP bug is fixed.
//void simpleScan2()
//{
//	static const int aSize=160000;
//	int A[aSize];
//
//	srand(200);
//	for (int i=0; i < aSize; i++) {
//		A[i] = rand() + 100;
//		//std::cout << A[i] << " ";
//	};
//
//	std::string fName(__FUNCTION__ );   // return function name
//	fName += ":";  
//
//	//---
//	// Lambda version of 
//	int stlScanLamda = std::partial_sum(A, A+aSize, 0, [=] (int x, int y) {
//		return x>=y ? x:y;
//	});
//
//	int boltScanLamda = bolt::inclusive_scan(A, A+aSize, 0, [=] (int x, int y) restrict(cpu,amp) {
//		return x>=y ? x:y;
//	});
//	checkResult(fName + "LamdaMax", stlScanLamda, boltScanLamda);
//
//	//----
//	// Functor class:
//
//	struct scanner {
//		int operator() (int x, int y) restrict(cpu,amp) {
//			return y>x ? y:x; 
//		};
//	};
//	int stlScan2 = std::partial_sum(A, A+aSize, 0, scanner());
//
//
//	int boltScan1 = bolt::inclusive_scan(A, A+aSize, 0, scanner());
//	checkResult(fName + "maxFunctor", stlScan2, boltScan1);
//
//	int boltScan2 = bolt::inclusive_scan(A, A+aSize, 0, bolt::maximum<int>());
//	checkResult(fName + "bolt::maximum", stlScan2, boltScan1);
//
//
//
//	//int maxInt = std::numeric_limits<int>::max();
//	int maxInt = 32767;
//	checkResult("Min",
//		std::partial_sum(A, A+aSize, maxInt, bolt::minimum<int>()),  // use bolt::minimum here since std:: doesn't define minimum
//		bolt::inclusive_scan   (A, A+aSize, maxInt, bolt::minimum<int>()) );
//};
//
//// Demonstrates:
//// * Use of other types with reduction - ie float.
//// * Accepts template parameters to allow more test cases to be run through the code.
//template <typename T, typename BinaryFunction>
//void simpleScan3(const char *name, BinaryFunction binary_function, int aSize, double errorThresh=0.0)
//{
//	std::vector<T> A(aSize);
//
//	srand(200);
//	for (int i=0; i < aSize; i++) {
//		A[i] = static_cast<T> (rand() + 100);
//		//std::cout << A[i] << " ";
//	};
//
//	T init(0); 
//	T stlRes  = std::partial_sum(A.begin(), A.end(), init, binary_function);
//	T boltRes = bolt::inclusive_scan   (A.begin(), A.end(), init, binary_function);
//
//
//	std::string fName(__FUNCTION__ );   // return function name
//	fName += ":";  
//
//
//	// Note - due to floating point ordering, we can get slightly different answers...
//	checkResult (fName+ name, stlRes, boltRes, errorThresh);
//};
//
//#if 1
//// Demonstrates:
////   * Use of bolt::inclusive_scan with user-specified class object.
//
//
//struct Complex {
//	// No constructors allowed in types used in tiled memory:
//	//Complex(const float x=0.0, const float y=0.0) RESTRICT_AMP_CPU : _x(x), _y(y)   {};	
//
//	void set(const float x=0.0, const float y=0.0) {
//		_x = x;
//		_y = y; 
//	};
//
//	bool operator!=(const Complex &c) {
//		return (c._x != _x) || (c._y != _y);
//	};
//
//	friend std::ostream& operator<< (std::ostream &out,  Complex &c);
//
//	float _x;
//	float _y;
//};
//
//std::ostream& operator<< (std::ostream &out,  Complex &c) {
//	out << "(" << c._x  << " , " << c._y << ")";
//	return out;
//};
//
//
//void simpleScan4()
//{
//	static int const aSize = 1024;
//	Complex AA[aSize];
//	for (int i=0; i < aSize; i++) {
//		AA[i].set(float(i), float(i*2.0 + 7.0));
//	};
//
//	struct scanner {
//		Complex operator() (const Complex &a, const Complex &b) restrict(amp,cpu){
//			Complex t;
//			t._x = a._x + b._x;
//			t._y = a._y + b._y;
//			return t;
//		};
//	};
//
//	Complex init;  init.set(0.0,0.0);
//	Complex stlScan1  = std::partial_sum(AA, AA+aSize, init, scanner());
//	Complex boltScan1 = bolt::inclusive_scan   (AA, AA+aSize, init, scanner());
//
//	std::string fName(__FUNCTION__ );   // return function name
//	fName += ":";  
//	checkResult(fName+"ComplexWithFunctor", stlScan1, boltScan1);
//
//
//#if 1
//	Complex stlScan2 = std::partial_sum(AA, AA+aSize, init, [=] (Complex a, Complex b) -> Complex {
//		Complex t;
//		t._x = a._x + b._x;
//		t._y = a._y + b._y;
//		return t;
//	});
//
//	Complex boltScan2 = bolt::inclusive_scan(AA, AA+aSize, init, [=]  (Complex a, Complex b) restrict(cpu,amp)  -> Complex  {
//		Complex t;
//		t._x = a._x + b._x;
//		t._y = a._y + b._y;
//		return t;
//	});
//#endif
//
//	checkResult(fName+"ComplexWithLamda", stlScan2, boltScan2);
//	//printf ("stlScan=%f.%f,  boltScan=%f.%f\n", stlScan._x, stlScan._y, boltScan._x, boltScan._y);
//}
//
//
//#endif
//

// Test driver function
void simpleScan()
{
	simpleScanArray( );
	//simpleScan1(1024);
	//simpleScan1(1024000);
	//simpleScan2();

	//simpleScan3<float> ("sum", bolt::plus<float>(), 1000000, .0001/*errorThresh*/);
	//simpleScan3<float> ("min", bolt::minimum<float>(), 1000000);
	//simpleScan3<float> ("max", bolt::maximum<float>(), 1000000);

	//simpleScan4();
};


int _tmain(int argc, _TCHAR* argv[])
{
	simpleScan();
	return 0;
}