#include "stdafx.h"
#include <stdio.h>

#include <numeric>
#include <limits>
#include <bolt/functional.h>
#include <bolt/reduce.h>


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

	int boltReduce = bolt::reduce(A.begin(), A.end(), 0, bolt::plus<int>());
	int boltReduce2 = bolt::reduce(A.begin(), A.end());  // same as above...

	printf ("Sum: stl=%d,  bolt=%d %d\n", stlReduce, boltReduce, boltReduce2);
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

	int stlReduce = std::accumulate(A, A+aSize, 0, [=] (int x, int y) {
		return x>=y ? x:y;
	});


	// Lambda no worky in developer preview AMP:
#if 0
	int boltReduce = bolt::reduce(A, A+aSize, 0, [=] (int x, int y) RESTRICT_AMP {
		return x>=y ? x:y;
	});
#endif

	struct reducer {
		int _dummy ; // workaround dev preview bug that does not allow empty classes
		int operator() (int x, int y) RESTRICT_AMP_CPU{
			return y>x ? y:x; 
		};
	};

	int stlReduce2 = std::accumulate(A, A+aSize, 0, reducer());

	int boltReduce = bolt::reduce(A, A+aSize, 0, reducer());
	int boltReduce2 = bolt::reduce(A, A+aSize, 0, bolt::maximum<int>());


	printf ("Max: stl=%d, %d,  bolt=%d %d\n", stlReduce, stlReduce2, boltReduce, boltReduce2);

	//int maxInt = std::numeric_limits<int>::max();
	int maxInt = 32767;
	printf ("Min: stl=%d,  bolt=%d\n", 
		std::accumulate(A, A+aSize, maxInt, bolt::minimum<int>()),  // use bolt::minimum here since std:: doesn't define minimum
		bolt::reduce   (A, A+aSize, maxInt, bolt::minimum<int>()) );
};

// Demonstrates:
// * Use of other types with reduction - ie float.
// * Accepts template parameters to allow more test cases to be run through the code.
template <typename T, typename BinaryFunction>
void simpleReduce3(const char *name, BinaryFunction binary_function, int aSize)
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

	std::cout <<  name << ":  stl =" << stlRes << "  bolt=" << boltRes << std::endl;

	// Note - due to floating point ordering, we can get slightly different answers...
	if (stlRes != boltRes) {
		std::cout << "ERROR! STL != BOLT\n";
	};
};

#if 0
// Demonstrates:
//   * Use of bolt::reduce with user-specified class object.
// Crashes FXC
void simpleReduce4()
{
	struct Complex {
		// No constructors allowed in types used in tiled memory:
		//Complex(const float x=0.0, const float y=0.0) RESTRICT_AMP_CPU : _x(x), _y(y)   {};	

		void set(const float x=0.0, const float y=0.0) {
			_x = x;
			_y = y; 
		};

		float _x;
		float _y;
	};

	static int const aSize = 1024;
	Complex AA[aSize];
	for (int i=0; i < aSize; i++) {
		AA[i].set(float(i), float(i*2.0 + 7.0));
	};

	struct reducer {
		int _dummy ; // workaround dev preview bug that does not allow empty classes
		Complex operator() (const Complex &a, const Complex &b) RESTRICT_AMP_CPU{
			Complex t;
			t._x = a._x + b._x;
			t._y = a._y + b._y;
			return t;
		};
	};

	Complex init;  init.set(0.0,0.0);
	Complex stlReduce  = std::accumulate(AA, AA+aSize, init, reducer());
	Complex boltReduce = bolt::reduce   (AA, AA+aSize, init, reducer());


	// No lamda for C++AMP:
#if 0
	Complex stlReduce = std::accumulate(AA, AA+aSize, init, [=] (Complex a, Complex b) -> Complex {
		Complex t;
		t._x = a._x + b._x;
		t._y = a._y + b._y;
		return t;
	});

	Complex boltReduce = bolt::reduce(AA, AA+aSize, init, [=]  (Complex a, Complex b) RESTRICT_AMP_CPU  -> Complex  {
		Complex t;
		t._x = a._x + b._x;
		t._y = a._y + b._y;
		return t;
	});
#endif

	printf ("stlReduce=%f.%f,  boltReduce=%f.%f\n", stlReduce._x, stlReduce._y, boltReduce._x, boltReduce._y);
}
#endif


// Test driver function
void simpleReduce()
{
	simpleReduce1(1024);
	simpleReduce1(1024000);
	simpleReduce2();

	simpleReduce3<float> ("sum", bolt::plus<float>(), 1000000);
	simpleReduce3<float> ("min", bolt::minimum<float>(), 1000000);
	simpleReduce3<float> ("max", bolt::maximum<float>(), 1000000);
};


int _tmain(int argc, _TCHAR* argv[])
{
	simpleReduce();
	return 0;
}