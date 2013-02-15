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
#ifdef ENABLE_TBB
//TBB Includes
#include "tbb/parallel_reduce.h"
#include "tbb/blocked_range.h"
#include "tbb/task_scheduler_init.h"
#endif

namespace bolt {
    namespace cl {

        template<typename ForwardIterator> 
      ForwardIterator max_element(ForwardIterator first, 
            ForwardIterator last, 
            const std::string& cl_code)
        {
            typedef typename std::iterator_traits<ForwardIterator>::value_type T;
            return min_element(bolt::cl::control::getDefault(), first, last, bolt::cl::greater<T>(), cl_code);
        };


        template<typename ForwardIterator,typename BinaryPredicate> 
        ForwardIterator max_element(ForwardIterator first, 
            ForwardIterator last,  
            BinaryPredicate binary_op, 
            const std::string& cl_code)  
        {
            return min_element(bolt::cl::control::getDefault(), first, last, binary_op, cl_code);
        };



        template<typename ForwardIterator> 
        ForwardIterator  max_element(bolt::cl::control &ctl,
            ForwardIterator first, 
            ForwardIterator last, 
            const std::string& cl_code)
        {
            typedef typename std::iterator_traits<ForwardIterator>::value_type T;
            return min_element(ctl, first, last, bolt::cl::greater<T>(),cl_code);
        };

        // This template is called by all other "convenience" version of max_element.
        // It also implements the CPU-side mappings of the algorithm for SerialCpu and MultiCoreCpu
        template<typename ForwardIterator, typename BinaryPredicate> 
        ForwardIterator max_element(bolt::cl::control &ctl,
            ForwardIterator first, 
            ForwardIterator last,  
            BinaryPredicate binary_op, 
            const std::string& cl_code)  
        {
            const bolt::cl::control::e_RunMode runMode = ctl.forceRunMode();  // could be dynamic choice some day.

            if (runMode == bolt::cl::control::SerialCpu) {
                return std::min_element(first, last, binary_op);
            } else {
                return detail::min_element_detect_random_access(ctl, first, last, binary_op, cl_code,
                    std::iterator_traits< ForwardIterator >::iterator_category( ) );
            }
        };

    }

};

namespace bolt {
    namespace cl {

        template<typename ForwardIterator> 
      ForwardIterator min_element(ForwardIterator first, 
            ForwardIterator last, 
            const std::string& cl_code)
        {
            typedef typename std::iterator_traits<ForwardIterator>::value_type T;
            return min_element(bolt::cl::control::getDefault(), first, last, bolt::cl::less<T>(), cl_code);
        };


        template<typename ForwardIterator,typename BinaryPredicate> 
        ForwardIterator min_element(ForwardIterator first, 
            ForwardIterator last,  
            BinaryPredicate binary_op, 
            const std::string& cl_code)  
        {
            return min_element(bolt::cl::control::getDefault(), first, last, binary_op, cl_code);
        };



        template<typename ForwardIterator> 
        ForwardIterator  min_element(bolt::cl::control &ctl,
            ForwardIterator first, 
            ForwardIterator last, 
            const std::string& cl_code)
        {
            typedef typename std::iterator_traits<ForwardIterator>::value_type T;
            return min_element(ctl, first, last, bolt::cl::less<T>(),cl_code);
        };

        // This template is called by all other "convenience" version of min_element.
        // It also implements the CPU-side mappings of the algorithm for SerialCpu and MultiCoreCpu
        template<typename ForwardIterator, typename BinaryPredicate> 
        ForwardIterator min_element(bolt::cl::control &ctl,
            ForwardIterator first, 
            ForwardIterator last,  
            BinaryPredicate binary_op, 
            const std::string& cl_code)  
        {
            const bolt::cl::control::e_RunMode runMode = ctl.forceRunMode();  // could be dynamic choice some day.

            if (runMode == bolt::cl::control::SerialCpu) {
                return std::min_element(first, last, binary_op);
            } else {
                return detail::min_element_detect_random_access(ctl, first, last, binary_op, cl_code,
                    std::iterator_traits< ForwardIterator >::iterator_category( ) );
            }
        };

    }

};


namespace bolt {
    namespace cl {
        namespace detail {
            struct kernelParamsReduce
            {
                const std::string inPtrType;
                const std::string inIterType;
                const std::string binaryFuncName;

                kernelParamsReduce( const std::string& iPtrType, const std::string& iIterType, const std::string& binaryFuncType ): 
                inPtrType( iPtrType ), inIterType( iIterType ), 
                binaryFuncName( binaryFuncType )
                {}
            };

            // FIXME - move to cpp file
            struct CallCompiler_Reduce {
                static void constructAndCompile(::cl::Kernel *masterKernel,  std::string cl_code, kernelParamsReduce* kp, const control *ctl) {

                    const std::string instantiationString = 
                        "// Host generates this instantiation string with user-specified value type and functor\n"
                        "template __attribute__((mangled_name(min_elementInstantiated)))\n"
                        "__attribute__((reqd_work_group_size(64,1,1)))\n"
                        "kernel void min_elementTemplate(\n"
                        "global " + kp->inPtrType + "* input_ptr,\n"
                         + kp->inIterType + " output_iter,\n"
                        "const int length,\n"
                        "global " + kp->binaryFuncName + "* userFunctor,\n"
                        "global " + kp->inPtrType + "* result,\n"
                        "local " + kp->inPtrType + "* scratch,\n"
                        "local " + kp->inPtrType + "* scratch_index\n"
                        ");\n\n";

                    bolt::cl::constructAndCompileString( masterKernel, "min_element", min_element_kernels, instantiationString, cl_code, kp->inPtrType, kp->binaryFuncName, *ctl);

                };
            };

            //----
            // This is the base implementation of reduction that is called by all of the convenience wrappers below.
            // first and last must be iterators from a DeviceVector
            template<typename DVInputIterator, typename BinaryPredicate> 
            int min_element_enqueue(bolt::cl::control &ctl, 
                const DVInputIterator& first,
                const DVInputIterator& last, 
                const BinaryPredicate& binary_op, 
                const std::string& cl_code )
            {
                typedef typename std::iterator_traits< DVInputIterator >::value_type iType;

                static boost::once_flag initOnlyOnce;
                static ::cl::Kernel masterKernel;

                kernelParamsReduce args( TypeName< iType >::get( ), TypeName< DVInputIterator >::get( ), 
                    TypeName< BinaryPredicate >::get( ) );

                std::string typeDefinitions = cl_code + ClCode< iType >::get( ) + ClCode< DVInputIterator >::get( );
                typeDefinitions += ClCode< BinaryPredicate >::get( );

                // For user-defined types, the user must create a TypeName trait which returns the name of the class 
                //  - note use of TypeName<>::get to retreive the name here.
                boost::call_once( initOnlyOnce, boost::bind( CallCompiler_Reduce::constructAndCompile, &masterKernel, 
                    typeDefinitions, &args, &ctl) );

                // Set up shape of launch grid and buffers:
                cl_uint computeUnits     = ctl.device().getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>();
                int wgPerComputeUnit =  ctl.wgPerComputeUnit(); 
                size_t numWG = computeUnits * wgPerComputeUnit;

                cl_int l_Error = CL_SUCCESS;
                const size_t wgSize  = masterKernel.getWorkGroupInfo< CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE >( 
                    ctl.device( ), &l_Error );
                V_OPENCL( l_Error, "Error querying kernel for CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE" );

                // Create buffer wrappers so we can access the host functors, for read or writing in the kernel
                ALIGNED( 256 ) BinaryPredicate aligned_reduce( binary_op );
                //::cl::Buffer userFunctor(ctl.context(), CL_MEM_USE_HOST_PTR|CL_MEM_READ_ONLY, sizeof( aligned_reduce ),
                //  &aligned_reduce );
                control::buffPointer userFunctor = ctl.acquireBuffer( sizeof( aligned_reduce ), 
                    CL_MEM_USE_HOST_PTR|CL_MEM_READ_ONLY, &aligned_reduce );

                // ::cl::Buffer result(ctl.context(), CL_MEM_ALLOC_HOST_PTR|CL_MEM_WRITE_ONLY, sizeof( iType ) * numWG);
                control::buffPointer result = ctl.acquireBuffer( sizeof( iType ) * numWG, 
                    CL_MEM_ALLOC_HOST_PTR|CL_MEM_WRITE_ONLY );

                cl_uint szElements = static_cast< cl_uint >( std::distance( first, last ) );

                V_OPENCL( masterKernel.setArg(0, first.getBuffer( ) ), "Error setting kernel argument" );
                V_OPENCL( masterKernel.setArg(1, first.gpuPayloadSize( ), &first.gpuPayload( ) ), "Error setting a kernel argument" );
                V_OPENCL( masterKernel.setArg(2, szElements), "Error setting kernel argument" );
                V_OPENCL( masterKernel.setArg(3, *userFunctor), "Error setting kernel argument" );
                V_OPENCL( masterKernel.setArg(4, *result), "Error setting kernel argument" );

                ::cl::LocalSpaceArg loc,loc2;
                loc.size_ = wgSize*sizeof(iType);
                loc2 = loc;
                V_OPENCL( masterKernel.setArg(5, loc), "Error setting kernel argument" );
                V_OPENCL( masterKernel.setArg(6, loc2), "Error setting kernel argument" );


                l_Error = ctl.commandQueue().enqueueNDRangeKernel(
                    masterKernel, 
                    ::cl::NullRange, 
                    ::cl::NDRange(numWG * wgSize), 
                    ::cl::NDRange(wgSize));
                V_OPENCL( l_Error, "enqueueNDRangeKernel() failed for reduce() kernel" );

                ::cl::Event l_mapEvent;
                iType *h_result = (iType*)ctl.commandQueue().enqueueMapBuffer(*result, false, CL_MAP_READ, 0, 
                    sizeof(iType)*numWG, NULL, &l_mapEvent, &l_Error );
                V_OPENCL( l_Error, "Error calling map on the result buffer" );

                //  Finish the tail end of the reduction on host side; the compute device reduces within the workgroups, 
                //  with one result per workgroup
                size_t ceilNumWG = static_cast< size_t >( std::ceil( static_cast< float >( szElements ) / wgSize) );
                bolt::cl::minimum<size_t>  min_size_t;
                size_t numTailReduce = min_size_t( ceilNumWG, numWG );

                bolt::cl::wait(ctl, l_mapEvent);

                int minele_indx = static_cast< iType >( h_result[0] );
                iType minele =  *(first + h_result[0]) ;

                for(int i = 1; i < numTailReduce; ++i)
                {

                    bool stat = binary_op(minele,*(first + h_result[i]));
                    minele = stat ? minele : *(first + h_result[i]);
                    minele_indx =  stat ? minele_indx : h_result[i];                  
                     
                }

                return minele_indx;
            }

            template<typename ForwardIterator, typename BinaryPredicate> 
            ForwardIterator min_element_detect_random_access(bolt::cl::control &ctl, 
                const ForwardIterator& first,
                const ForwardIterator& last, 
                const BinaryPredicate& binary_op, 
                const std::string& cl_code, 
                std::input_iterator_tag)  
            {
                //  TODO:  It should be possible to support non-random_access_iterator_tag iterators, if we copied the data 
                //  to a temporary buffer.  Should we?
                static_assert( false, "Bolt only supports random access iterator types" );
            }


            template<typename ForwardIterator, typename BinaryPredicate> 
            ForwardIterator min_element_detect_random_access(bolt::cl::control &ctl, 
                const ForwardIterator& first,
                const ForwardIterator& last, 
                const BinaryPredicate& binary_op, 
                const std::string& cl_code, 
                std::random_access_iterator_tag)  
            {
                return min_element_pick_iterator( ctl, first, last,  binary_op, cl_code,
                    std::iterator_traits< ForwardIterator >::iterator_category( ) );
            }

            // This template is called after we detect random access iterators
            // This is called strictly for any non-device_vector iterator
            template<typename ForwardIterator, typename BinaryPredicate> 
            ForwardIterator min_element_pick_iterator(bolt::cl::control &ctl, 
                const ForwardIterator& first,
                const ForwardIterator& last, 
                const BinaryPredicate& binary_op, 
                const std::string& cl_code,
                std::random_access_iterator_tag )
            {
                /*************/
                typedef typename std::iterator_traits<ForwardIterator>::value_type iType;
                size_t szElements = (size_t)(last - first); 
                if (szElements == 0)
                    return last;
                /*TODO - probably the forceRunMode should be replaced by getRunMode and setRunMode*/
                // Its a dynamic choice. See the reduce Test Code
                // What should we do if the run mode is automatic. Currently it goes to the last else statement
                //How many threads we should spawn? 
                //Need to look at how to control the number of threads spawned.
                const bolt::cl::control::e_RunMode runMode = ctl.forceRunMode();  
                if (runMode == bolt::cl::control::SerialCpu) {
                    std::cout << "The SerialCpu version of reduce is not implemented yet." << std ::endl;
                    throw ::cl::Error( CL_INVALID_OPERATION, "The SerialCpu version of reduce is not implemented yet." );
                    return last;
                } else if (runMode == bolt::cl::control::MultiCoreCpu) {

                    std::cout << "The MultiCoreCpu version of reduce is not enabled. " << std ::endl;
                    throw ::cl::Error( CL_INVALID_OPERATION, "The MultiCoreCpu version of reduce is not enabled to be built." );
                    return last;

                } else {
                device_vector< iType > dvInput( first, last, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, ctl );
                int  dvminele = min_element_enqueue( ctl, dvInput.begin(), dvInput.end(), binary_op, cl_code);
                                                                              
                return first + dvminele ;
                
                }
            };

            // This template is called after we detect random access iterators
            // This is called strictly for iterators that are derived from device_vector< T >::iterator
            template<typename DVInputIterator, typename BinaryPredicate> 
            DVInputIterator min_element_pick_iterator(bolt::cl::control &ctl, 
                const DVInputIterator& first,
                const DVInputIterator& last, 
                const BinaryPredicate& binary_op, 
                const std::string& cl_code,
                bolt::cl::device_vector_tag )
            {
                size_t szElements = (size_t)(last - first); 
                if (szElements == 0)
                    return last;

                const bolt::cl::control::e_RunMode runMode = ctl.forceRunMode();  
                if (runMode == bolt::cl::control::SerialCpu) {
                    std::cout << "The SerialCpu version of min_element is not implemented yet." << std ::endl;
                    throw ::cl::Error( CL_INVALID_OPERATION, "The SerialCpu version of min_element is not implemented yet." );
                    return last;
                } else if (runMode == bolt::cl::control::MultiCoreCpu) {
                    /*TODO - ASK  - should we copy the device_vector to host memory, process the result and then store back the result into the device_vector.*/
                    std::cout << "The MultiCoreCpu version of min_element on device_vector is not supported." << std ::endl;
                    throw ::cl::Error( CL_INVALID_OPERATION, "The MultiCoreCpu version of min_element on device_vector is not supported." );
                    return last;
                } else {
                int pos =  min_element_enqueue( ctl, first, last,  binary_op, cl_code);
                return first+pos;
                }
            }

            // This template is called after we detect random access iterators
            // This is called strictly for iterators that are derived from device_vector< T >::iterator
            template<typename DVInputIterator, typename BinaryPredicate> 
            DVInputIterator min_element_pick_iterator(bolt::cl::control &ctl, 
                const DVInputIterator& first,
                const DVInputIterator& last, 
                const BinaryPredicate& binary_op, 
                const std::string& cl_code,
                bolt::cl::fancy_iterator_tag )
            {
                int pos = min_element_enqueue( ctl, first, last,  binary_op, cl_code);
                return first+pos;
            }


        }
    }
}

#endif
