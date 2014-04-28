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
#if !defined( BOLT_CL_PERMUTATION_ITERATOR_H )
#define BOLT_CL_PERMUTATION_ITERATOR_H
#include "bolt/amp/bolt.h"
#include "bolt/amp/iterator/iterator_traits.h"
#include "bolt/amp/iterator/counting_iterator.h"

/*! \file bolt/cl/iterator/permutation_iterator.h
    \brief
*/


namespace bolt {
namespace amp {

  struct permutation_iterator_tag
      : public fancy_iterator_tag
      {   // identifying tag for random-access iterators
      };

      template< typename key_type, typename element_type >
      class permutation_iterator: public std::iterator< permutation_iterator_tag, typename element_type, int>
      {
        public:
         typedef typename std::iterator< permutation_iterator_tag, typename element_type, int>::difference_type
         difference_type;

         typedef permutation_iterator<key_type,element_type> perm_iterator;
         typedef typename iterator_traits<element_type>::value_type value_type;
         typedef concurrency::array_view< value_type > arrayview_type;



        // Default constructor
         permutation_iterator():key_iterator(), element_iterator(), m_Index(0) {}

        //  Basic constructor requires a reference to the container and a positional element
        permutation_iterator( key_type ikey, element_type ivalue, const control& ctl = control::getDefault( ) ):
        key_iterator ( ikey ), element_iterator ( ivalue ){}

        //  This copy constructor allows an iterator to convert into a perm_iterator, but not vica versa
        template< typename OtherKeyType, typename OtherValType >
        permutation_iterator( const permutation_iterator< OtherKeyType, OtherValType >& rhs ):m_Index( rhs.m_Index ){}

        //  This copy constructor allows an iterator to convert into a perm_iterator, but not vica versa
        permutation_iterator< key_type, element_type >& operator= ( const permutation_iterator< key_type, element_type >& rhs )
        {
            if( this == &rhs )
                return *this;

            key_iterator = rhs.key_iterator;
            element_iterator = rhs.element_iterator;

            m_Index = rhs.m_Index;
            return *this;
        }
            
        permutation_iterator< key_type, element_type >& operator+= ( const  difference_type & n ) const restrict (cpu,amp)
        {
            advance( n );
            return *this;
        }
            
        const permutation_iterator< key_type, element_type > operator+ ( const difference_type & n ) const restrict (cpu,amp)
        {
            permutation_iterator< key_type, element_type > result( *this );
            result.advance( n );
            return result;
        }

        const permutation_iterator< key_type, element_type > operator- ( const difference_type & n ) const restrict (cpu,amp)
        {
            permutation_iterator< key_type, element_type > result( *this );
            result.advance( -n );
            return result;
        }

        const concurrency::array_view<int> & getBuffer( perm_iterator itr ) const
        {
            return *value;
        }
        

        const permutation_iterator< key_type, element_type > & getContainer( ) const
        {
            return *this;
        }

        difference_type operator- ( const permutation_iterator< key_type, element_type >& rhs ) const
        {
            return element_iterator.getIndex() - rhs.element_iterator.getIndex();
        }

        //  Public member variables
        difference_type m_Index;

        //private:

        //  Used for templatized copy constructor and the templatized equal operator
        template < typename, typename > friend class permutation_iterator;

        //  For a permutation_iterator, do nothing on an advance
        void advance( difference_type n ) restrict ( cpu, amp ) 
        {
            m_Index += n;
        }


        // Pre-increment
        permutation_iterator< key_type, element_type > operator++ ( ) const
        {
            permutation_iterator< key_type, element_type > result( *this );
            result.advance( 1 );
            return result;
        }

        // Post-increment
        permutation_iterator< key_type, element_type > operator++ ( int ) const
        {
            permutation_iterator< key_type, element_type > result( *this );
            result.advance( 1 );
            return result;
        }

        // Pre-decrement
        permutation_iterator< key_type, element_type > operator--( ) const
        {
            permutation_iterator< key_type, element_type > result( *this );
            result.advance( -1 );
            return result;
        }

        // Post-decrement
        permutation_iterator< key_type, element_type > operator--( int ) const
        {
            permutation_iterator< key_type, element_type > result( *this );
            result.advance( -1 );
            return result;
        }

        difference_type getIndex() const
        {
            return m_Index;
        }

        template< typename OtherKey, typename OtherValue >
        bool operator== ( const permutation_iterator< OtherKey, OtherValue >& rhs ) const
        {
            bool sameIndex = true;//(rhs.m_initValue == m_initValue) && (rhs.m_Index == m_Index);

            return sameIndex;
        }

        template< typename OtherKey, typename OtherValue >
        bool operator!= ( const permutation_iterator< OtherKey, OtherValue >& rhs ) const
        {
            bool sameIndex = true;//(rhs.m_initValue != m_initValue) || (rhs.m_Index != m_Index);

            return sameIndex;
        }

        // Do we need this? Debug error
        template< typename OtherKey, typename OtherValue >
        bool operator< ( const permutation_iterator< OtherKey, OtherValue >& rhs ) const
        {
            bool sameIndex = (m_Index < rhs.m_Index);

            return sameIndex;
        }

        // Dereference operators
        value_type& operator*() const restrict(cpu,amp)
        {
          value_type temp_index = key_iterator[m_Index];
          return element_iterator[temp_index];
        }

        value_type& operator[](int x) restrict(cpu,amp)
        {
          value_type temp_index = key_iterator[x];
          return element_iterator[temp_index];
        }

        value_type& operator[](int x) const restrict(cpu,amp)
        {
          value_type temp_index = key_iterator[x];
          return element_iterator[temp_index];
        }

        key_type key_iterator;
        element_type element_iterator;
      };


  template< typename keyType, typename valueType >
  permutation_iterator< keyType, valueType > make_permutation_iterator( keyType key, valueType value )
  {
      permutation_iterator< keyType, valueType > tmp( key, value );
      return tmp;
  }

}
}


#endif
