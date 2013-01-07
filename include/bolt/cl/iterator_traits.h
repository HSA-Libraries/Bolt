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

#pragma once
#if !defined( ITERATOR_TRAITS_H )
#define ITERATOR_TRAITS_H

/*! \file iterator_traits.h
    \brief Defines new iterator_traits structures used by the Bolt runtime to make runtime decisions on how to 
    dispatch calls to various supported backends
*/

// #include <iterator>

namespace bolt {
namespace cl {

    struct fancy_iterator_tag
        : public std::random_access_iterator_tag
        {   // identifying tag for random-access iterators
        };

    template< typename Iterator >
    struct iterator_traits
    {
        typedef typename Iterator::iterator_category iterator_category;
        typedef typename Iterator::value_type value_type;
        typedef typename Iterator::difference_type difference_type;
        typedef typename Iterator::pointer pointer;
        typedef typename Iterator::reference reference;
    };

    template< class T >
    struct iterator_traits< T* >
    {
        typedef std::random_access_iterator_tag iterator_category;
        typedef T value_type;
        //  difference_type set to int for OpenCL backend
        typedef int difference_type;
        typedef T* pointer;
        typedef T& reference;
    };

    template< class T >
    struct iterator_traits< const T* >
    {
        typedef std::random_access_iterator_tag iterator_category;
        typedef T value_type;
        //  difference_type set to int for OpenCL backend
        typedef int difference_type;
        typedef const T* pointer;
        typedef const T& reference;
    };

}
};

#endif