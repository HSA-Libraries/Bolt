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

#if !defined( BOLT_BTBB_SCATTER_H )
#define BOLT_BTBB_SCATTER_H
#pragma once

#include "tbb/blocked_range.h"
#include "tbb/task_scheduler_init.h"
#include "tbb/tbb.h"
#include "tbb/parallel_for.h"

/*! \file bolt/cl/scatter.h
    \brief 
*/


namespace bolt {
    namespace btbb {

        /*! \addtogroup algorithms
         */

        /*! \addtogroup transformations
        *   \ingroup algorithms
        *   \p transform applies a specific function object to each element pair in the specified input ranges, and
        *   writes the result into the specified output range. For common code between the host
        *   and device, one can take a look at the ClCode and TypeName implementations. See Bolt Tools for Split-Source
        *   for a detailed description.
        */

        /*! \addtogroup CL-transform
        *   \ingroup transformations
        *   \{
        */


       /*! \brief This version of \p transform applies a unary operation on input sequences and stores the result in
         * the  corresponding position in an output sequence.The input and output sequences can coincide, resulting in
         * an in-place transformation.
         *
         * \param ctl \b Optional Control structure to control command-queue, debug, tuning, etc.See bolt::cl::control.
         *  \param first The beginning of the first input sequence.
         *  \param last The end of the first input sequence.
         *  \param result The beginning of the output sequence.
         * \param op The tranformation operation.
         * \param user_code Optional OpenCL&tm; code to be passed to the OpenCL compiler. The cl_code is inserted
         *   first in the generated code, before the cl_code trait.
         *  \return The end of the output sequence.
         *
         *  \tparam InputIterator1 is a model of InputIterator
         *                        and \c InputIterator1's \c value_type is convertible to \c BinaryFunction's
         * \c first_argument_type.
         *  \tparam InputIterator2 is a model of InputIterator
         *                        and \c InputIterator2's \c value_type is convertible to \c BinaryFunction's
         * \c second_argument_type.
         *  \tparam OutputIterator is a model of OutputIterator
         *
         *  \details The following code snippet demonstrates how to use \p transform
         *
         *  \code
         *  #include <bolt/cl/transform.h>
         *  #include <bolt/cl/functional.h>
         *
         *  int input1[10] = {-5, 0, 2, 3, 2, 4, -2, 1, 2, 3};
         *  int output[10];
         *  bolt::cl::square<int> op;
         *  bolt::cl::::transform(input1, input1 + 10, output, op);
         *
         *  // output is now {25, 0, 4, 9, 4, 16, 4, 1, 4, 9};
         *  \endcode
         *
         *  \sa http://www.sgi.com/tech/stl/transform.html
         *  \sa http://www.sgi.com/tech/stl/InputIterator.html
         *  \sa http://www.sgi.com/tech/stl/OutputIterator.html
         *  \sa http://www.sgi.com/tech/stl/UnaryFunction.html
         *  \sa http://www.sgi.com/tech/stl/BinaryFunction.html
         */

// scatter API
        template< typename InputIterator1,
                  typename InputIterator2,
                  typename OutputIterator >
        void scatter( InputIterator1 first,
                      InputIterator1 last,
                      InputIterator2 map,
                      OutputIterator result);

       
// scatter_if API
        
        template< typename InputIterator1,
                  typename InputIterator2,
                  typename InputIterator3,
                  typename OutputIterator >
        void scatter_if( InputIterator1 first1,
                         InputIterator1 last1,
                         InputIterator2 map,
                         InputIterator3 stencil,
                         OutputIterator result);

     
        template< typename InputIterator1,
                  typename InputIterator2,
                  typename InputIterator3,
                  typename OutputIterator,
                  typename BinaryPredicate >
        void scatter_if( InputIterator1 first1,
                         InputIterator1 last1,
                         InputIterator2 map,
                         InputIterator3 stencil,
                         OutputIterator result,
                         BinaryPredicate pred);


        /*!   \}  */
    }
}

#include <bolt/btbb/detail/scatter.inl>
#endif
