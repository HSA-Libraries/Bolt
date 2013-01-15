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
#if !defined( CONSTANT_ITERATOR_H )
#define CONSTANT_ITERATOR_H
#include "bolt/cl/bolt.h"
#include "bolt/cl/iterator/iterator_traits.h"

namespace bolt {
namespace cl {

    struct constant_iterator_tag
        : public fancy_iterator_tag
        {   // identifying tag for random-access iterators
        };

    //  This represents the host side definition of the constant_iterator template
    //BOLT_TEMPLATE_FUNCTOR3( constant_iterator, int, float, double,
        template< typename T >
        class constant_iterator
        {
        public:
            typedef constant_iterator_tag iterator_category;
            typedef T value_type;
            typedef size_t difference_type;
            typedef size_t size_type;
            typedef T* pointer;
            typedef T& reference;

            constant_iterator( value_type init ): m_constValue( init )
            {};

            value_type operator[]( size_type ) const
            {
                return m_constValue;
            }

            value_type operator*( ) const
            {
                return m_constValue;
            }

            //  Implementation 
            value_type m_constValue;
            static const size_t m_Index = 0;
        };
    //)

    //  This string represents the device side definition of the constant_iterator template
    static std::string deviceConstantIterator = STRINGIFY_CODE( 
        template< typename T >
        class constant_iterator
        {
        public:
            typedef int iterator_category;      // device code does not understand std:: tags
            typedef T value_type;
            typedef size_t difference_type;
            typedef size_t size_type;
            typedef T* pointer;
            typedef T& reference;

            constant_iterator( value_type init ): m_constValue( init )
            {};

            value_type operator[]( size_type ) const
            {
                return m_constValue;
            }

            value_type operator*( ) const
            {
                return m_constValue;
            }

            value_type  m_constValue;
            static const size_t m_Index = 0;
        };
    );

    BOLT_CREATE_TYPENAME( constant_iterator<int> );
    BOLT_CREATE_CLCODE( constant_iterator<int>, deviceConstantIterator );

    BOLT_TEMPLATE_REGISTER_NEW_TYPE( constant_iterator, int, float );
    BOLT_TEMPLATE_REGISTER_NEW_TYPE( constant_iterator, int, double );

    template< typename Type >
    constant_iterator< Type > make_constant_iterator( Type constValue )
    {
        constant_iterator< Type > tmp( constValue );
        return tmp;
    }

}
}

#endif