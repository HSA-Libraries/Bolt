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

// (C) Copyright Toon Knapen    2001.
// (C) Copyright David Abrahams 2003.
// (C) Copyright Roland Richter 2003.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOLT_PERMUTATION_ITERATOR_H
#define BOLT_PERMUTATION_ITERATOR_H

//#include <iterator>
#include <type_traits>

#include <bolt/cl/iterator/iterator_adaptor.h>
#include <bolt/cl/iterator/iterator_facade.h>
#include <bolt/cl/iterator/iterator_traits.h>
#include <bolt/cl/device_vector.h>


namespace bolt {
namespace cl {
  struct permutation_iterator_tag
      : public fancy_iterator_tag
        {  };

template< class ElementIterator
        , class IndexIterator>
class permutation_iterator
  : public iterator_adaptor< 
             permutation_iterator<ElementIterator, IndexIterator>
           , IndexIterator, typename bolt::cl::iterator_traits<ElementIterator>::value_type
           , use_default, typename bolt::cl::iterator_traits<ElementIterator>::reference>
{
  typedef iterator_adaptor< 
            permutation_iterator<ElementIterator, IndexIterator>
          , IndexIterator, typename bolt::cl::iterator_traits<ElementIterator>::value_type
          , use_default, typename bolt::cl::iterator_traits<ElementIterator>::reference> super_t;

  friend class iterator_core_access;

public:
  permutation_iterator() : m_elt_iter() {}

  explicit permutation_iterator(ElementIterator x, IndexIterator y) 
      : super_t(y), m_elt_iter(x) {}

  template<class OtherElementIterator, class OtherIndexIterator>
  permutation_iterator(
      permutation_iterator<OtherElementIterator, OtherIndexIterator> const& r
      , typename enable_if_convertible<OtherElementIterator, ElementIterator>::type* = 0
      , typename enable_if_convertible<OtherIndexIterator, IndexIterator>::type* = 0
      )
    : super_t(r.base()), m_elt_iter(r.m_elt_iter)
  {}

private:
    typename super_t::reference dereference() const
        { return *(m_elt_iter + *this->base()); }


public:
    ElementIterator m_elt_iter;
};


template <class ElementIterator, class IndexIterator>
permutation_iterator<ElementIterator, IndexIterator> 
make_permutation_iterator( ElementIterator e, IndexIterator i )
{
    return permutation_iterator<ElementIterator, IndexIterator>( e, i );
}


   //  This string represents the device side definition of the Transform Iterator template
    static std::string devicePermutationIteratorTemplate = 
        std::string("#if !defined(BOLT_CL_PERMUTATION_ITERATOR) \n") +
        STRINGIFY_CODE(
            #define BOLT_CL_PERMUTATION_ITERATOR \n
            namespace bolt { namespace cl { \n
            template< typename IndexIterator, typename ElementIterator > \n
            class permutation_iterator \n
            { \n
                public:    \n
                    typedef int iterator_category;        \n
                    typedef typename ElementIterator::value_type value_type; \n
                    typedef typename IndexIterator::value_type index_type; \n
                    typedef int difference_type; \n
                    typedef int size_type; \n
                    typedef value_type* pointer; \n
                    typedef value_type& reference; \n
    
                    permutation_iterator( value_type init ): m_StartIndex( init ), m_Ptr( 0 ) \n
                    {} \n
    
                    void init( global value_type* ptr )\n
                    { \n
                        m_Ptr = ptr; \n
                    } \n

                    value_type operator[]( size_type threadID ) const \n
                    { \n
                       return m_f(m_Ptr[ m_StartIndex + threadID ]); \n
                    } \n

                    value_type operator*( ) const \n
                    { \n
                        return m_ElementPtr[m_IndexPtr[ m_StartIndex + threadID ] ]; \n
                    } \n

                    size_type m_StartIndex; \n
                    global value_type* m_ElementPtr; \n
                    global index_type* m_IndexPtr; \n
            }; \n
            } } \n
        )
        +  std::string("#endif \n"); 

} // End of namespace cl
} // End of namespace bolt
#endif
