#pragma once


#define __CL_ENABLE_EXCEPTIONS
#define CL_USE_DEPRECATED_OPENCL_1_1_APIS
#include <CL/cl.hpp>

#include <string>

#include <bolt/cl/control.h>
#include <bolt/cl/clcode.h>

namespace bolt {
	namespace cl {
		extern std::string fileToString(const std::string &fileName);
		extern ::cl::Kernel compileFunctor(const std::string &kernelCodeString, const std::string kernelName, const std::string compileOptions, const control &c);
		extern void constructAndCompile(::cl::Kernel *masterKernel, const std::string &apiName, const std::string instantiationString, std::string userCode, std::string valueTypeName,  std::string functorTypeName, const control &c);
	};
};





BOLT_CREATE_TYPENAME(int);
BOLT_CREATE_TYPENAME(float);
BOLT_CREATE_TYPENAME(double);
