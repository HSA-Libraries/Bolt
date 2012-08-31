#include <bolt/cl/bolt.h>
#include <bolt/cl/transform.h>
#include <algorithm>

#include "utils.h"

// Instantiate host-side version of the saxpy functor:
#include "saxpy_functor.h"

std::string mycode = bolt::cl::fileToString("saxpy_functor.h");
BOLT_CREATE_TYPENAME(SaxpyFunctor);
BOLT_CREATE_CLCODE(SaxpyFunctor, mycode);



void readFromFileTest()
{
	std::string fName = __FUNCTION__ ;
	fName += ":";



	const int sz=2000;

	SaxpyFunctor s(100);
	std::vector<float> x(sz); // initialization not shown
	std::vector<float> y(sz); // initialization not shown
	std::vector<float> z(sz);
	bolt::cl::transform(x.begin(), x.end(), y.begin(), z.begin(), s);

	std::vector<float> stdZ(sz);
	std::transform(x.begin(), x.end(), y.begin(), stdZ.begin(), s);

	checkResults(fName, stdZ.begin(), stdZ.end(), z.begin());
};