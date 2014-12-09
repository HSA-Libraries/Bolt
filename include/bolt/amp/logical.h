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

#if !defined( BOLT_AMP_LOGICAL_H )
#define BOLT_AMP_LOGICAL_H
#pragma once

#include <bolt/amp/bolt.h>
#include <bolt/amp/device_vector.h>

#include <string>
#include <iostream>

/*! \file bolt/amp/logical.h
    \brief 
*/

namespace bolt {
    namespace amp {

        /*! \addtogroup algorithms
         */

        /*! \addtogroup logical
        *   \ingroup algorithms
        *   \p all_of determines whether all elements in a range satisfy a predicate.
		*   \p any_of determines whether any element in a range satisfies a predicate. 
		*   \p none_of determines whether no element in a range satisfies a predicate.
        */

        /*! \addtogroup AMP-logical
        *   \ingroup logical
        *   \{
        */

        /*! all_of returns true if pred(*i) is true for every iterator i in the range [first, last) and false otherwise.
         *  any_of returns true if pred(*i) is true for any iterator i in the range [first, last) and false otherwise.
		 *  none_of returns true if there is no iterator i in the range [first, last) such that pred(*i) is true, and false otherwise.
         *
         * \param ctl \b Optional Control structure to control accelerator, debug, tuning, etc.See bolt::amp::control.
         * \param first Beginning of the source copy sequence.
         * \param last  End of the source copy sequence.
		 * \param pred  The predicate to test
         * \return bool variable
         *
         * \tparam InputIterator is a model of InputIterator
         *                        and \c InputIterator's \c value_type is convertible to \c UnaryFunction's
         *
         *  \details The following demonstrates how to use \p alll_of, any_of and none_of
         *
         *  \code
         *  #include <bolt/amp/logical.h>
         *  ...
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
		 *  template <typename T>
         *  struct is_negative
         *  {
         *      bool operator()(T x) const restrict(amp, cpu)
         *      {
         *        if (x<0)
         *  		  return true;
         *  	  else
         *  		  return false;
         *      }
         *  };
		 *
		 *  int length = 1024;
         *  std::vector<int> vecSrc(length);
		 *  std::vector<int> vecSrc2(length);
         *
		 *  for(int i = 0; i<length; i++)
		 *  {
		 *       vecSrc[i] = i * 2;
		 *       vecSrc2[i] = i;
		 *  }
		 *
		 *  int n = rand()% (length+1);
         *  bolt::amp::control ctrl = control::getDefault();
         *  ...
         *
         *  bool out1 = bolt::amp::all_of(ctrl, vecSrc.begin(), vecSrc.begin() + n, is_even()); //true
		 *  bool out2 = bolt::amp::all_of(ctrl, vecSrc2.begin(), vecSrc2.end(), is_even()); //false
		 *
		 *  bool out3 = bolt::amp::any_of(ctrl, vecSrc.begin(), vecSrc.end(), is_negative()); //false
		 *  bool out4 = bolt::amp::any_of(ctrl, vecSrc2.begin(), vecSrc2.end(), is_even()); //true
		 *
		 *  bool out5 = bolt::amp::none_of(ctrl, vecSrc.begin(), vecSrc.end(), is_negative()); //true
		 *  bool out6 = bolt::amp::none_of(ctrl, vecSrc2.begin(), vecSrc2.begin() + n, is_even()); //false
		 *
         *
         *  \endcode
         *
         */
        
		 template<typename InputIterator, typename Predicate>
         bool all_of(bolt::amp::control &ctl, InputIterator first, InputIterator last, Predicate pred);

		 template<typename InputIterator, typename Predicate>
         bool all_of(InputIterator first, InputIterator last, Predicate pred);

		 template<typename InputIterator, typename Predicate>
         bool any_of(bolt::amp::control &ctl, InputIterator first, InputIterator last, Predicate pred);

		 template<typename InputIterator, typename Predicate>
         bool any_of(InputIterator first, InputIterator last, Predicate pred);

		 template<typename InputIterator, typename Predicate>
         bool none_of(bolt::amp::control &ctl, InputIterator first, InputIterator last, Predicate pred);

		 template<typename InputIterator, typename Predicate>
         bool none_of(InputIterator first, InputIterator last, Predicate pred);
    };
};

#include <bolt/amp/detail/logical.inl>
#endif