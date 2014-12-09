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

/*! \file bolt/amp/remove.h
    \brief  Removes every element which is equal to specified value or that which satisfies specified condition (if any) in the given input range.
*/

#pragma once
#if !defined( BOLT_AMP_UNIQUE_H )
#define BOLT_AMP_UNIQUE_H

#include <amp.h>
#include "bolt/amp/functional.h"

#include "bolt/amp/bolt.h"
#include <string>
#include <assert.h>

/*! \file unique.h
*/


namespace bolt
{
	namespace amp
	{
        /*! \addtogroup algorithms
         */

        /*! \addtogroup transformations
        *   \ingroup algorithms
        *   \p unique For each group of consecutive elements in the range [first, last) with the same value, unique removes all but the first element of the group.
		*   unique_copy copies elements from the range [first, last) to a range beginning with output, except that in a consecutive group of duplicate elements only the first one is copied.
        */

        /*! \addtogroup AMP-unique
        *   \ingroup transformations
        *   \{
        */


        /*! This version of \p unique removes all but the first element of the group in a group of consecutive elements in the range [first, last) with the same value.
		 *  The return value is an iterator new_last such that no two consecutive elements in the range [first, new_last) are equal. 
		 *  \p unique_copy copies elements from the range [first, last) to a range beginning with output,
		 *  except that in a consecutive group of duplicate elements only the first one is copied. 
		 *  The return value is the end of the range to which the elements are copied.
         *
         *  \param ctl \b Optional Control structure to control accelerator, debug, tuning, etc.See bolt::amp::control.
         *  \param first The beginning of the first input sequence.
         *  \param last The end of the first input sequence.
		 *  \param pred The predicate to test on every value of the range [first,last). 
         *  \return ForwardIterator or OutputIterator
         *
         *  \tparam ForwardIterator is a model of InputIterator
         *                        and \c InputIterator's \c value_type is convertible to \c UnaryFunction's
		 *
         *  The following code snippet demonstrates how to use \p replace and \p replace_if.
         *
         *  \code
         *  #include <bolt/amp/unique.h>
         *
         *  int input1[10] = {-5,  0,  0,  3,  3,  3, 3,  1,  1,  1};
		 *  int input2[10] = {-5,  0,  0,  3,  3,  3, 3,  1,  1,  1};
		 *  int output[10];
         *
         *  //Create an AMP Control object using the default accelerator
         *  ::Concurrency::accelerator accel(::Concurrency::accelerator::default_accelerator);
         *  bolt::amp::control ctl(accel);
		 *  
         *  bolt::amp::unique(ctl, input1, input1 + 10, bolt::amp::equal_to<int>());
         *
         *  // input1 is now {-5,  0,  3,   1};
		 *
		 *  bolt::amp::unique_copy(ctl, input2, input2 + 10, output, bolt::amp::equal_to<int>()));
		 *
		 *  // output is now {-5,  0,  3,   1};
		 *
		 *
         *  \endcode
         *
         *  \sa http://www.sgi.com/tech/stl/unique.html
         */


		 template<typename ForwardIterator>
         ForwardIterator unique(control &ctl, ForwardIterator first, ForwardIterator last);

	     template<typename ForwardIterator>
         ForwardIterator unique( ForwardIterator first, ForwardIterator last);

		 template<typename ForwardIterator, typename BinaryPredicate>
         ForwardIterator unique(control &ctl, 
                       ForwardIterator first,
                       ForwardIterator last,
                       BinaryPredicate binary_pred);
		 
		 template<typename ForwardIterator, typename BinaryPredicate>
         ForwardIterator unique(ForwardIterator first,
                       ForwardIterator last,
                       BinaryPredicate binary_pred);

		 template<typename InputIterator, typename OutputIterator>
         OutputIterator unique_copy(control &ctl, 
                           InputIterator first,
                           InputIterator last,
                           OutputIterator output);

		 template<typename InputIterator, typename OutputIterator>
         OutputIterator unique_copy(InputIterator first,
                           InputIterator last,
                           OutputIterator output);

		 template<typename InputIterator, typename OutputIterator, typename BinaryPredicate>
         OutputIterator unique_copy(control &ctl,  
                           InputIterator first,
                           InputIterator last,
                           OutputIterator output,
                           BinaryPredicate binary_pred);

		 template<typename InputIterator, typename OutputIterator, typename BinaryPredicate>
         OutputIterator unique_copy(InputIterator first,
                           InputIterator last,
                           OutputIterator output,
                           BinaryPredicate binary_pred);



     /*!   \}  */

	}//amp namespace ends
}//bolt namespace ends

#include <bolt/amp/detail/unique.inl>

#endif // AMP_UNIQUE_H


