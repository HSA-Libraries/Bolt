#include "stdafx.h"
#include <stdio.h>

#include <numeric>
#include <limits>
#include <bolt/AMP/functional.h>
#include <bolt/AMP/reduce.h>

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

// Simple test case for bolt::reduce:
// Sum together specified numbers, compare against STL::accumulate function.
// Demonstrates:
//    * use of bolt with STL::array iterators
//    * use of bolt with default plus 
//    * use of bolt with explicit plus argument
void simpleReduceArray( )
{
	const unsigned int arraySize = 4096;

	std::array< int, arraySize > A;

	for (int i=0; i < arraySize; i++) {
		A[i] = i;
	};

	int stlReduce = std::accumulate(A.begin(), A.end(), 0);

	int boltReduce = bolt::reduce(A.begin(), A.end(), 0, bolt::plus<int>());
	int boltReduce2 = bolt::reduce(A.begin(), A.end());  // same as above...
	int boltReduce3 = bolt::reduce(A.rbegin(), A.rend());  // reverse iterators should not be supported

	printf ("Sum: stl=%d,  bolt=%d %d %d\n", stlReduce, boltReduce, boltReduce2, boltReduce3 );
};


// Simple test case for bolt::reduce:
// Sum together specified numbers, compare against STL::accumulate function.
// Demonstrates:
//    * use of bolt with STL::vector iterators
//    * use of bolt with default plus 
//    * use of bolt with explicit plus argument
void simpleReduce1(const int vSize)
{
	std::vector<int> V(vSize);
	std::vector<bool> B(vSize);
	std::list<int> L;

	for (int i=0; i < vSize; i++) {
		V[i] = i;
		B[i] = false;
	};

	//	Don't need to test a big list, as it's slow to destroy itself in debug build and interface should reject anyway
	L.push_back( 0 );
	L.push_back( 1 );
	L.push_back( 2 );


	int stlReduce = std::accumulate(V.begin(), V.end(), 0);

	int boltReduce = bolt::reduce(V.begin(), V.end(), 0, bolt::plus<int>());
	int boltReduce2 = bolt::reduce(V.begin(), V.end());  // same as above...
	int boltReduce3 = bolt::reduce(V.rbegin(), V.rend());  // reverse iterators should not be supported
	int boltReduceList = bolt::reduce(L.begin(), L.end());  // bi-directional iterators should not be supported
	int boltReduceBool = bolt::reduce(B.begin(), B.end());  // std::vector<bool> iterators should not be supported

	printf ("Sum: stl=%d,  bolt=%d %d %d %d %d\n", stlReduce, boltReduce, boltReduce2, boltReduce3, boltReduceList, boltReduceBool );
};



// Demonstrates:
//   * Call bolt:reduce to compute maximum and minimum - same call works for multiple functions
//   * Pass regular C-style array rather than iterator to bolt::reduce function call.
//   * Define a user-specified functor object  - see 'reducer', used to compute max
//   * Bolt lamda will work once C++AMP bug is fixed.
void simpleReduce2()
{
	static const int aSize=160000;
	int A[aSize];

	srand(200);
	for (int i=0; i < aSize; i++) {
		A[i] = rand() + 100;
		//std::cout << A[i] << " ";
	};

	std::string fName(__FUNCTION__ );   // return function name
	fName += ":";  

	//---
	// Lambda version of 
	int stlReduceLamda = std::accumulate(A, A+aSize, 0, [=] (int x, int y) {
		return x>=y ? x:y;
	});

	int boltReduceLamda = bolt::reduce(A, A+aSize, 0, [=] (int x, int y) restrict(cpu,amp) {
		return x>=y ? x:y;
	});
	checkResult(fName + "LamdaMax", stlReduceLamda, boltReduceLamda);

	//----
	// Functor class:

	struct reducer {
		int operator() (int x, int y) restrict(cpu,amp) {
			return y>x ? y:x; 
		};
	};
	int stlReduce2 = std::accumulate(A, A+aSize, 0, reducer());


	int boltReduce1 = bolt::reduce(A, A+aSize, 0, reducer());
	checkResult(fName + "maxFunctor", stlReduce2, boltReduce1);

	int boltReduce2 = bolt::reduce(A, A+aSize, 0, bolt::maximum<int>());
	checkResult(fName + "bolt::maximum", stlReduce2, boltReduce1);



	//int maxInt = std::numeric_limits<int>::max();
	int maxInt = 32767;
	checkResult("Min",
		std::accumulate(A, A+aSize, maxInt, bolt::minimum<int>()),  // use bolt::minimum here since std:: doesn't define minimum
		bolt::reduce   (A, A+aSize, maxInt, bolt::minimum<int>()) );
};

// Demonstrates:
// * Use of other types with reduction - ie float.
// * Accepts template parameters to allow more test cases to be run through the code.
template <typename T, typename BinaryFunction>
void simpleReduce3(const char *name, BinaryFunction binary_function, int aSize, double errorThresh=0.0)
{
	std::vector<T> A(aSize);

	srand(200);
	for (int i=0; i < aSize; i++) {
		A[i] = static_cast<T> (rand() + 100);
		//std::cout << A[i] << " ";
	};

	T init(0); 
	T stlRes  = std::accumulate(A.begin(), A.end(), init, binary_function);
	T boltRes = bolt::reduce   (A.begin(), A.end(), init, binary_function);


	std::string fName(__FUNCTION__ );   // return function name
	fName += ":";  


	// Note - due to floating point ordering, we can get slightly different answers...
	checkResult (fName+ name, stlRes, boltRes, errorThresh);
};

#if 1
// Demonstrates:
//   * Use of bolt::reduce with user-specified class object.


struct Complex {
	// No constructors allowed in types used in tiled memory:
	//Complex(const float x=0.0, const float y=0.0) RESTRICT_AMP_CPU : _x(x), _y(y)   {};	

	void set(const float x=0.0, const float y=0.0) {
		_x = x;
		_y = y; 
	};

	bool operator!=(const Complex &c) {
		return (c._x != _x) || (c._y != _y);
	};

	friend std::ostream& operator<< (std::ostream &out,  Complex &c);

	float _x;
	float _y;
};

std::ostream& operator<< (std::ostream &out,  Complex &c) {
	out << "(" << c._x  << " , " << c._y << ")";
	return out;
};


void simpleReduce4()
{
	static int const aSize = 1024;
	Complex AA[aSize];
	for (int i=0; i < aSize; i++) {
		AA[i].set(float(i), float(i*2.0 + 7.0));
	};

	struct reducer {
		Complex operator() (const Complex &a, const Complex &b) restrict(amp,cpu){
			Complex t;
			t._x = a._x + b._x;
			t._y = a._y + b._y;
			return t;
		};
	};

	Complex init;  init.set(0.0,0.0);
	Complex stlReduce1  = std::accumulate(AA, AA+aSize, init, reducer());
	Complex boltReduce1 = bolt::reduce   (AA, AA+aSize, init, reducer());

	std::string fName(__FUNCTION__ );   // return function name
	fName += ":";  
	checkResult(fName+"ComplexWithFunctor", stlReduce1, boltReduce1);


#if 1
	Complex stlReduce2 = std::accumulate(AA, AA+aSize, init, [=] (Complex a, Complex b) -> Complex {
		Complex t;
		t._x = a._x + b._x;
		t._y = a._y + b._y;
		return t;
	});

	Complex boltReduce2 = bolt::reduce(AA, AA+aSize, init, [=]  (Complex a, Complex b) restrict(cpu,amp)  -> Complex  {
		Complex t;
		t._x = a._x + b._x;
		t._y = a._y + b._y;
		return t;
	});
#endif

	checkResult(fName+"ComplexWithLamda", stlReduce2, boltReduce2);
	//printf ("stlReduce=%f.%f,  boltReduce=%f.%f\n", stlReduce._x, stlReduce._y, boltReduce._x, boltReduce._y);
}


#endif


// Test driver function
void simpleReduce()
{
	simpleReduceArray( );
	simpleReduce1(1024);
	simpleReduce1(1024000);
	simpleReduce2();

	simpleReduce3<float> ("sum", bolt::plus<float>(), 1000000, .0001/*errorThresh*/);
	simpleReduce3<float> ("min", bolt::minimum<float>(), 1000000);
	simpleReduce3<float> ("max", bolt::maximum<float>(), 1000000);

	simpleReduce4();
};


int _tmain(int argc, _TCHAR* argv[])
{
	simpleReduce();
	return 0;
}