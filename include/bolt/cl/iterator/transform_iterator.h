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

/*! \file bolt/cl/iterator/counting_iterator.h
    \brief Return Incremented Value on dereferencing.
*/

namespace bolt {
namespace cl {

    struct transform_iterator_tag
        : public fancy_iterator_tag
        {   // identifying tag for random-access iterators
        };

    //  This represents the host side definition of the counting_iterator template
    //BOLT_TEMPLATE_FUNCTOR3( counting_iterator, int, float, double,
        template< typename UnaryFunction,
                  typename Iterator >
        class transform_iterator: public boost::iterator_facade< transform_iterator< UnaryFunction, Iterator >, typename Iterator::value_type,
            transform_iterator_tag, typename Iterator::value_type, int >
        {
        public:

		    typedef typename boost::iterator_facade< transform_iterator< UnaryFunction, Iterator >,
		                                             typename Iterator::value_type,
            										 transform_iterator_tag,
            										 typename Iterator::value_type, int >::difference_type  difference_type;

		    typedef typename transform_iterator< UnaryFunction, Iterator >::value_type value_type;

            struct Payload
            {
                value_type m_Value;
            };

            //  Basic constructor requires a reference to the container and a positional element
            // transform iterators are read only.
            // TODO - a specialization for counting and constant iterator is required.
            transform_iterator( const Iterator it, UnaryFunction u_f, const control& ctl = control::getDefault( ) ):
                m_Index( 0 )
            {
                //static_assert( std::is_convertible< value_type, typename std::iterator_traits< InputIterator >::value_type >::value,
                //    "iterator value_type does not convert to device_vector value_type" );
                //static_assert( !std::is_polymorphic< value_type >::value, "AMD C++ template extensions do not support the virtual keyword yet" );
				cl_int l_Error = CL_SUCCESS;
                m_f = u_f;
                m_index = it.getIndex();

            }

            //  This copy constructor allows an iterator to convert into a const_iterator, but not vica versa
            template< typename OtherIterator >
            transform_iterator( const transform_iterator< UnaryFunction, OtherIterator >& rhs ): m_devMemory( rhs.m_devMemory ),
                m_Index( rhs.m_Index ), m_initValue( rhs.m_initValue )
            {
            }

			//non mutable so not allowed.
            /*transform_iterator< UnaryFunction, Iterator::value_type >& operator = ( const transform_iterator< UnaryFunction, Iterator::value_type >& rhs )
            {
                if( this == &rhs )
                    return *this;

                m_devMemory = rhs.m_devMemory;
                m_initValue = rhs.m_initValue;
                m_Index = rhs.m_Index;
                return *this;
            }*/


            transform_iterator< UnaryFunction, Iterator >& operator+= ( const difference_type & n )
            {
                advance( n );
                return *this;
            }

            const transform_iterator< UnaryFunction, Iterator > operator+ ( const difference_type & n ) const
            {
                transform_iterator< UnaryFunction, Iterator > result( *this );
                result.advance( n );
                return result;
            }

            const ::cl::Buffer& getBuffer( ) const
            {
                return m_devMemory;
            }

            const transform_iterator< UnaryFunction, Iterator > & getContainer( ) const
            {
                return *this;
            }

            Payload gpuPayload( ) const
            {
                Payload payload = { m_initValue };
                return payload;
            }

            const difference_type gpuPayloadSize( ) const
            {
                return sizeof( Payload );
            }


            difference_type distance_to( const transform_iterator< UnaryFunction, Iterator >& rhs ) const
            {
                //return static_cast< typename iterator_facade::difference_type >( 1 );
                return rhs.m_Index - m_Index;
            }

            //  Public member variables
            difference_type m_Index;

        private:
            //  Implementation detail of boost.iterator
            friend class boost::iterator_core_access;

            //  Used for templatized copy constructor and the templatized equal operator
            template < typename, typename > friend class transform_iterator;

            //  For a counting_iterator, do nothing on an advance
            void advance(difference_type n )
            {
                m_Index += n;
            }

            void increment( )
            {
                advance( 1 );
            }

            void decrement( )
            {
                advance( -1 );
            }

            template< typename OtherIterator >
            bool equal( const transform_iterator< UnaryFunction, OtherIterator >& rhs ) const
            {
                bool sameIndex = (rhs.m_initValue == m_initValue) && (rhs.m_Index == m_Index);
                return sameIndex;
            }

            typename boost::iterator_facade< transform_iterator< UnaryFunction, Iterator >, typename Iterator::value_type,
                                             transform_iterator_tag, typename Iterator::value_type, int >::reference  dereference( ) const
            {
                return m_initValue + m_Index;
            }

            //::cl::Buffer m_devMemory;
            value_type m_initValue;
            UnaryFunction m_f;
            Iterator &it;
            
        };
    //)

    //  This string represents the device side definition of the counting_iterator template
    static std::string deviceTransformIterator = STRINGIFY_CODE(

        namespace bolt { namespace cl { \n
        template< typename UnaryFunction, typename Iterator > \n
        class transform_iterator \n
        { \n
        public: \n
            UnaryFunction m_f; \n
            typedef int iterator_category;      // device code does not understand std:: tags \n
            typedef Iterator::value_type  value_type; \n
            typedef Iterator::difference_type difference_type; \n
            typedef Iterator::size_type size_type; \n
            typedef Iterator::pointer pointer; \n
            typedef Iterator::reference reference; \n

			transform_iterator(Iterator const& x, UnaryFunction f): m_StartIndex( init ), m_Ptr( 0 ), m_f(f) \n
			{};\n
			\n

            void init( global value_type* ptr ) \n
            { \n

                m_Ptr = ptr; \n
            }; \n

            value_type operator[]( size_type threadID ) const \n
            { \n
                return m_f( m_Ptr[m_StartIndex + threadID] ); \n
            } \n

            value_type operator*( ) const \n
            { \n
                return m_f( m_Ptr[m_StartIndex + threadID] ); \n
            } \n

            value_type m_StartIndex; \n

        }; \n
    } } \n
    );


    template< typename UnaryFunction, typename Iterator >
    transform_iterator< UnaryFunction, Iterator > make_transform_iterator( Iterator it, UnaryFunction fun )
    {
        transform_iterator< UnaryFunction, Iterator > tmp( it, fun );
        return tmp;
    }

}
}

//BOLT_CREATE_TYPENAME( bolt::cl::transform_iterator< int > );
//BOLT_CREATE_CLCODE( bolt::cl::transform_iterator< int >, bolt::cl::deviceTransformIterator );

//BOLT_TEMPLATE_REGISTER_NEW_TYPE( bolt::cl::transform_iterator, int, unsigned int );
//BOLT_TEMPLATE_REGISTER_NEW_TYPE( bolt::cl::transform_iterator, int, float );
//BOLT_TEMPLATE_REGISTER_NEW_TYPE( bolt::cl::transform_iterator, int, double );
//BOLT_TEMPLATE_REGISTER_NEW_TYPE( bolt::cl::transform_iterator, int, cl_long );

#endif
