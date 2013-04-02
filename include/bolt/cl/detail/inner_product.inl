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

/*
TODO:
1. Optimize the Kernel. In this version transform and reduce are called directly which performs better. 
2. Found a caveat in Multi-GPU scenario (Evergreen+Tahiti). Which basically applies to most of the routines.
*/

#if !defined( INNERPRODUCT_INL )
#define INNERPRODUCT_INL
#define USE_KERNEL 0
#pragma once

#include <boost/thread/once.hpp>
#include <boost/bind.hpp>
#include <type_traits> 
#include <bolt/cl/detail/reduce.inl>
#include <bolt/cl/detail/transform.inl>

#include "bolt/cl/bolt.h"

namespace bolt {
    namespace cl {
        // default control, two-input transform, std:: iterator
        template<typename InputIterator, typename OutputType, typename BinaryFunction1, typename BinaryFunction2> 
        OutputType inner_product(bolt::cl::control& ctl, InputIterator first1, InputIterator last1, InputIterator first2,
             OutputType init, BinaryFunction1 f1, BinaryFunction2 f2, const std::string& user_code )
        {
            return detail::inner_product_detect_random_access( ctl, first1, last1, first2, init, f1, f2, user_code, 
                std::iterator_traits< InputIterator >::iterator_category( ) );
        }

        // default control, two-input transform, std:: iterator
        template<typename InputIterator, typename OutputType, typename BinaryFunction1, typename BinaryFunction2> 
        OutputType inner_product( InputIterator first1, InputIterator last1, InputIterator first2, OutputType init, 
            BinaryFunction1 f1, BinaryFunction2 f2, const std::string& user_code )
        {
            return detail::inner_product_detect_random_access( control::getDefault(), first1, last1, first2, init, f1, 
                f2, user_code, std::iterator_traits< InputIterator >::iterator_category( ) );
        }
        template<typename InputIterator, typename OutputType> 
        OutputType inner_product(bolt::cl::control& ctl, InputIterator first1, InputIterator last1, InputIterator first2,
            OutputType init, const std::string& user_code )
        {
            typedef typename std::iterator_traits<InputIterator>::value_type iType;
            return detail::inner_product_detect_random_access( ctl, first1, last1, first2, init, bolt::cl::plus< iType >( ),
                bolt::cl::multiplies< iType >( ), user_code, std::iterator_traits< InputIterator >::iterator_category( ) );
        }

        // default control, two-input transform, std:: iterator
        template<typename InputIterator, typename OutputType> 
        OutputType inner_product( InputIterator first1, InputIterator last1, InputIterator first2, OutputType init, 
            const std::string& user_code )
        {
            typedef typename std::iterator_traits<InputIterator>::value_type iType;
            return detail::inner_product_detect_random_access( control::getDefault(), first1, last1, first2, init, 
                bolt::cl::plus< iType >( ), bolt::cl::multiplies< iType >( ), user_code, 
                std::iterator_traits< InputIterator >::iterator_category( ) );
        }
               

    }//end of cl namespace
};//end of bolt namespace


namespace bolt {
    namespace cl {
        namespace detail {

#if USE_KERNEL
             struct CallCompiler_InnerProduct {
                static void constructAndCompile(::cl::Kernel *masterKernel,  std::string cl_code, std::string valueTypeName,
                    std::string functorTypeName1, std::string functorTypeName2, const control *ctl) {

                    const std::string instantiationString = 
                        "// Host generates this instantiation string with user-specified value type and functor\n"
                        "template __attribute__((mangled_name(innerProductInstantiated)))\n"
                        "__attribute__((reqd_work_group_size(64,1,1)))\n"
                        "kernel void innerProductTemplate(\n"
                        "global " + valueTypeName + "* A,\n"
                        "global " + valueTypeName + "* B,\n"
                        "const int length,\n"
                        "global " + functorTypeName1 + "* userFunctor,\n"
                        "global " + functorTypeName2 + "* userFunctor2,\n"
                        "global " + valueTypeName + "* result,\n"
                        "local " + valueTypeName + "* scratch\n"
                        ");\n\n";

                    std::string functorTypeNames = functorTypeName1 + " , " + functorTypeName2;

                    bolt::cl::constructAndCompileString( masterKernel, "innerProduct", inner_product_kernels, 
                        instantiationString, cl_code, valueTypeName, functorTypeNames, *ctl);

                };
            };

            
#endif
            // Wrapper that uses default control class, iterator interface
            template<typename InputIterator, typename OutputType, typename BinaryFunction1, typename BinaryFunction2> 
            OutputType inner_product_detect_random_access( bolt::cl::control& ctl, const InputIterator& first1, 
                const InputIterator& last1, const InputIterator& first2, const OutputType& init, const BinaryFunction1& f1, 
                const BinaryFunction2& f2, const std::string& user_code, std::input_iterator_tag )
            {
                //  TODO:  It should be possible to support non-random_access_iterator_tag iterators, if we copied the data 
                //  to a temporary buffer.  Should we?
                static_assert( false, "Bolt only supports random access iterator types" );
            };

            template<typename InputIterator, typename OutputType, typename BinaryFunction1, typename BinaryFunction2> 
            OutputType inner_product_detect_random_access( bolt::cl::control& ctl, const InputIterator& first1, 
                const InputIterator& last1, const InputIterator& first2, const OutputType& init, const BinaryFunction1& f1, 
                const BinaryFunction2& f2, const std::string& user_code, std::random_access_iterator_tag )
            {
                return inner_product_pick_iterator( ctl, first1, last1, first2, init, f1, f2, user_code,
                std::iterator_traits< InputIterator >::iterator_category( ) );
            };

            /*! \brief This template function overload is used to seperate device_vector iterators from all other iterators
                \detail This template is called by the non-detail versions of inclusive_scan, it already assumes random access
             *  iterators.  This overload is called strictly for non-device_vector iterators
            */
            template<typename InputIterator, typename OutputType, typename BinaryFunction1,typename BinaryFunction2> 
            OutputType inner_product_pick_iterator(bolt::cl::control &ctl,  const InputIterator& first1, 
                const InputIterator& last1, const InputIterator& first2, const OutputType& init, const BinaryFunction1& f1, 
                const BinaryFunction2& f2, const std::string& user_code, std::random_access_iterator_tag )
            {
                typedef std::iterator_traits<InputIterator>::value_type iType;
                size_t sz = (last1 - first1); 
                if (sz == 0)
                    return -1;

                // Use host pointers memory since these arrays are only read once - no benefit to copying.

                // Map the input iterator to a device_vector
                device_vector< iType > dvInput( first1, last1, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, ctl );
                device_vector< iType > dvInput2( first2, sz, CL_MEM_USE_HOST_PTR|CL_MEM_READ_ONLY, true, ctl );

                // Map the output iterator to a device_vector
                //Make the fourth argument as true; We will use it as a temporary buffer for copying the transform result to reduce
                //device_vector< iType > dvOutput( result, sz, CL_MEM_USE_HOST_PTR|CL_MEM_READ_WRITE, false, ctl );

                return inner_product_enqueue( ctl, dvInput.begin( ), dvInput.end( ), dvInput2.begin( ), init, f1, f2, user_code );

                // This should immediately map/unmap the buffer
                //dvOutput.data( );
            }

            // This template is called by the non-detail versions of inclusive_scan, it already assumes random access iterators
            // This is called strictly for iterators that are derived from device_vector< T >::iterator
            template<typename DVInputIterator, typename OutputType, typename BinaryFunction1, typename BinaryFunction2> 
            OutputType inner_product_pick_iterator(bolt::cl::control &ctl,  const DVInputIterator& first1, 
                const DVInputIterator& last1, const DVInputIterator& first2, const OutputType& init, const BinaryFunction1& f1, 
                const BinaryFunction2& f2, const std::string& user_code, bolt::cl::device_vector_tag )
            {
                return inner_product_enqueue( ctl, first1, last1, first2, init, f1, f2, user_code );
            }

            // This template is called by the non-detail versions of inclusive_scan, it already assumes random access iterators
            // This is called strictly for iterators that are derived from device_vector< T >::iterator
            template<typename DVInputIterator, typename OutputType, typename BinaryFunction1, typename BinaryFunction2> 
            OutputType inner_product_pick_iterator(bolt::cl::control &ctl,  const DVInputIterator& first1, 
                const DVInputIterator& last1, const DVInputIterator& first2, const OutputType& init, const BinaryFunction1& f1, 
                const BinaryFunction2& f2, const std::string& user_code, bolt::cl::fancy_iterator_tag )
            {
                return inner_product_enqueue( ctl, first1, last1, first2, init, f1, f2, user_code );
            }

            template< typename DVInputIterator, typename OutputType, typename BinaryFunction1,typename BinaryFunction2> 
            OutputType inner_product_enqueue(bolt::cl::control &ctl, const DVInputIterator& first1, 
                const DVInputIterator& last1, const DVInputIterator& first2, const OutputType& init, const BinaryFunction1& f1, 
                const BinaryFunction2& f2, const std::string& cl_code)
            {

#if USE_KERNEL
                //NOTE: Kernel version was found to be inefficient. Optimize!!
                static boost::once_flag initOnlyOnce;
                static  ::cl::Kernel masterKernel;


                // For user-defined types, the user must create a TypeName trait which returns the name of the class - note use of TypeName<>::get to retreive the name here.
                boost::call_once( initOnlyOnce, boost::bind( CallCompiler_InnerProduct::constructAndCompile, &masterKernel, 
                    "\n//--user Code\n" + cl_code + "\n//---Functions\n" + ClCode<BinaryFunction1>::get() + ClCode<BinaryFunction2>::get(), 
                    TypeName<OutputType>::get(), TypeName<BinaryFunction1>::get(), TypeName<BinaryFunction2>::get(), &ctl ) );
                
                // Set up shape of launch grid and buffers:
                // FIXME, read from device attributes.
                int computeUnits     = ctl.device().getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>();  // round up if we don't know. 
                int wgPerComputeUnit =  ctl.wgPerComputeUnit(); 
                int numWG = computeUnits * wgPerComputeUnit;

                cl_int l_Error = CL_SUCCESS;
                const size_t wgSize  = masterKernel.getWorkGroupInfo< CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE >( ctl.device( ), &l_Error );                
                V_OPENCL( l_Error, "Error querying kernel for CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE" );

                // Create Buffer wrappers so we can access the host functors, for read or writing in the kernel
                ALIGNED( 256 ) BinaryFunction1 aligned_binary1( f1 );
                ALIGNED( 256 ) BinaryFunction2 aligned_binary2( f2 );

                ::cl::Buffer userFunctor1(ctl.context(), CL_MEM_USE_HOST_PTR, sizeof(aligned_binary1), &aligned_binary1 );   
                ::cl::Buffer userFunctor2(ctl.context(), CL_MEM_USE_HOST_PTR, sizeof(aligned_binary2), &aligned_binary2 );
                ::cl::Buffer result(ctl.context(), CL_MEM_ALLOC_HOST_PTR|CL_MEM_WRITE_ONLY, sizeof(OutputType) * numWG);

                ::cl::Kernel k = masterKernel;  // hopefully create a copy of the kernel. FIXME, doesn't work.

                cl_uint szElements = static_cast< cl_uint >( std::distance( first1, last1 ) );
                
                /***** This is a temporaray fix - Same as Transform Reduce *****/
                /*What if  requiredWorkGroups > numWG? Do you want to loop or increase the work group size or increase the per item processing?*/
                int requiredWorkGroups = (int)ceil((float)szElements/wgSize); 
                if (requiredWorkGroups < numWG)
                    numWG = requiredWorkGroups;
                /**********************/

                V_OPENCL( k.setArg(0, first1.getBuffer( ) ), "Error setting kernel argument" );
                V_OPENCL( k.setArg(1, first2.getBuffer( ) ), "Error setting kernel argument" );
                V_OPENCL( k.setArg(2, szElements), "Error setting kernel argument" );
                V_OPENCL( k.setArg(3, userFunctor1), "Error setting kernel argument" );
                V_OPENCL( k.setArg(4, userFunctor2), "Error setting kernel argument" );                       
                V_OPENCL( k.setArg(5, result), "Error setting kernel argument" );

                ::cl::LocalSpaceArg loc;
                loc.size_ = wgSize*sizeof(OutputType);
                V_OPENCL( k.setArg(6, loc), "Error setting kernel argument" );
                
                l_Error = ctl.commandQueue().enqueueNDRangeKernel( 
                    k, 
                    ::cl::NullRange, 
                    ::cl::NDRange(numWG * wgSize), 
                    ::cl::NDRange(wgSize) );
                V_OPENCL( l_Error, "enqueueNDRangeKernel() failed for inner_product() kernel" );

                ::cl::Event l_mapEvent;
                OutputType *h_result = (OutputType*)ctl.commandQueue().enqueueMapBuffer(result, false, CL_MAP_READ, 0, sizeof(OutputType)*numWG, NULL, &l_mapEvent, &l_Error );
                V_OPENCL( l_Error, "Error calling map on the result buffer" );

                //  Finish the tail end of the reduction on host side; the compute device reduces within the workgroups, with one result per workgroup
                size_t ceilNumWG = static_cast< size_t >( std::ceil( static_cast< float >( szElements ) / wgSize) );
                bolt::cl::minimum< size_t >  min_size_t;
                size_t numTailReduce = min_size_t( ceilNumWG, numWG );

                bolt::cl::wait(ctl, l_mapEvent);

                OutputType acc = static_cast< OutputType >( init );
                for(int i = 0; i < numTailReduce; ++i)
                {
                    acc = f1( acc, h_result[ i ] );
                }

                return acc;

#endif

                //Should we directly call transform and reduce routines or launch a separate kernel?
                typedef std::iterator_traits<DVInputIterator>::value_type iType;
                ::cl::Event innerproductEvent;

                cl_uint distVec = static_cast< cl_uint >( std::distance( first1, last1 ) );
                if( distVec == 0 )
                    return -1;

                device_vector< iType > tempDV( distVec, 0, CL_MEM_READ_WRITE, false, ctl );
                detail::transform_enqueue( ctl, first1, last1, first2, tempDV.begin() ,f2,cl_code);
                return detail::reduce_enqueue( ctl, tempDV.begin(), tempDV.end(), init, f1, cl_code);
                bolt::cl::wait(ctl, innerproductEvent);

            };

        }//End OF detail namespace
    }//End OF cl namespace
}//End OF bolt namespace

#endif
