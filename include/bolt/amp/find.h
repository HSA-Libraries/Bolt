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

#if !defined( BOLT_AMP_FIND_H )
#define BOLT_AMP_FIND_H
#pragma once

#include <bolt/amp/bolt.h>
#include <bolt/amp/device_vector.h>

#include <string>
#include <iostream>

/*! \file bolt/amp/find.h
    \brief 
*/

namespace bolt {
    namespace amp {

        /*! \addtogroup algorithms
         */

        /*! \addtogroup find
        *   \ingroup algorithms
        *   \p find is used to determine if a particular element = value / element satisfying a predicate is present in the specified range or not. 
        */

        /*! \addtogroup AMP-find
        *   \ingroup find
        *   \{
        */

        /*! find returns the first iterator i in the range [first, last) such that *i == value or last if no such iterator exists.
		 *  find_if returns the first iterator i in the range [first, last) such that pred(*i) is true or last if no such iterator exists.
		 *  find_if_not returns the first iterator i in the range [first, last) such that pred(*i) is false or last if no such iterator exists.
         *
         *
         * \param ctl \b Optional Control structure to control accelerator, debug, tuning, etc.See bolt::amp::control.
         * \param first Beginning of the source copy sequence.
         * \param last  End of the source copy sequence.
		 * \param value value to search
		 * \param pred  The predicate to test
         * \param result The first iterator i such that *i == value or pred(*i) is true and last otherwise.
         *
         * \tInputIterator is a model of Input Iterator and InputIterator's value_type is equality comparable to type T.  
         * \tT is a model of EqualityComparable. 
         *
         *  \details The following demonstrates how to use \p find, find_if and find_if_not
         *
         *  \code
         *  #include <bolt/amp/find.h>
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
		 *  for(int i = 0; i<length; i++)
		 *  {
		 *       vecSrc[i] = i;
		 *       vecSrc2[i] = i * 2;
		 *  }
		 *
         *  bolt::amp::control ctrl = control::getDefault();
         *  ...
         *  int index = rand()%length;
		 *  int val = vecSrc[index];
		 *
		 *  int val2 = rand();
		 *
		 *  //A Hit
         *  std::vector<int>::iterator itr = bolt::amp::find(ctrl, vecSrc.begin(), vecSrc.end(), val); 
		 *  //*itr = val
		 *
		 *  //May be Hit / Miss
		 *  std::vector<int>::iterator itr2  = bolt::amp::find(ctrl, vecSrc.begin(), vecSrc.end(), val2); 
		 * //*itr2 = val if hit / itr2 = vecSrc2.end() if miss
		 *
		 *  //A Hit
         *  std::vector<int>::iterator itr3 = bolt::amp::find_if(ctrl, vecSrc.begin(), vecSrc.end(), is_even()); 
		 *  //*itr3 = 0 the first value itself satisfies the even condition
		 *
		 *  //A Miss
		 *  std::vector<int>::iterator itr4  = bolt::amp::find_if(ctrl, vecSrc.begin(), vecSrc.end(), is_negative()); 
		 *  //itr4 = vecSrc.end() since no element is negative.
		 *
		 *  //A Hit
         *  std::vector<int>::iterator itr5 = bolt::amp::find_if_not(ctrl, vecSrc.begin(), vecSrc.end(), is_even()); 
		 *  //*itr5 = 1 the second value itself violates the even condition
		 *
		 *  //A Hit
		 *  std::vector<int>::iterator itr6  = bolt::amp::find_if_not(ctrl, vecSrc.begin(), vecSrc.end(), is_negative()); 
		 * //itr6 = 0 the first value itself violates the is_negative condition
		 *
		 *  //A Miss
		 *  std::vector<int>::iterator itr7  = bolt::amp::find_if_not(ctrl, vecSrc2.begin(), vecSrc2.end(), is_even()); 
		 * //itr7 = vecSrc2.end() since no element violates the condition is_even
         *
         *  \endcode
         *
         *
         *  \sa http://www.sgi.com/tech/stl/find.html
         *  \sa http://www.sgi.com/tech/stl/InputIterator.html
         */
        
		 template <typename InputIterator, typename T>
		 InputIterator find(InputIterator first,
				            InputIterator last,
						    const T& value);
 		 template <typename InputIterator, typename T>
		 InputIterator find(bolt::amp::control &ctl,
						    InputIterator first,
				            InputIterator last,
						    const T& value);

 
		 template <typename InputIterator, typename Predicate>
		 InputIterator find_if(InputIterator first,
							   InputIterator last,
							   Predicate pred);
		 template <typename InputIterator, typename Predicate>
		 InputIterator find_if(bolt::amp::control &ctl,
							   InputIterator first,
							   InputIterator last,
							   Predicate pred);
 

		 template <typename InputIterator, typename Predicate>
		 InputIterator find_if_not(InputIterator first,
								   InputIterator last,
								   Predicate pred);
		 template <typename InputIterator, typename Predicate>
		 InputIterator find_if_not(bolt::amp::control &ctl,
								   InputIterator first,
								   InputIterator last,
								   Predicate pred);

    };
};

#include <bolt/amp/detail/find.inl>
#endif