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
#pragma once
#if !defined( BOLT_CL_TRANSFORM_ITERATOR_H )
#define BOLT_CL_TRANSFORM_ITERATOR_H
#include "bolt/cl/bolt.h"
#include "bolt/cl/iterator/iterator_traits.h"
#include <boost/iterator/iterator_facade.hpp>
#include <boost/iterator/transform_iterator.hpp>
//#include <boost/utility/result_of.hpp>

namespace bolt
{
namespace cl 
{

    struct transform_iterator_tag
        : public fancy_iterator_tag
        {   // identifying tag for random-access iterators
        };
    template <class UnaryFunc, class Iterator>
    class transform_iterator : public boost::transform_iterator <UnaryFunc, Iterator>
    {
          
    public:
        typedef typename std::iterator_traits<Iterator>::value_type      value_type;
        //typedef typename std::iterator_traits<Iterator>::size_type     size_type;
        typedef typename std::iterator_traits<Iterator>::difference_type difference_type;
        typedef typename std::iterator_traits<Iterator>::pointer         pointer;
        typedef transform_iterator_tag                                   iterator_category;
        typedef typename UnaryFunc                                       unary_func;

        //typedef typename Iterator::container                             container;
        //friend class boost::transform_iterator <UnaryFunc, Iterator>;
        transform_iterator(Iterator const& x, UnaryFunc f): boost::transform_iterator<UnaryFunc, Iterator>( x,  f), m_it( x )//, m_Index(0)
          { 
              //ClCode<bolt::cl::transform_iterator<UnaryFunc, Iterator> >::addDependency(ClCode<UnaryFunc>::get());//BOLT_ADD_DEPENDENCY(UnaryFunc,);
          }

        struct Payload
        {
            difference_type m_Index;
            difference_type m_Ptr1[ 3 ];  // Represents device pointer, big enough for 32 or 64bit
            UnaryFunc       m_f;
        };

        /*TODO - RAVI Probably I can acheive this using friend class device_vector. But the problem would be 
                 multiple defintions of functions like advance()*/        
        template<typename Container >
        Container& getContainer( ) const
        {
            return m_it.getContainer( );
        }

        const Payload  gpuPayload( ) const
        {
            Payload payload = { 0/*m_Index*/, { 0, 0, 0 } };
            return payload;
        }

        /*TODO - This should throw a compilation error if the Iterator is of type std::vector*/
        const difference_type gpuPayloadSize( ) const
        {
            cl_int l_Error = CL_SUCCESS;
            //::cl::Device which_device;
            //l_Error  = m_it.getContainer().m_commQueue.getInfo(CL_QUEUE_DEVICE,&which_device );	

            cl_uint deviceBits = 32;// = which_device.getInfo< CL_DEVICE_ADDRESS_BITS >( );
            //  Size of index and pointer
            cl_uint szUF = sizeof(UnaryFunc);
            szUF = (szUF+3) &(~3);
            difference_type payloadSize = sizeof( difference_type ) + ( deviceBits >> 3 ) + szUF;

            //  64bit devices need to add padding for 8 byte aligned pointer
            if( deviceBits == 64 )
                payloadSize += 4;

            return payloadSize;
            //return m_it.gpuPayloadSize( );
        }
        //difference_type m_Index;
        Iterator m_it;  
    };

    //  This string represents the device side definition of the Transform Iterator template
    static std::string deviceTransformIteratorTemplate = STRINGIFY_CODE(
        namespace bolt { namespace cl { \n
        template< typename UnaryFunc, typename Iterator > \n
        class transform_iterator \n
        { \n
            public:    \n
                typedef int iterator_category;        \n
                typedef typename Iterator::value_type value_type; \n
                typedef int difference_type; \n
                typedef int size_type; \n
                typedef value_type* pointer; \n
                typedef value_type& reference; \n

                transform_iterator( value_type init ): m_StartIndex( init ), m_Ptr( 0 ) \n
                {}; \n

                void init( global value_type* ptr )\n
                { \n
                    m_Ptr = ptr; \n
                }; \n

                value_type operator[]( size_type threadID ) const \n
                { \n
                   return m_f(m_Ptr[ m_StartIndex + threadID ]); \n
                } \n

                value_type operator*( ) const \n
                { \n
                    return m_f(m_Ptr[ m_StartIndex + threadID ]); \n
                } \n

                size_type m_StartIndex; \n
                global value_type* m_Ptr; \n
                UnaryFunc          m_f; \n
        }; \n
    } } \n
    );
}
}




#endif
