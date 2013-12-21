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
#if !defined( BOLT_CL_COUNTING_ITERATOR_H )
#define BOLT_CL_COUNTING_ITERATOR_H
#include "bolt/amp/bolt.h"
#include "bolt/amp/iterator/iterator_traits.h"

/*! \file bolt/cl/iterator/counting_iterator.h
    \brief Return Same Value or counting Value on dereferencing.
*/


namespace bolt {
namespace amp {

    struct counting_iterator_tag
        : public fancy_iterator_tag
        {   // identifying tag for random-access iterators
        };

        template< typename value_type >
        class counting_iterator: public std::iterator< counting_iterator_tag, typename value_type, int>
        {
        public:
             typedef typename std::iterator< counting_iterator_tag, typename value_type, int>::difference_type
             difference_type;

             typedef concurrency::array_view< value_type > arrayview_type;
             typedef counting_iterator<value_type> const_iterator;
           

            //  Basic constructor requires a reference to the container and a positional element
            counting_iterator( value_type init, const control& ctl = control::getDefault( ) ): 
                m_initValue( init ), m_Index( 0 ) {}

            //  This copy constructor allows an iterator to convert into a const_iterator, but not vica versa
           template< typename OtherType >
           counting_iterator( const counting_iterator< OtherType >& rhs ):m_Index( rhs.m_Index ),
               m_initValue( rhs.m_initValue ) {}

            //  This copy constructor allows an iterator to convert into a const_iterator, but not vica versa
            counting_iterator< value_type >& operator= ( const counting_iterator< value_type >& rhs )
            {
                if( this == &rhs )
                    return *this;

                m_initValue = rhs.m_initValue;
                m_Index = rhs.m_Index;
                return *this;
            }
                
            counting_iterator< value_type >& operator+= ( const  difference_type & n )
            {
                advance( n );
                return *this;
            }
                
            const counting_iterator< value_type > operator+ ( const difference_type & n ) const
            {
                counting_iterator< value_type > result( *this );
                result.advance( n );
                return result;
            }

            const counting_iterator< value_type > & getBuffer( const_iterator itr ) const
            {
                return *this;
            }
            

            const counting_iterator< value_type > & getContainer( ) const
            {
                return *this;
            }

            difference_type operator- ( const counting_iterator< value_type >& rhs ) const
            {
                return m_Index - rhs.m_Index;
            }

            //  Public member variables
            difference_type m_Index;

       //private:

            //  Used for templatized copy constructor and the templatized equal operator
            template < typename > friend class counting_iterator;

            //  For a counting_iterator, do nothing on an advance
            void advance( difference_type n )
            {
                m_Index += n;
            }

            // Pre-increment
            counting_iterator< value_type > operator++ ( ) const
            {
                counting_iterator< value_type > result( *this );
                result.advance( 1 );
                return result;
            }

            // Post-increment
            counting_iterator< value_type > operator++ ( int ) const
            {
                counting_iterator< value_type > result( *this );
                result.advance( 1 );
                return result;
            }

            // Pre-decrement
            counting_iterator< value_type > operator--( ) const
            {
                counting_iterator< value_type > result( *this );
                result.advance( -1 );
                return result;
            }

            // Post-decrement
            counting_iterator< value_type > operator--( int ) const
            {
                counting_iterator< value_type > result( *this );
                result.advance( -1 );
                return result;
            }

            difference_type getIndex() const
            {
                return m_Index;
            }

            template< typename OtherType >
            bool operator== ( const counting_iterator< OtherType >& rhs ) const
            {
                bool sameIndex = (rhs.m_initValue == m_initValue) && (rhs.m_Index == m_Index);

                return sameIndex;
            }

            template< typename OtherType >
            bool operator!= ( const counting_iterator< OtherType >& rhs ) const
            {
                bool sameIndex = (rhs.m_initValue != m_initValue) || (rhs.m_Index != m_Index);

                return sameIndex;
            }

            // Do we need this? Debug error
            template< typename OtherType >
            bool operator< ( const counting_iterator< OtherType >& rhs ) const
            {
                bool sameIndex = (m_Index < rhs.m_Index);

                return sameIndex;
            }

            // Dereference operators
            int operator*() const restrict(cpu,amp)
            {
                int xy = m_initValue + m_Index;
                return xy;
            }


            int operator[](int x) const restrict(cpu,amp)
            {
              int temp = x + m_initValue;
              return temp;
            }


            value_type m_initValue;
        };


    template< typename Type >
    counting_iterator< Type > make_counting_iterator( Type initValue )
    {
        counting_iterator< Type > tmp( initValue );
        return tmp;
    }

}
}


#endif
