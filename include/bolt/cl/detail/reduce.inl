#include <algorithm>

// #include <bolt/tbb/reduce.h>

namespace bolt {
	namespace cl {

		// This template is called by all other "convenience" version of reduce.
		// It also implements the CPU-side mappings of the algorithm for SerialCpu and MultiCoreCpu
		template<typename InputIterator, typename T, typename BinaryFunction> 
		T reduce(const bolt::cl::control &ctl,
			InputIterator first, 
			InputIterator last,  
			T init,
			BinaryFunction binary_op, 
			const std::string cl_code)  
		{
			typedef typename std::iterator_traits<InputIterator>::value_type T;
			const bolt::cl::control::e_RunMode runMode = ctl.forceRunMode();  // could be dynamic choice some day.
			if (runMode == bolt::cl::control::SerialCpu) {
				return std::accumulate(first, last, init, binary_op);
			//} else if (runMode == bolt::cl::control::MultiCoreCpu) {
			//	TbbReduceWrapper wrapper();
			//	tbb::parallel_reduce(tbb:blocked_range<T>(first, last), wrapper);
			} else {

				size_t szElements = (int)(last - first); 

				::cl::Buffer A(ctl.context(), CL_MEM_USE_HOST_PTR|CL_MEM_READ_ONLY, sizeof(T) * szElements, const_cast<T*>(&*first));

				return detail::reduce(ctl, A, init, binary_op, cl_code);
			}
		};


		template<typename InputIterator> 
		typename std::iterator_traits<InputIterator>::value_type
			reduce(const bolt::cl::control &ctl,
			InputIterator first, 
			InputIterator last, 
			const std::string cl_code)
		{
			typedef typename std::iterator_traits<InputIterator>::value_type T;
			return reduce(ctl, first, last, T(0), bolt::cl::plus<T>(), cl_code);
		};



		template<typename InputIterator, typename T, typename BinaryFunction> 
		T reduce(InputIterator first, 
			InputIterator last,  
			T init,
			BinaryFunction binary_op, 
			const std::string cl_code)  
		{
			return reduce(bolt::cl::control::getDefault(), first, last, init, binary_op, cl_code);
		};


		template<typename InputIterator> 
		typename std::iterator_traits<InputIterator>::value_type
			reduce(InputIterator first, 
			InputIterator last, 
			const std::string cl_code)
		{
			typedef typename std::iterator_traits<InputIterator>::value_type T;
			return reduce(bolt::cl::control::getDefault(), first, last, T(0), bolt::cl::plus<T>(), cl_code);
		};
	}
};


namespace bolt {
	namespace cl {
		namespace detail {

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
				std::call_once(initOnlyOnce, detail::CallCompiler_Reduce::constructAndCompile, &masterKernel, cl_code + ClCode<BinaryFunction>::get(), TypeName<T>::get(),  TypeName<BinaryFunction>::get(), ctl);


				// Set up shape of launch grid and buffers:
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




		}
	}
}
