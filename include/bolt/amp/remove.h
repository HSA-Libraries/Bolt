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
#if !defined( BOLT_AMP_REMOVE_H )
#define BOLT_AMP_REMOVE_H

#include <amp.h>
#include "bolt/amp/functional.h"

#include "bolt/amp/bolt.h"
#include <string>
#include <assert.h>

/*! \file remove.h
*/


namespace bolt
{
	namespace amp
	{
        /*! \addtogroup algorithms
         */

        /*! \addtogroup transformations
        *   \ingroup algorithms
        *   \p remove removes from the range [first, last) all elements that are equal to value. 
		*   remove_if removes from the range [first, last) every element x such that pred(x) is true. 
        */

        /*! \addtogroup AMP-remove
        *   \ingroup transformations
        *   \{
        */


        /*! This version of \p remove / remove_if removes every element in input sequence that is equal to value or that which satisfies the predicate 
		 *  and stores the result in the corresponding position in input sequence itself.
		 * The iterators in the range [new_last,last) are all still dereferenceable, but the elements that they point to are unspecified. 
		 * It is stable, meaning that the relative order of elements that are not removed is unchanged.
         *
         *  \param ctl \b Optional Control structure to control accelerator, debug, tuning, etc.See bolt::amp::control.
         *  \param first The beginning of the first input sequence.
         *  \param last The end of the first input sequence.
		 *  \param value The value  present in input sequence that needs to be removed.
		 *  \param stencil The beginning of the stencil sequence.
		 *  \param pred The predicate to test on every value of the range [first,last). 
         *  \return InputIterator. Input itself is modified and returned.
         *
         *  \tparam InputIterator is a model of InputIterator
         *                        and \c InputIterator's \c value_type is convertible to \c UnaryFunction's
         *  \tparam T is a model of Assignable.
		 *
         *  The following code snippet demonstrates how to use \p replace and \p replace_if.
         *
         *  \code
         *  #include <bolt/amp/remove.h>
         *
         *  int input[10] = {-5,  0,  2,  3,  2,  4, -2,  1,  2,  3};
		 *  int stencil[10] = {0,  1,  1,  1,  0,  1, 1,  1,  0,  1};
         *
         *  //Create an AMP Control object using the default accelerator
         *  ::Concurrency::accelerator accel(::Concurrency::accelerator::default_accelerator);
         *  bolt::amp::control ctl(accel);
		 *  
		 *  template <typename T>
         *  struct is_even
         *  {
         *      bool operator()(T x) const restrict(amp, cpu)
         *      {
         *        if (((int)x) % 2)
         *  		  return true;
         *  	  else
         *  		  return false;
         *      }
         *  };
		 *  
         *  bolt::amp::remove(ctl, input, input + 10, 3);
         *
         *  // As this is inplace call, input array will become {-5,  0,  2,   2,  4, -2,  1,  2,  0,  0};
		 *
		 *  bolt::amp::remove_if(ctl, input, input + 10, is_even<int>());
		 *
		 *  // Applying remove_if on above input array after applying remove call 
		 *				will change input array to {-5,   1,  0,  0,  0,  0,  0,  0,  0,  0};
		 *
		 *  bolt::amp::remove_if(ctl,  input, input + 10, stencil, bolt::amp::identity<int>(), 10);
		 *
		 *  // input array after remove_if with stencil is {-5,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0};
		 *
         *  \endcode
         *
         *  \sa http://www.sgi.com/tech/stl/replace.html
         */

		 template<typename ForwardIterator, typename Predicate>
         ForwardIterator remove_if(control &ctl,
			                ForwardIterator first,
                            ForwardIterator last,
                            Predicate pred);


	     template<typename ForwardIterator, typename Predicate>
         ForwardIterator remove_if(ForwardIterator first,
                            ForwardIterator last,
                            Predicate pred);


		 template< typename ForwardIterator, typename InputIterator, typename Predicate>
         ForwardIterator remove_if( control &ctl,
			                ForwardIterator first,
                            ForwardIterator last,
                            InputIterator stencil,
                            Predicate pred);


		 template< typename ForwardIterator, typename InputIterator, typename Predicate>
         ForwardIterator remove_if(ForwardIterator first,
                            ForwardIterator last,
                            InputIterator stencil,
                            Predicate pred);

		 template< typename ForwardIterator, typename T>
         ForwardIterator remove(control &ctl,
			             ForwardIterator first,
                         ForwardIterator last,
                         const T &value);

		 template< typename ForwardIterator, typename T>
         ForwardIterator remove(ForwardIterator first,
                         ForwardIterator last,
                         const T &value);

		 /*! This version of \p remove_copy / remove_copy_if copies elements from the range [first, last) to the range [result, result + (last-first)), 
		 *  except that any element equal to value or that which satisfies the predicate is not copied.
         *
         *  \param ctl \b Optional Control structure to control accelerator, debug, tuning, etc.See bolt::amp::control.
         *  \param first The beginning of the first input sequence.
         *  \param last The end of the first input sequence.
		 *  \param OutputIterator is a model of OutputIterator
		 *  \param value The value  present in input sequence that needs to be removed.
		 *  \param pred The predicate to test on every value of the range [first,last). 
         *  \return OutputIterator result is returned.
         *
         *  \tparam InputIterator is a model of InputIterator
         *                        and \c InputIterator's \c value_type is convertible to \c UnaryFunction's
         *  \tparam T is a model of Assignable.
		 *
         *  The following code snippet demonstrates how to use \p remove_copy and \p remove_copy_if.
         *
         *  \code
         *  #include <bolt/amp/remove.h>
         *
         *  int input[10] = {-5,  0,  2,  3,  2,  4, -2,  1,  2,  3};
		 *  int output[10];
         *
         *  //Create an AMP Control object using the default accelerator
         *  ::Concurrency::accelerator accel(::Concurrency::accelerator::default_accelerator);
         *  bolt::amp::control ctl(accel);
		 *  
		 *  template <typename T>
         *  struct is_even
         *  {
         *      bool operator()(T x) const restrict(amp, cpu)
         *      {
         *        if (((int)x) % 2)
         *  		  return true;
         *  	  else
         *  		  return false;
         *      }
         *  };
		 *  
         *  bolt::amp::remove_copy(ctl, input, input + 10, output,  3);
         *
         *  // output is now {-5,  0,  2,  2,  4, -2,  1,  2,  0, 0};
		 *
		 *  bolt::amp::remove_copy_if(ctl, input, input + 10, output, is_even<int>());
		 *
		 *  // output is now {-5,  1,  0,  0,  0,  0,  0,  0,  0,  0};
		 *
         *  \endcode
         *
         *  \sa http://www.sgi.com/tech/stl/remove.html
         */

		template<typename InputIterator, typename OutputIterator, typename Predicate>
        OutputIterator remove_copy_if(control &ctl,
			                    InputIterator first,
                                InputIterator last,
                                OutputIterator result,
                                Predicate pred);


		template<typename InputIterator, typename OutputIterator, typename Predicate>
        OutputIterator remove_copy_if(InputIterator first,
                                InputIterator last,
                                OutputIterator result,
                                Predicate pred);


		template<typename InputIterator1, typename InputIterator2, typename OutputIterator, typename Predicate>
        OutputIterator remove_copy_if(control &ctl,
			                    InputIterator1 first,
                                InputIterator1 last,
                                InputIterator2 stencil,
                                OutputIterator result,
                                Predicate pred);

		template<typename InputIterator1, typename InputIterator2, typename OutputIterator, typename Predicate>
        OutputIterator remove_copy_if(InputIterator1 first,
                                InputIterator1 last,
                                InputIterator2 stencil,
                                OutputIterator result,
                                Predicate pred);


		template<typename InputIterator, typename OutputIterator, typename T>
        OutputIterator remove_copy(control &ctl,
			                 InputIterator first,
                             InputIterator last,
                             OutputIterator result,
                             const T &value);

        template<typename InputIterator, typename OutputIterator, typename T>
        OutputIterator remove_copy(InputIterator first,
                             InputIterator last,
                             OutputIterator result,
                             const T &value);


     /*!   \}  */

	}//amp namespace ends
}//bolt namespace ends

#include <bolt/amp/detail/remove.inl>

#endif // AMP_REMOVE_H


