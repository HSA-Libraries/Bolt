/***************************************************************************                                                                                     
*   Copyright 2012 - 2013 Advanced Micro Devices, Inc.                                     
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



        template<typename InputIterator> 
        typename std::iterator_traits<InputIterator>::value_type
            reduce(InputIterator first, 
            InputIterator last, 
            const std::string& cl_code)
        {
            typedef typename std::iterator_traits<InputIterator>::value_type iType;
            return reduce(bolt::cl::control::getDefault(), first, last, iType(), bolt::cl::plus<iType>(), cl_code);
        };

        template<typename InputIterator> 
        typename std::iterator_traits<InputIterator>::value_type
            reduce(bolt::cl::control &ctl,
            InputIterator first, 
            InputIterator last, 
            const std::string& cl_code)
        {
            typedef typename std::iterator_traits<InputIterator>::value_type iType;
            return reduce(ctl, first, last, iType(), bolt::cl::plus< iType >( ), cl_code);
        };



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
         T  reduce(InputIterator first, 
            InputIterator last, 
            T init,
            const std::string& cl_code)
        {
            typedef typename std::iterator_traits<InputIterator>::value_type iType;
            return reduce(bolt::cl::control::getDefault(), first, last, init, bolt::cl::plus<iType>(), cl_code);
        };

        template<typename InputIterator, typename T> 
         T  reduce(bolt::cl::control &ctl,
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
            bolt::cl::control::e_RunMode runMode = ctl.getForceRunMode();  // could be dynamic choice some day.
            if(runMode == bolt::cl::control::Automatic)
            {
                  runMode = ctl.getDefaultPathToRun();
            }
            if (runMode == bolt::cl::control::SerialCpu) {
                return std::accumulate(first, last, init, binary_op);
            } else {
                return detail::reduce_detect_random_access(ctl, first, last, init, binary_op, cl_code,
                    std::iterator_traits< InputIterator >::iterator_category( ) );
            }
        }

    }

};


namespace bolt {
    namespace cl {
        namespace detail {

            ///////////////

        enum ReduceTypes {reduce_iValueType, reduce_iIterType, reduce_BinaryFunction, reduce_end };

        ///////////////////////////////////////////////////////////////////////
        //Kernel Template Specializer
        ///////////////////////////////////////////////////////////////////////
        class Reduce_KernelTemplateSpecializer : public KernelTemplateSpecializer
            {
            public:

            Reduce_KernelTemplateSpecializer() : KernelTemplateSpecializer()
                {
                    addKernelName( "reduceTemplate" );
                }

            const ::std::string operator() ( const ::std::vector<::std::string>& typeNames ) const
            {
                const std::string templateSpecializationString = 
                        "// Host generates this instantiation string with user-specified value type and functor\n"
                        "template __attribute__((mangled_name(" + name(0) + "Instantiated)))\n"
                        "__attribute__((reqd_work_group_size(64,1,1)))\n"
                        "kernel void reduceTemplate(\n"
                        "global " + typeNames[reduce_iValueType] + "* input_ptr,\n"
                         + typeNames[reduce_iIterType] + " output_iter,\n"
                        "const int length,\n"
                        "global " + typeNames[reduce_BinaryFunction] + "* userFunctor,\n"
                        "global " + typeNames[reduce_iValueType] + "* result,\n"
                        "local " + typeNames[reduce_iValueType] + "* scratch\n"
                        ");\n\n";

                return templateSpecializationString;
            }
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
                bool flag;

                //TODO - Decide on how many threads to spawn? Usually it should be equal to th enumber of cores
                //You might need to look at the tbb::split and there there cousin's 
                //
                Reduce(const BinaryFunction &_op) : op(_op), value(0) {}
                Reduce(const BinaryFunction &_op, const T &init) : op(_op), value(init), flag(FALSE) {}
                Reduce() : value(0) {}
                Reduce( Reduce& s, tbb::split ) : flag(TRUE), op(s.op) {}
                void operator()( const tbb::blocked_range<T*>& r ) {
                    T temp = value;
                    for( T* a=r.begin(); a!=r.end(); ++a ) {
                      if(flag){
                        temp = *a;
                        flag = FALSE;
                      }
                      else
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
                bolt::cl::control::e_RunMode runMode = ctl.getForceRunMode();  // could be dynamic choice some day.
                if(runMode == bolt::cl::control::Automatic)
                {
                    runMode = ctl.getDefaultPathToRun();
                }
                if (runMode == bolt::cl::control::SerialCpu) {
                    return std::accumulate(first, last, init,binary_op) ;
                } else if (runMode == bolt::cl::control::MultiCoreCpu) {
#ifdef ENABLE_TBB
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
                typedef typename std::iterator_traits<DVInputIterator>::value_type iType;
                size_t szElements = static_cast<size_t>(std::distance(first, last) ); 
                if (szElements == 0)
                    return init;

                bolt::cl::control::e_RunMode runMode = ctl.getForceRunMode();  // could be dynamic choice some day.
                if(runMode == bolt::cl::control::Automatic)
                {
                    runMode = ctl.getDefaultPathToRun();
                }
                if (runMode == bolt::cl::control::SerialCpu) {
                    ::cl::Event serialCPUEvent;
                    cl_int l_Error = CL_SUCCESS;
                    T reduceResult;
                    /*Map the device buffer to CPU*/
                    iType *reduceInputBuffer = (iType*)ctl.getCommandQueue().enqueueMapBuffer(first.getBuffer(), false, CL_MAP_READ|CL_MAP_WRITE,0, sizeof(iType) * szElements, 
                                               NULL, &serialCPUEvent, &l_Error );       
                    serialCPUEvent.wait();
                    reduceResult = std::accumulate(reduceInputBuffer, reduceInputBuffer + szElements, init, binary_op) ;
                    /*Unmap the device buffer back to device memory. This will copy the host modified buffer back to the device*/
                    ctl.getCommandQueue().enqueueUnmapMemObject(first.getBuffer(), reduceInputBuffer);
                     return reduceResult;
                } else if (runMode == bolt::cl::control::MultiCoreCpu) {
#ifdef ENABLE_TBB
                    ::cl::Event multiCoreCPUEvent;
                    cl_int l_Error = CL_SUCCESS;
                   /*Map the device buffer to CPU*/
                   iType *reduceInputBuffer = (iType*)ctl.getCommandQueue().enqueueMapBuffer(first.getBuffer(), false, CL_MAP_READ,0, sizeof(iType) * szElements, 
                                               NULL, &multiCoreCPUEvent, &l_Error );
                    multiCoreCPUEvent.wait();
                    tbb::task_scheduler_init initialize(tbb::task_scheduler_init::automatic);
                    Reduce<iType, BinaryFunction> reduce_op(binary_op, init);
                    tbb::parallel_reduce( tbb::blocked_range<iType*>( reduceInputBuffer, reduceInputBuffer + szElements), reduce_op );
                    /*Unmap the device buffer back to device memory. This will copy the host modified buffer back to the device*/
                    ctl.getCommandQueue().enqueueUnmapMemObject(first.getBuffer(), reduceInputBuffer);
                    return reduce_op.value;
#else
                    std::cout << "The MultiCoreCpu version of reduce is not enabled. " << std ::endl;
                    throw ::cl::Error( CL_INVALID_OPERATION, "The MultiCoreCpu version of reduce is not enabled to be built." );
                    return init;
#endif
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


                std::vector<std::string> typeNames( reduce_end);
                typeNames[reduce_iValueType] = TypeName< T >::get( );
                typeNames[reduce_iIterType] = TypeName< DVInputIterator >::get( );
                typeNames[reduce_BinaryFunction] = TypeName< BinaryFunction >::get();

                std::vector<std::string> typeDefinitions;
                PUSH_BACK_UNIQUE( typeDefinitions, ClCode< T >::get() )
                PUSH_BACK_UNIQUE( typeDefinitions, ClCode< DVInputIterator >::get() )
                PUSH_BACK_UNIQUE( typeDefinitions, ClCode< BinaryFunction  >::get() )

                //bool cpuDevice = ctl.device().getInfo<CL_DEVICE_TYPE>() == CL_DEVICE_TYPE_CPU;
                /*\TODO - Do CPU specific kernel work group size selection here*/
                //const size_t kernel0_WgSize = (cpuDevice) ? 1 : WAVESIZE*KERNEL02WAVES;
                std::string compileOptions;
                //std::ostringstream oss;
                //oss << " -DKERNEL0WORKGROUPSIZE=" << kernel0_WgSize;

                Reduce_KernelTemplateSpecializer ts_kts;
                std::vector< ::cl::Kernel > kernels = bolt::cl::getKernels(
                    ctl,
                    typeNames,
                    &ts_kts,
                    typeDefinitions,
                    reduce_kernels,
                    compileOptions);




                // Set up shape of launch grid and buffers:
                cl_uint computeUnits     = ctl.getDevice().getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>();
                int wgPerComputeUnit =  ctl.getWGPerComputeUnit(); 
                size_t numWG = computeUnits * wgPerComputeUnit;

                cl_int l_Error = CL_SUCCESS;
                const size_t wgSize  = kernels[0].getWorkGroupInfo< CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE >( 
                    ctl.getDevice( ), &l_Error );
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

                V_OPENCL( kernels[0].setArg(0, first.getBuffer( ) ), "Error setting kernel argument" );
                V_OPENCL( kernels[0].setArg(1, first.gpuPayloadSize( ), &first.gpuPayload( ) ), "Error setting a kernel argument" );
                V_OPENCL( kernels[0].setArg(2, szElements), "Error setting kernel argument" );
                V_OPENCL( kernels[0].setArg(3, *userFunctor), "Error setting kernel argument" );
                V_OPENCL( kernels[0].setArg(4, *result), "Error setting kernel argument" );

                ::cl::LocalSpaceArg loc;
                loc.size_ = wgSize*sizeof(iType);
                V_OPENCL( kernels[0].setArg(5, loc), "Error setting kernel argument" );

                l_Error = ctl.getCommandQueue().enqueueNDRangeKernel(
                    kernels[0], 
                    ::cl::NullRange, 
                    ::cl::NDRange(numWG * wgSize), 
                    ::cl::NDRange(wgSize));
                V_OPENCL( l_Error, "enqueueNDRangeKernel() failed for reduce() kernel" );

                ::cl::Event l_mapEvent;
                iType *h_result = (iType*)ctl.getCommandQueue().enqueueMapBuffer(*result, false, CL_MAP_READ, 0, 
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
