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

#if !defined( BOLT_CL_BINARY_SEARCH_H )
#define BOLT_CL_BINARY_SEARCH_H
#pragma once

#include <bolt/cl/bolt.h>
#include <string>

/*! \file bolt/cl/binary_search.h
    \brief Returns true if the search element is found in the given input range and false otherwise.
*/

namespace bolt {
    namespace cl {
      
        template<typename ForwardIterator, typename T>
        bool binary_search(bolt::cl::control &ctl,
            ForwardIterator first,
            ForwardIterator last,
            const T & value,
            const std::string& cl_code="");

        template<typename ForwardIterator, typename T>
        bool binary_search(ForwardIterator first,
            ForwardIterator last,
            const T & value,
            const std::string& cl_code="");

  
        template<typename ForwardIterator, typename T, typename StrictWeakOrdering>
        bool binary_search(bolt::cl::control &ctl,
            ForwardIterator first,
            ForwardIterator last,
            const T & value,
            StrictWeakOrdering comp,
            const std::string& cl_code="");


        template<typename ForwardIterator, typename T, typename StrictWeakOrdering>
        bool binary_search(ForwardIterator first,
            ForwardIterator last,
            const T & value,
            StrictWeakOrdering comp,
            const std::string& cl_code="");

        /*template<typename ForwardIterator, typename typename InputIterator, typename OutputIterator>
        OutputIterator binary_search(bolt::cl::control &ctl,
            ForwardIterator first,
            ForwardIterator last,
            InputIterator values_first,
            InputIterator values_last,
            OutputIterator result,
            const std::string& cl_code="");

        template<typename ForwardIterator, typename typename InputIterator, typename OutputIterator>
        OutputIterator binary_search(ForwardIterator first,
            ForwardIterator last,
            InputIterator values_first,
            InputIterator values_last,
            OutputIterator result,
            const std::string& cl_code="");

        template<typename ForwardIterator, typename typename InputIterator, typename OutputIterator, typename StrictWeakOrdering>
        OutputIterator binary_search(bolt::cl::control &ctl,
            ForwardIterator first,
            ForwardIterator last,
            InputIterator values_first,
            InputIterator values_last,
            OutputIterator result,
            StrictWeakOrdering comp,
            const std::string& cl_code="");

        template<typename ForwardIterator, typename typename InputIterator, typename OutputIterator, typename StrictWeakOrdering>
        OutputIterator binary_search(ForwardIterator first,
            ForwardIterator last,
            InputIterator values_first,
            InputIterator values_last,
            OutputIterator result,
            StrictWeakOrdering comp,
            const std::string& cl_code="");*/

    }// end of bolt::cl namespace
}// end of bolt namespace

#include <bolt/cl/detail/binary_search.inl>
#endif