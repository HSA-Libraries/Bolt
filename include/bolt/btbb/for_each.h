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

#pragma once
#if !defined( BOLT_BTBB_FOR_EACH_H )
#define BOLT_BTBB_FOR_EACH_H

/*! \file bolt/tbb/for_each.h
    \brief Iterates over the range of input elements and applies the unary function specified over them.
*/


namespace bolt {
    namespace btbb {

		template<typename InputIterator , typename UnaryFunction >   
        void for_each (InputIterator first, InputIterator last, UnaryFunction f); 

		template<typename InputIterator , typename Size , typename UnaryFunction >  
        void for_each_n  ( InputIterator  first,  Size  n,  UnaryFunction  f); 

    };
};


#include <bolt/btbb/detail/for_each.inl>

#endif