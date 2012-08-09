// TransformTest.cpp : Defines the entry point for the console application.
//

#include <bolt/cl/sort.h>
#include <bolt/cl/functional.h>
#include <bolt/cl/control.h>

#include <iostream>
#include <algorithm>  // for testing against STL functions.
#include <vector>
#include "myocl.h"


bool myfunction (int i,int j) { return (i<j); }

struct myclass {
  bool operator() (int i,int j) { return (i<j);}
} myobject;


int main(int argc, char* argv[])
{
    int myints[] = {32,71,12,45,26,80,53,33};
    std::vector<int> stdvector (myints, myints+8);               // 32 71 12 45 26 80 53 33
    std::vector<int> boltvector (myints, myints+8);               // 32 71 12 45 26 80 53 33
    std::vector<int>::iterator it;
    int aSize =1024;

    //std::vector<int> boltvector(aSize);

	//for (int i=0; i < aSize; i++) {
	//	boltvector[i] = i;
	//}; 
	MyOclContext ocl = initOcl(CL_DEVICE_TYPE_GPU, 0);
	bolt::cl::control c(ocl._queue);  // construct control structure from the queue.

    // using default comparison (operator <):
    std::sort (stdvector.begin(), stdvector.begin()+4);           //(12 32 45 71)26 80 53 33
    bolt::cl::sort(boltvector.begin(), boltvector.end()+4);           //(12 32 45 71)26 80 53 33
    // using function as comp
    std::sort (stdvector.begin()+4, stdvector.end(), myfunction); // 12 32 45 71(26 33 53 80)
    bolt::cl::sort (boltvector.begin()+4, boltvector.end(), bolt::cl::maximum<int>()); // 12 32 45 71(26 33 53 80)
    // using object as comp
    std::sort (stdvector.begin(), stdvector.end(), myobject);     //(12 26 32 33 45 53 71 80)
    //bolt::cl::sort (c, boltvector.begin(), boltvector.end(), myobject);     //(12 26 32 33 45 53 71 80)
    // print out content:
    std::cout << "std::sort vector contains:";
    for (it=stdvector.begin(); it!=stdvector.end(); ++it)
        std::cout << " " << *it;
    std::cout << std::endl;
    std::cout << "bolt::cl::sort vector contains:";
    for (it=boltvector.begin(); it!=boltvector.end(); ++it)
        std::cout << " " << *it;

    std::cout << std::endl;
    getchar();
	return 0;
}

