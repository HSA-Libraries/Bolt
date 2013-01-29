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

#if !defined( COPY_INL )
#define COPY_INL
#pragma once

#include <boost/thread/once.hpp>
#include <boost/bind.hpp>
#include <type_traits> 

#include "bolt/cl/bolt.h"

namespace bolt {
    namespace cl {

        // user control
        template<typename InputIterator, typename OutputIterator> 
        OutputIterator copy(const bolt::cl::control &ctl,  InputIterator first, InputIterator last, OutputIterator result, 
            const std::string& user_code)
        {
            int n = std::distance( first, last );
            return detail::copy_detect_random_access( ctl, first, n, result, user_code, 
                std::iterator_traits< InputIterator >::iterator_category( ) );
        }

        // default control
        template<typename InputIterator, typename OutputIterator> 
        OutputIterator copy( InputIterator first, InputIterator last, OutputIterator result, 
            const std::string& user_code)
        {
            int n = (int) std::distance( first, last );
            return detail::copy_detect_random_access( control::getDefault(), first, n, result, user_code, 
                std::iterator_traits< InputIterator >::iterator_category( ) );
        }

        // default control
        template<typename InputIterator, typename Size, typename OutputIterator> 
        OutputIterator copy_n(InputIterator first, Size n, OutputIterator result, 
            const std::string& user_code)
        {
            return detail::copy_detect_random_access( control::getDefault(), first, n, result, user_code, 
                std::iterator_traits< InputIterator >::iterator_category( ) );
        }

        // user control
        template<typename InputIterator, typename Size, typename OutputIterator> 
        OutputIterator copy_n(const bolt::cl::control &ctl, InputIterator first, Size n, OutputIterator result, 
            const std::string& user_code)
        {
            return detail::copy_detect_random_access( ctl, first, n, result, user_code, 
                std::iterator_traits< InputIterator >::iterator_category( ) );
        }


    }//end of cl namespace
};//end of bolt namespace


namespace bolt {
    namespace cl {
        namespace detail {

            // Wrapper that uses default control class, iterator interface
            template<typename InputIterator, typename Size, typename OutputIterator> 
            OutputIterator copy_detect_random_access( const bolt::cl::control& ctl, const InputIterator& first, const Size& n, 
                const OutputIterator& result, const std::string& user_code, std::input_iterator_tag )
            {
                static_assert( false, "Bolt only supports random access iterator types" );
                return NULL;
            };

            template<typename InputIterator, typename Size, typename OutputIterator> 
            OutputIterator copy_detect_random_access( const bolt::cl::control& ctl, const InputIterator& first, const Size& n, 
                const OutputIterator& result, const std::string& user_code, std::random_access_iterator_tag )
            {
                if (n > 0)
                {
                    copy_pick_iterator( ctl, first, n, result, user_code, 
                        std::iterator_traits< InputIterator >::iterator_category( ),
                        std::iterator_traits< InputIterator >::iterator_category( ) );
                }
                return (result+n);
            };

            /*! \brief This template function overload is used to seperate device_vector iterators from all other iterators
                \detail This template is called by the non-detail versions of inclusive_scan, it already assumes random access
             *  iterators.  This overload is called strictly for non-device_vector iterators
            */
            template<typename InputIterator, typename Size, typename OutputIterator> 
            void copy_pick_iterator(const bolt::cl::control &ctl,  const InputIterator& first, const Size& n, 
                    const OutputIterator& result, const std::string& user_code, std::random_access_iterator_tag,
                    std::random_access_iterator_tag )
            {
                typedef std::iterator_traits<InputIterator>::value_type iType;
                typedef std::iterator_traits<OutputIterator>::value_type oType;

                // Use host pointers memory since these arrays are only read once - no benefit to copying.

                // Map the input iterator to a device_vector
                device_vector< iType >  dvInput( first,  n, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, true, ctl );

                // Map the output iterator to a device_vector
                device_vector< oType > dvOutput( result, n, CL_MEM_USE_HOST_PTR | CL_MEM_WRITE_ONLY, false, ctl );

                copy_enqueue( ctl, dvInput.begin( ), n, dvOutput.begin( ), user_code );

                // This should immediately map/unmap the buffer
                dvOutput.data( );
            }

            // This template is called by the non-detail versions of inclusive_scan, it already assumes random access iterators
            // This is called strictly for iterators that are derived from device_vector< T >::iterator
            template<typename DVInputIterator, typename Size, typename DVOutputIterator> 
            void copy_pick_iterator(const bolt::cl::control &ctl,  const DVInputIterator& first, const Size& n,
                const DVOutputIterator& result, const std::string& user_code, bolt::cl::device_vector_tag,
                bolt::cl::device_vector_tag )
            {
                copy_enqueue( ctl, first, n, result, user_code );
            }

            // This template is called by the non-detail versions of inclusive_scan, it already assumes random access iterators
            // This is called strictly for iterators that are derived from device_vector< T >::iterator
            template<typename DVInputIterator, typename Size, typename DVOutputIterator> 
            void copy_pick_iterator(const bolt::cl::control &ctl,  const DVInputIterator& first, const Size& n,
                const DVOutputIterator& result, const std::string& user_code, bolt::cl::fancy_iterator_tag,
                std::random_access_iterator_tag )
            {
                copy_enqueue( ctl, first, n, result, user_code );
            }

            template<typename DVInputIterator, typename Size, typename DVOutputIterator> 
            void copy_pick_iterator(const bolt::cl::control &ctl,  const DVInputIterator& first, const Size& n,
                const DVOutputIterator& result, const std::string& user_code, std::random_access_iterator_tag, 
                bolt::cl::fancy_iterator_tag )
            {
                static_assert( false, "It is not possible to copy into fancy iterators. They are not mutable" );
            }

            template< typename DVInputIterator, typename Size, typename DVOutputIterator > 
            void copy_enqueue(const bolt::cl::control &ctl, const DVInputIterator& first, const Size& n, 
                const DVOutputIterator& result, const std::string& cl_code)
            {
                typedef std::iterator_traits<DVInputIterator>::value_type iType;
                typedef std::iterator_traits<DVOutputIterator>::value_type oType;

                ::cl::Event copyEvent;
                cl_int l_Error = ctl.commandQueue().enqueueCopyBuffer(
                    first.getBuffer(),
                    result.getBuffer(),
                    first->getIndex(),
                    result->getIndex(),
                    n*sizeof(iType),
                    //0,
                    NULL,
                    &copyEvent);
                V_OPENCL( l_Error, "enqueueCopyBuffer() failed for copy()" );
                bolt::cl::wait(ctl, copyEvent);
            };
        }//End OF detail namespace
    }//End OF cl namespace
}//End OF bolt namespace

#endif
