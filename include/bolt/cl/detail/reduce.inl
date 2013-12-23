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


#if !defined( BOLT_CL_REDUCE_INL )
#define BOLT_CL_REDUCE_INL
#pragma once

#include <algorithm>

#include <boost/thread/once.hpp>
#include <boost/bind.hpp>
#include "bolt/cl/bolt.h"
#include "bolt/cl/functional.h"
#ifdef ENABLE_TBB
//TBB Includes
#include "bolt/btbb/reduce.h"
#endif


namespace bolt {
    namespace cl {


namespace detail {

        enum ReduceTypes {reduce_iValueType, reduce_iIterType, reduce_BinaryFunction,reduce_resType, reduce_end };

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

            const ::std::string operator() ( const ::std::vector< ::std::string>& typeNames ) const
            {
                const std::string templateSpecializationString =
                        "// Host generates this instantiation string with user-specified value type and functor\n"
                        "template __attribute__((mangled_name(" + name(0) + "Instantiated)))\n"
                        "__attribute__((reqd_work_group_size(256,1,1)))\n"
                        "kernel void reduceTemplate(\n"
                        "global " + typeNames[reduce_iValueType] + "* input_ptr,\n"
                         + typeNames[reduce_iIterType] + " output_iter,\n"
                        "const int length,\n"
                        "global " + typeNames[reduce_BinaryFunction] + "* userFunctor,\n"
                        "global " + typeNames[reduce_resType] + "* result,\n"
                        "local " + typeNames[reduce_resType] + "* scratch\n"
                        ");\n\n";

                return templateSpecializationString;
            }
            };


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
                typeNames[reduce_iValueType] = TypeName< iType >::get( );
                typeNames[reduce_iIterType] = TypeName< DVInputIterator >::get( );
                typeNames[reduce_BinaryFunction] = TypeName< BinaryFunction >::get();
                typeNames[reduce_resType] = TypeName< T >::get( );

                std::vector<std::string> typeDefinitions;
                PUSH_BACK_UNIQUE( typeDefinitions, ClCode< iType >::get() )
                PUSH_BACK_UNIQUE( typeDefinitions, ClCode< DVInputIterator >::get() )
                PUSH_BACK_UNIQUE( typeDefinitions, ClCode< BinaryFunction  >::get() )
                PUSH_BACK_UNIQUE( typeDefinitions, ClCode< T >::get() )

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
                int wgPerComputeUnit =  64; //ctl.getWGPerComputeUnit();  // This boosts up the performance
                size_t numWG = computeUnits * wgPerComputeUnit;

                cl_int l_Error = CL_SUCCESS;

				// Bumped up wgSize to achieve higher ALU usage and occupancy
                const size_t wgSize  = 256 ;//kernels[0].getWorkGroupInfo< CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE >(
                    //ctl.getDevice( ), &l_Error );

                V_OPENCL( l_Error, "Error querying kernel for CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE" );

                // Create buffer wrappers so we can access the host functors, for read or writing in the kernel
                ALIGNED( 256 ) BinaryFunction aligned_reduce( binary_op );
                //::cl::Buffer userFunctor(ctl.context(), CL_MEM_USE_HOST_PTR|CL_MEM_READ_ONLY, sizeof(aligned_reduce),
                //  &aligned_reduce );
                control::buffPointer userFunctor = ctl.acquireBuffer( sizeof( aligned_reduce ),
                    CL_MEM_USE_HOST_PTR|CL_MEM_READ_ONLY, &aligned_reduce );

                // ::cl::Buffer result(ctl.context(), CL_MEM_ALLOC_HOST_PTR|CL_MEM_WRITE_ONLY, sizeof( iType )*numWG);
                control::buffPointer result = ctl.acquireBuffer( sizeof( T ) * numWG,
                    CL_MEM_ALLOC_HOST_PTR|CL_MEM_WRITE_ONLY );

                cl_uint szElements = static_cast< cl_uint >( first.distance_to(last ) );
                typename DVInputIterator::Payload first_payload = first.gpuPayload( ) ;

                V_OPENCL( kernels[0].setArg(0, first.getContainer().getBuffer() ), "Error setting kernel argument" );
                V_OPENCL( kernels[0].setArg(1, first.gpuPayloadSize( ),&first_payload),"Error setting a kernel argument" );
                V_OPENCL( kernels[0].setArg(2, szElements), "Error setting kernel argument" );
                V_OPENCL( kernels[0].setArg(3, *userFunctor), "Error setting kernel argument" );
                V_OPENCL( kernels[0].setArg(4, *result), "Error setting kernel argument" );

                ::cl::LocalSpaceArg loc;
                loc.size_ = wgSize*sizeof(T);
                V_OPENCL( kernels[0].setArg(5, loc), "Error setting kernel argument" );

                l_Error = ctl.getCommandQueue().enqueueNDRangeKernel(
                    kernels[0],
                    ::cl::NullRange,
                    ::cl::NDRange(numWG * wgSize),
                    ::cl::NDRange(wgSize));

                V_OPENCL( l_Error, "enqueueNDRangeKernel() failed for reduce() kernel" );

                ::cl::Event l_mapEvent;
                T *h_result = (T*)ctl.getCommandQueue().enqueueMapBuffer(*result, false, CL_MAP_READ, 0,
                    sizeof(T)*numWG, NULL, &l_mapEvent, &l_Error );
                V_OPENCL( l_Error, "Error calling map on the result buffer" );

                //  Finish the tail end of the reduction on host side;the compute device reduces within the workgroups,
                //  with one result per workgroup
                size_t ceilNumWG = static_cast< size_t >( std::ceil( static_cast< float >( szElements ) / wgSize) );
                bolt::cl::minimum<size_t>  min_size_t;
                size_t numTailReduce = min_size_t( ceilNumWG, numWG );

                bolt::cl::wait(ctl, l_mapEvent);

                T acc = init;
                for(unsigned int i = 0; i < numTailReduce; ++i)
                {
                    acc =(T) binary_op(acc, h_result[i]);
                }


				::cl::Event unmapEvent;

				V_OPENCL( ctl.getCommandQueue().enqueueUnmapMemObject(*result,  h_result, NULL, &unmapEvent ),
					"shared_ptr failed to unmap host memory back to device memory" );
				V_OPENCL( unmapEvent.wait( ), "failed to wait for unmap event" );

                return acc;
            };


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
                #if defined(BOLT_DEBUG_LOG)
                BOLTLOG::CaptureLog *dblog = BOLTLOG::CaptureLog::getInstance();
                #endif
                switch(runMode)
                {
                case bolt::cl::control::OpenCL :
                    {
                        #if defined(BOLT_DEBUG_LOG)
                        dblog->CodePathTaken(BOLTLOG::BOLT_REDUCE,BOLTLOG::BOLT_OPENCL_GPU,"::Reduce::OPENCL_GPU");
                        #endif
                        device_vector< iType > dvInput( first, last, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, ctl );
                        return reduce_enqueue( ctl, dvInput.begin(), dvInput.end(), init, binary_op, cl_code);
                    }

                case bolt::cl::control::MultiCoreCpu:
                    #ifdef ENABLE_TBB
                        #if defined(BOLT_DEBUG_LOG)
                        dblog->CodePathTaken(BOLTLOG::BOLT_REDUCE,BOLTLOG::BOLT_MULTICORE_CPU,"::Reduce::MULTICORE_CPU");
                        #endif
                        return bolt::btbb::reduce(first,last,init,binary_op);
                    #else
                        throw std::runtime_error( "The MultiCoreCpu version of reduce is not enabled to be built! \n" );
                    #endif

                case bolt::cl::control::SerialCpu:
                        #if defined(BOLT_DEBUG_LOG)
                        dblog->CodePathTaken(BOLTLOG::BOLT_REDUCE,BOLTLOG::BOLT_SERIAL_CPU,"::Reduce::SERIAL_CPU");
                        #endif
                        return std::accumulate(first, last, init,binary_op);

                default:
                        #if defined(BOLT_DEBUG_LOG)
                        dblog->CodePathTaken(BOLTLOG::BOLT_REDUCE,BOLTLOG::BOLT_SERIAL_CPU,"::Reduce::SERIAL_CPU");
                        #endif
                    return std::accumulate(first, last, init,binary_op);

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
                #if defined(BOLT_DEBUG_LOG)
                BOLTLOG::CaptureLog *dblog = BOLTLOG::CaptureLog::getInstance();
                #endif

                if (szElements == 0)
                    return init;

                bolt::cl::control::e_RunMode runMode = ctl.getForceRunMode();  // could be dynamic choice some day.
                if(runMode == bolt::cl::control::Automatic)
                {
                    runMode = ctl.getDefaultPathToRun();
                }

                switch(runMode)
                {
                case bolt::cl::control::OpenCL :
                    #if defined(BOLT_DEBUG_LOG)
                         dblog->CodePathTaken(BOLTLOG::BOLT_REDUCE,BOLTLOG::BOLT_OPENCL_GPU,"::Reduce::OPENCL_GPU");
                    #endif
                        return reduce_enqueue( ctl, first, last, init, binary_op, cl_code);


                case bolt::cl::control::MultiCoreCpu:
                    #ifdef ENABLE_TBB
                    {
                        #if defined(BOLT_DEBUG_LOG)
                        dblog->CodePathTaken(BOLTLOG::BOLT_REDUCE,BOLTLOG::BOLT_MULTICORE_CPU,"::Reduce::MULTICORE_CPU");
                        #endif
                      typename bolt::cl::device_vector< iType >::pointer reduceInputBuffer =  first.getContainer( ).data( );
                      return bolt::btbb::reduce(  &reduceInputBuffer[first.m_Index],&reduceInputBuffer[ last.m_Index ],
                                                  init, binary_op);
                    }
                    #else
                    {
                        throw std::runtime_error( "The MultiCoreCpu version of reduce is not enabled to be built! \n" );
                    }
                    #endif

                case bolt::cl::control::SerialCpu:
                    {
                        #if defined(BOLT_DEBUG_LOG)
                        dblog->CodePathTaken(BOLTLOG::BOLT_REDUCE,BOLTLOG::BOLT_SERIAL_CPU,"::Reduce::SERIAL_CPU");
                        #endif
                       typename bolt::cl::device_vector< iType >::pointer reduceInputBuffer =  first.getContainer( ).data( );
                      return std::accumulate(  &reduceInputBuffer[first.m_Index], &reduceInputBuffer[ last.m_Index ],
                                               init, binary_op);
                    }

                default: /* Incase of runMode not set/corrupted */
                    {
                        #if defined(BOLT_DEBUG_LOG)
                        dblog->CodePathTaken(BOLTLOG::BOLT_REDUCE,BOLTLOG::BOLT_SERIAL_CPU,"::Reduce::SERIAL_CPU");
                        #endif
                       typename bolt::cl::device_vector< iType >::pointer reduceInputBuffer =  first.getContainer( ).data( );
                      return std::accumulate(  &reduceInputBuffer[first.m_Index], &reduceInputBuffer[ last.m_Index ],
                                               init, binary_op);
                    }

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
                typedef typename std::iterator_traits<DVInputIterator>::value_type iType;
                size_t szElements = static_cast<size_t>(std::distance(first, last) );
                if (szElements == 0)
                    return init;

                bolt::cl::control::e_RunMode runMode = ctl.getForceRunMode();  // could be dynamic choice some day.
                if(runMode == bolt::cl::control::Automatic)
                {
                    runMode = ctl.getDefaultPathToRun();
                }
                #if defined(BOLT_DEBUG_LOG)
                BOLTLOG::CaptureLog *dblog = BOLTLOG::CaptureLog::getInstance();
                #endif
                switch(runMode)
                {
                case bolt::cl::control::OpenCL :
                    #if defined(BOLT_DEBUG_LOG)
                    dblog->CodePathTaken(BOLTLOG::BOLT_REDUCE,BOLTLOG::BOLT_OPENCL_GPU,"::Reduce::OPENCL_GPU");
                    #endif
                        return reduce_enqueue( ctl, first, last, init, binary_op, cl_code);

                case bolt::cl::control::MultiCoreCpu:
                    #ifdef ENABLE_TBB
                    {
                        #if defined(BOLT_DEBUG_LOG)
                        dblog->CodePathTaken(BOLTLOG::BOLT_REDUCE,BOLTLOG::BOLT_MULTICORE_CPU,"::Reduce::MULTICORE_CPU");
                        #endif
                      return bolt::btbb::reduce(first,last,init,binary_op);
                    }
                    #else
                    {
                        throw std::runtime_error( "The MultiCoreCpu version of reduce is not enabled to be built! \n" );
                    }
                    #endif

                case bolt::cl::control::SerialCpu:
                    #if defined(BOLT_DEBUG_LOG)
                    dblog->CodePathTaken(BOLTLOG::BOLT_REDUCE,BOLTLOG::BOLT_SERIAL_CPU,"::Reduce::SERIAL_CPU");
                    #endif
                     return std::accumulate(first,last, init, binary_op);

                default: /* Incase of runMode not set/corrupted */
                    #if defined(BOLT_DEBUG_LOG)
                    dblog->CodePathTaken(BOLTLOG::BOLT_REDUCE,BOLTLOG::BOLT_SERIAL_CPU,"::Reduce::SERIAL_CPU");
                    #endif
                    return std::accumulate(first,last, init, binary_op);

                }

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
                   typename std::iterator_traits< DVInputIterator >::iterator_category( ) );
            }

            template<typename T, typename DVInputIterator, typename BinaryFunction>
            T reduce_detect_random_access(bolt::cl::control &ctl,

                const DVInputIterator& first,
                const DVInputIterator& last,
                const T& init,
                const BinaryFunction& binary_op,
                const std::string& cl_code,
                std::input_iterator_tag)
            {
              //  TODO: It should be possible to support non-random_access_iterator_tag iterators,if we copied the data
              //  to a temporary buffer.  Should we?
                static_assert( std::is_same< DVInputIterator, std::input_iterator_tag  >::value, "Bolt only supports random access iterator types" );
            }

        }






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
            return detail::reduce_detect_random_access(ctl, first, last, init, binary_op, cl_code,
                   typename std::iterator_traits< InputIterator >::iterator_category( ) );
        }

    }

};



#endif //BOLT_CL_REDUCE_INL
