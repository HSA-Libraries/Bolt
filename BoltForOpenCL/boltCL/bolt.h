#pragma once


#define __CL_ENABLE_EXCEPTIONS
#define CL_USE_DEPRECATED_OPENCL_1_1_APIS
#include <CL/cl.hpp>

#include <string>

namespace bolt {
	extern std::string fileToString(const std::string &fileName);
	extern cl::Kernel compileFunctor(const std::string &kernelCodeString, const std::string kernelName);
};


// Creates a string and a regular version of the functor - need both the host and the CL def for the class
// The other way to create this is with header files that are included in both host and CL files.
#define BOLT_FUNCTOR(F) #F ;  F


//---
// TypeName trait implementation
template <typename T>
struct TypeName
{
    static const char* get()
    {
		return "unknown";
        //return typeid(T).name();  // FIXME - try this on Windows.
    }
};


// Create a template trait to return a string version of the class name.  Usage:
// class MyClass {...};
// CREATE_TYPENAME(MyClass);
#define CREATE_TYPENAME(A) template<> struct TypeName<A> { static const char *get() { return #A; }};
