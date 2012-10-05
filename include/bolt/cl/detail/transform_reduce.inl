/***************************************************************************                                                                                     
*   Copyright 2012 Advanced Micro Devices, Inc.                                     
*                                                                                    
*   Licensed under the Apache License, Version 2.0 (the "License");   
*   you may not use this file except in compliance with the License.                 
*   You may obtain a copy of the License at                                          
*                                                                                    
*       http://www.apache.org/licenses/LICENSE-2.0                      
*                                                                                    
*   Unless required by applicable law or agreed to in writing, software              
*   distributed under the License is distributed on an "AS IS" BASIS,              
*   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.         
*   See the License for the specific language governing permissions and              
*   limitations under the License.                                                   

***************************************************************************/                                                                                     

#if !defined( TRANSFORM_REDUCE_INL )
#define TRANSFORM_REDUCE_INL
#pragma once

#include <string>
#include <iostream>
#include <numeric>

#include <boost/thread/once.hpp>
#include <boost/bind.hpp>

#include "bolt/cl/bolt.h"

namespace bolt {
    namespace cl {

        // The following two functions are visible in .h file
        // Wrapper that user passes a control class
        template<typename T, typename InputIterator, typename UnaryFunction, typename BinaryFunction> 
        T transform_reduce(const control& ctl, InputIterator first, InputIterator last,  
            UnaryFunction transform_op, 
            T init,  BinaryFunction reduce_op, const std::string& user_code )  
        {
            return detail::transform_reduce_detect_random_access( ctl, first, last, transform_op, init, reduce_op, user_code, 
                std::iterator_traits< InputIterator >::iterator_category( ) );
        };

        // Wrapper that generates default control class
        template<typename T, typename InputIterator, typename UnaryFunction, typename BinaryFunction> 
        T transform_reduce(InputIterator first, InputIterator last,
            UnaryFunction transform_op,
            T init,  BinaryFunction reduce_op, const std::string& user_code )
        {
            return detail::transform_reduce_detect_random_access( control::getDefault(), first, last, transform_op, init, reduce_op, user_code, 
                std::iterator_traits< InputIterator >::iterator_category( ) );
        };
    };
};


namespace bolt {
    namespace cl {
        namespace  detail {
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

                    bolt::cl::constructAndCompileString( masterKernel, "transform_reduce", transform_reduce_kernels, instantiationString, user_code, valueTypeName, functorNames, c);
                    // bolt::cl::constructAndCompile(masterKernel, "transform_reduce", instantiationString, user_code, valueTypeName, functorNames, c);

                };
            };

        //  The following two functions disallow non-random access functions
        // Wrapper that uses default control class, iterator interface
        template<typename T, typename InputIterator, typename UnaryFunction, typename BinaryFunction> 
        T transform_reduce_detect_random_access(const control &ctl, InputIterator first, InputIterator last,
            UnaryFunction transform_op,
            T init,  BinaryFunction reduce_op, const std::string& user_code, std::input_iterator_tag )
        {
            //  TODO:  It should be possible to support non-random_access_iterator_tag iterators, if we copied the data 
            //  to a temporary buffer.  Should we?
            static_assert( false, "Bolt only supports random access iterator types" );
        };

        // Wrapper that uses default control class, iterator interface
        template<typename T, typename InputIterator, typename UnaryFunction, typename BinaryFunction> 
        T transform_reduce_detect_random_access(const control& ctl, InputIterator first, InputIterator last,
            UnaryFunction transform_op,
            T init,  BinaryFunction reduce_op, const std::string& user_code, std::random_access_iterator_tag )
        {
            return transform_reduce_pick_iterator( ctl, first, last, transform_op, init, reduce_op, user_code);
        };

        // This template is called by the non-detail versions of transform_reduce, it already assumes random access iterators
        // This is called strictly for any non-device_vector iterator
        template<typename T, typename InputIterator, typename UnaryFunction, typename BinaryFunction> 
        typename std::enable_if< !std::is_base_of<typename device_vector<typename std::iterator_traits<InputIterator>::value_type>::iterator,InputIterator>::value, T >::type
        transform_reduce_pick_iterator(const control &c, InputIterator first, InputIterator last, UnaryFunction transform_op, 
            T init,  BinaryFunction reduce_op, const std::string& user_code )
        {
            typedef std::iterator_traits<InputIterator>::value_type T;
            size_t szElements = (last - first); 
            if (szElements == 0)
                    return init;

            const bolt::cl::control::e_RunMode runMode = c.forceRunMode();  // could be dynamic choice some day.
            if (runMode == bolt::cl::control::SerialCpu)
            {
                //Create a temporary array to store the transform result;
                std::vector<T> output(szElements);

                std::transform(first, last, output.begin(),transform_op);
                return std::accumulate(output.begin(), output.end(), init);

            } else if (runMode == bolt::cl::control::MultiCoreCpu) {
                std::cout << "The MultiCoreCpu version of transform_reduce is not implemented yet." << std ::endl;
                return init;
            } else {
                // Map the input iterator to a device_vector
                device_vector< T > dvInput( first, last, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE, c );

                return  transform_reduce_enqueue( c, dvInput.begin( ), dvInput.end( ), transform_op, init, reduce_op, user_code );
            }
        };

        // This template is called by the non-detail versions of transform_reduce, it already assumes random access iterators
        // This is called strictly for iterators that are derived from device_vector< T >::iterator
        template<typename T, typename DVInputIterator, typename UnaryFunction, typename BinaryFunction> 
        typename std::enable_if< std::is_base_of<typename device_vector<typename std::iterator_traits<DVInputIterator>::value_type>::iterator,DVInputIterator>::value, T >::type
        transform_reduce_pick_iterator(const control &c, DVInputIterator first, DVInputIterator last, UnaryFunction transform_op, 
            T init,  BinaryFunction reduce_op, const std::string& user_code )
        {
            typedef std::iterator_traits<DVInputIterator>::value_type T;
            size_t szElements = (last - first); 
            if (szElements == 0)
                    return init;

            const bolt::cl::control::e_RunMode runMode = c.forceRunMode();  // could be dynamic choice some day.
            if (runMode == bolt::cl::control::SerialCpu)
            {
                //  TODO:  Need access to the device_vector .data method to get a host pointer
                throw ::cl::Error( CL_INVALID_DEVICE, "transform_reduce device_vector CPU device not implemented" );

                //Create a temporary array to store the transform result;
                std::vector<T> output(szElements);

                std::transform(first, last, output.begin(),transform_op);
                return std::accumulate(output.begin(), output.end(), init);

            }
            else if (runMode == bolt::cl::control::MultiCoreCpu)
            {
                //  TODO:  Need access to the device_vector .data method to get a host pointer
                throw ::cl::Error( CL_INVALID_DEVICE, "transform_reduce device_vector CPU device not implemented" );

                std::cout << "The MultiCoreCpu version of transform_reduce is not implemented yet." << std ::endl;
                return init;
            }

            return  transform_reduce_enqueue( c, first, last, transform_op, init, reduce_op, user_code );
        };

            template<typename T, typename DVInputIterator, typename UnaryFunction, typename BinaryFunction> 
            T transform_reduce_enqueue(const control& c, DVInputIterator first, DVInputIterator last, UnaryFunction transform_op,
                T init, BinaryFunction reduce_op, const std::string& user_code="")
            {
                static boost::once_flag initOnlyOnce;
                static  ::cl::Kernel masterKernel;
                unsigned debugMode = 0; //FIXME, use control

                //std::call_once(initOnlyOnce, CallCompiler_TransformReduce::constructAndCompile, &masterKernel, 
                //    "\n//--user Code\n" + user_code + "\n//---Functions\n" + ClCode<UnaryFunction>::get() + ClCode<BinaryFunction>::get() , 
                //    TypeName<T>::get(), TypeName<UnaryFunction>::get(), TypeName<BinaryFunction>::get());
                boost::call_once( initOnlyOnce, boost::bind( CallCompiler_TransformReduce::constructAndCompile, &masterKernel, 
                    "\n//--user Code\n" + user_code + "\n//---Functions\n" + ClCode<UnaryFunction>::get() + ClCode<BinaryFunction>::get() , 
                    TypeName<T>::get(), TypeName<UnaryFunction>::get(), TypeName<BinaryFunction>::get() ) );


                // Set up shape of launch grid and buffers:
                // FIXME, read from device attributes.
                int computeUnits     = c.device().getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>();  // round up if we don't know. 
                int wgPerComputeUnit =  c.wgPerComputeUnit(); 
                int resultCnt = computeUnits * wgPerComputeUnit;

                cl_int l_Error = CL_SUCCESS;
                const size_t wgSize  = masterKernel.getWorkGroupInfo< CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE >( c.device( ), &l_Error );
                V_OPENCL( l_Error, "Error querying kernel for CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE" );

                // Create Buffer wrappers so we can access the host functors, for read or writing in the kernel
                ::cl::Buffer transformFunctor(c.context(), CL_MEM_USE_HOST_PTR, sizeof(transform_op), &transform_op );   
                ::cl::Buffer reduceFunctor(c.context(), CL_MEM_USE_HOST_PTR, sizeof(reduce_op), &reduce_op );
                ::cl::Buffer result(c.context(), CL_MEM_ALLOC_HOST_PTR|CL_MEM_WRITE_ONLY, sizeof(T) * resultCnt);

                ::cl::Kernel k = masterKernel;  // hopefully create a copy of the kernel. FIXME, doesn't work.

                cl_uint szElements = static_cast< cl_uint >( std::distance( first, last ) );
                
                /***** This is a temporaray fix *****/
                /*What if  requiredWorkGroups > resultCnt? Do you want to loop or increase the work group size or increase the per item processing?*/
                int requiredWorkGroups = (int)ceil((float)szElements/wgSize); 
                if (requiredWorkGroups < resultCnt)
                    resultCnt = requiredWorkGroups;
                /**********************/

                V_OPENCL( k.setArg(0, first->getBuffer( ) ), "Error setting kernel argument" );
                V_OPENCL( k.setArg(1, szElements), "Error setting kernel argument" );
                V_OPENCL( k.setArg(2, transformFunctor), "Error setting kernel argument" );
                V_OPENCL( k.setArg(3, init), "Error setting kernel argument" );
                V_OPENCL( k.setArg(4, reduceFunctor), "Error setting kernel argument" );
                V_OPENCL( k.setArg(5, result), "Error setting kernel argument" );

                ::cl::LocalSpaceArg loc;
                loc.size_ = wgSize*sizeof(T);
                V_OPENCL( k.setArg(6, loc), "Error setting kernel argument" );

                // FIXME.  Need to ensure global size is a multiple of local WG size ,etc.
                // FIXME. Need to ensure that only required amount of work groups are spawned.
                l_Error = c.commandQueue().enqueueNDRangeKernel( 
                    k, 
                    ::cl::NullRange, 
                    ::cl::NDRange(resultCnt * wgSize), 
                    ::cl::NDRange(wgSize),
                                NULL,
                                NULL);
                V_OPENCL( l_Error, "enqueueNDRangeKernel() failed for transform_reduce() kernel" );

                V_OPENCL( c.commandQueue().finish(), "Error calling finish on the command queue" );
                // FIXME: replace with map:
                // FIXME: Note this depends on supplied functor having a version which can be compiled to run on the CPU
                std::vector<T> outputArray(resultCnt);

                T *h_result = (T*)c.commandQueue().enqueueMapBuffer(result, true, CL_MAP_READ, 0, sizeof(T)*resultCnt, NULL, NULL, &l_Error );
                V_OPENCL( l_Error, "Error calling map on the result buffer" );

                T acc = init;
                for(int i = 0; i < resultCnt; ++i){
                    acc = reduce_op(h_result[i], acc);
                }
                return acc;
            };
        }// end of namespace detail
    }// end of namespace cl
}// end of namespace bolt

#endif