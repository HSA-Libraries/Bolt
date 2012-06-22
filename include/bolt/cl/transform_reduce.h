#pragma once

#include <bolt/cl/bolt.h>
#include <mutex>
#include <string>
#include <iostream>

namespace bolt {
	namespace cl {
		// FIXME - move to cpp file
		struct CallCompiler_TransformReduce {
			static void constructAndCompile(::cl::Kernel *masterKernel,  std::string user_code, std::string valueTypeName,  std::string transformFunctorTypeName, std::string reduceFunctorTypeName) {

				const std::string instantiationString = 
					"// Host generates this instantiation string with user-specified value type and functor\n"
					"template __attribute__((mangled_name(transform_reduceInstantiated)))\n"
					"kernel void transform_reduceTemplate(\n"
					"global " + valueTypeName + "* A,\n"
					"const int length,\n"
					"global " + transformFunctorTypeName + "* transformFunctor,\n"
					"const " + valueTypeName + " init,\n"
					"global " + reduceFunctorTypeName + "* reduceFunctor,\n"
					"global " + valueTypeName + "* result,\n"
					"local " + valueTypeName + "* scratch\n"
					");\n\n";

				std::string functorNames = transformFunctorTypeName + " , " + reduceFunctorTypeName; // create for debug message

				bolt::cl::control c = bolt::cl::control::getDefault();  // FIXME- this needs to be passed a parm but we have too many arguments for call_once
				bolt::cl::constructAndCompile(masterKernel, "transform_reduce", instantiationString, user_code, valueTypeName, functorNames, c);

			};
		};


		template<typename T, typename UnaryFunction, typename BinaryFunction> 
		T transform_reduce(const control c, ::cl::Buffer A, UnaryFunction transform_op,
			T init, BinaryFunction reduce_op, const std::string user_code="")
		{
			static std::once_flag initOnlyOnce;
			static  ::cl::Kernel masterKernel;
			unsigned debugMode = 0; //FIXME, use control

			// For user-defined types, the user must create a TypeName trait which returns the name of the class - note use of TypeName<>::get to retreive the name here.
			std::call_once(initOnlyOnce, CallCompiler_TransformReduce::constructAndCompile, &masterKernel, 
				"\n//--user Code\n" + user_code + "\n//---Functions\n" + ClCode<UnaryFunction>::get() + ClCode<BinaryFunction>::get() , 
				TypeName<T>::get(), TypeName<UnaryFunction>::get(), TypeName<BinaryFunction>::get());


			// Set up shape of launch grid and buffers:
			// FIXME, read from device attributes.
			int computeUnits     = 20;  // round up if we don't know. 
			int wgPerComputeUnit =  6; 
			int resultCnt = computeUnits * wgPerComputeUnit;
			const int wgSize = 64; 

			// Create buffer wrappers so we can access the host functors, for read or writing in the kernel
			::cl::Buffer transformFunctor(c.context(), CL_MEM_USE_HOST_PTR, sizeof(transform_op), &transform_op );   
			::cl::Buffer reduceFunctor(c.context(), CL_MEM_USE_HOST_PTR, sizeof(reduce_op), &reduce_op );


			//std::cout << "sizeof(reduce_op functor)=" << sizeof(reduce_op) << std::endl;
			::cl::Buffer result(c.context(), CL_MEM_WRITE_ONLY, sizeof(T) * resultCnt);

			::cl::Kernel k = masterKernel;  // hopefully create a copy of the kernel. FIXME, doesn't work.

			int szElements = (int)A.getInfo<CL_MEM_SIZE>() / sizeof(T);   // FIXME - remove typecast.  Kernel only can handle 32-bit size...

			k.setArg(0, A);
			k.setArg(1, szElements);
			k.setArg(2, transformFunctor);
			k.setArg(3, init);
			k.setArg(4, reduceFunctor);
			k.setArg(5, result);
			::cl::LocalSpaceArg loc;
			loc.size_ = wgSize*sizeof(T);
			k.setArg(6, loc);

			// FIXME.  Need to ensure global size is a multiple of local WG size ,etc.

			c.commandQueue().enqueueNDRangeKernel(
				k, 
				::cl::NullRange, 
				::cl::NDRange(resultCnt * wgSize), 
				::cl::NDRange(wgSize));

			// FIXME - replace with map:
			// FIXME: Note this depends on supplied functor having a version which can be compiled to run on the CPU
			std::vector<T> outputArray(resultCnt);
			c.commandQueue().enqueueReadBuffer(result, true, 0, sizeof(T)*resultCnt, outputArray.data());
			T acc = init;            
			for(int i = 0; i < resultCnt; ++i){
				acc = reduce_op(outputArray[i], acc);
			}
			return acc;
		};


		//---
		// Function definition that accepts iterators and converts to buffers.

		template<typename T, typename InputIterator, typename UnaryFunction, typename BinaryFunction> 
		T transform_reduce(const control &c, InputIterator first1, InputIterator last1,  
			UnaryFunction transform_op, 
			T init,  BinaryFunction reduce_op, const std::string user_code="" )  
		{

			typedef std::iterator_traits<InputIterator>::value_type T;
			size_t szElements = (int)(last1 - first1); 

			// FIXME - use host pointers and map/unmap for host pointers, since they are only written once.
			::cl::Buffer A(c.context(), CL_MEM_READ_ONLY, sizeof(T) * szElements);
			c.commandQueue().enqueueWriteBuffer(A, false, 0, sizeof(T) * szElements, &*first1);

			return  transform_reduce(c, A, transform_op, init, reduce_op, user_code);

		};


		// Wrapper that uses default control class, iterator interface
		template<typename T, typename InputIterator, typename UnaryFunction, typename BinaryFunction> 
		T transform_reduce(InputIterator first1, InputIterator last1,  
			UnaryFunction transform_op, 
			T init,  BinaryFunction reduce_op, const std::string user_code="" )  
		{
			return transform_reduce(bolt::cl::control::getDefault(), first1, last1, transform_op, init, reduce_op, user_code);
		};
	};
};

// FIXME -review use of string vs const string.  Should TypeName<> return a std::string?
// FIXME - add line numbers to pretty-print kernel log file.
// FIXME - experiment with passing functors as objects rather than as parameters.  (Args can't return state to host, but OK?)
