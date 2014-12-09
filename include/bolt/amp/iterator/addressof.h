/***************************************************************************
*   © 2012,2014 Advanced Micro Devices, Inc. All rights reserved.
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
#ifndef BOLT_AMP_ADDRESSOF_H
#define BOLT_AMP_ADDRESSOF_H
#include <bolt/amp/device_vector.h>
#include <bolt/amp/iterator/counting_iterator.h>
#include <bolt/amp/iterator/constant_iterator.h>
#include <bolt/amp/iterator/transform_iterator.h>
#include <bolt/amp/iterator/permutation_iterator.h>

namespace bolt{
namespace amp{

    template < typename Iterator >
    Iterator
    create_mapped_iterator(bolt::amp::permutation_iterator_tag, Iterator first1, int sz, bool var, ::bolt::amp::control &ctl)
    {
        return first1;
    }  
 
	template <typename Iterator>
    Iterator
    create_mapped_iterator(bolt::amp::transform_iterator_tag, Iterator first1, int sz, bool var, ::bolt::amp::control &ctl)
    {
        return first1;
    }   
	
	template <typename Iterator>
    typename bolt::amp::device_vector<typename Iterator::value_type>::iterator 
    create_mapped_iterator(bolt::amp::device_vector_tag, Iterator itr, int sz, bool var, ::bolt::amp::control &ctl)
    {
        return itr;
    }
 
	template <typename Iterator>
    typename bolt::amp::device_vector<typename Iterator::value_type>::iterator 
    create_mapped_iterator(std::random_access_iterator_tag, Iterator first1, int sz, bool var, ::bolt::amp::control &ctl)
    {
        typedef typename std::iterator_traits<Iterator>::value_type  iType;
        typedef typename std::iterator_traits<Iterator>::pointer pointer1;
        
        pointer1 first_pointer1 = std::addressof(*first1);

        device_vector< iType, concurrency::array_view > dvInput1( first_pointer1, sz, var, ctl );
        return dvInput1.begin();

    }


    template <typename T>
    typename bolt::amp::device_vector<T>::iterator 
    create_mapped_iterator(std::random_access_iterator_tag, T* first1, int sz, bool var, ::bolt::amp::control &ctl)
    {
        device_vector< T, concurrency::array_view > dvInput1( first1, sz, var, ctl );
        return dvInput1.begin();
    }

    template <typename Iterator>
    const constant_iterator<typename Iterator::value_type> 
    create_mapped_iterator(bolt::amp::constant_iterator_tag, Iterator itr, int sz, bool var, ::bolt::amp::control &ctl)
    {
        return itr;
    }

    template <typename Iterator>
    const counting_iterator<typename Iterator::value_type> 
    create_mapped_iterator(bolt::amp::counting_iterator_tag, Iterator itr, int sz, bool var, ::bolt::amp::control &ctl)
    {
        return itr;
    }

  

    template <typename Iterator>
    Iterator
    create_mapped_iterator(bolt::amp::transform_iterator_tag, bolt::amp::control &ctl, Iterator &itr) 
    {
        return itr;
    }   
    
    template <typename Iterator>
    Iterator
    create_mapped_iterator(bolt::amp::permutation_iterator_tag, bolt::amp::control &ctl, Iterator &itr) 
    {      
        return itr;
    }   


	template <typename T>
    T * 
    create_mapped_iterator(std::random_access_iterator_tag, bolt::amp::control &ctl, T* first1)
    {
        return first1;
    }


    template <typename Iterator>
    typename std::vector<typename Iterator::value_type>::iterator 
    create_mapped_iterator(std::random_access_iterator_tag, bolt::amp::control &ctl, Iterator &itr)
    {
        return itr;
    }


    template <typename Iterator>
    typename std::iterator_traits<Iterator>::value_type *
    create_mapped_iterator(bolt::amp::device_vector_tag, bolt::amp::control &ctl, Iterator &itr)
    {
        typedef typename std::iterator_traits<Iterator>::value_type iType1;
        bolt::amp::device_vector< iType1 >::pointer first1Ptr = itr.getContainer( ).data( );

        return first1Ptr + itr.m_Index;
    }

    template <typename Iterator>
    const constant_iterator<typename Iterator::value_type> &
    create_mapped_iterator(bolt::amp::constant_iterator_tag, bolt::amp::control &ctl, Iterator &itr)
    {
        return itr;
    }
    
    template <typename Iterator>
    const counting_iterator<typename Iterator::value_type> &
    create_mapped_iterator(bolt::amp::counting_iterator_tag, bolt::amp::control &ctl, Iterator &itr)
    {
        return itr;
    }

 }
}


#endif
