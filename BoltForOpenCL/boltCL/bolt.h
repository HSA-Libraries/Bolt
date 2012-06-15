#pragma once


#define __CL_ENABLE_EXCEPTIONS
#define CL_USE_DEPRECATED_OPENCL_1_1_APIS
#include <CL/cl.hpp>

#include <string>

namespace clbolt {
	extern std::string fileToString(const std::string &fileName);
	extern cl::Kernel compileFunctor(const std::string &kernelCodeString, const std::string kernelName);
	extern void constructAndCompile(cl::Kernel *masterKernel, const std::string &apiName, const std::string instantiationString, std::string userCode, std::string valueTypeName,  std::string functorTypeName);
};




//---
// TypeName trait implementation - this is the base version that is typically overridden
template <typename T>
struct TypeName
{
    static std::string get()
    {
		return std::string("ERROR (bolt): Unknown typename; define missing TypeName<") + typeid(T).name() + ">";  
    }
};


//---
// ClCode trait implementation - this is the base version that is typically overridden to asscociate OpenCL code with this class.
template <typename T>
struct ClCode
{
    static std::string get()
    {
		return "";
    }
};


// Create a template trait to return a string version of the class name.  Usage:
// class MyClass {...};
// BOLT_CREATE_TYPENAME(MyClass);
#define BOLT_CREATE_TYPENAME(T) template<> struct TypeName<T> { static std::string get() { return #T; }};
#define BOLT_CREATE_CLCODE(T,CODE_STRING) template<> struct ClCode<T> { static std::string get() { return CODE_STRING; }};


BOLT_CREATE_TYPENAME(int);
BOLT_CREATE_TYPENAME(float);
BOLT_CREATE_TYPENAME(double);


// Creates a string and a regular version of the functor F, and automatically sets up the ClCode trait to associate the code string with the specified class T 
#define BOLT_FUNCTOR(T, F)  F ; BOLT_CREATE_TYPENAME(T); BOLT_CREATE_CLCODE(T,#F);


// Return a string with the specified function F, and also create code that is fed to the host compiler.
#define BOLT_CODE_STRING(F)  #F; F ; 

