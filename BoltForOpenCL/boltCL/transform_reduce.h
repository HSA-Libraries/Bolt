#pragma once

#include <boltCL/bolt.h>
#include <mutex>
#include <string>
#include <iostream>

namespace boltcl {

	// FIXME - move to cpp file
	struct CallCompiler_TransformReduce {
		static void constructAndCompile(cl::Kernel *masterKernel,  std::string userCode, std::string valueTypeName,  std::string transformFunctorTypeName, std::string reduceFunctorTypeName) {

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

			boltcl::constructAndCompile(masterKernel, "transform_reduce", instantiationString, userCode, valueTypeName, functorNames);

		};
	};


	template<typename T, typename UnaryFunction, typename BinaryFunction> 
	T transform_reduce(cl::Buffer A, UnaryFunction transform_op, const std::string &transform_op_str,
             T init, BinaryFunction reduce_op, const std::string &reduce_op_str)
	{
			static std::once_flag initOnlyOnce;
			static  cl::Kernel masterKernel;
			// For user-defined types, the user must create a TypeName trait which returns the name of the class - note use of TypeName<>::get to retreive the name here.
			std::call_once(initOnlyOnce, CallCompiler_TransformReduce::constructAndCompile, &masterKernel, transform_op_str + reduce_op_str, TypeName<T>::get(), TypeName<UnaryFunction>::get(), TypeName<BinaryFunction>::get());


			// Set up shape of launch grid and buffers:
			// FIXME, read from device attributes.
			int computeUnits     = 20;  // round up if we don't know. 
			int wgPerComputeUnit =  6; 
			int resultCnt = computeUnits * wgPerComputeUnit;
			const int wgSize = 64; 

            // Create buffer wrappers so we can access the host functors, for read or writing in the kernel
			cl::Buffer transformFunctor(CL_MEM_USE_HOST_PTR, sizeof(transform_op), &transform_op );   
			cl::Buffer reduceFunctor(CL_MEM_USE_HOST_PTR, sizeof(reduce_op), &reduce_op );   // Create buffer wrapper so we can access host parameters.


			//std::cout << "sizeof(reduce_op functor)=" << sizeof(reduce_op) << std::endl;
			cl::Buffer result(CL_MEM_WRITE_ONLY, sizeof(T) * resultCnt);

			cl::Kernel k = masterKernel;  // hopefully create a copy of the kernel. FIXME, doesn't work.

			int sz = (int)A.getInfo<CL_MEM_SIZE>();  // FIXME - remove typecast.  Kernel only can handle 32-bit size...

			k.setArg(0, A);
			k.setArg(1, sz);
			k.setArg(2, transformFunctor);
			k.setArg(3, init);
			k.setArg(4, reduceFunctor);
			k.setArg(5, result);
			cl::LocalSpaceArg loc;
            loc.size_ = wgSize*sizeof(T);
			k.setArg(6, loc);

			 // FIXME.  Need to ensure global size is a multiple of local WG size ,etc.

			cl::CommandQueue::getDefault().enqueueNDRangeKernel(
				k, 
				cl::NullRange, 
				cl::NDRange(resultCnt * wgSize), 
				cl::NDRange(wgSize));

			// FIXME - replace with map:
			// FIXME: Note this depends on supplied functor having a version which can be compiled to run on the CPU
			std::vector<T> outputArray(resultCnt);
            cl::enqueueReadBuffer(result, true, 0, sizeof(T)*resultCnt, outputArray.data());
            T acc = init;            
            for(int i = 0; i < resultCnt; ++i){
                acc = reduce_op(outputArray[i], acc);
            }
			return acc;
	};


	//---
	// Function definition that accepts iterators and converts to buffers.
    
	template<typename T, typename InputIterator, typename UnaryFunction, typename BinaryFunction> 
	T transform_reduce(InputIterator first1, InputIterator last1,  
		UnaryFunction transform_op, const std::string transform_op_str,
		T init,  BinaryFunction reduce_op, const std::string reduce_op_str)  
	{

			typedef std::iterator_traits<InputIterator>::value_type T;
			size_t sz = (int)(last1 - first1); 

			// FIXME - use host pointers and map/unmap for host pointers, since they are only written once.
			cl::Buffer A(CL_MEM_READ_ONLY, sizeof(T) * sz);
			cl::enqueueWriteBuffer(A, false, 0, sizeof(T) * sz, &*first1);

			return  transform_reduce(A, transform_op, transform_op_str, init, reduce_op, reduce_op_str);

	};
};

// FIXME -review use of string vs const string.  Should TypeName<> return a std::string?
// FIXME - add line numbers to pretty-print kernel log file.
// FIXME - experiment with passing functors as objects rather than as parameters.  (Args can't return state to host, but OK?)
