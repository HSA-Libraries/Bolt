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


#include <bolt/cl/iterator/iterator_traits.h>

namespace bolt
{
namespace cl
{


template<typename InputIterator>
typename bolt::cl::iterator_traits<InputIterator>::difference_type
    distance(InputIterator first, InputIterator last);


} } //namespace bolt::cl
#include <bolt/cl/detail/distance.inl>


