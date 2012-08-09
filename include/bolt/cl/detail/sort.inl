#include <algorithm>


namespace bolt {
    namespace cl {

        // This template is called by all other "convenience" version of sort.
        // It also implements the CPU-side mappings of the algorithm for SerialCpu and MultiCoreCpu
        template<typename RandomAccessIterator, typename Compare> 
        void sort(const bolt::cl::control &ctl,
            RandomAccessIterator first, 
            RandomAccessIterator last,  
            Compare comp, 
            const std::string cl_code)  
        {
            typedef typename std::iterator_traits<RandomAccessIterator>::value_type T;
            RandomAccessIterator  temp;
            const bolt::cl::control::e_RunMode runMode = ctl.forceRunMode();  // could be dynamic choice some day.
            if (runMode == bolt::cl::control::SerialCpu) {
                std::sort(first, last, comp);
                return;
            } else if (runMode == bolt::cl::control::MultiCoreCpu) {
                std::cout << "The MultiCoreCpu version of sort is not implemented yet." << std ::endl;
            } else {
               
                size_t szElements = (int)(last - first); 
                // Map the input iterator to a cl buffer
                ::cl::Buffer A(ctl.context(), CL_MEM_USE_HOST_PTR|CL_MEM_READ_WRITE, sizeof(T) * szElements, const_cast<T*>(&*first));
                //Now call the actual cl algorithm
                detail::sort<T>(ctl, A, comp, cl_code);
                //Map the buffer back to the host
                ctl.commandQueue().enqueueMapBuffer(A, true, CL_MAP_READ | CL_MAP_WRITE, 0/*offset*/, szElements);
                
                /*temp=first;
                while(temp!=last)
                {
                    std::cout << " " << *temp;
                    temp++;
                }*/
                return;
            }
        }

        template<typename RandomAccessIterator> 
        void sort(const bolt::cl::control &ctl,
            RandomAccessIterator first, 
            RandomAccessIterator last, 
            const std::string cl_code)
        {
            typedef typename std::iterator_traits<RandomAccessIterator>::value_type T;
            sort(ctl, first, last, bolt::cl::maximum<T>(), cl_code);
            return;
        }



        template<typename RandomAccessIterator, typename BinaryFunction> 
        void sort(RandomAccessIterator first, 
            RandomAccessIterator last,  
            BinaryFunction binary_op, 
            const std::string cl_code)  
        {
            return sort(bolt::cl::control::getDefault(), first, last, binary_op, cl_code);
        }


        template<typename RandomAccessIterator> 
        void sort(RandomAccessIterator first, 
            RandomAccessIterator last, 
            const std::string cl_code)
        {
            typedef typename std::iterator_traits<RandomAccessIterator>::value_type T;
            sort(bolt::cl::control::getDefault(), first, last, bolt::cl::minimum<T>(), cl_code);
        }
    }
};

        //TODO::COME BACK HERE
namespace bolt {
    namespace cl {
        namespace detail {

            // FIXME - move to cpp file
            struct CallCompiler_Sort {
                static void constructAndCompile(::cl::Kernel *masterKernel,  std::string cl_code, std::string valueTypeName,  std::string functorTypeName, const control &ctl) {

                    const std::string instantiationString = 
                        "// Host generates this instantiation string with user-specified value type and functor\n"
                        "template __attribute__((mangled_name(sortInstantiated)))\n"
                        "__attribute__((reqd_work_group_size(1,1,1)))\n"
                        "kernel void sortTemplate(\n"
                        "global " + valueTypeName + "* A,\n"
                        "const uint stage,\n"
                        "const uint passOfStage,\n"
                        "global " + functorTypeName + "* userComp,\n"
                        "local " + valueTypeName + "* scratch\n"
                        ");\n\n";

                    bolt::cl::constructAndCompile(masterKernel, "sort", instantiationString, cl_code, valueTypeName, functorTypeName, ctl);

                }
            };


            template<typename T, typename Compare> 
            void sort(const bolt::cl::control &ctl, ::cl::Buffer A,
                Compare comp, std::string cl_code="")  
            {
                static std::once_flag initOnlyOnce;
                static  ::cl::Kernel masterKernel;
                // For user-defined types, the user must create a TypeName trait which returns the name of the class - note use of TypeName<>::get to retreive the name here.
                std::call_once(initOnlyOnce, detail::CallCompiler_Sort::constructAndCompile, &masterKernel, cl_code + ClCode<Compare>::get(), TypeName<T>::get(), TypeName<Compare>::get(), ctl);


                // Set up shape of launch grid and buffers:
                int computeUnits     = ctl.device().getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>();
                int wgPerComputeUnit =  ctl.wgPerComputeUnit(); 
                int resultCnt = computeUnits * wgPerComputeUnit;
                const int wgSize = 64; 
                unsigned int temp,numStages,stage,passOfStage;
                numStages = 0;

                // Create buffer wrappers so we can access the host functors, for read or writing in the kernel
                ::cl::Buffer userFunctor(ctl.context(), CL_MEM_USE_HOST_PTR, sizeof(comp), &comp );   // Create buffer wrapper so we can access host parameters.

                ::cl::Kernel k = masterKernel;  // hopefully create a copy of the kernel. FIXME, doesn't work.

                int szElements = (int)A.getInfo<CL_MEM_SIZE>() / sizeof(T);  // FIXME - remove typecast.  Kernel only can handle 32-bit size...
                for(temp = szElements; temp > 1; temp >>= 1)
                    ++numStages;
                k.setArg(0, A);
                //1 and 2 we will add inside the loop.
                k.setArg(3, userFunctor);
                ::cl::LocalSpaceArg loc;
                loc.size_ = wgSize*sizeof(T);
                k.setArg(4, loc);

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
                                ::cl::NDRange(1),
                                NULL,
                                NULL);
                        ctl.commandQueue().finish();
                    }//end of for passStage = 0:stage-1
                }//end of for stage = 0:numStage-1
            }//end of sort()
        }//namespace detail
    }//namespace cl
}//namespace bolt
