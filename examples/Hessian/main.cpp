// ConsoleApplication1.cpp : Defines the entry point for the console application.
//

//#include "stdafx.h"

#include <iostream>
#include <vector>

#include "hessian.h"



#define __CL_ENABLE_EXCEPTIONS
#define CL_USE_DEPRECATED_OPENCL_1_1_APIS
#include <CL/cl.h>
#include <CL/cl.hpp>




int main(int argc, char* argv[])
{
	
	//runHessian("..\\inputs\\marina.bmp", 100, p_runHessian);
	runHessian(argc, argv);

	printAllLOC();


	return 0;
}
