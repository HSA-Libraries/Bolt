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
        typedef transform_iterator_tag iterator_category;
        //friend class boost::transform_iterator <UnaryFunc, Iterator>;
        transform_iterator(Iterator const& x, UnaryFunc f): boost::transform_iterator<UnaryFunc, Iterator>( x,  f)
          { }
    };
}
}

#endif
