#if !defined( SORT_INL )
#define SORT_INL
#pragma once

#include <algorithm>
#include <type_traits>
#include <bolt/cl/functional.h>
#include <bolt/cl/device_vector.h>

#include <boost/thread/once.hpp>
#include <boost/bind.hpp>

#define WGSIZE 64

namespace bolt {
    namespace cl {
        template<typename RandomAccessIterator> 
        void sort(RandomAccessIterator first, 
            RandomAccessIterator last, 
            const std::string& cl_code)
        {
            typedef std::iterator_traits< RandomAccessIterator >::value_type T;

            detail::sort_detect_random_access( control::getDefault( ), first, last, less< T >( ), cl_code, 
                std::iterator_traits< RandomAccessIterator >::iterator_category( ) );
            return;
        }

        template<typename RandomAccessIterator, typename StrictWeakOrdering> 
        void sort(RandomAccessIterator first, 
            RandomAccessIterator last,  
            StrictWeakOrdering comp, 
            const std::string& cl_code)  
        {
            detail::sort_detect_random_access( control::getDefault( ), first, last, comp, cl_code, 
                std::iterator_traits< RandomAccessIterator >::iterator_category( ) );
            return;
        }

        template<typename RandomAccessIterator> 
        void sort(const control &ctl,
            RandomAccessIterator first, 
            RandomAccessIterator last, 
            const std::string& cl_code)
        {
            typedef std::iterator_traits< RandomAccessIterator >::value_type T;

            detail::sort_detect_random_access(ctl, first, last, less< T >( ), cl_code, 
                std::iterator_traits< RandomAccessIterator >::iterator_category( ) );
            return;
        }

        template<typename RandomAccessIterator, typename StrictWeakOrdering> 
        void sort(const control &ctl,
            RandomAccessIterator first, 
            RandomAccessIterator last,  
            StrictWeakOrdering comp, 
            const std::string& cl_code)  
        {
            detail::sort_detect_random_access(ctl, first, last, comp, cl_code, 
                std::iterator_traits< RandomAccessIterator >::iterator_category( ) );
            return;
        }

    }
};


namespace bolt {
    namespace cl {
        namespace detail {

            struct CallCompiler_Sort {
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

            // Wrapper that uses default control class, iterator interface
            template<typename RandomAccessIterator, typename StrictWeakOrdering> 
            void sort_detect_random_access( const control &ctl, RandomAccessIterator first, RandomAccessIterator last,
                StrictWeakOrdering comp, const std::string& cl_code, std::input_iterator_tag )
            {
                //  TODO:  It should be possible to support non-random_access_iterator_tag iterators, if we copied the data 
                //  to a temporary buffer.  Should we?
                static_assert( false, "Bolt only supports random access iterator types" );
            };

            template<typename RandomAccessIterator, typename StrictWeakOrdering> 
            void sort_detect_random_access( const control &ctl, RandomAccessIterator first, RandomAccessIterator last,
                StrictWeakOrdering comp, const std::string& cl_code, std::random_access_iterator_tag )
            {
                return sort_pick_iterator(ctl, first, last, comp, cl_code);
            };

            //Device Vector specialization
            template<typename DVRandomAccessIterator, typename StrictWeakOrdering> 
            typename std::enable_if< std::is_base_of<typename device_vector<typename std::iterator_traits<DVRandomAccessIterator>::value_type>::iterator,DVRandomAccessIterator>::value >::type
            sort_pick_iterator(const control &ctl, DVRandomAccessIterator first, DVRandomAccessIterator last,
                StrictWeakOrdering comp, const std::string& cl_code) 
            {
                // User defined Data types are not supported with device_vector. Hence we have a static assert here.
                // The code here should be in compliant with the routine following this routine.
                size_t szElements = (size_t)(last - first); 
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
                    sort_enqueue(ctl,first,last,comp,cl_code);
                }
                return;
            }
            
            //Non Device Vector specialization.
            //This implementation creates a cl::Buffer and passes the cl buffer to the sort specialization whichtakes the cl buffer as a parameter. 
            //In the future, Each input buffer should be mapped to the device_vector and the specialization specific to device_vector should be called. 
            template<typename RandomAccessIterator, typename StrictWeakOrdering> 
            typename std::enable_if< !std::is_base_of<typename device_vector<typename std::iterator_traits<RandomAccessIterator>::value_type>::iterator,RandomAccessIterator>::value >::type
            sort_pick_iterator(const control &ctl, RandomAccessIterator first, RandomAccessIterator last,
                StrictWeakOrdering comp, const std::string& cl_code)  
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
                    device_vector< T > dvInputOutput( first, last, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE, ctl );
                    //Now call the actual cl algorithm
                    sort_enqueue(ctl,dvInputOutput.begin(),dvInputOutput.end(),comp,cl_code);
                    //Map the buffer back to the host
                    dvInputOutput.data( );
                    return;
                }
            }

            template<typename DVRandomAccessIterator, typename StrictWeakOrdering> 
            void sort_enqueue(const control &ctl, DVRandomAccessIterator first, DVRandomAccessIterator last,
                StrictWeakOrdering comp, const std::string& cl_code)  
            {
                    typedef typename std::iterator_traits< DVRandomAccessIterator >::value_type T;
                    static  boost::once_flag initOnlyOnce;
                    static  ::cl::Kernel masterKernel;
                    size_t szElements = (size_t)(last - first);
                    size_t temp;
                    // For user-defined types, the user must create a TypeName trait which returns the name of the class - note use of TypeName<>::get to retreive the name here.
                    boost::call_once( initOnlyOnce, boost::bind( CallCompiler_Sort::constructAndCompile, &masterKernel, cl_code + ClCode<T>::get(), TypeName<T>::get(), TypeName<StrictWeakOrdering>::get(), ctl) );

                    // Set up shape of launch grid and buffers:
                    int computeUnits     = ctl.device().getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>();
                    int wgPerComputeUnit =  ctl.wgPerComputeUnit(); 
                    int resultCnt = computeUnits * wgPerComputeUnit;
                    cl_int l_Error = CL_SUCCESS;
                    size_t wgSize  = masterKernel.getWorkGroupInfo< CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE >( ctl.device( ), &l_Error );
                    std::cout << "wgsize = " << wgSize;
                    V_OPENCL( l_Error, "Error querying kernel for CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE" );

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

                    ::cl::Buffer A = first->getBuffer( );
                    ::cl::Buffer userFunctor(ctl.context(), CL_MEM_USE_HOST_PTR, sizeof(comp), &comp );   // Create buffer wrapper so we can access host parameters.
    
                    ::cl::Kernel k = masterKernel;  // hopefully create a copy of the kernel. FIXME, doesn't work.
                    numStages = 0;
                    for(temp = szElements; temp > 1; temp >>= 1)
                        ++numStages;
                    V_OPENCL( k.setArg(0, A), "Error setting a kernel argument" );
                    V_OPENCL( k.setArg(3, userFunctor), "Error setting a kernel argument" );
                    for(stage = 0; stage < numStages; ++stage) 
                    {
                        // stage of the algorithm
                        V_OPENCL( k.setArg(1, stage), "Error setting a kernel argument" );
                        // Every stage has stage + 1 passes
                        for(passOfStage = 0; passOfStage < stage + 1; ++passOfStage) {
                            // pass of the current stage
                            V_OPENCL( k.setArg(2, passOfStage), "Error setting a kernel argument" );
                            /* 
                             * Enqueue a kernel run call.
                             * Each thread writes a sorted pair.
                             * So, the number of  threads (global) should be half the length of the input buffer.
                             */
                            l_Error = ctl.commandQueue().enqueueNDRangeKernel(
                                    k, 
                                    ::cl::NullRange,
                                    ::cl::NDRange(szElements/2),
                                    ::cl::NDRange(wgSize),
                                    NULL,
                                    NULL);
                            V_OPENCL( l_Error, "enqueueNDRangeKernel() failed for sort() kernel" );
                            V_OPENCL( ctl.commandQueue().finish(), "Error calling finish on the command queue" );
                        }//end of for passStage = 0:stage-1
                    }//end of for stage = 0:numStage-1
                    //Map the buffer back to the host
                    ctl.commandQueue().enqueueMapBuffer(A, true, CL_MAP_READ | CL_MAP_WRITE, 0/*offset*/, sizeof(T) * szElements, NULL, NULL, &l_Error );
                    V_OPENCL( l_Error, "Error calling map on the result buffer" );
                    return;
            }

        }//namespace bolt::cl::detail
    }//namespace bolt::cl
}//namespace bolt

#endif