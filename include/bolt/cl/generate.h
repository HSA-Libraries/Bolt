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

#if !defined( GENERATE_H )
#define GENERATE_H
#pragma once

#include <bolt/cl/bolt.h>
#include <bolt/cl/device_vector.h>

#include <string>
#include <iostream>

namespace bolt {
    namespace cl {

        /*! \addtogroup algorithms
         */

        /*! \addtogroup transformations
        *   \ingroup algorithms
        *   \p generate fills a range with values generated from a Generator,
        *   a function of no arguments.
        */ 
        
        /*! \addtogroup transform
        *   \ingroup transformations
        *   \{
        */

        /*! generate assigns gen(), to each element in the range [first,last).
         *  
         *  \param ctl      Optional control structure to control command-queue, debug, tuning, etc.  See bolt::cl::control.
         *  \param first    The first element in the range of interest.
         *  \param last     The last element in the range of interest.
         *  \param gen      A function argument, taking no parameters, used to generate values to assign to elements in the range [first,last).
         *  \param cl_code  Optional OpenCL(TM) code to be prepended to any OpenCL kernels used by this function.
         *
         *  \tparam ForwardIterator is a model of Forward Iterator, and \c InputIterator \c is mutable.
         *  \tparam Generator is a model of Generator, and \c Generator's \c result_type is convertible to \c ForwardIterator's \c value_type.

         *
         *  The following code snippet demonstrates how to fill a device_vector with random numbers, using the standard C library function rand.
         *
         *  \code
         *  #include <bolt/cl/generate.h>
         *  #include <bolt/device_vector.h>
         *  #include <stdlib.h>
         *  ...
         *  bolt::device_vector<int> v(10);
         *  srand(101082);
         *  bolt::cl::generate(v.begin(), v.end(), rand);
         *
         *  // the elements of v are now pseudo-random numbers
         *  \endcode
         *
         *  \sa http://www.sgi.com/tech/stl/generate.html
         */
        template<typename ForwardIterator, typename Generator> 
        void generate( ForwardIterator first, ForwardIterator last, Generator gen, const std::string& cl_code="");

        template<typename ForwardIterator, typename Generator> 
        void generate( bolt::cl::control &ctl, ForwardIterator first, ForwardIterator last, Generator gen, const std::string& cl_code="");


        /*! generate_n assigns the result of invoking gen, a function object that takes no arguments, to each element in the range [first,first+n).
         *  The return value is first + n.
         *  
         *  \param ctl      Optional control structure to control command-queue, debug, tuning, etc.  See bolt::cl::control.
         *  \param first The first element in the range of interest.
         *  \param n     The size of the range of interest.
         *  \param gen   A function argument, taking no parameters, used to generate values to assign to elements in the range [first,first+n).
         *  \param cl_code Optional OpenCL(TM) code to be prepended to any OpenCL kernels used by this function.
         *
         *  \tparam OutputIterator	is a model of Output Iterator
         *  \tparam Size            is an integral type (either signed or unsigned).
         *  \tparam Generator       is a model of Generator, and Generator's result_type is convertible to a type in OutputIterator's set of value_types.
         *
         *  \return first+n.
         *
         *  The following code snippet demonstrates how to fill a device_vector with random numbers, using the standard C library function rand.
         *
         *  \code
         *  #include <bolt/cl/generate.h>
         *  #include <bolt/device_vector.h>
         *  #include <stdlib.h>
         *  ...
         *  bolt::device_vector<int> v(10);
         *  srand(101082);
         *  bolt::cl::generate_n(v.begin(), 10, rand);
         *
         *  // the elements of v are now pseudo-random numbers
         *  \endcode
         *
         *  \sa http://www.sgi.com/tech/stl/generate.html
         */
        template<typename OutputIterator, typename Size, typename Generator> 
        OutputIterator generate_n( OutputIterator first, Size n, Generator gen, const std::string& cl_code="");

        template<typename OutputIterator, typename Size, typename Generator> 
        OutputIterator generate_n( bolt::cl::control &ctl, OutputIterator first, Size n, Generator gen, const std::string& cl_code="");


        /*!   \}  */
    };
};

#include <bolt/cl/detail/generate.inl>
#endif
