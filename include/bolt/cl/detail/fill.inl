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

#if !defined( FILL_INL )
#define FILL_INL
#pragma once

#include <boost/thread/once.hpp>
#include <boost/bind.hpp>
#include <type_traits>

#define STATIC /*static*/  /* FIXME - hack to approximate buffer pool management of the functor containing the buffer */

#include "bolt/cl/bolt.h"

namespace bolt {
    namespace cl {

        // default control, start->stop
        template<typename ForwardIterator, typename T>
        void fill( ForwardIterator first, ForwardIterator last, const T & value, const std::string& cl_code)
        {
            detail::fill_detect_random_access( bolt::cl::control::getDefault(), first, last, value, cl_code, std::iterator_traits< ForwardIterator >::iterator_category( ) );
        }

        // user specified control, start->stop
        template<typename ForwardIterator, typename T>
        void fill( const bolt::cl::control &ctl, ForwardIterator first, ForwardIterator last, const T & value, const std::string& cl_code)
        {
            detail::fill_detect_random_access( ctl, first, last, value, cl_code, std::iterator_traits< ForwardIterator >::iterator_category( ) );
        }

        // default control, start-> +n
        template<typename OutputIterator, typename Size, typename T>
        OutputIterator fill_n( OutputIterator first, Size n, const T & value, const std::string& cl_code)
        {
            detail::fill_detect_random_access( bolt::cl::control::getDefault(), first, first+n, value, cl_code, std::iterator_traits< OutputIterator >::iterator_category( ) );
            return (first+n);
        }

        // user specified control, start-> +n
        template<typename OutputIterator, typename Size, typename T>
        OutputIterator fill_n( const bolt::cl::control &ctl, OutputIterator first, Size n, const T & value, const std::string& cl_code)
        {
            detail::fill_detect_random_access( ctl, first, n, value, cl_code, std::iterator_traits< OutputIterator >::iterator_category( ) );
            return (first+n);
        }

    }//end of cl namespace
};//end of bolt namespace


namespace bolt {
    namespace cl {
        namespace detail {

            /*****************************************************************************
             * Random Access
             ****************************************************************************/

            // fill no support
            template<typename ForwardIterator, typename T>
            void fill_detect_random_access( const bolt::cl::control &ctl, ForwardIterator first, ForwardIterator last, const T & value, const std::string &cl_code, std::forward_iterator_tag )
            {
                static_assert( false, "Bolt only supports random access iterator types" );
            }

            // fill random-access
            template<typename ForwardIterator, typename T>
            void fill_detect_random_access( const bolt::cl::control &ctl, ForwardIterator first, ForwardIterator last, const T & value, const std::string &cl_code, std::random_access_iterator_tag )
            {
                fill_pick_iterator(ctl, first, last, value, cl_code);
            }


            /*****************************************************************************
             * Pick Iterator
             ****************************************************************************/

            /*! \brief This template function overload is used to seperate device_vector iterators from all other iterators
                \detail This template is called by the non-detail versions of fill, it already assumes random access
             *  iterators.  This overload is called strictly for non-device_vector iterators
            */
            template<typename ForwardIterator, typename T>
            typename std::enable_if< !(std::is_base_of<typename device_vector<typename std::iterator_traits<ForwardIterator>::value_type>::iterator, ForwardIterator>::value), void >::type
            fill_pick_iterator(const bolt::cl::control &ctl,  const ForwardIterator &first, const ForwardIterator &last, const T & value, const std::string &user_code)
            {
                typedef std::iterator_traits<ForwardIterator>::value_type Type;

                size_t sz = (last - first);
                if (sz < 1)
                    return;

                // Use host pointers memory since these arrays are only write once - no benefit to copying.
                // Map the forward iterator to a device_vector
                device_vector< Type > range( first, sz, CL_MEM_USE_HOST_PTR | CL_MEM_WRITE_ONLY, false, ctl );

                fill_enqueue( ctl, range.begin( ), range.end( ), value, user_code );

                range.data( );
            }

            // This template is called by the non-detail versions of fill, it already assumes random access iterators
            // This is called strictly for iterators that are derived from device_vector< T >::iterator
            template<typename DVForwardIterator, typename T>
            typename std::enable_if< (std::is_base_of<typename device_vector<typename std::iterator_traits<DVForwardIterator>::value_type>::iterator,DVForwardIterator>::value), void >::type
            fill_pick_iterator(const bolt::cl::control &ctl,  const DVForwardIterator &first, const DVForwardIterator &last,  const T & value, const std::string& user_code)
            {
                fill_enqueue( ctl, first, last, value, user_code );          
               
            }


            /*****************************************************************************
             * Fill Enqueue
             ****************************************************************************/

            template< typename DVForwardIterator, typename T >
            void fill_enqueue(const bolt::cl::control &ctl, const DVForwardIterator &first, const DVForwardIterator &last, const T & value, const std::string& cl_code)
            {
                typedef std::iterator_traits<DVForwardIterator>::value_type Type;
                // how many elements to fill
                cl_uint sz = static_cast< cl_uint >( std::distance( first, last ) );
                if (sz < 1)
                    return;
               
				cl_int l_Error= CL_SUCCESS;
				::cl::Event fillEvent;				
				ctl.commandQueue().enqueueFillBuffer(first->getBuffer(),value,0,sz*sizeof(T),NULL,&fillEvent);
				/*enqueueFillBuffer API:
				cl_int enqueueFillBuffer(const Buffer& buffer,
				PatternType pattern,
				::size_t offset,
				::size_t size,
				const VECTOR_CLASS<Event>* events = NULL,
				Event* event = NULL) const
				*/
				V_OPENCL( l_Error, "clEnqueueFillBuffer() failed" );
				               
                bolt::cl::wait(ctl, fillEvent);
            }; // end fill_enqueue

        }//End OF detail namespace
    }//End OF cl namespace
}//End OF bolt namespace

#endif
