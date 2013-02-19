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
            reduce(bolt::cl::control &ctl,
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
        T reduce(bolt::cl::control &ctl,
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
                        "template __attribute__((mangled_name(reduceInstantiated)))\n"
                        "__attribute__((reqd_work_group_size(64,1,1)))\n"
                        "kernel void reduceTemplate(\n"
                        "global " + kp->inPtrType + "* input_ptr,\n"
                         + kp->inIterType + " output_iter,\n"
                        "const int length,\n"
                        "global " + kp->binaryFuncName + "* userFunctor,\n"
                        "global " + kp->inPtrType + "* result,\n"
                        "local " + kp->inPtrType + "* scratch\n"
                        ");\n\n";

                    bolt::cl::constructAndCompileString( masterKernel, "reduce", reduce_kernels, instantiationString, cl_code, kp->inPtrType, kp->binaryFuncName, *ctl);

                };
            };
#ifdef ENABLE_TBB
            /*For documentation on the reduce object see below link
             *http://threadingbuildingblocks.org/docs/help/reference/algorithms/parallel_reduce_func.htm
             *The imperative form of parallel_reduce is used. 
             *
            */
            template <typename T, typename BinaryFunction>
            struct Reduce {
                T value;
                BinaryFunction op;
                //TODO - Decide on how many threads to spawn? Usually it should be equal to th enumber of cores
                //You might need to look at the tbb::split and there there cousin's 
                //
                Reduce(const BinaryFunction &_op) : op(_op), value(0) {}
                Reduce(const BinaryFunction &_op, const T &init) : op(_op), value(init) {}
                Reduce() : value(0) {}
                Reduce( Reduce& s, tbb::split ) : value(0) {}
                void operator()( const tbb::blocked_range<T*>& r ) {
                    T temp = value;
					//printf("r.size() = %d\n", r.size());
                    for( T* a=r.begin(); a!=r.end(); ++a ) {
                        temp = op(temp,*a);
                    }
                    value = temp;
                }
                //Join is called by the parent thread after the child finishes to execute.
                void join( Reduce& rhs ) 
                {
                    value = op(value,rhs.value);
                }
            };
#endif
            template<typename T, typename DVInputIterator, typename BinaryFunction> 
            T reduce_detect_random_access(bolt::cl::control &ctl, 
                const DVInputIterator& first,
                const DVInputIterator& last, 
                const T& init,
                const BinaryFunction& binary_op, 
                const std::string& cl_code, 
                std::input_iterator_tag)  
            {
                //  TODO:  It should be possible to support non-random_access_iterator_tag iterators, if we copied the data 
                //  to a temporary buffer.  Should we?
                static_assert( false, "Bolt only supports random access iterator types" );
            }

            template<typename T, typename DVInputIterator, typename BinaryFunction> 
            T reduce_detect_random_access(bolt::cl::control &ctl, 
                const DVInputIterator& first,
                const DVInputIterator& last, 
                const T& init,
                const BinaryFunction& binary_op, 
                const std::string& cl_code, 
                std::random_access_iterator_tag)  
            {
                return reduce_pick_iterator( ctl, first, last, init, binary_op, cl_code,
                    std::iterator_traits< DVInputIterator >::iterator_category( ) );
            }

            // This template is called after we detect random access iterators
            // This is called strictly for any non-device_vector iterator
            template<typename T, typename InputIterator, typename BinaryFunction> 
            T reduce_pick_iterator(bolt::cl::control &ctl, 
                const InputIterator& first,
                const InputIterator& last, 
                const T& init,
                const BinaryFunction& binary_op, 
                const std::string& cl_code,
                std::random_access_iterator_tag )
            {
                /*************/
                typedef typename std::iterator_traits<InputIterator>::value_type iType;
                size_t szElements = (size_t)(last - first); 
                if (szElements == 0)
                    return init;
                /*TODO - probably the forceRunMode should be replaced by getRunMode and setRunMode*/
                // Its a dynamic choice. See the reduce Test Code
                // What should we do if the run mode is automatic. Currently it goes to the last else statement
                //How many threads we should spawn? 
                //Need to look at how to control the number of threads spawned.
                const bolt::cl::control::e_RunMode runMode = ctl.forceRunMode();  
                if (runMode == bolt::cl::control::SerialCpu) {
                    std::cout << "The SerialCpu version of reduce is not implemented yet." << std ::endl;
                    throw ::cl::Error( CL_INVALID_OPERATION, "The SerialCpu version of reduce is not implemented yet." );
                    return init;
                } else if (runMode == bolt::cl::control::MultiCoreCpu) {
#ifdef ENABLE_TBB
                    std::cout << "The MultiCoreCpu version of reduce uses TBB." << std ::endl;
					tbb::task_scheduler_init initialize(tbb::task_scheduler_init::automatic);
                    Reduce<iType, BinaryFunction> reduce_op(binary_op, init);
                    tbb::parallel_reduce( tbb::blocked_range<iType*>( &*first, (iType*)&*(last-1) + 1), reduce_op );
                    return reduce_op.value;
#else
                    std::cout << "The MultiCoreCpu version of reduce is not enabled. " << std ::endl;
                    throw ::cl::Error( CL_INVALID_OPERATION, "The MultiCoreCpu version of reduce is not enabled to be built." );
                    return init;
#endif
                } else {
                device_vector< iType > dvInput( first, last, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, ctl );
                return reduce_enqueue( ctl, dvInput.begin(), dvInput.end(), init, binary_op, cl_code);
                }
            };

            // This template is called after we detect random access iterators
            // This is called strictly for iterators that are derived from device_vector< T >::iterator
            template<typename T, typename DVInputIterator, typename BinaryFunction> 
            T reduce_pick_iterator(bolt::cl::control &ctl, 
                const DVInputIterator& first,
                const DVInputIterator& last, 
                const T& init,
                const BinaryFunction& binary_op, 
                const std::string& cl_code,
                bolt::cl::device_vector_tag )
            {
                size_t szElements = (size_t)(last - first); 
                if (szElements == 0)
                    return init;

                const bolt::cl::control::e_RunMode runMode = ctl.forceRunMode();  
                if (runMode == bolt::cl::control::SerialCpu) {
                    std::cout << "The SerialCpu version of reduce is not implemented yet." << std ::endl;
                    throw ::cl::Error( CL_INVALID_OPERATION, "The SerialCpu version of reduce is not implemented yet." );
                    return init;
                } else if (runMode == bolt::cl::control::MultiCoreCpu) {
                    /*TODO - ASK  - should we copy the device_vector to host memory, process the result and then store back the result into the device_vector.*/
                    std::cout << "The MultiCoreCpu version of reduce on device_vector is not supported." << std ::endl;
                    throw ::cl::Error( CL_INVALID_OPERATION, "The MultiCoreCpu version of reduce on device_vector is not supported." );
                    return init;
                } else {
                return reduce_enqueue( ctl, first, last, init, binary_op, cl_code);
                }
            }

            // This template is called after we detect random access iterators
            // This is called strictly for iterators that are derived from device_vector< T >::iterator
            template<typename T, typename DVInputIterator, typename BinaryFunction> 
            T reduce_pick_iterator(bolt::cl::control &ctl, 
                const DVInputIterator& first,
                const DVInputIterator& last, 
                const T& init,
                const BinaryFunction& binary_op, 
                const std::string& cl_code,
                bolt::cl::fancy_iterator_tag )
            {
                return reduce_enqueue( ctl, first, last, init, binary_op, cl_code);
            }

            //----
            // This is the base implementation of reduction that is called by all of the convenience wrappers below.
            // first and last must be iterators from a DeviceVector
            template<typename T, typename DVInputIterator, typename BinaryFunction> 
            T reduce_enqueue(bolt::cl::control &ctl, 
                const DVInputIterator& first,
                const DVInputIterator& last, 
                const T& init,
                const BinaryFunction& binary_op, 
                const std::string& cl_code )
            {
                typedef typename std::iterator_traits< DVInputIterator >::value_type iType;

                static boost::once_flag initOnlyOnce;
                static ::cl::Kernel masterKernel;

                kernelParamsReduce args( TypeName< iType >::get( ), TypeName< DVInputIterator >::get( ), 
                    TypeName< BinaryFunction >::get( ) );

                std::string typeDefinitions = cl_code + ClCode< iType >::get( ) + ClCode< DVInputIterator >::get( );
                typeDefinitions += ClCode< BinaryFunction >::get( );

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
                ALIGNED( 256 ) BinaryFunction aligned_reduce( binary_op );
                //::cl::Buffer userFunctor(ctl.context(), CL_MEM_USE_HOST_PTR|CL_MEM_READ_ONLY, sizeof( aligned_reduce ),
                //  &aligned_reduce );
                control::buffPointer userFunctor = ctl.acquireBuffer( sizeof( aligned_reduce ), 
                    CL_MEM_USE_HOST_PTR|CL_MEM_READ_ONLY, &aligned_reduce );

                // ::cl::Buffer result(ctl.context(), CL_MEM_ALLOC_HOST_PTR|CL_MEM_WRITE_ONLY, sizeof( iType ) * numWG);
                control::buffPointer result = ctl.acquireBuffer( sizeof( iType ) * numWG, 
                    CL_MEM_ALLOC_HOST_PTR|CL_MEM_WRITE_ONLY );

                cl_uint szElements = static_cast< cl_uint >( first.distance_to(last ) );

                V_OPENCL( masterKernel.setArg(0, first.getBuffer( ) ), "Error setting kernel argument" );
                V_OPENCL( masterKernel.setArg(1, first.gpuPayloadSize( ), &first.gpuPayload( ) ), "Error setting a kernel argument" );
                V_OPENCL( masterKernel.setArg(2, szElements), "Error setting kernel argument" );
                V_OPENCL( masterKernel.setArg(3, *userFunctor), "Error setting kernel argument" );
                V_OPENCL( masterKernel.setArg(4, *result), "Error setting kernel argument" );

                ::cl::LocalSpaceArg loc;
                loc.size_ = wgSize*sizeof(iType);
                V_OPENCL( masterKernel.setArg(5, loc), "Error setting kernel argument" );

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
