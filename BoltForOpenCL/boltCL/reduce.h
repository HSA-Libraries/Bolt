#pragma once

#include <boltCL/bolt.h>
#include <mutex>
#include <string>
#include <iostream>

namespace boltcl {

	// FIXME - move to cpp file
	struct CallCompiler_Reduce {
		static void constructAndCompile(cl::Kernel *masterKernel,  std::string userCode, std::string valueTypeName,  std::string functorTypeName) {

			const std::string instantiationString = 
				"// Host generates this instantiation string with user-specified value type and functor\n"
				"template __attribute__((mangled_name(reduceInstantiated)))\n"
				"kernel void reduceTemplate(\n"
				"global " + valueTypeName + "* A,\n"
				"const int length,\n"
				"const " + valueTypeName + " init,\n"
				"global " + valueTypeName + "* result,\n"
				"global " + functorTypeName + "* userFunctor,\n"
				"local " + valueTypeName + "* scratch\n"
				");\n\n";

			boltcl::constructAndCompile(masterKernel, "reduce", instantiationString, userCode, valueTypeName, functorTypeName);

		};
	};


	template<typename T, typename BinaryFunction> 
	T reduce(cl::Buffer A, T init,
		BinaryFunction binary_op, std::string userCode, std::string functorTypeName="")  
	{
			static std::once_flag initOnlyOnce;
			static  cl::Kernel masterKernel;
			// For user-defined types, the user must create a TypeName trait which returns the name of the class - note use of TypeName<>::get to retreive the name here.
			std::call_once(initOnlyOnce, CallCompiler_Reduce::constructAndCompile, &masterKernel, userCode, TypeName<T>::get(), functorTypeName.empty() ? TypeName<BinaryFunction>::get() : functorTypeName);


			// Set up shape of launch grid and buffers:
			// FIXME, read from device attributes.
			int computeUnits     = 20;  // round up if we don't know. 
			int wgPerComputeUnit =  6; 
			int resultCnt = computeUnits * wgPerComputeUnit;
			const int wgSize = 64; 

			cl::Buffer userFunctor(CL_MEM_READ_ONLY|CL_MEM_USE_HOST_PTR, sizeof(binary_op), &binary_op );   // Create buffer wrapper so we can access host parameters.
			//std::cout << "sizeof(Functor)=" << sizeof(binary_op) << std::endl;
			cl::Buffer result(CL_MEM_WRITE_ONLY, sizeof(T) * resultCnt);


		
			cl::Kernel k = masterKernel;  // hopefully create a copy of the kernel. FIXME, doesn't work.

			int sz = (int)A.getInfo<CL_MEM_SIZE>();  // FIXME - remove typecast.  Kernel only can handle 32-bit size...

			k.setArg(0, A);
			k.setArg(1, sz);
			k.setArg(2, init);
			k.setArg(3, result);
			k.setArg(4, userFunctor);
			cl::LocalSpaceArg loc;
            loc.size_ = wgSize*sizeof(T);
			k.setArg(5, loc);

			 // FIXME.  Need to ensure global size is a multiple of local WG size ,etc.

			cl::CommandQueue::getDefault().enqueueNDRangeKernel(
				k, 
				cl::NullRange, 
				cl::NDRange(resultCnt * wgSize), 
				cl::NDRange(wgSize));

			// FIXME - replace with map:
			std::vector<T> outputArray(resultCnt);
            cl::enqueueReadBuffer(result, true, 0, sizeof(T)*resultCnt, outputArray.data());
            T acc = init;            
            for(int i = 0; i < resultCnt; ++i){
                acc = binary_op(outputArray[i], acc);
            }
			return acc;
	};


	//---
	// Function definition that accepts iterators and converts to buffers.
	template<typename T, typename InputIterator, typename BinaryFunction> 
	T reduce(InputIterator first1, InputIterator last1,  T init,
		BinaryFunction binary_op, const std::string userCode, std::string functorTypeName="")  
	{

			typedef std::iterator_traits<InputIterator>::value_type T;
			size_t sz = (int)(last1 - first1); 

			// FIXME - use host pointers and map/unmap for host pointers, since they are only written once.
			cl::Buffer A(CL_MEM_READ_ONLY, sizeof(T) * sz);
			cl::enqueueWriteBuffer(A, false, 0, sizeof(T) * sz, &*first1);

			return  reduce(A, init, binary_op, userCode, functorTypeName);

	};
};
