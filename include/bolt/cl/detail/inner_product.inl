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

#if !defined( BOLT_CL_INNERPRODUCT_INL )
#define BOLT_CL_INNERPRODUCT_INL
#define USE_KERNEL 0
#pragma once

#include <boost/thread/once.hpp>
#include <boost/bind.hpp>
#include <type_traits>
#include <bolt/cl/detail/reduce.inl>
#include <bolt/cl/detail/transform.inl>

#include "bolt/cl/bolt.h"

//TBB Includes
#ifdef ENABLE_TBB
#include "bolt/btbb/inner_product.h"
#endif

namespace bolt {
    namespace cl {




namespace detail {

#if USE_KERNEL
             struct CallCompiler_InnerProduct {
                static void constructAndCompile(::cl::Kernel *masterKernel,  std::string cl_code,
                    std::string valueTypeName, std::string functorTypeName1, std::string functorTypeName2,
                    const control *ctl) {

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


            template< typename DVInputIterator, typename OutputType, typename BinaryFunction1,typename BinaryFunction2>
            OutputType inner_product_enqueue(bolt::cl::control &ctl, const DVInputIterator& first1,
                const DVInputIterator& last1, const DVInputIterator& first2, const OutputType& init,
                const BinaryFunction1& f1, const BinaryFunction2& f2, const std::string& cl_code)
            {

                //Should we directly call transform and reduce routines or launch a separate kernel?
                typedef typename std::iterator_traits<DVInputIterator>::value_type iType;
                ::cl::Event innerproductEvent;

                cl_uint distVec = static_cast< cl_uint >( std::distance( first1, last1 ) );
                if( distVec == 0 )
                    return -1;

                device_vector< iType > tempDV( distVec, 0, CL_MEM_READ_WRITE, false, ctl );
                detail::transform_enqueue( ctl, first1, last1, first2, tempDV.begin() ,f2,cl_code);
                return detail::reduce_enqueue( ctl, tempDV.begin(), tempDV.end(), init, f1, cl_code);
                bolt::cl::wait(ctl, innerproductEvent);

            };



            /*! \brief This template function overload is used to seperate device_vector iterators from all
                other iterators
                \detail This template is called by the non-detail versions of inclusive_scan,
                it already assumes random access
             *  iterators.  This overload is called strictly for non-device_vector iterators
            */
            template<typename InputIterator, typename OutputType, typename BinaryFunction1,typename BinaryFunction2>
            OutputType inner_product_pick_iterator(bolt::cl::control &ctl,  const InputIterator& first1,
                const InputIterator& last1, const InputIterator& first2, const OutputType& init,
                const BinaryFunction1& f1,
                const BinaryFunction2& f2, const std::string& user_code, std::random_access_iterator_tag )
            {
                typedef typename std::iterator_traits<InputIterator>::value_type iType;
                size_t sz = (last1 - first1);
                if (sz == 0)
                    return -1;

                bolt::cl::control::e_RunMode runMode = ctl.getForceRunMode();  // could be dynamic choice some day.
                if(runMode == bolt::cl::control::Automatic)
                {
                     runMode = ctl.getDefaultPathToRun();
                }

				#if defined(BOLT_DEBUG_LOG)
                BOLTLOG::CaptureLog *dblog = BOLTLOG::CaptureLog::getInstance();
                #endif
				
                if( runMode == bolt::cl::control::SerialCpu)
                {
				    #if defined(BOLT_DEBUG_LOG)
                    dblog->CodePathTaken(BOLTLOG::BOLT_INNERPRODUCT,BOLTLOG::BOLT_SERIAL_CPU,"::Inner_Product::SERIAL_CPU");
                    #endif
						
                    #if defined( _WIN32 )
                           return std::inner_product(first1, last1, stdext::checked_array_iterator<iType*>(&(*first2), sz ), init, f1, f2);
                    #else
                    return std::inner_product(first1, last1, first2, init, f1, f2);
                    #endif
                }
                else if(runMode == bolt::cl::control::MultiCoreCpu)
                {
                    #ifdef ENABLE_TBB
					       #if defined(BOLT_DEBUG_LOG)
                           dblog->CodePathTaken(BOLTLOG::BOLT_INNERPRODUCT,BOLTLOG::BOLT_MULTICORE_CPU,"::Inner_Product::MULTICORE_CPU");
                           #endif
                           return bolt::btbb::inner_product(first1, last1, first2, init, f1, f2);
                    #else
                           throw std::runtime_error("MultiCoreCPU Version of inner_product not Enabled! \n");
                    #endif
                }
                else
                {

				    #if defined(BOLT_DEBUG_LOG)
                    dblog->CodePathTaken(BOLTLOG::BOLT_INNERPRODUCT,BOLTLOG::BOLT_OPENCL_GPU,"::Inner_Product::OPENCL_GPU");
                    #endif
						
                    // Use host pointers memory since these arrays are only read once - no benefit to copying.

                    // Map the input iterator to a device_vector
                    device_vector< iType > dvInput( first1, last1, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, ctl );
                    device_vector< iType > dvInput2( first2, sz, CL_MEM_USE_HOST_PTR|CL_MEM_READ_ONLY, true, ctl );

                    // Map the output iterator to a device_vector
                    //Make the fourth argument as true; We will use it as a temporary buffer
                    //for copying the transform result to reduce
                    //device_vector< iType > dvOutput( result, sz, CL_MEM_USE_HOST_PTR|CL_MEM_READ_WRITE, false, ctl );

                    return inner_product_enqueue( ctl, dvInput.begin( ), dvInput.end( ), dvInput2.begin( ),
                                                   init, f1, f2, user_code );

                    // This should immediately map/unmap the buffer
                    //dvOutput.data( );
                }
            }

            // This template is called by the non-detail versions of inclusive_scan,
            // it already assumes random access iterators
            // This is called strictly for iterators that are derived from device_vector< T >::iterator
            template<typename DVInputIterator, typename OutputType, typename BinaryFunction1, typename BinaryFunction2>
            OutputType inner_product_pick_iterator(bolt::cl::control &ctl,  const DVInputIterator& first1,
                const DVInputIterator& last1,const DVInputIterator& first2,const OutputType& init,
                const BinaryFunction1&f1,const BinaryFunction2& f2, const std::string& user_code,
                bolt::cl::device_vector_tag )
            {

                size_t sz = (last1 - first1);

                typedef typename std::iterator_traits< DVInputIterator >::value_type iType1;
                bolt::cl::control::e_RunMode runMode = ctl.getForceRunMode();  // could be dynamic choice some day.
                if(runMode == bolt::cl::control::Automatic)
                {
                     runMode = ctl.getDefaultPathToRun();
                }
                #if defined(BOLT_DEBUG_LOG)
                BOLTLOG::CaptureLog *dblog = BOLTLOG::CaptureLog::getInstance();
                #endif
				
                if( runMode == bolt::cl::control::SerialCpu)
                {
				    #if defined(BOLT_DEBUG_LOG)
                    dblog->CodePathTaken(BOLTLOG::BOLT_INNERPRODUCT,BOLTLOG::BOLT_SERIAL_CPU,"::Inner_Product::SERIAL_CPU");
                    #endif
					
                    typename bolt::cl::device_vector< iType1 >::pointer firstPtr =  first1.getContainer( ).data( );
                    typename bolt::cl::device_vector< iType1 >::pointer first2Ptr =  first2.getContainer( ).data( );

                    #if defined( _WIN32 )
                       return std::inner_product(  &firstPtr[ first1.m_Index ],
                                                &firstPtr[ last1.m_Index ],
                                                stdext::make_checked_array_iterator( &first2Ptr[ first2.m_Index ], sz),
                                                init, f1, f2);
                    #else
                    return std::inner_product(  &firstPtr[ first1.m_Index ],
                                                &firstPtr[ last1.m_Index ],
                                                &first2Ptr[ first2.m_Index ], init, f1, f2);
                    #endif
                }
                else if(runMode == bolt::cl::control::MultiCoreCpu)
                {
                #ifdef ENABLE_TBB
				   #if defined(BOLT_DEBUG_LOG)
                   dblog->CodePathTaken(BOLTLOG::BOLT_INNERPRODUCT,BOLTLOG::BOLT_MULTICORE_CPU,"::Inner_Product::MULTICORE_CPU");
                   #endif
						   
                   typename bolt::cl::device_vector< iType1 >::pointer firstPtr =  first1.getContainer( ).data( );
                   typename bolt::cl::device_vector< iType1 >::pointer first2Ptr =  first2.getContainer( ).data( );
                    return bolt::btbb::inner_product(  &firstPtr[ first1.m_Index ],  &firstPtr[ last1.m_Index ],
                                                &first2Ptr[ first2.m_Index ], init, f1, f2);
                #else
                           throw std::runtime_error("MultiCoreCPU Version of inner_product not Enabled! \n");
                #endif
                }
                else
                {
				    #if defined(BOLT_DEBUG_LOG)
                    dblog->CodePathTaken(BOLTLOG::BOLT_INNERPRODUCT,BOLTLOG::BOLT_OPENCL_GPU,"::Inner_Product::OPENCL_GPU");
                    #endif
                    return inner_product_enqueue( ctl, first1, last1, first2, init, f1, f2, user_code );
                }
            }

            // This template is called by the non-detail versions of inclusive_scan,
            // it already assumes random access iterators
            // This is called strictly for iterators that are derived from device_vector< T >::iterator
            template<typename DVInputIterator, typename OutputType, typename BinaryFunction1,typename BinaryFunction2>
            OutputType inner_product_pick_iterator(bolt::cl::control &ctl,  const DVInputIterator& first1,
                const DVInputIterator& last1, const DVInputIterator& first2, const OutputType& init,
                const BinaryFunction1& f1, const BinaryFunction2& f2, const std::string& user_code,
                bolt::cl::fancy_iterator_tag )
            {
                typedef typename std::iterator_traits<DVInputIterator>::value_type iType;
                size_t sz = std::distance( first1, last1 );

                bolt::cl::control::e_RunMode runMode = ctl.getForceRunMode();  // could be dynamic choice some day.
                if(runMode == bolt::cl::control::Automatic)
                {
                     runMode = ctl.getDefaultPathToRun();
                }
                #if defined(BOLT_DEBUG_LOG)
                BOLTLOG::CaptureLog *dblog = BOLTLOG::CaptureLog::getInstance();
                #endif
				
                if( runMode == bolt::cl::control::SerialCpu)
                {
				    #if defined(BOLT_DEBUG_LOG)
                    dblog->CodePathTaken(BOLTLOG::BOLT_INNERPRODUCT,BOLTLOG::BOLT_SERIAL_CPU,"::Inner_Product::SERIAL_CPU");
                    #endif
					
                    #if defined( _WIN32 )
                    return std::inner_product(  first1,
                                                last1,
                                                first2,
                                                init, f1, f2  );
                    #else
                           return std::inner_product(  first1,
                                                last1,
                                                first2,
                                                init, f1, f2  );
                    #endif

                }
                else if(runMode == bolt::cl::control::MultiCoreCpu)
                {
                    #ifdef ENABLE_TBB
					       #if defined(BOLT_DEBUG_LOG)
                           dblog->CodePathTaken(BOLTLOG::BOLT_INNERPRODUCT,BOLTLOG::BOLT_MULTICORE_CPU,"::Inner_Product::MULTICORE_CPU");
                           #endif
                           return bolt::btbb::inner_product(first1, last1, first2, init, f1, f2);
                    #else
                           throw std::runtime_error("MultiCoreCPU Version of inner_product not Enabled! \n");
                    #endif
                }
                else
                {
				    #if defined(BOLT_DEBUG_LOG)
                    dblog->CodePathTaken(BOLTLOG::BOLT_INNERPRODUCT,BOLTLOG::BOLT_OPENCL_GPU,"::Inner_Product::OPENCL_GPU");
                    #endif
                    return inner_product_enqueue( ctl, first1, last1, first2, init, f1, f2, user_code );
                }
            }




            template<typename InputIterator, typename OutputType, typename BinaryFunction1, typename BinaryFunction2>
            OutputType inner_product_detect_random_access( bolt::cl::control& ctl, const InputIterator& first1,
                const InputIterator& last1, const InputIterator& first2, const OutputType& init,
                const BinaryFunction1& f1,
                const BinaryFunction2& f2, const std::string& user_code, std::random_access_iterator_tag )
            {
                return inner_product_pick_iterator( ctl, first1, last1, first2, init, f1, f2, user_code,
                typename std::iterator_traits< InputIterator >::iterator_category( ) );
            };

	    // Wrapper that uses default control class, iterator interface
            template<typename InputIterator, typename OutputType, typename BinaryFunction1, typename BinaryFunction2>
            OutputType inner_product_detect_random_access( bolt::cl::control& ctl, const InputIterator& first1,
                const InputIterator& last1, const InputIterator& first2, const OutputType& init,
                const BinaryFunction1& f1, const BinaryFunction2& f2, const std::string& user_code,
                std::input_iterator_tag )
            {
                //  TODO:  It should be possible to support non-random_access_iterator_tag iterators,
                //  if we copied the data
                //  to a temporary buffer.  Should we?
                static_assert(std::is_same< InputIterator, std::input_iterator_tag >::value  , "Bolt only supports random access iterator types" );
            };



        }//End OF detail namespace








        // default control, two-input transform, std:: iterator
        template<typename InputIterator, typename OutputType, typename BinaryFunction1, typename BinaryFunction2>
         OutputType inner_product(bolt::cl::control& ctl, InputIterator first1, InputIterator last1,
         InputIterator first2, OutputType init, BinaryFunction1 f1, BinaryFunction2 f2,
         const std::string& user_code )
        {
            return detail::inner_product_detect_random_access( ctl, first1, last1, first2, init, f1, f2, user_code,
                typename std::iterator_traits< InputIterator >::iterator_category( ) );
        }

        // default control, two-input transform, std:: iterator
        template<typename InputIterator, typename OutputType, typename BinaryFunction1, typename BinaryFunction2>
        OutputType inner_product( InputIterator first1, InputIterator last1, InputIterator first2, OutputType init,
            BinaryFunction1 f1, BinaryFunction2 f2, const std::string& user_code )
        {
            return detail::inner_product_detect_random_access( control::getDefault(), first1, last1, first2, init, f1,
                f2, user_code, typename std::iterator_traits< InputIterator >::iterator_category( ) );
        }
        template<typename InputIterator, typename OutputType>
        OutputType inner_product(bolt::cl::control& ctl,InputIterator first1,InputIterator last1,InputIterator first2,
            OutputType init, const std::string& user_code )
        {
            typedef typename std::iterator_traits<InputIterator>::value_type iType;
            return detail::inner_product_detect_random_access(ctl, first1,last1,first2,init,bolt::cl::plus< iType >( ),
                bolt::cl::multiplies< iType >( ), user_code, typename std::iterator_traits<InputIterator>::iterator_category());
        }

        // default control, two-input transform, std:: iterator
        template<typename InputIterator, typename OutputType>
        OutputType inner_product( InputIterator first1, InputIterator last1, InputIterator first2, OutputType init,
            const std::string& user_code )
        {
            typedef typename std::iterator_traits<InputIterator>::value_type iType;
            return detail::inner_product_detect_random_access( control::getDefault(), first1, last1, first2, init,
                bolt::cl::plus< iType >( ), bolt::cl::multiplies< iType >( ), user_code,
                typename std::iterator_traits< InputIterator >::iterator_category( ) );
        }


    }//end of cl namespace
};//end of bolt namespace



#endif
