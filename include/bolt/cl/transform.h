#pragma once

#include <bolt/cl/bolt.h>
#include <mutex>
#include <string>
#include <iostream>

namespace bolt {
	namespace cl {

		// FIXME - move to cpp file
		struct CallCompiler_Transform {
			static void init_(::cl::Kernel *masterKernel, std::string user_code, std::string valueTypeName,  std::string functorTypeName, const control &c) {

				std::string instantiationString = 
					"// Host generates this instantiation string with user-specified value type and functor\n"
					"template __attribute__((mangled_name(transformInstantiated)))\n"
					"kernel void transformTemplate(\n"
					"global " + valueTypeName + "* A,\n"
					"global " + valueTypeName + "* B,\n"
					"global " + valueTypeName + "* Z,\n"
					"const int length,\n"
					"global " + functorTypeName + "* userFunctor);\n\n";


				bolt::cl::constructAndCompile(masterKernel, "transform", instantiationString, user_code, valueTypeName, functorTypeName, c);

			};
		};


		template<typename T, typename BinaryFunction> 
		void transform2(const control &c, ::cl::Buffer A, ::cl::Buffer B, ::cl::Buffer Z, 
			BinaryFunction f, std::string user_code="")  {

				::cl::Buffer userFunctor(c.context(), CL_MEM_READ_ONLY|CL_MEM_USE_HOST_PTR, sizeof(f), &f );   // Create buffer wrapper so we can access host parameters.
				//std::cout << "sizeof(Functor)=" << sizeof(f) << std::endl;

				static std::once_flag initOnlyOnce;
				static  ::cl::Kernel masterKernel;
				// For user-defined types, the user must create a TypeName trait which returns the name of the class - note use of TypeName<>::get to retreive the name here.
				std::call_once(initOnlyOnce, CallCompiler_Transform::init_, &masterKernel, user_code + ClCode<BinaryFunction>::get(), TypeName<T>::get(), TypeName<BinaryFunction>::get(), c);
				//std::call_once(initOnlyOnce, CallCompiler::init_, &masterKernel, TypeName<T>::get(), functorTypeName.empty() ? TypeName<BinaryFunction>::get() : functorTypeName);

				::cl::Kernel k = masterKernel;  // hopefully create a copy of the kernel.  FIXME, need to create-kernel from the program.
				int sz = (int)A.getInfo<CL_MEM_SIZE>(); // FIXME, ensure size is 32-bits

				k.setArg(0, A);
				k.setArg(1, B);
				k.setArg(2, Z);
				k.setArg(3, sz);
				k.setArg(4, userFunctor);

				const int wgSize = 256; // FIXME.  Need to ensure global size is a multiple of this ,etc.

				c.commandQueue().enqueueNDRangeKernel(
					k, 
					::cl::NullRange, 
					::cl::NDRange(sz), 
					::cl::NDRange(wgSize));
		};


		//---
		// Function definition that accepts iterators and converts to buffers.
		template<typename InputIterator, typename OutputIterator, typename BinaryFunction> 
		void transform(const control &c,  InputIterator first1, InputIterator last1, InputIterator first2, OutputIterator result, 
			BinaryFunction f, const std::string user_code="")  {

				typedef std::iterator_traits<InputIterator>::value_type T;
				int sz = (int)(last1 - first1); // FIXME - use size_t

				// Use host pointers memory since these arrays are only read once - no benefit to copying.
				::cl::Buffer A(c.context(), CL_MEM_USE_HOST_PTR|CL_MEM_READ_ONLY,  sizeof(T) * sz, const_cast<T*>(&*first1));
				::cl::Buffer B(c.context(), CL_MEM_USE_HOST_PTR|CL_MEM_READ_ONLY,  sizeof(T) * sz, const_cast<T*>(&*first2));

				::cl::Buffer Z(c.context(), CL_MEM_USE_HOST_PTR|CL_MEM_WRITE_ONLY, sizeof(T) * sz, &*result);

				transform2<T> (c, A, B, Z, f, user_code);

				c.commandQueue().enqueueMapBuffer(Z, true, CL_MAP_READ | CL_MAP_WRITE, 0/*offset*/, sz);
		};




		//---
		// convenience functions that accept control structure as first parameter:

		// default control, two-input transform, buffer inputs
		template<typename T, typename BinaryFunction> 
		void transform(::cl::Buffer A, ::cl::Buffer B, ::cl::Buffer Z, 
			BinaryFunction f, std::string user_code="")  
		{
			transform2<T>(control::getDefault(), A, B, Z, f, user_code);
		}


		// default control, two-input transform, std:: iterator
		template<typename InputIterator, typename OutputIterator, typename BinaryFunction> 
		void transform( InputIterator first1, InputIterator last1, InputIterator first2, OutputIterator result, 
			BinaryFunction f, const std::string user_code="")  
		{
			transform(control::getDefault(), first1, last1, first2, result, f, user_code);
		};
	};
};
