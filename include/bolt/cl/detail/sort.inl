#include <algorithm>
#include <type_traits>
#include <bolt/cl/functional.h>
#include <bolt/cl/device_vector.h>
#define WGSIZE 64

namespace bolt {
    namespace cl {

        template<typename RandomAccessIterator, typename StrictWeakOrdering> 
        void sort(const control &ctl,
            RandomAccessIterator first, 
            RandomAccessIterator last,  
            StrictWeakOrdering comp, 
            const std::string cl_code)  
        {
            detail::sort(ctl, first, last, comp, cl_code);
            return;
        }

        template<typename RandomAccessIterator> 
        void sort(const control &ctl,
            RandomAccessIterator first, 
            RandomAccessIterator last, 
            const std::string cl_code)
        {
            detail::sort(ctl, first, last, cl_code);
            return;
        }

        template<typename RandomAccessIterator, typename StrictWeakOrdering> 
        void sort(RandomAccessIterator first, 
            RandomAccessIterator last,  
            StrictWeakOrdering comp, 
            const std::string cl_code)  
        {
            sort(bolt::cl::control::getDefault(), first, last, comp, cl_code);
            return;
        }

        template<typename RandomAccessIterator> 
        void sort(RandomAccessIterator first, 
            RandomAccessIterator last, 
            const std::string cl_code)
        {
            sort(bolt::cl::control::getDefault(), first, last, cl_code);
            return;
        }
    }
};


namespace bolt {
    namespace cl {
        namespace detail {

            struct CallCompiler_Sort {
                static void constructAndCompileBasic(::cl::Kernel *masterKernel,  std::string cl_code, std::string valueTypeName,  const control &ctl) {

                    const std::string instantiationString = 
                        "// Host generates this instantiation string with user-specified value type and functor\n"
                        "template __attribute__((mangled_name(sortInstantiated)))\n"
                        "kernel void sortTemplateBasic(\n"
                        "global " + valueTypeName + "* A,\n"
                        "const uint stage,\n"
                        "const uint passOfStage\n"
                        ");\n\n";

                    bolt::cl::constructAndCompile(masterKernel, "sort", instantiationString, cl_code, valueTypeName, "", ctl);
                }

                static void constructAndCompile(::cl::Kernel *masterKernel,  std::string cl_code_dataType, std::string valueTypeName,  std::string compareTypeName, const control &ctl) {

                    const std::string instantiationString = 
                        "// Host generates this instantiation string with user-specified value type and functor\n"
                        "template __attribute__((mangled_name(sortInstantiated)))\n"
                        "kernel void sortTemplate(\n"
                        "global " + valueTypeName + "* A,\n"
                        "const uint stage,\n"
                        "const uint passOfStage,\n"
                        "global " + compareTypeName + " * userComp\n"
                        ");\n\n";

                    bolt::cl::constructAndCompile(masterKernel, "sort", instantiationString, cl_code_dataType, valueTypeName, "", ctl);
                }

            }; //End of struct CallCompiler_Sort  

            //Device Vector specialization
            template<typename RandomAccessIterator, typename StrictWeakOrdering> 
            typename std::enable_if< std::is_base_of<typename device_vector<typename std::iterator_traits<RandomAccessIterator>::value_type>::iterator,RandomAccessIterator>::value >::type
            sort(const control &ctl, RandomAccessIterator first, RandomAccessIterator last,
                StrictWeakOrdering comp, std::string cl_code) 
            {
                // User defined Data types are not supported with device_vector. Hence we have a static assert here.
                // The code here should be in compliant with the routine following this routine.
                typedef typename std::iterator_traits<RandomAccessIterator>::value_type T;
                size_t temp,szElements = (size_t)(last - first); 
                if (szElements == 0 )
                        return;

                const bolt::cl::control::e_RunMode runMode = ctl.forceRunMode();  // could be dynamic choice some day.
				if (runMode == bolt::cl::control::SerialCpu) {
                    //  TODO:  Need access to the device_vector .data method to get a host pointer
                    throw ::cl::Error( CL_INVALID_DEVICE, "Sort of device_vector CPU device not implemented" );
                    return;
                } else if (runMode == bolt::cl::control::MultiCoreCpu) {
                    std::cout << "The MultiCoreCpu version of device_vector sort is not implemented yet." << std ::endl;
                } else {
                    static  std::once_flag initOnlyOnce;
                    static  ::cl::Kernel masterKernel;
                    // Set up shape of launch grid and buffers:
                    int computeUnits     = ctl.device().getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>();
                    int wgPerComputeUnit =  ctl.wgPerComputeUnit(); 
                    int resultCnt = computeUnits * wgPerComputeUnit;
                    int wgSize = WGSIZE; //TODO hard coded to the number of cores in the AMD SIMD engine.
                                           //The instantiation string should also be changed. 
					if((szElements/2) < wgSize)
					{
						wgSize = (int)szElements/2;
					}
                    unsigned int numStages,stage,passOfStage;
                    if(((szElements-1) & (szElements)) != 0)
                    {
                        std::cout << "The BOLT sort routine device_vector does not support non power of 2 buffer size." << std ::endl;
	                    throw ::cl::Error( CL_INVALID_DEVICE, "The BOLT sort routine device_vector does not support non power of 2 buffer size." );
						return;
                    }

                    ::cl::Buffer A = (*first).getBuffer( );
                    ::cl::Buffer userFunctor(ctl.context(), CL_MEM_USE_HOST_PTR, sizeof(comp), &comp );   // Create buffer wrapper so we can access host parameters.
    
                    // For user-defined types, the user must create a TypeName trait which returns the name of the class - note use of TypeName<>::get to retreive the name here.
                    //std::call_once(initOnlyOnce, detail::CallCompiler_Sort::constructAndCompile, &masterKernel, cl_code + ClCode<T>::get(), TypeName<T>::get(), ctl);
                    std::call_once(initOnlyOnce, detail::CallCompiler_Sort::constructAndCompile, &masterKernel, cl_code + ClCode<T>::get(), TypeName<T>::get(), TypeName<StrictWeakOrdering>::get(), ctl);
                    ::cl::Kernel k = masterKernel;  // hopefully create a copy of the kernel. FIXME, doesn't work.
                    numStages = 0;
                    for(temp = szElements; temp > 1; temp >>= 1)
                        ++numStages;
                    k.setArg(0, A);
                    k.setArg(3, userFunctor);
                    for(stage = 0; stage < numStages; ++stage) 
                    {
                        // stage of the algorithm
                        k.setArg(1, stage);
                        // Every stage has stage + 1 passes
                        for(passOfStage = 0; passOfStage < stage + 1; ++passOfStage) {
                            // pass of the current stage
                            k.setArg(2, passOfStage);
                            /* 
                             * Enqueue a kernel run call.
                             * Each thread writes a sorted pair.
                             * So, the number of  threads (global) should be half the length of the input buffer.
                             */
                            ctl.commandQueue().enqueueNDRangeKernel(
                                    k, 
                                    ::cl::NullRange,
                                    ::cl::NDRange(szElements/2),
                                    ::cl::NDRange(wgSize),
                                    NULL,
                                    NULL);
                            ctl.commandQueue().finish();
                        }//end of for passStage = 0:stage-1
                    }//end of for stage = 0:numStage-1
                    //Map the buffer back to the host
                    ctl.commandQueue().enqueueMapBuffer(A, true, CL_MAP_READ | CL_MAP_WRITE, 0/*offset*/, sizeof(T) * szElements);
                    return;
                }
                return;
            }

            //Device Vector specialization
            template<typename RandomAccessIterator> 
            typename std::enable_if< std::is_base_of<typename device_vector<typename std::iterator_traits<RandomAccessIterator>::value_type>::iterator,RandomAccessIterator>::value >::type
            sort(const control &ctl, RandomAccessIterator first, RandomAccessIterator last,
                std::string cl_code)
            {
                typedef typename std::iterator_traits<RandomAccessIterator>::value_type T;
                size_t temp,szElements = (size_t)(last - first); 
                if (szElements == 0 )
                        return;

                const bolt::cl::control::e_RunMode runMode = ctl.forceRunMode();  // could be dynamic choice some day.
                if (runMode == bolt::cl::control::SerialCpu) {
                    //  TODO:  Need access to the device_vector .data method to get a host pointer
                    throw ::cl::Error( CL_INVALID_DEVICE, "Sort of device_vector CPU device not implemented" );
                    return;
                } else if (runMode == bolt::cl::control::MultiCoreCpu) {
                    std::cout << "The MultiCoreCpu version of device_vector sort is not implemented yet." << std ::endl;
	                throw ::cl::Error( CL_INVALID_DEVICE, "The BOLT sort routine device_vector does not support non power of 2 buffer size." );
					return;
                } else {
                    static  std::once_flag initOnlyOnce;
                    static  ::cl::Kernel masterKernel;
                    // Set up shape of launch grid and buffers:
                    int computeUnits     = ctl.device().getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>();
                    int wgPerComputeUnit =  ctl.wgPerComputeUnit(); 
                    int resultCnt = computeUnits * wgPerComputeUnit;
                    int wgSize = WGSIZE; //TODO hard coded to the number of cores in the AMD SIMD engine.
                                           //The instantiation string should also be changed.   
					if((szElements/2) < wgSize)
					{
						wgSize = (int)szElements/2;
					}
                    unsigned int numStages,stage,passOfStage;
                    if(((szElements-1) & (szElements)) != 0)
                    {
                        std::cout << "The BOLT sort routine does not support non power of 2 buffer size." << std ::endl;
                        throw ::cl::Error( CL_INVALID_DEVICE, "The BOLT sort routine device_vector does not support non power of 2 buffer size." );
                        return;
                    }

                    ::cl::Buffer A = (*first).getBuffer( );

                    // For user-defined types, the user must create a TypeName trait which returns the name of the class - note use of TypeName<>::get to retreive the name here.
                    std::call_once(initOnlyOnce, detail::CallCompiler_Sort::constructAndCompileBasic, &masterKernel, cl_code + ClCode<T>::get(), TypeName<T>::get(), ctl);
                    ::cl::Kernel k = masterKernel;  // hopefully create a copy of the kernel. FIXME, doesn't work.
                    numStages = 0;
                    for(temp = szElements; temp > 1; temp >>= 1)
                        ++numStages;
                    k.setArg(0, A);
                    for(stage = 0; stage < numStages; ++stage) 
                    {
                        // stage of the algorithm
                        k.setArg(1, stage);
                        // Every stage has stage + 1 passes
                        for(passOfStage = 0; passOfStage < stage + 1; ++passOfStage) {
                            // pass of the current stage
                            k.setArg(2, passOfStage);
                            /* 
                             * Enqueue a kernel run call.
                             * Each thread writes a sorted pair.
                             * So, the number of  threads (global) should be half the length of the input buffer.
                             */
                            ctl.commandQueue().enqueueNDRangeKernel(
                                    k, 
                                    ::cl::NullRange,
                                    ::cl::NDRange(szElements/2),
                                    ::cl::NDRange(wgSize),
                                    NULL,
                                    NULL);
                            ctl.commandQueue().finish();
                        }//end of for passStage = 0:stage-1
                    }//end of for stage = 0:numStage-1
                    //Map the buffer back to the host
                    ctl.commandQueue().enqueueMapBuffer(A, true, CL_MAP_READ | CL_MAP_WRITE, 0/*offset*/, sizeof(T) * szElements);
                    return;
                }
                return;
            }

            //Non Device Vector specialization.
            //This implementation creates a cl::Buffer and passes the cl buffer to the sort specialization whichtakes the cl buffer as a parameter. 
            //In the future, Each input buffer should be mapped to the device_vector and the specialization specific to device_vector should be called. 
            template<typename RandomAccessIterator> 
            typename std::enable_if< !std::is_base_of<typename device_vector<typename std::iterator_traits<RandomAccessIterator>::value_type>::iterator,RandomAccessIterator>::value >::type
            sort(const control &ctl, RandomAccessIterator first, RandomAccessIterator last,
                std::string cl_code)  
            {
                typedef typename std::iterator_traits<RandomAccessIterator>::value_type T;
                size_t szElements = (size_t)(last - first); 
                if (szElements == 0 )
                        return;

                const bolt::cl::control::e_RunMode runMode = ctl.forceRunMode();  // could be dynamic choice some day.
                if ((runMode == bolt::cl::control::SerialCpu) || (szElements < WGSIZE)) {
                    std::sort(first, last);
                    return;
                } else if (runMode == bolt::cl::control::MultiCoreCpu) {
                    std::cout << "The MultiCoreCpu version of sort is not implemented yet." << std ::endl;
                } else {
                    if(((szElements-1) & (szElements)) != 0)
                    {
                        std::cout << "The BOLT sort routine does not support non power of 2 buffer size. Falling back to CPU std::sort" << std ::endl;
						std::sort(first,last);
                        return;
                    }
                    ::cl::Buffer A(ctl.context(), CL_MEM_USE_HOST_PTR|CL_MEM_READ_WRITE, sizeof(T) * szElements, const_cast< T* >( &*first ));
                    //Now call the actual cl algorithm
					//When device_vector is fully functional and performant then the below call shall be replaced by the device vector interface.
                    sort<T>(ctl,A,cl_code);
                    //Map the buffer back to the host
                    ctl.commandQueue().enqueueMapBuffer(A, true, CL_MAP_READ | CL_MAP_WRITE, 0/*offset*/, sizeof(T) * szElements);
                    return;
                }
            }
            
            //Non Device Vector specialization.
            //This implementation creates a cl::Buffer and passes the cl buffer to the sort specialization whichtakes the cl buffer as a parameter. 
            //In the future, Each input buffer should be mapped to the device_vector and the specialization specific to device_vector should be called. 
            template<typename RandomAccessIterator, typename StrictWeakOrdering> 
            typename std::enable_if< !std::is_base_of<typename device_vector<typename std::iterator_traits<RandomAccessIterator>::value_type>::iterator,RandomAccessIterator>::value >::type
            sort(const control &ctl, RandomAccessIterator first, RandomAccessIterator last,
                StrictWeakOrdering comp, std::string cl_code)  
            {
                typedef typename std::iterator_traits<RandomAccessIterator>::value_type T;
                size_t szElements = (size_t)(last - first); 
                if (szElements == 0)
                    return;

                const bolt::cl::control::e_RunMode runMode = ctl.forceRunMode();  // could be dynamic choice some day.
                if ((runMode == bolt::cl::control::SerialCpu) || (szElements < WGSIZE)) {
                    std::sort(first, last, comp);
                    return;
                } else if (runMode == bolt::cl::control::MultiCoreCpu) {
                    std::cout << "The MultiCoreCpu version of sort is not implemented yet." << std ::endl;
                } else {
                    if(((szElements-1) & (szElements)) != 0)
                    {
                        std::cout << "The BOLT sort routine does not support non power of 2 buffer size. Falling back to CPU std::sort" << std ::endl;
						std::sort(first,last,comp);
                        return;
                    }
                    ::cl::Buffer A(ctl.context(), CL_MEM_USE_HOST_PTR|CL_MEM_READ_WRITE, sizeof(T) * szElements, const_cast< T* >( &*first ));
					// Create buffer wrappers so we can access the host functors, for read or writing in the kernel
					::cl::Buffer userFunctor(ctl.context(), CL_MEM_USE_HOST_PTR, sizeof(comp), &comp );   // Create buffer wrapper so we can access host parameters.
                    //Now call the actual cl algorithm
                    sort<T,StrictWeakOrdering>(ctl,A,userFunctor,cl_code);
                    //Map the buffer back to the host
                    ctl.commandQueue().enqueueMapBuffer(A, true, CL_MAP_READ | CL_MAP_WRITE, 0/*offset*/, sizeof(T) * szElements);
                    return;
                }
            }

// The below specializations can be removed when the device_vector supports the User defined data types. 
//For Now these are still kept. The function call to the cl buffer will be removed in the future. 
            template<typename T, typename StrictWeakOrdering> 
            void sort(const control &ctl, ::cl::Buffer A,
                ::cl::Buffer userFunctor, std::string cl_code)  
            {
                static std::once_flag initOnlyOnce;
                static  ::cl::Kernel masterKernel;

                int computeUnits     = ctl.device().getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>();
                int wgPerComputeUnit =  ctl.wgPerComputeUnit(); 
                int resultCnt = computeUnits * wgPerComputeUnit;
                int wgSize = WGSIZE; //TODO hard coded to the number of cores in the AMD SIMD engine.
                                       //The instantiation string should also be changed.
                unsigned int temp,numStages,stage,passOfStage;
                
                //Calculate the number of stages.
                numStages = 0;
                int szElements = (int)A.getInfo<CL_MEM_SIZE>() / sizeof(T);  // FIXME - remove typecast.  Kernel only can handle 32-bit size...
				if((szElements/2) < wgSize)
				{
					wgSize = (int)szElements/2;
				}

                for(temp = szElements; temp > 1; temp >>= 1)
                    ++numStages;

                // For user-defined types, the user must create a TypeName trait which returns the name of the class - note use of TypeName<>::get to retreive the name here.
                std::call_once(initOnlyOnce, detail::CallCompiler_Sort::constructAndCompile, &masterKernel, cl_code + ClCode<T>::get(), TypeName<T>::get(), TypeName<StrictWeakOrdering>::get(), ctl);
                ::cl::Kernel k = masterKernel;  // hopefully create a copy of the kernel. FIXME, doesn't work.

                k.setArg(0, A);
                //1 and 2 we will add inside the loop.
                k.setArg(3, userFunctor);
                for(stage = 0; stage < numStages; ++stage) 
                {
                    // stage of the algorithm
                    k.setArg(1, stage);
                    // Every stage has stage + 1 passes
                    for(passOfStage = 0; passOfStage < stage + 1; ++passOfStage) {
                        // pass of the current stage
                        k.setArg(2, passOfStage);
                        /* 
                         * Enqueue a kernel run call.
                         * Each thread writes a sorted pair.
                         * So, the number of  threads (global) should be half the length of the input buffer.
                         */
                        ctl.commandQueue().enqueueNDRangeKernel(
                                k, 
                                ::cl::NullRange,
                                ::cl::NDRange(szElements/2),
                                ::cl::NDRange(wgSize),
                                NULL,
                                NULL);
                        ctl.commandQueue().finish();
                    }//end of for passStage = 0:stage-1
                }//end of for stage = 0:numStage-1
            }//end of sort()

            template<typename T> 
            void sort(const control &ctl, ::cl::Buffer A,
                std::string cl_code)  
            {
                //TODO :: This should be compiled always. The static was removed here because there was a failure when we try to pass a different comparison options. 
                static  std::once_flag initOnlyOnce;
                static  ::cl::Kernel masterKernel;

                int computeUnits     = ctl.device().getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>();
                int wgPerComputeUnit =  ctl.wgPerComputeUnit(); 
                int resultCnt = computeUnits * wgPerComputeUnit;
                int wgSize = WGSIZE; //TODO hard coded to the number of cores in the AMD SIMD engine.
                                       //The instantiation string should also be changed.   
                unsigned int temp,numStages,stage,passOfStage;

                //Calculate the number of stages.
                numStages = 0;
                int szElements = (int)A.getInfo<CL_MEM_SIZE>() / sizeof(T);  
				if((szElements/2) < wgSize)
				{
					wgSize = (int)szElements/2;
				}
                for(temp = szElements; temp > 1; temp >>= 1)
                    ++numStages;

                // For user-defined types, the user must create a TypeName trait which returns the name of the class - note use of TypeName<>::get to retreive the name here.
                std::call_once(initOnlyOnce, detail::CallCompiler_Sort::constructAndCompileBasic, &masterKernel, cl_code + ClCode<T>::get(), TypeName<T>::get(), ctl);
                ::cl::Kernel k = masterKernel;  // hopefully create a copy of the kernel. FIXME, doesn't work.

                k.setArg(0, A);
                //1 and 2 we will add inside the loop.
                
                for(stage = 0; stage < numStages; ++stage) 
                {
                    // stage of the algorithm
                    k.setArg(1, stage);
                    // Every stage has stage + 1 passes
                    for(passOfStage = 0; passOfStage < stage + 1; ++passOfStage) {
                        // pass of the current stage
                        k.setArg(2, passOfStage);
                        /* 
                         * Enqueue a kernel run call.
                         * Each thread writes a sorted pair.
                         * So, the number of  threads (global) should be half the length of the input buffer.
                         */
                        ctl.commandQueue().enqueueNDRangeKernel(
                                k, 
                                ::cl::NullRange,
                                ::cl::NDRange(szElements/2),
                                ::cl::NDRange(wgSize),
                                NULL,
                                NULL);
                        ctl.commandQueue().finish();
                    }//end of for passStage = 0:stage-1
                }//end of for stage = 0:numStage-1
            }//end of sort()

        }//namespace bolt::cl::detail
    }//namespace bolt::cl
}//namespace bolt
