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

/*! \file bolt/amp/count.h
    \brief Counts the number of elements in the specified range.
*/


#pragma once
#if !defined( AMP_COUNT_H )
#define AMP_COUNT_H

#include "bolt/amp/bolt.h"
#include "bolt/amp/functional.h"
#include "bolt/amp/transform_reduce.h"
#include "bolt/amp/iterator/iterator_traits.h"

namespace bolt {
    namespace amp {

        /*! \addtogroup algorithms
         */

        /*! \addtogroup reductions
        *   \ingroup algorithms

        /*! \addtogroup amp-counting
        *  \ingroup reductions
        *  \{
        */

        namespace detail
        {

            /*! \brief CountIfEqual: A bolt functor which matches if the object value is same as the input value
             */

            template <typename T>
            struct CountIfEqual {
                CountIfEqual(const T &targetValue) restrict (amp, cpu)  : _targetValue(targetValue)
                { };

                bool operator() (const T &x) const restrict (amp, cpu) {
                    return x == _targetValue;
                };

            private:
                T _targetValue;
            };
        }

        /*!
        * \brief \p count counts the number of elements in the specified range which compare equal to the specified 
        * \p value.
        *
        * \param ctl \b Optional Control structure to control accelerator, debug, tuning, etc.  See bolt::amp::control.
        * \param first The first position in the sequence to be counted.
        * \param last The last position in the sequence to be counted.
        * \param value This is the value to be searched.
        * \return  The number of elements which matches \p value.
        *
        * \code
        *  int a[14] = {0, 10, 42, 55, 13, 13, 42, 19, 42, 11, 42, 99, 13, 77};
        *
        * size_t countOf42 = bolt::amp::count (A, A+14, 42);
        *  // countOf42 contains 4.
        * \endcode
        *

        */

        template<typename InputIterator, typename EqualityComparable>
        typename bolt::amp::iterator_traits<InputIterator>::difference_type
            count(control& ctl,
            InputIterator first,
            InputIterator last,
            EqualityComparable &value)
        {
            typedef typename std::iterator_traits<InputIterator>::value_type T;
            return bolt::amp::count_if(ctl, first, last, detail::CountIfEqual<T>(value));
        };


        template<typename InputIterator, typename EqualityComparable>
        typename bolt::amp::iterator_traits<InputIterator>::difference_type
            count(InputIterator first,
            InputIterator last,
            EqualityComparable &value)
        {
            typedef typename std::iterator_traits<InputIterator>::value_type T;
            return bolt::amp::count_if(first, last, detail::CountIfEqual<T>(value));
        };


        /*!
        * \brief \p count_if counts the number of elements in the specified range for which the specified \p predicate 
        * is \p true.
        *
        * \param ctl \b Optional Control structure to control accelerator, debug, tuning, etc.  See bolt::amp::control.
        * \param first The first position in the sequence to be counted.
        * \param last The last position in the sequence to be counted.
        * \param predicate The predicate. The count is incremented for each element which returns true when passed to the predicate function.
        * \returns The number of elements for which \p predicate is true.
        * \code
        *  template < typename T >
        *  struct IsEven
        *  {
        *      bool operator()(const T& value) const restrict (amp,cpu)
        *      {return ( (value%2) == 0 ) ;
        *      };
        *  };
        *  ...
        *  //Main
        *
        *  //Create an AMP Control object using the default accelerator
        *  ::Concurrency::accelerator accel(::Concurrency::accelerator::default_accelerator);
        *  bolt::amp::control ctl(accel);
        *
        *  int a[14] = {0, 10, 42, 55, 13, 13, 42, 19, 42, 11, 42, 99, 13, 77};
        *
        *
        *  size_t countIsEven = bolt::amp::count (ctl, A, A+14, IsEven);
        * // countIsEven contains 6 even elements.
        * \endcode
        *
        *
        */

        template<typename InputIterator, typename Predicate>
        typename bolt::amp::iterator_traits<InputIterator>::difference_type
            count_if(control& ctl,
            InputIterator first,
            InputIterator last,
            Predicate predicate)
        {
            typedef typename bolt::amp::iterator_traits<InputIterator>::value_type CountType;
			      //typedef typename bolt::amp::iterator_traits<InputIterator>::difference_type ResultType;

            // C++ AMP has a limitation: Can't use __int64. Calling transform_reduce with int as
            // return type seems to be a good option.
            typedef int ResultType;

            ResultType result = static_cast< ResultType >( transform_reduce( ctl, first, last,
                predicate, static_cast< CountType >( 0 ), bolt::amp::plus< CountType >( )) );

            return result;
        };


        template<typename InputIterator, typename Predicate>
        typename bolt::amp::iterator_traits<InputIterator>::difference_type
            count_if(InputIterator first,
            InputIterator last,
            Predicate predicate)
        {
            typedef typename bolt::amp::iterator_traits<InputIterator>::value_type CountType;
			      //typedef typename bolt::amp::iterator_traits<InputIterator>::difference_type ResultType;

            // C++ AMP has a limitation: Can't use __int64. Calling transform_reduce with int as
            // return type seems to be a good option.
            typedef int ResultType;

            ResultType result = static_cast< ResultType >( transform_reduce( first, last,
                predicate, static_cast< CountType >( 0 ), bolt::amp::plus< CountType >( )) );

            return result;
        };

        /*!   \}  */
    };
};
#endif