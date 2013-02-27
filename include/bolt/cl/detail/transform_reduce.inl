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

#ifdef ENABLE_TBB
//TBB Includes
#include "tbb/parallel_reduce.h"
#include "tbb/blocked_range.h"
#include "tbb/task_scheduler_init.h"
#endif



namespace bolt {
namespace cl {

    // The following two functions are visible in .h file
    // Wrapper that user passes a control class
    template<typename InputIterator, typename UnaryFunction, typename T, typename BinaryFunction> 
    T transform_reduce( control& ctl, InputIterator first, InputIterator last,  
        UnaryFunction transform_op, 
        T init,  BinaryFunction reduce_op, const std::string& user_code )  
    {
        return detail::transform_reduce_detect_random_access( ctl, first, last, transform_op, init, reduce_op, user_code, 
            std::iterator_traits< InputIterator >::iterator_category( ) );
    };

    // Wrapper that generates default control class
    template<typename InputIterator, typename UnaryFunction, typename T, typename BinaryFunction> 
    T transform_reduce(InputIterator first, InputIterator last,
        UnaryFunction transform_op,
        T init,  BinaryFunction reduce_op, const std::string& user_code )
    {
        return detail::transform_reduce_detect_random_access( control::getDefault(), first, last, transform_op, init, reduce_op, user_code, 
            std::iterator_traits< InputIterator >::iterator_category( ) );
    };


namespace  detail {
    struct kernelParamsTransformReduce
    {
        const std::string inValueNakedType;
        const std::string inValueIterType;
        const std::string outValueNakedType;
        const std::string transformFunctorTypeName;
        const std::string reduceFunctorTypeName;

        kernelParamsTransformReduce( const std::string& iTypePtr, const std::string& iTypeIter, 
            const std::string& oTypePtr, const std::string& transFuncType, const std::string& redFuncType ): 
        inValueNakedType( iTypePtr ), inValueIterType( iTypeIter ), 
        outValueNakedType( oTypePtr ),
        transformFunctorTypeName( transFuncType ), reduceFunctorTypeName( redFuncType )
        {}
    };

    struct CallCompiler_TransformReduce {
        static void constructAndCompile(
            ::cl::Kernel *masterKernel,
            std::string user_code,
            kernelParamsTransformReduce* kp,
            const control *ctl) {

            const std::string instantiationString = 
                "// Host generates this instantiation string with user-specified value type and functor\n"
                "template __attribute__((mangled_name(transform_reduceInstantiated)))\n"
                "__attribute__((reqd_work_group_size(64,1,1)))\n"
                "kernel void transform_reduceTemplate(\n"
                "global " + kp->inValueNakedType + "* input_ptr,\n"
                 + kp->inValueIterType + " iIter,\n"
                "const int length,\n"
                "global " + kp->transformFunctorTypeName + "* transformFunctor,\n"
                "const " + kp->outValueNakedType + " init,\n"
                "global " + kp->reduceFunctorTypeName + "* reduceFunctor,\n"
                "global " + kp->outValueNakedType + "* result,\n"
                "local " + kp->outValueNakedType + "* scratch\n"
                ");\n\n";

            std::string functorNames = kp->transformFunctorTypeName + " , " + kp->reduceFunctorTypeName; // create for debug message

            bolt::cl::constructAndCompileString( masterKernel, "transform_reduce", transform_reduce_kernels, 
                instantiationString, user_code, kp->outValueNakedType, functorNames, *ctl);
        };
    };


#ifdef ENABLE_TBB
            /*For documentation on the reduce object see below link
             *http://threadingbuildingblocks.org/docs/help/reference/algorithms/parallel_reduce_func.htm
             *The imperative form of parallel_reduce is used. 
             *
            */
            template <typename T, typename UnaryFunction, typename BinaryFunction>
            struct Transform_Reduce {
                T value;
                BinaryFunction reduce_op;
                UnaryFunction transform_op;
                bool flag;
               
                //TODO - Decide on how many threads to spawn? Usually it should be equal to th enumber of cores
                //You might need to look at the tbb::split and there there cousin's 
                //
                Transform_Reduce(const UnaryFunction &_opt, const BinaryFunction &_opr) : transform_op(_opt), reduce_op(_opr) ,value(0){}
                Transform_Reduce(const UnaryFunction &_opt, const BinaryFunction &_opr, const T &init) : transform_op(_opt), reduce_op(_opr), value(init), flag(FALSE){}
                
                Transform_Reduce(): value(0) {}
                Transform_Reduce( Transform_Reduce& s, tbb::split ):flag(TRUE),transform_op(s.transform_op),reduce_op(s.reduce_op){}
                 void operator()( const tbb::blocked_range<T*>& r ) {
                    T reduce_temp = value, transform_temp;
                    for( T* a=r.begin(); a!=r.end(); ++a ) {
                      transform_temp = transform_op(*a);
                      if(flag){
                        reduce_temp = transform_temp;
                        flag = FALSE;
                      }
                      else
                        reduce_temp = reduce_op(reduce_temp,transform_temp);
                    }
                    value = reduce_temp;
                }
                 //Join is called by the parent thread after the child finishes to execute.
                void join( Transform_Reduce& rhs ) {
                    value = reduce_op(value,rhs.value);
                }
            };
#endif


        //  The following two functions disallow non-random access functions
        // Wrapper that uses default control class, iterator interface
        template<typename InputIterator, typename UnaryFunction, typename T, typename BinaryFunction> 
        T transform_reduce_detect_random_access( control &ctl, const InputIterator& first, const InputIterator& last,
            const UnaryFunction& transform_op,
            const T& init, const BinaryFunction& reduce_op, const std::string& user_code, std::input_iterator_tag )
        {
            //  TODO:  It should be possible to support non-random_access_iterator_tag iterators, if we copied the data 
            //  to a temporary buffer.  Should we?
            static_assert( false, "Bolt only supports random access iterator types" );
        };

        // Wrapper that uses default control class, iterator interface
        template<typename InputIterator, typename UnaryFunction, typename T, typename BinaryFunction> 
        T transform_reduce_detect_random_access( control& ctl, const InputIterator& first, const InputIterator& last,
            const UnaryFunction& transform_op,
            const T& init, const BinaryFunction& reduce_op, const std::string& user_code, std::random_access_iterator_tag )
        {
            return transform_reduce_pick_iterator( ctl, first, last, transform_op, init, reduce_op, user_code, 
                std::iterator_traits< InputIterator >::iterator_category( ) );
        };

        // This template is called by the non-detail versions of transform_reduce, it already assumes random access iterators
        // This is called strictly for any non-device_vector iterator
        template<typename InputIterator, typename UnaryFunction, typename oType, typename BinaryFunction> 
        oType transform_reduce_pick_iterator(
            control &c,
            const InputIterator& first,
            const InputIterator& last,
            const UnaryFunction& transform_op, 
            const oType& init,
            const BinaryFunction& reduce_op,
            const std::string& user_code,
            std::random_access_iterator_tag )
        {
            typedef std::iterator_traits<InputIterator>::value_type iType;
            size_t szElements = (last - first); 
            if (szElements == 0)
                    return init;

            const bolt::cl::control::e_RunMode runMode = c.forceRunMode();  // could be dynamic choice some day.
            if (runMode == bolt::cl::control::SerialCpu)
            {
                //Create a temporary array to store the transform result;
                std::vector<oType> output(szElements);

                std::transform(first, last, output.begin(),transform_op);
                return std::accumulate(output.begin(), output.end(), init);

            } else if (runMode == bolt::cl::control::MultiCoreCpu) {

#ifdef ENABLE_TBB
                    tbb::task_scheduler_init initialize(tbb::task_scheduler_init::automatic);
                    Transform_Reduce<oType, UnaryFunction, BinaryFunction> transform_reduce_op(transform_op, reduce_op, init);
                    tbb::parallel_reduce( tbb::blocked_range<iType*>( &*first, (iType*)&*(last-1) + 1), transform_reduce_op );
                    return transform_reduce_op.value;
#else
                    std::cout << "The MultiCoreCpu version of transform_reduce is not implemented yet." << std ::endl;
                    throw ::cl::Error( CL_INVALID_OPERATION, "The MultiCoreCpu version of reduce is not enabled to be built." );
                    return init;
#endif  
            } else {
                // Map the input iterator to a device_vector
                device_vector< iType > dvInput( first, last, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE, c );

                return  transform_reduce_enqueue( c, dvInput.begin( ), dvInput.end( ), transform_op, init, reduce_op, user_code );
            }
        };

        // This template is called by the non-detail versions of transform_reduce, it already assumes random access iterators
        // This is called strictly for iterators that are derived from device_vector< T >::iterator
        template<typename DVInputIterator, typename UnaryFunction, typename oType, typename BinaryFunction> 
        oType transform_reduce_pick_iterator(
            control &c,
            const DVInputIterator& first,
            const DVInputIterator& last,
            const UnaryFunction& transform_op, 
            const oType& init,
            const BinaryFunction& reduce_op,
            const std::string& user_code,
            bolt::cl::device_vector_tag )
        {
            typedef std::iterator_traits<DVInputIterator>::value_type iType;
            size_t szElements = static_cast<size_t>(std::distance(first, last) ); 
            if (szElements == 0)
                    return init;

            const bolt::cl::control::e_RunMode runMode = c.forceRunMode();  // could be dynamic choice some day.
            if (runMode == bolt::cl::control::SerialCpu)
            {
                ::cl::Event serialCPUEvent;
                cl_int l_Error = CL_SUCCESS;
                oType trans_reduceResult;
                /*Map the device buffer to CPU*/
                iType *trans_reduceInputBuffer = (iType*)c.commandQueue().enqueueMapBuffer(first.getBuffer(), false, 
                                   CL_MAP_READ|CL_MAP_WRITE, 0, sizeof(iType) * szElements, NULL, &serialCPUEvent, &l_Error );
                serialCPUEvent.wait();
                std::vector<oType> output(szElements);
                std::transform(trans_reduceInputBuffer, trans_reduceInputBuffer + szElements, output.begin(), transform_op);
                trans_reduceResult = std::accumulate(output.begin(), output.end(), init) ;
                /*Unmap the device buffer back to device memory. This will copy the host modified buffer back to the device*/
                c.commandQueue().enqueueUnmapMemObject(first.getBuffer(), trans_reduceInputBuffer);
                return trans_reduceResult;

            }
            else if (runMode == bolt::cl::control::MultiCoreCpu)
            {
#ifdef ENABLE_TBB
                ::cl::Event multiCoreCPUEvent;
                cl_int l_Error = CL_SUCCESS;
                /*Map the device buffer to CPU*/
                iType *trans_reduceInputBuffer = (iType*)c.commandQueue().enqueueMapBuffer(first.getBuffer(), false, 
                                   CL_MAP_READ|CL_MAP_WRITE, 0, sizeof(iType) * szElements, NULL, &multiCoreCPUEvent, &l_Error );
                multiCoreCPUEvent.wait();

                tbb::task_scheduler_init initialize(tbb::task_scheduler_init::automatic);
                Transform_Reduce<oType, UnaryFunction, BinaryFunction> transform_reduce_op(transform_op, reduce_op, init);
                tbb::parallel_reduce( tbb::blocked_range<iType*>(trans_reduceInputBuffer, (trans_reduceInputBuffer + szElements)), transform_reduce_op );
                c.commandQueue().enqueueUnmapMemObject(first.getBuffer(), trans_reduceInputBuffer);
                return transform_reduce_op.value;
#else
                std::cout << "The MultiCoreCpu version of transform reduce is not enabled. " << std ::endl;
                throw ::cl::Error( CL_INVALID_OPERATION, "The MultiCoreCpu version of transform reduce is not enabled to be built." );
                return init;
#endif
            }

            return  transform_reduce_enqueue( c, first, last, transform_op, init, reduce_op, user_code );
        };

        template<typename DVInputIterator, typename UnaryFunction, typename oType, typename BinaryFunction> 
        oType transform_reduce_enqueue(
            control& ctl,
            const DVInputIterator& first,
            const DVInputIterator& last,
            const UnaryFunction& transform_op,
            const oType& init,
            const BinaryFunction& reduce_op,
            const std::string& user_code="")
        {
            static boost::once_flag initOnlyOnce;
            static  ::cl::Kernel masterKernel;
            unsigned debugMode = 0; //FIXME, use control

            typedef std::iterator_traits< DVInputIterator  >::value_type iType;

            kernelParamsTransformReduce args( TypeName< iType >::get( ), TypeName< DVInputIterator >::get( ),  TypeName< oType >::get( ), 
                TypeName< UnaryFunction >::get( ), TypeName< BinaryFunction >::get( ) );

            // For user-defined types, the user must create a TypeName trait which returns the name of the class 
            //  - note use of TypeName<>::get to retrieve the name here.
            std::string typeDefinitions = user_code + ClCode< iType >::get( ) + ClCode< DVInputIterator >::get( ) +
                        ClCode< UnaryFunction >::get( ) + ClCode< BinaryFunction >::get( );
            if( !boost::is_same< iType, oType >::value )
            {
                typeDefinitions += ClCode< oType >::get( );
            }

            boost::call_once( initOnlyOnce,
                boost::bind(
                    CallCompiler_TransformReduce::constructAndCompile,
                    &masterKernel, 
                    typeDefinitions, 
                    &args,
                    &ctl
                )
            );

            // Set up shape of launch grid and buffers:
            // FIXME, read from device attributes.
            int computeUnits     = ctl.device().getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>();  // round up if we don't know. 
            int wgPerComputeUnit =  ctl.wgPerComputeUnit(); 
            int numWG = computeUnits * wgPerComputeUnit;

            cl_int l_Error = CL_SUCCESS;
            const size_t wgSize  = masterKernel.getWorkGroupInfo< CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE >( ctl.device( ), &l_Error );
            V_OPENCL( l_Error, "Error querying kernel for CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE" );

            // Create Buffer wrappers so we can access the host functors, for read or writing in the kernel
            ALIGNED( 256 ) UnaryFunction aligned_unary( transform_op );
            ALIGNED( 256 ) BinaryFunction aligned_binary( reduce_op );

            //::cl::Buffer transformFunctor(ctl.context(), CL_MEM_USE_HOST_PTR, sizeof(aligned_unary), &aligned_unary );   
            //::cl::Buffer reduceFunctor(ctl.context(), CL_MEM_USE_HOST_PTR, sizeof(aligned_binary), &aligned_binary );
            control::buffPointer transformFunctor = ctl.acquireBuffer( sizeof( aligned_unary ), CL_MEM_USE_HOST_PTR|CL_MEM_READ_ONLY, &aligned_unary );
            control::buffPointer reduceFunctor = ctl.acquireBuffer( sizeof( aligned_binary ), CL_MEM_USE_HOST_PTR|CL_MEM_READ_ONLY, &aligned_binary );

            // ::cl::Buffer result(ctl.context(), CL_MEM_ALLOC_HOST_PTR|CL_MEM_WRITE_ONLY, sizeof(T) * numWG);
            control::buffPointer result = ctl.acquireBuffer( sizeof( oType ) * numWG, CL_MEM_ALLOC_HOST_PTR|CL_MEM_WRITE_ONLY );

            ::cl::Kernel k = masterKernel;  // hopefully create a copy of the kernel. FIXME, doesn't work.

            cl_uint szElements = static_cast< cl_uint >( std::distance( first, last ) );
                
            /***** This is a temporaray fix *****/
            /*What if  requiredWorkGroups > numWG? Do you want to loop or increase the work group size or increase the per item processing?*/
            int requiredWorkGroups = (int)ceil((float)szElements/wgSize); 
            if (requiredWorkGroups < numWG)
                numWG = requiredWorkGroups;
            /**********************/

            V_OPENCL( k.setArg( 0, first.getBuffer( ) ), "Error setting kernel argument" );
            V_OPENCL( k.setArg( 1, first.gpuPayloadSize( ), &first.gpuPayload( ) ), "Error setting kernel argument" );
            V_OPENCL( k.setArg( 2, szElements), "Error setting kernel argument" );
            V_OPENCL( k.setArg( 3, *transformFunctor), "Error setting kernel argument" );
            V_OPENCL( k.setArg( 4, init), "Error setting kernel argument" );
            V_OPENCL( k.setArg( 5, *reduceFunctor), "Error setting kernel argument" );
            V_OPENCL( k.setArg( 6, *result), "Error setting kernel argument" );

            ::cl::LocalSpaceArg loc;
            loc.size_ = wgSize*sizeof(oType);
            V_OPENCL( k.setArg( 7, loc ), "Error setting kernel argument" );

            l_Error = ctl.commandQueue().enqueueNDRangeKernel( 
                k, 
                ::cl::NullRange, 
                ::cl::NDRange(numWG * wgSize), 
                ::cl::NDRange(wgSize) );
            V_OPENCL( l_Error, "enqueueNDRangeKernel() failed for transform_reduce() kernel" );

            ::cl::Event l_mapEvent;
            oType *h_result = (oType*)ctl.commandQueue().enqueueMapBuffer(*result, false, CL_MAP_READ, 0, sizeof(oType)*numWG, NULL, &l_mapEvent, &l_Error );
            V_OPENCL( l_Error, "Error calling map on the result buffer" );

            //  Finish the tail end of the reduction on host side; the compute device reduces within the workgroups, with one result per workgroup
            size_t ceilNumWG = static_cast< size_t >( std::ceil( static_cast< float >( szElements ) / wgSize) );
            bolt::cl::minimum< size_t >  min_size_t;
            size_t numTailReduce = min_size_t( ceilNumWG, numWG );

            bolt::cl::wait(ctl, l_mapEvent);

            oType acc = static_cast< oType >( init );
            for(int i = 0; i < numTailReduce; ++i)
            {
                acc = reduce_op( acc, h_result[ i ] );
            }

            return acc;
        };

}// end of namespace detail
}// end of namespace cl
}// end of namespace bolt

#endif
