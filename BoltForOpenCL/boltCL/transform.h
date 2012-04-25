#pragma once

#include <boltCL/bolt.h>
#include <mutex>
#include <string>
#include <iostream>

namespace boltcl {

	// FIXME - move to cpp file
	struct CallCompiler{
		//static void init_(cl::Kernel *masterKernel,  std::string userCode, std::string &valueTypeName, const std::string &functorTypeName) {
		static void init_(cl::Kernel *masterKernel, std::string userCode, std::string valueTypeName,  std::string functorTypeName) {


			//FIXME, when this becomes more stable move the kernel code to a string in bolt.cpp
			// Note unfortunate dependency here on relative file path of run directory and location of boltcl dir.
			std::string transformFunctionString = boltcl::fileToString( "transform_kernels.cl" ); 

			std::string instantiationString = 
				"// Host generates this instantiation string with user-specified value type and functor\n"
				"template __attribute__((mangled_name(transformInstantiated)))\n"
				"kernel void transformTemplate(\n"
				"global " + valueTypeName + "* A,\n"
				"global " + valueTypeName + "* B,\n"
				"global " + valueTypeName + "* Z,\n"
				"const int length,\n"
				"global " + functorTypeName + "* userFunctor);\n\n";

			std::string codeStr = userCode + "\n\n" + transformFunctionString +   instantiationString;

			if (1) {
				std::cout << "Compiling...\n";
				std::cout << "ValueType  ='" << valueTypeName << "'" << std::endl;
				std::cout << "FunctorType='" << functorTypeName << "'" << std::endl;

				//std::cout << "code=" << std::endl << codeStr;
			}

			*masterKernel = boltcl::compileFunctor(codeStr, "transformInstantiated");
		};
	};


	template<typename T, typename BinaryFunction> 
	void transform(cl::Buffer A, cl::Buffer B, cl::Buffer Z, 
		BinaryFunction f, std::string userCode, std::string functorTypeName="")  {


			cl::Buffer userFunctor(CL_MEM_READ_ONLY|CL_MEM_USE_HOST_PTR, sizeof(f), &f );   // Create buffer wrapper so we can access host parameters.
			//std::cout << "sizeof(Functor)=" << sizeof(f) << std::endl;

			static std::once_flag initOnlyOnce;
			static  cl::Kernel masterKernel;
			// For user-defined types, the user must create a TypeName trait which returns the name of the class - note use of TypeName<>::get to retreive the name here.
			std::call_once(initOnlyOnce, CallCompiler::init_, &masterKernel, userCode, TypeName<T>::get(), functorTypeName.empty() ? TypeName<BinaryFunction>::get() : functorTypeName);
			//std::call_once(initOnlyOnce, CallCompiler::init_, &masterKernel, TypeName<T>::get(), functorTypeName.empty() ? TypeName<BinaryFunction>::get() : functorTypeName);

			cl::Kernel k = masterKernel;  // hopefully create a copy of the kernel.  FIXME, need to create-kernel from the program.
			int sz = (int)A.getInfo<CL_MEM_SIZE>(); // FIXME, ensure size is 32-bits

			k.setArg(0, A);
			k.setArg(1, B);
			k.setArg(2, Z);
			k.setArg(3, sz);
			k.setArg(4, userFunctor);

			const int wgSize = 256; // FIXME.  Need to ensure global size is a multiple of this ,etc.

			cl::CommandQueue::getDefault().enqueueNDRangeKernel(
				k, 
				cl::NullRange, 
				cl::NDRange(sz), 
				cl::NDRange(wgSize));

	};


	//---
	// Function definition that accepts iterators and converts to buffers.
	template<typename InputIterator, typename OutputIterator, typename BinaryFunction> 
	void transform(InputIterator first1, InputIterator last1, InputIterator first2, OutputIterator result, 
		BinaryFunction f, const std::string userCode, std::string functorTypeName="")  {

			typedef std::iterator_traits<InputIterator>::value_type T;
			int sz = (int)(last1 - first1); // FIXME - use size_t

			// FIXME - use host pointers and map/unmap for host pointers, since they are only written once.
			cl::Buffer A(CL_MEM_READ_ONLY, sizeof(T) * sz);
			cl::enqueueWriteBuffer(A, false, 0, sizeof(T) * sz, &*first1);
			cl::Buffer B(CL_MEM_READ_ONLY, sizeof(T) * sz);
			cl::enqueueWriteBuffer(B, false, 0, sizeof(T) * sz, &*first2);

			cl::Buffer Z(CL_MEM_WRITE_ONLY|CL_MEM_USE_HOST_PTR, sizeof(T) * sz, &*result);

			transform<T> (A, B, Z, f, userCode, functorTypeName);

			// FIXME - mapBuffer returns pointer
			cl::CommandQueue::getDefault().enqueueMapBuffer(Z, true, CL_MAP_READ | CL_MAP_WRITE, 0/*offset*/, sz);
	};
};
