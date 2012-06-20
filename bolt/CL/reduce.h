#pragma once

#include <bolt/CL/bolt.h>
#include <bolt/CL/functional.h>
#include <mutex>
#include <string>
#include <iostream>

namespace clbolt {

	// FIXME - move to cpp file
	struct CallCompiler_Reduce {
		static void constructAndCompile(cl::Kernel *masterKernel,  std::string user_code, std::string valueTypeName,  std::string functorTypeName, const control &c) {

			const std::string instantiationString = 
				"// Host generates this instantiation string with user-specified value type and functor\n"
				"template __attribute__((mangled_name(reduceInstantiated)))\n"
				"__attribute__((reqd_work_group_size(64,1,1)))\n"
				"kernel void reduceTemplate(\n"
				"global " + valueTypeName + "* A,\n"
				"const int length,\n"
				"const " + valueTypeName + " init,\n"
				"global " + functorTypeName + "* userFunctor,\n"
				"global " + valueTypeName + "* result,\n"
				"local " + valueTypeName + "* scratch\n"
				");\n\n";

			clbolt::constructAndCompile(masterKernel, "reduce", instantiationString, user_code, valueTypeName, functorTypeName, c);

		};
	};

	//----
	// This is the base implementation of reduction that is called by all of the convenience wrappers below.
	template<typename T, typename BinaryFunction> 
	T reduce(const clbolt::control &c, cl::Buffer A, T init,
		BinaryFunction binary_op, std::string user_code="")  
	{
		static std::once_flag initOnlyOnce;
		static  cl::Kernel masterKernel;
		// For user-defined types, the user must create a TypeName trait which returns the name of the class - note use of TypeName<>::get to retreive the name here.
		std::call_once(initOnlyOnce, CallCompiler_Reduce::constructAndCompile, &masterKernel, user_code + ClCode<BinaryFunction>::get(), TypeName<T>::get(),  TypeName<BinaryFunction>::get(), c);


		// Set up shape of launch grid and buffers:
		// FIXME, read from device attributes.
		int computeUnits     = c.device().getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>();
		int wgPerComputeUnit =  c.wgPerComputeUnit(); 
		int resultCnt = computeUnits * wgPerComputeUnit;
		const int wgSize = 64; 

		// Create buffer wrappers so we can access the host functors, for read or writing in the kernel
		cl::Buffer userFunctor(c.context(), CL_MEM_USE_HOST_PTR, sizeof(binary_op), &binary_op );   // Create buffer wrapper so we can access host parameters.
		//std::cout << "sizeof(Functor)=" << sizeof(binary_op) << std::endl;
		
		cl::Buffer result(c.context(), CL_MEM_ALLOC_HOST_PTR|CL_MEM_WRITE_ONLY, sizeof(T) * resultCnt);


		cl::Kernel k = masterKernel;  // hopefully create a copy of the kernel. FIXME, doesn't work.

		int szElements = (int)A.getInfo<CL_MEM_SIZE>() / sizeof(T);  // FIXME - remove typecast.  Kernel only can handle 32-bit size...

		k.setArg(0, A);
		k.setArg(1, szElements);
		k.setArg(2, init);
		k.setArg(3, userFunctor);
		k.setArg(4, result);
		cl::LocalSpaceArg loc;
		loc.size_ = wgSize*sizeof(T);
		k.setArg(5, loc);


		c.commandQueue().enqueueNDRangeKernel(
			k, 
			cl::NullRange, 
			cl::NDRange(resultCnt * wgSize), 
			cl::NDRange(wgSize));

		// FIXME - also need to provide a version of this code that does the summation on the GPU, when the buffer is already located there?

		T *h_result = (T*)c.commandQueue().enqueueMapBuffer(result, true, CL_MAP_READ, 0, sizeof(T)*resultCnt);
		T acc = init;            
		for(int i = 0; i < resultCnt; ++i){
			acc = binary_op(h_result[i], acc);
		}
		return acc;
	};



	// Function definition that accepts iterators and converts to buffers.
	// Allows user to specify control structure as first argument.
	template<typename T, typename InputIterator, typename BinaryFunction> 
	T reduce(const clbolt::control &c, InputIterator first1, InputIterator last1,  T init,
		BinaryFunction binary_op, const std::string user_code="")  
	{
		typedef typename std::iterator_traits<InputIterator>::value_type InputType;

		size_t szElements = (int)(last1 - first1); 

		cl::Buffer A(c.context(), CL_MEM_USE_HOST_PTR|CL_MEM_READ_ONLY, sizeof(T) * szElements, const_cast<InputType*>(&*first1));

		return reduce(c, A, init, binary_op, user_code);
	};


	//---
	// Function definition that accepts iterators and converts to buffers.
	// Uses default first argument.
	template<typename T, typename InputIterator, typename BinaryFunction> 
	T reduce(InputIterator first1, InputIterator last1,  T init,
		BinaryFunction binary_op, const std::string user_code="")  
	{
		return reduce(clbolt::control::getDefault(), first1, last1, init, binary_op, user_code);
	};


	//---
	// Function definition that accepts iterators and converts to buffers.
	// Uses default first argument.
	// Uses default init=0 and clbolt::plus as the default operator.
	template<typename InputIterator> 
	typename std::iterator_traits<InputIterator>::value_type
	reduce(const clbolt::control &c, InputIterator first1, InputIterator last1,  
	    typename std::iterator_traits<InputIterator>::value_type init=0,
		const std::string user_code="")  
	{
		typedef typename std::iterator_traits<InputIterator>::value_type InputType;
		return reduce(c, first1, last1, init, clbolt::plus<InputType>(), user_code);
	};


	// Default control, host iterator, default operator
	template<typename InputIterator> 
	typename std::iterator_traits<InputIterator>::value_type
		reduce(InputIterator first1, InputIterator last1, 
		const std::string user_code="")  
	{
		typedef typename std::iterator_traits<InputIterator>::value_type InputType;
		return reduce(clbolt::control::getDefault(), first1, last1, InputType(0), clbolt::plus<InputType>(), user_code);
	};
};

// FIXME - variations:  (specify control vs default control) * (std::vector vs cl::Buffer) * (specify {init, binary_op} vs default {0,"plus"})