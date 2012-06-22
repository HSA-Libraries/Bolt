#pragma once

#include <bolt/cl/bolt.h>
#include <bolt/cl/functional.h>
#include <mutex>
#include <string>
#include <iostream>

namespace bolt {
	namespace cl {
        

        /*! \addtogroup algorithms
         */

		/*! \addtogroup reductions
        *   \ingroup algorithms
        *   Family of operations for reductions for boiling data down to a small set by summation, counting, finding min/max, and more.
        */

		/*! \addtogroup reduce
        *   \ingroup reductions
        *   \{
        */


		// FIXME - move to cpp file
		struct CallCompiler_Reduce {
			static void constructAndCompile(::cl::Kernel *masterKernel,  std::string cl_code, std::string valueTypeName,  std::string functorTypeName, const control &ctl) {

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

				bolt::cl::constructAndCompile(masterKernel, "reduce", instantiationString, cl_code, valueTypeName, functorTypeName, ctl);

			};
		};



		/*! \p reduce returns the result of combining all the elements in the specified range using the specified binary_op.  
		* The classic example is a summation, where the binary_op is the plus operator.  By default, 
		* the binary operator is "plus<>()".  
		*
		* This version accepts an OpenCL(TM) buffer that should be reduced.  Note the type of the data must be specified by explicitly 
		* instantiating the template, ie /p reduce<int>(...).  All elements in the buffer are reduced.
		*
		* \p reduce requires that the binary reduction op ("binary_op") is cummutative.  The order in which \p reduce applies the binary_op
		* is not deterministic.
		*
		* The \p reduce operation is similar the std::accumulate function.  See http://www.sgi.com/tech/stl/accumulate.html.
		*
		* \param ctl Control structure to control command-queue, debug, tuning. See FIXME.
		* \param A   The buffer that should be reduced.
		* \param init  The initial value for the accumulator.
		* \param binary_op  The binary operation used to combine two values.   By default, the binary operation is plus<>().
		* \param cl_code (optional) OpenCL(TM) code to be passed to the OpenCL compiler. The cl_code is inserted before the binary_op and the reduction kernel template. 
		* \return The result of the reduction.
		*

		*
		* \code
		* #include <bolt/cl/reduce.h>
		*
		* int a[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
		* cl::Buffer A(CL_MEM_USE_HOST_PTR, sizeof(int) * 10, a); // create a buffer from a.
		*
		* int sum = bolt::cl::reduce<int>(ctl, A, ); // note type of date in the buffer ("int") explicitly specified.
		* // sum = 55
		*  \endcode
		*/

		//----
		// This is the base implementation of reduction that is called by all of the convenience wrappers below.
		template<typename T, typename BinaryFunction> 
		T reduce(const bolt::cl::control &ctl, ::cl::Buffer A, T init,
			BinaryFunction binary_op, std::string cl_code="")  
		{
			static std::once_flag initOnlyOnce;
			static  ::cl::Kernel masterKernel;
			// For user-defined types, the user must create a TypeName trait which returns the name of the class - note use of TypeName<>::get to retreive the name here.
			std::call_once(initOnlyOnce, CallCompiler_Reduce::constructAndCompile, &masterKernel, cl_code + ClCode<BinaryFunction>::get(), TypeName<T>::get(),  TypeName<BinaryFunction>::get(), ctl);


			// Set up shape of launch grid and buffers:
			// FIXME, read from device attributes.
			int computeUnits     = ctl.device().getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>();
			int wgPerComputeUnit =  ctl.wgPerComputeUnit(); 
			int resultCnt = computeUnits * wgPerComputeUnit;
			const int wgSize = 64; 

			// Create buffer wrappers so we can access the host functors, for read or writing in the kernel
			::cl::Buffer userFunctor(ctl.context(), CL_MEM_USE_HOST_PTR, sizeof(binary_op), &binary_op );   // Create buffer wrapper so we can access host parameters.
			//std::cout << "sizeof(Functor)=" << sizeof(binary_op) << std::endl;

			::cl::Buffer result(ctl.context(), CL_MEM_ALLOC_HOST_PTR|CL_MEM_WRITE_ONLY, sizeof(T) * resultCnt);


			::cl::Kernel k = masterKernel;  // hopefully create a copy of the kernel. FIXME, doesn't work.

			int szElements = (int)A.getInfo<CL_MEM_SIZE>() / sizeof(T);  // FIXME - remove typecast.  Kernel only can handle 32-bit size...

			k.setArg(0, A);
			k.setArg(1, szElements);
			k.setArg(2, init);
			k.setArg(3, userFunctor);
			k.setArg(4, result);
			::cl::LocalSpaceArg loc;
			loc.size_ = wgSize*sizeof(T);
			k.setArg(5, loc);


			ctl.commandQueue().enqueueNDRangeKernel(
				k, 
				::cl::NullRange, 
				::cl::NDRange(resultCnt * wgSize), 
				::cl::NDRange(wgSize));

			// FIXME - also need to provide a version of this code that does the summation on the GPU, when the buffer is already located there?

			T *h_result = (T*)ctl.commandQueue().enqueueMapBuffer(result, true, CL_MAP_READ, 0, sizeof(T)*resultCnt);
			T acc = init;            
			for(int i = 0; i < resultCnt; ++i){
				acc = binary_op(h_result[i], acc);
			}
			return acc;
		};


		//FIXME - figure how to remove this use of reduce2
		template<typename T, typename BinaryFunction> 
		T reduce2(::cl::Buffer A, T init,
			BinaryFunction binary_op, std::string cl_code="")  
		{
			return reduce(bolt::cl::control::getDefault(), A, init, binary_op, cl_code);
		}




		/*! \p reduce returns the result of combining all the elements in the specified range using the specified binary_op.  
		* The classic example is a summation, where the binary_op is the plus operator.  By default, 
		* the binary operator is "plus<>()".  The version takes a bolt::cl::control structure as a first argument.
		*
		* \p reduce requires that the binary reduction op ("binary_op") is cummutative.  The order in which \p reduce applies the binary_op
		* is not deterministic.
		*
		* The \p reduce operation is similar the std::accumulate function.  See http://www.sgi.com/tech/stl/accumulate.html.
		*
		* \param ctl Control structure to control command-queue, debug, tuning. See FIXME.
		* \param first The first position in the sequence to be reduced.
		* \param last  The last position in the sequence to be reduced.
		* \param init  The initial value for the accumulator.
		* \param binary_op  The binary operation used to combine two values.   By default, the binary operation is plus<>().
		* \param cl_code Optional OpenCL(TM) code to be passed to the OpenCL compiler. The cl_code is inserted first in the generated code, before the cl_code trait.
		* \return The result of the reduction.
		*
		* The following code example shows the use of \p reduce to find the max of 10 numbers, 
		* specifying a specific command-queue and enabling debug messages.
		* \code
		* #include <bolt/cl/reduce.h>
		*
		* int a[10] = {2, 9, 3, 7, 5, 6, 3, 8, 3, 4};
		*
		* cl::CommandQueue myCommandQueue = ...
		*
		* bolt::cl::control ctl(myCommandQueue); // specify an OpenCL(TM) command queue to use for executing the reduce.
		* ctl.debug(bolt::cl::control::debug::SaveCompilerTemps); // save IL and ISA files for generated kernel
		*
		* int max = bolt::cl::reduce(ctl, a, a+10, -1, bolt::cl:maximum<int>());
		* // max = 9
		*  \endcode
		*/
		template<typename InputIterator, typename T, typename BinaryFunction> 
		T reduce(const bolt::cl::control &ctl,
			InputIterator first, 
			InputIterator last,  
			T init,
			BinaryFunction binary_op=bolt::cl::plus<T>(), 
			const std::string cl_code="")  
		{
			typedef typename std::iterator_traits<InputIterator>::value_type T;

			size_t szElements = (int)(last - first); 

			::cl::Buffer A(ctl.context(), CL_MEM_USE_HOST_PTR|CL_MEM_READ_ONLY, sizeof(T) * szElements, const_cast<T*>(&*first));

			return reduce(ctl, A, init, binary_op, cl_code);
		};




		/*! \p reduce returns the result of combining all the elements in the specified range using the specified binary_op.  
		* The classic example is a summation, where the binary_op is the plus operator.  By default, the initial value is "0" 
		* and the binary operator is "plus<>()".
		*
		* \p reduce requires that the binary reduction op ("binary_op") is cummutative.  The order in which \p reduce applies the binary_op
		* is not deterministic.
		*
		* The \p reduce operation is similar the std::accumulate function.  See http://www.sgi.com/tech/stl/accumulate.html.
		*
		* \param ctl Control structure to control command-queue, debug, tuning. See FIXME.
		* \param first The first position in the sequence to be reduced.
		* \param last  The last position in the sequence to be reduced.
		* \param cl_code Optional OpenCL(TM) code to be passed to the OpenCL compiler. The cl_code is inserted first in the generated code, before the cl_code trait.
		* \return The result of the reduction.
		*
		* The following code example shows the use of \p reduce to sum 10 numbers, using the default plus operator.
		* \code
		* #include <bolt/cl/reduce.h>
		*
		* int a[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
		*
		* cl::CommandQueue myCommandQueue = ...
		*
		* bolt::cl::control ctl(myCommandQueue); // specify an OpenCL(TM) command queue to use for executing the reduce.
		* ctl.debug(bolt::cl::control::debug::SaveCompilerTemps); // save IL and ISA files for generated kernel
		*
		* int sum = bolt::cl::reduce(ctl, a, a+10);
		* // sum = 55
		*  \endcode
		*/
		template<typename InputIterator> 
		typename std::iterator_traits<InputIterator>::value_type
			reduce(const bolt::cl::control &ctl,
			InputIterator first, 
			InputIterator last, 
			const std::string cl_code="")
		{
			typedef typename std::iterator_traits<InputIterator>::value_type T;
			return reduce(ctl, first, last, T(0), bolt::cl::plus<T>(), cl_code);
		};




		/*! \p reduce returns the result of combining all the elements in the specified range using the specified binary_op.  
		* The classic example is a summation, where the binary_op is the plus operator.  By default, 
		* the binary operator is "plus<>()".
		*
		* \p reduce requires that the binary reduction op ("binary_op") is cummutative.  The order in which \p reduce applies the binary_op
		* is not deterministic.
		*
		* The \p reduce operation is similar the std::accumulate function.  See http://www.sgi.com/tech/stl/accumulate.html.
		*
		* \param first The first position in the sequence to be reduced.
		* \param last  The last position in the sequence to be reduced.
		* \param init  The initial value for the accumulator.
		* \param binary_op  The binary operation used to combine two values.   By default, the binary operation is plus<>().
		* \param cl_code Optional OpenCL(TM) code to be passed to the OpenCL compiler. The cl_code is inserted first in the generated code, before the cl_code trait.
		* \return The result of the reduction.
		*
		* The following code example shows the use of \p reduce to sum 10 numbers plus 100, using the default plus operator.
		* \code
		* #include <bolt/cl/reduce.h>
		*
		* int a[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
		*
		* int sum = bolt::cl::reduce(a, a+10, 100);
		* // sum = 155
		*  \endcode
		*
		* The following code example shows the use of \p reduce to find the max of 10 numbers:
		* \code
		* #include <bolt/cl/reduce.h>
		*
		* int a[10] = {2, 9, 3, 7, 5, 6, 3, 8, 3, 4};
		*
		* int max = bolt::cl::reduce(a, a+10, -1, bolt::cl:maximum<int>());
		* // max = 9
		*  \endcode
		*/
		template<typename InputIterator, typename T, typename BinaryFunction> 
		T reduce(InputIterator first, 
			InputIterator last,  
			T init,
			BinaryFunction binary_op=bolt::cl::plus<T>(), 
			const std::string cl_code="")  
		{
			return reduce(bolt::cl::control::getDefault(), first, last, init, binary_op, cl_code);
		};


		/*! \p reduce returns the result of combining all the elements in the specified range using the specified binary_op.  
		* The classic example is a summation, where the binary_op is the plus operator.  By default, the initial value is "0" 
		* and the binary operator is "plus<>()".
		*
		* \p reduce requires that the binary reduction op ("binary_op") is cummutative.  The order in which \p reduce applies the binary_op
		* is not deterministic.
		*
		* The \p reduce operation is similar the std::accumulate function.  See http://www.sgi.com/tech/stl/accumulate.html.
		*
		* \param first The first position in the sequence to be reduced.
		* \param last  The last position in the sequence to be reduced.
		* \param cl_code Optional OpenCL(TM) code to be passed to the OpenCL compiler. The cl_code is inserted first in the generated code, before the cl_code trait.
		* \return The result of the reduction.
		*
		* The following code example shows the use of \p reduce to sum 10 numbers, using the default plus operator.
		* \code
		* #include <bolt/cl/reduce.h>
		*
		* int a[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
		*
		* int sum = bolt::cl::reduce(a, a+10);
		* // sum = 55
		*  \endcode
		*
		*/
		template<typename InputIterator> 
		typename std::iterator_traits<InputIterator>::value_type
			reduce(InputIterator first, 
			InputIterator last, 
			const std::string cl_code="")
		{
			typedef typename std::iterator_traits<InputIterator>::value_type T;
			return reduce(bolt::cl::control::getDefault(), first, last, T(0), bolt::cl::plus<T>(), cl_code);
		};
	};
};

// FIXME - variations:  (specify control vs default control) * (std::vector vs cl::Buffer) * (specify {init, binary_op} vs default {0,"plus"})
// FIXME - add documentation for template types
