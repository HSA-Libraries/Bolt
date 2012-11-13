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

#if !defined( COPY_H )
#define COPY_H
#pragma once

#include <bolt/cl/bolt.h>
#include <bolt/cl/device_vector.h>

#include <string>
#include <iostream>

namespace bolt {
    namespace cl {

        /*! \addtogroup algorithms
         */

        /*! \addtogroup copying
        *   \ingroup algorithms
        *   \p copy copies elements from the range [first, last) to the range [result, result + (last - first)). 
        */ 
        
        /*! \addtogroup copy
        *   \ingroup copyations
        *   \{
        */

        /*! copy copies elements from the range [first, last) to the range [result, result + (last - first)).
         *  That is, it performs the assignments *result = *first, *(result + 1) = *(first + 1), and so on.
         *  Generally, for every integer n from 0 to last - first, copy performs the assignment *(result + n) = *(first + n).
         *  Unlike std::copy, copy offers no guarantee on order of operation.
         *  As a result, calling copy with overlapping source and destination ranges has undefined behavior.
         *    
         *  \param ctl Control structure to control command-queue, debug, tuning, etc.  See bolt::cl::control.
         *  \param first The beginning of the sequence to copy.
         *  \param last  The end of the sequence to copy.
         *  \param result The destination sequence.
         *  \return result + (last - first).
         *
         *  \tparam InputIterator is a model of InputIterator
         *                        and \c InputIterator1's \c value_type must be convertible to \c OutputIterator's \c value_type.
         *  \tparam OutputIterator is a model of OutputIterator
         *
         *  The following code snippet demonstrates how to use \p copy to copy from one range to another.
         *
         *  \code
         *  #include <bolt/cl/copy.h>
         *  ...
         *
         *  std::vector<int> vec0(100);
         *  std::vector<int> vec1(100);
         *  ...
         *
         *  bolt::cl::copy(vec0.begin(), vec0.end(), vec1.begin());
         *
         *  // vec1 is now a copy of vec0
         *  \endcode
         *
         *  \sa http://www.sgi.com/tech/stl/copy.html
         *  \sa http://www.sgi.com/tech/stl/InputIterator.html
         *  \sa http://www.sgi.com/tech/stl/OutputIterator.html
         */
        template<typename InputIterator, typename OutputIterator> 
        OutputIterator copy(const bolt::cl::control &ctl,  InputIterator first, InputIterator last, OutputIterator result, 
            const std::string& user_code="");

        template<typename InputIterator, typename OutputIterator> 
        OutputIterator copy( InputIterator first, InputIterator last, OutputIterator result, 
            const std::string& user_code="");

        template<typename InputIterator, typename Size, typename OutputIterator> 
        OutputIterator copy_n(InputIterator first, Size n, OutputIterator result, 
            const std::string& user_code="");

        template<typename InputIterator, typename Size, typename OutputIterator> 
        OutputIterator copy_n(const bolt::cl::control &ctl, InputIterator first, Size n, OutputIterator result, 
            const std::string& user_code="");

        /*!   \}  */
    };
};

#include <bolt/cl/detail/copy.inl>
#endif