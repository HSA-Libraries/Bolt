#pragma once


#define __CL_ENABLE_EXCEPTIONS
#define CL_USE_DEPRECATED_OPENCL_1_1_APIS
#include <CL/cl.hpp>

#include <string>

namespace boltcl {
	extern std::string fileToString(const std::string &fileName);
	extern cl::Kernel compileFunctor(const std::string &kernelCodeString, const std::string kernelName);
	extern void constructAndCompile(cl::Kernel *masterKernel, const std::string &apiName, const std::string instantiationString, std::string userCode, std::string valueTypeName,  std::string functorTypeName);
};


// Creates a string and a regular version of the functor - need both the host and the CL def for the class
// The other way to create this is with header files that are included in both host and CL files.
#define BOLT_FUNCTOR(F) #F ;  F


//---
// TypeName trait implementation
template <typename T>
struct TypeName
{
	std::string msg_;

    static const char* get()
    {
		return "ERROR: Unknown typename; define missing TypeName<> missing";

        //return typeid(T).name();  // FIXME - try this on Windows.
    }
};


// Create a template trait to return a string version of the class name.  Usage:
// class MyClass {...};
// CREATE_TYPENAME(MyClass);
#define CREATE_TYPENAME(A) template<> struct TypeName<A> { static const char *get() { return #A; }};


CREATE_TYPENAME(int);
CREATE_TYPENAME(float);
CREATE_TYPENAME(double);

