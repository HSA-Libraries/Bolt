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

/*! \file bolt/amp/replace.h
    \brief  Replaces every element which satisfies specified condition (if any) in the given input range with specified new value.
*/

#pragma once
#if !defined( BOLT_AMP_REPLACE_H )
#define BOLT_AMP_REPLACE_H

#include <amp.h>
#include "bolt/amp/functional.h"

#include "bolt/amp/bolt.h"
#include <string>
#include <assert.h>

/*! \file replace.h
*/


namespace bolt
{
	namespace amp
	{
        /*! \addtogroup algorithms
         */

        /*! \addtogroup transformations
        *   \ingroup algorithms
        *   \p replace replaces every element in the range [first, last) equal to old_value with new_value. 
		*   replace_if replaces every element in the range [first, last) that satisfies specified condition with new_value.
        */

        /*! \addtogroup AMP-replace
        *   \ingroup transformations
        *   \{
        */


        /*! This version of \p replace / replace_if replaces every element in input sequence that is equal to old_value or that which satisfies the predicate 
		 *  with new_value and stores the result in the corresponding position in input sequence itself.
         *
         *  \param ctl \b Optional Control structure to control accelerator, debug, tuning, etc.See bolt::amp::control.
         *  \param first The beginning of the first input sequence.
         *  \param last The end of the first input sequence.
		 *  \param old_value The value  present in input sequence that needs to be replaced.
		 *  \param new_value The new value to replace.
		 *  \param stencil The beginning of the stencil sequence.
		 *  \param pred The predicate to test on every value of the range [first,last). 
         *  \return Nothing. Input itself is modified.
         *
         *  \tparam InputIterator is a model of InputIterator
         *                        and \c InputIterator's \c value_type is convertible to \c UnaryFunction's
         *  \tparam T is a model of Assignable.
		 *
         *  The following code snippet demonstrates how to use \p replace and \p replace_if.
         *
         *  \code
         *  #include <bolt/amp/replace.h>
         *
         *  int input[10] = {-5,  0,  2,  3,  2,  4, -2,  1,  2,  3};
		 *  int stencil[10] = {0,  0,  1,  1,  0,  1, 1,  1,  0,  1};
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
         *  bolt::amp::replace(ctl, input, input + 10, 3, 300);
         *
         *  // input is now {-5,  0,  2,  300,  2,  4, -2,  1,  2,  300};
		 *
		 *  bolt::amp::replace_if(ctl, input, input + 10, is_even<int>(), -7);
		 *
		 *  // input is now {-5,  -7,  -7,  -7,  -7,  -7, -7,  1,  -7,  -7};
		 *
		 *  bolt::amp::replace_if(ctl,  input, input + 10, stencil, bolt::amp::identity<int>(), 10);
		 *
		 *  // input is now {-5,  -7,  10,  10,  -7,  10, 10,  10,  -7,  10};
		 *
         *  \endcode
         *
         *  \sa http://www.sgi.com/tech/stl/replace.html
         */

	    template<typename InputIterator, typename T>
        void replace(InputIterator first,
               InputIterator last,
               const T &old_value,
               const T &new_value);

		template<typename InputIterator, typename T>
        void replace(control &ctl,
			   InputIterator first,
               InputIterator last,
               const T &old_value,
               const T &new_value);

        template<typename InputIterator, typename Predicate, typename T>
        void replace_if(control &ctl,
                       InputIterator first,
                       InputIterator last,
					   Predicate pred, 
					   const T &new_value);

        template<typename InputIterator, typename Predicate, typename T>
        void replace_if(InputIterator first,
                       InputIterator last,
					   Predicate  	pred,
					   const T &new_value);

		template<typename InputIterator1, typename InputIterator2, typename Predicate, typename T>
		void  replace_if (InputIterator1 first, InputIterator1 last, InputIterator2 stencil,  Predicate pred, const T &new_value); 


		template<typename InputIterator1, typename InputIterator2, typename Predicate, typename T>
		void  replace_if (control &ctl, InputIterator1 first, InputIterator1 last, InputIterator2 stencil, Predicate pred, const T &new_value); 



		 /*! This version of \p replace_copy / replace_copy_if copies elements from the range [first, last) to the range [result, result + (last-first)), 
		 *  except that any element equal to old_value or that which satisfies the predicate is not copied; new_value is copied instead.
         *
         *  \param ctl \b Optional Control structure to control accelerator, debug, tuning, etc.See bolt::amp::control.
         *  \param first The beginning of the first input sequence.
         *  \param last The end of the first input sequence.
		 *  \param OutputIterator is a model of OutputIterator
		 *  \param old_value The value  present in input sequence that needs to be replaced.
		 *  \param new_value The new value to replace.
		 *  \param pred The predicate to test on every value of the range [first,last). 
         *  \return Nothing. Input itself is modified.
         *
         *  \tparam InputIterator is a model of InputIterator
         *                        and \c InputIterator's \c value_type is convertible to \c UnaryFunction's
         *  \tparam T is a model of Assignable.
		 *
         *  The following code snippet demonstrates how to use \p replace_copy and \p replace_copy_if.
         *
         *  \code
         *  #include <bolt/amp/replace.h>
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
         *  bolt::amp::replace_copy(ctl, input, input + 10, output,  3, 300);
         *
         *  // output is now {-5,  0,  2,  300,  2,  4, -2,  1,  2,  300};
		 *
		 *  bolt::amp::replace_copy_if(ctl, input, input + 10, output, is_even<int>(), -7);
		 *
		 *  // output is now {-5,  -7,  -7,  -7,  -7,  -7, -7,  1,  -7,  -7};
		 *
         *  \endcode
         *
         *  \sa http://www.sgi.com/tech/stl/replace.html
         */



		template< typename InputIterator, typename OutputIterator, typename Predicate, typename T>
        OutputIterator replace_copy_if(control &ctl,
                                 InputIterator first,
                                 InputIterator last,
                                 OutputIterator result,
                                 Predicate pred,
                                 const T &new_value);

		template< typename InputIterator, typename OutputIterator, typename Predicate, typename T>
        OutputIterator replace_copy_if(InputIterator first,
                                 InputIterator last,
                                 OutputIterator result,
                                 Predicate pred,
                                 const T &new_value);

		template<typename InputIterator1, typename InputIterator2, typename OutputIterator, typename Predicate, typename T>
        OutputIterator replace_copy_if(control &ctl,
                                 InputIterator1 first,
                                 InputIterator1 last,
                                 InputIterator2 stencil,
                                 OutputIterator result,
                                 Predicate pred,
                                 const T &new_value);

		template<typename InputIterator1, typename InputIterator2, typename OutputIterator, typename Predicate, typename T>
        OutputIterator replace_copy_if(InputIterator1 first,
                                 InputIterator1 last,
                                 InputIterator2 stencil,
                                 OutputIterator result,
                                 Predicate pred,
                                 const T &new_value);

		template<typename InputIterator, typename OutputIterator, typename T>
        OutputIterator replace_copy(control &ctl,
			                  InputIterator first,
                              InputIterator last,
                              OutputIterator result,
                              const T &old_value,
                              const T &new_value);

		template<typename InputIterator, typename OutputIterator, typename T>
        OutputIterator replace_copy(InputIterator first,
                              InputIterator last,
                              OutputIterator result,
                              const T &old_value,
                              const T &new_value);


     /*!   \}  */

	}//amp namespace ends
}//bolt namespace ends

#include <bolt/amp/detail/replace.inl>

#endif // AMP_REPLACE_H


