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

#if !defined( REDUCE_INL )
#define REDUCE_INL
#pragma once

#include <algorithm>

#include <boost/thread/once.hpp>
#include <boost/bind.hpp>

#include "bolt/cl/bolt.h"
#include "bolt/cl/functional.h"

namespace bolt {
    namespace cl {

        template<typename InputIterator, typename T, typename BinaryFunction> 
        T reduce(InputIterator first, 
            InputIterator last,  
            T init,
            BinaryFunction binary_op, 
            const std::string& cl_code)  
        {
            return reduce(bolt::cl::control::getDefault(), first, last, init, binary_op, cl_code);
        };


        template<typename InputIterator, typename T> 
        typename std::iterator_traits<InputIterator>::value_type
            reduce(InputIterator first, 
            InputIterator last, 
            T init,
            const std::string& cl_code)
        {
            typedef typename std::iterator_traits<InputIterator>::value_type iType;
            return reduce(bolt::cl::control::getDefault(), first, last, init, bolt::cl::plus<iType>(), cl_code);
        };

        template<typename InputIterator, typename T> 
        typename std::iterator_traits<InputIterator>::value_type
            reduce(const bolt::cl::control &ctl,
            InputIterator first, 
            InputIterator last, 
            T init,
            const std::string& cl_code)
        {
            typedef typename std::iterator_traits<InputIterator>::value_type iType;
            return reduce(ctl, first, last, init, bolt::cl::plus< iType >( ), cl_code);
        };

        // This template is called by all other "convenience" version of reduce.
        // It also implements the CPU-side mappings of the algorithm for SerialCpu and MultiCoreCpu
        template<typename InputIterator, typename T, typename BinaryFunction> 
        T reduce(const bolt::cl::control &ctl,
            InputIterator first, 
            InputIterator last,  
            T init,
            BinaryFunction binary_op, 
            const std::string& cl_code)  
        {
            const bolt::cl::control::e_RunMode runMode = ctl.forceRunMode();  // could be dynamic choice some day.

            if (runMode == bolt::cl::control::SerialCpu) {
                return std::accumulate(first, last, init, binary_op);
            } else {
                return detail::reduce_detect_random_access(ctl, first, last, init, binary_op, cl_code,
                    std::iterator_traits< InputIterator >::iterator_category( ) );
            }
        };

    }

};


namespace bolt {
    namespace cl {
        namespace detail {

            // FIXME - move to cpp file
            struct CallCompiler_Reduce {
                static void constructAndCompile(::cl::Kernel *masterKernel,  std::string cl_code, std::string valueTypeName,  std::string functorTypeName, const control *ctl) {

                    const std::string instantiationString = 
                        "// Host generates this instantiation string with user-specified value type and functor\n"
                        "template __attribute__((mangled_name(reduceInstantiated)))\n"
                        "__attribute__((reqd_work_group_size(64,1,1)))\n"
                        "kernel void reduceTemplate(\n"
                        "global " + valueTypeName + "* A,\n"
                        "const int length,\n"
                        "global " + functorTypeName + "* userFunctor,\n"
                        "global " + valueTypeName + "* result,\n"
                        "local " + valueTypeName + "* scratch\n"
                        ");\n\n";

                    bolt::cl::constructAndCompileString( masterKernel, "reduce", reduce_kernels, instantiationString, cl_code, valueTypeName, functorTypeName, *ctl);

                };
            };


            template<typename T, typename DVInputIterator, typename BinaryFunction> 
            T reduce_detect_random_access(const bolt::cl::control &ctl, 
                DVInputIterator first,
                DVInputIterator last, 
                T init,
                BinaryFunction binary_op, 
                const std::string& cl_code, 
                std::input_iterator_tag)  
            {
                //  TODO:  It should be possible to support non-random_access_iterator_tag iterators, if we copied the data 
                //  to a temporary buffer.  Should we?
                static_assert( false, "Bolt only supports random access iterator types" );
            }

            template<typename T, typename DVInputIterator, typename BinaryFunction> 
            T reduce_detect_random_access(const bolt::cl::control &ctl, 
                DVInputIterator first,
                DVInputIterator last, 
                T init,
                BinaryFunction binary_op, 
                const std::string& cl_code, 
                std::random_access_iterator_tag)  
            {
                return reduce_pick_iterator( ctl, first, last, init, binary_op, cl_code );
            }

            // This template is called after we detect random access iterators
            // This is called strictly for any non-device_vector iterator
            template<typename T, typename InputIterator, typename BinaryFunction> 
            typename std::enable_if< !std::is_base_of<typename device_vector<typename std::iterator_traits<InputIterator>::value_type>::iterator,InputIterator>::value, T >::type
                reduce_pick_iterator(const bolt::cl::control &ctl, 
                InputIterator first,
                InputIterator last, 
                T init,
                BinaryFunction binary_op, 
                const std::string& cl_code)
            {
                typedef typename std::iterator_traits<InputIterator>::value_type iType;
                device_vector< iType > dvInput( first, last, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, ctl );

                return reduce_enqueue( ctl, dvInput.begin(), dvInput.end(), init, binary_op, cl_code);
            };

            // This template is called after we detect random access iterators
            // This is called strictly for iterators that are derived from device_vector< T >::iterator
            template<typename T, typename DVInputIterator, typename BinaryFunction> 
            typename std::enable_if< std::is_base_of<typename device_vector<typename std::iterator_traits<DVInputIterator>::value_type>::iterator,DVInputIterator>::value, T >::type
                reduce_pick_iterator(const bolt::cl::control &ctl, 
                DVInputIterator first,
                DVInputIterator last, 
                T init,
                BinaryFunction binary_op, 
                const std::string& cl_code)
            {
                return reduce_enqueue( ctl, first, last, init, binary_op, cl_code);
            }

            //----
            // This is the base implementation of reduction that is called by all of the convenience wrappers below.
            // first and last must be iterators from a DeviceVector
            template<typename T, typename DVInputIterator, typename BinaryFunction> 
            T reduce_enqueue(const bolt::cl::control &ctl, 
                DVInputIterator first,
                DVInputIterator last, 
                T init,
                BinaryFunction binary_op, 
                const std::string& cl_code )
            {
                typedef typename std::iterator_traits< DVInputIterator >::value_type iType;

                static boost::once_flag initOnlyOnce;
                static ::cl::Kernel masterKernel;

                // For user-defined types, the user must create a TypeName trait which returns the name of the class - note use of TypeName<>::get to retreive the name here.
                boost::call_once( initOnlyOnce, boost::bind( CallCompiler_Reduce::constructAndCompile, &masterKernel, cl_code + ClCode<BinaryFunction>::get(), TypeName<iType>::get(),  TypeName<BinaryFunction>::get(), &ctl) );

                // Set up shape of launch grid and buffers:
                cl_uint computeUnits     = ctl.device().getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>();
                int wgPerComputeUnit =  ctl.wgPerComputeUnit(); 
                size_t numWG = computeUnits * wgPerComputeUnit;

                cl_int l_Error = CL_SUCCESS;
                const size_t wgSize  = masterKernel.getWorkGroupInfo< CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE >( ctl.device( ), &l_Error );
                V_OPENCL( l_Error, "Error querying kernel for CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE" );

                // Create buffer wrappers so we can access the host functors, for read or writing in the kernel
                ::cl::Buffer userFunctor(ctl.context(), CL_MEM_USE_HOST_PTR, sizeof(binary_op), &binary_op );   // Create buffer wrapper so we can access host parameters.
                //std::cout << "sizeof(Functor)=" << sizeof(binary_op) << std::endl;

                ::cl::Buffer result(ctl.context(), CL_MEM_ALLOC_HOST_PTR|CL_MEM_WRITE_ONLY, sizeof( iType ) * numWG);

                cl_uint szElements = static_cast< cl_uint >( std::distance( first, last ) );

                V_OPENCL( masterKernel.setArg(0, first->getBuffer( ) ), "Error setting kernel argument" );
                V_OPENCL( masterKernel.setArg(1, szElements), "Error setting kernel argument" );
                V_OPENCL( masterKernel.setArg(2, userFunctor), "Error setting kernel argument" );
                V_OPENCL( masterKernel.setArg(3, result), "Error setting kernel argument" );

                ::cl::LocalSpaceArg loc;
                loc.size_ = wgSize*sizeof(iType);
                V_OPENCL( masterKernel.setArg(4, loc), "Error setting kernel argument" );

                l_Error = ctl.commandQueue().enqueueNDRangeKernel(
                    masterKernel, 
                    ::cl::NullRange, 
                    ::cl::NDRange(numWG * wgSize), 
                    ::cl::NDRange(wgSize));
                V_OPENCL( l_Error, "enqueueNDRangeKernel() failed for reduce() kernel" );

                ::cl::Event l_mapEvent;
                iType *h_result = (iType*)ctl.commandQueue().enqueueMapBuffer(result, false, CL_MAP_READ, 0, sizeof(iType)*numWG, NULL, &l_mapEvent, &l_Error );
                V_OPENCL( l_Error, "Error calling map on the result buffer" );



                //  Finish the tail end of the reduction on host side; the compute device reduces within the workgroups, with one result per workgroup
                size_t ceilNumWG = static_cast< size_t >( std::ceil( static_cast< float >( szElements ) / wgSize) );
				bolt::cl::minimum<size_t>  min_size_t;
                size_t numTailReduce = min_size_t( ceilNumWG, numWG );

                bolt::cl::wait(ctl, l_mapEvent);

                iType acc = static_cast< iType >( init );
                for(int i = 0; i < numTailReduce; ++i)
                {
                    acc = binary_op(acc, h_result[i]);
                }

                return acc;
            };
        }
    }
}

#endif
