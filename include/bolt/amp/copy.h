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

#if !defined( BOLT_AMP_COPY_H )
#define BOLT_AMP_COPY_H
#pragma once

#include <bolt/amp/bolt.h>
#include <bolt/amp/device_vector.h>

#include <string>
#include <iostream>

/*! \file bolt/amp/copy.h
    \brief Copies each element from the sequence to result.
*/

namespace bolt {
    namespace amp {

        /*! \addtogroup algorithms
         */

        /*! \addtogroup copying
        *   \ingroup algorithms
        *   \p copy copies each element from the sequence [first, last) to [result, result + (last - first)).
        */

        /*! \addtogroup AMP-copy
        *   \ingroup copying
        *   \{
        */

        /*! copy copies each element from the sequence [first, last) to [result, result + (last - first)), i.e.,
         *  it assigns *result = *first, then *(result + 1) = *(first + 1), and so on.
         *
         *  Calling copy with overlapping source and destination ranges has undefined behavior, as the order
         *  of copying on the GPU is not guaranteed.
         *
         * \param ctl \b Optional Control structure to control accelerator, debug, tuning, etc.See bolt::amp::control.
         * \param first Beginning of the source copy sequence.
         * \param last  End of the source copy sequence.
         * \param result Beginning of the destination sequence.
         * \return result + (last - first).
         *
         * \tparam InputIterator is a model of InputIterator
         * and \c InputIterator's \c value_type must be convertible to \c OutputIterator's \c value_type.
         * \tparam OutputIterator is a model of OutputIterator
         *
         *  \details The following demonstrates how to use \p copy.
         *
         *  \code
         *  #include <bolt/amp/copy.h>
         *  ...
         *
         *  std::vector<float> vecSrc(128);
         *  std::vector<float> vecDest(128);
         *  bolt::amp::control ctrl = control::getDefault();
         *  ...
         *
         *  bolt::amp::copy(ctrl, vecSrc.begin(), vecSrc.end(), vecDest.begin());
         *
         *  // vecDest is now a copy of vecSrc
         *  \endcode
         *
         *  \sa http://www.sgi.com/tech/stl/copy.html
         *  \sa http://www.sgi.com/tech/stl/InputIterator.html
         *  \sa http://www.sgi.com/tech/stl/OutputIterator.html
         */
        template<typename InputIterator, typename OutputIterator>
        OutputIterator copy(
            const bolt::amp::control &ctl,
            InputIterator first,
            InputIterator last,
            OutputIterator result);

        template<typename InputIterator, typename OutputIterator>
        OutputIterator copy(
            InputIterator first,
            InputIterator last,
            OutputIterator result);

        /*! copy_n copies each element from the sequence [first, first+n) to [result, result + n), i.e.,
         *  it assigns *result = *first, then *(result + 1) = *(first + 1), and so on.
         *
         *  Calling copy_n with overlapping source and destination ranges has undefined behavior, as the order
         *  of copying on the GPU is not guaranteed.
         *

         *  \param ctl \b Optional Control structure to control accelerator, debug, tuning, etc.See bolt::amp::control.
         *  \param first Beginning of the source copy sequence.
         *  \param n  Number of elements to copy.
         *  \param result Beginning of the destination sequence.
         *  \return result + n.
         *
         *  \tparam InputIterator is a model of InputIterator
         *  and \c InputIterator's \c value_type must be convertible to \c OutputIterator's \c value_type.
         *  \tparam Size is an integral type.
         *  \tparam OutputIterator is a model of OutputIterator
         *
         * \details The following demonstrates how to use \p copy.
         *
         *  \code
         *  #include <bolt/amp/copy.h>
         *  ...
         *
         *  std::vector<float> vecSrc(128);
         *  std::vector<float> vecDest(128);
         *  ...
         *
         *  bolt::amp::copy_n(vecSrc.begin(), 128, vecDest.begin());
         *
         *  // vecDest is now a copy of vecSrc
         *  \endcode
         *
         *  \sa http://www.sgi.com/tech/stl/copy_n.html
         *  \sa http://www.sgi.com/tech/stl/InputIterator.html
         *  \sa http://www.sgi.com/tech/stl/OutputIterator.html
         */

        template<typename InputIterator, typename Size, typename OutputIterator>
        OutputIterator copy_n(
            const bolt::amp::control &ctl,
            InputIterator first,
            Size n,
            OutputIterator result);

        template<typename InputIterator, typename Size, typename OutputIterator>
        OutputIterator copy_n(
            InputIterator first,
            Size n,
            OutputIterator result);

        /*!   \}  */

/*! \addtogroup stream_compaction
 *  \{
 */


/*! This version of \p copy_if copies elements from the range <tt>[first,last)</tt>
 *  to a range beginning at \ presult, except that any element which causes \p pred
 *  to be \p pred to be \c false is not copied.
 *
 *  More precisely, for every integer \c n such that <tt>0 <= n < last-first</tt>,
 *  \p copy_if performs the assignment <tt>*result = *(first+n)</tt> and \p result
 *  is advanced one position if <tt>pred(*(first+n))</tt>. Otherwise, no assignment
 *  occurs and \p result is not advanced.
 *
 *  The algorithm's execution is parallelized as determined by \p system.
 *
 *  \param ctl \b Optional Control structure to control accelerator, debug, tuning, etc.See bolt::amp::control.
 *  \param first The beginning of the sequence from which to copy.
 *  \param last The end of the sequence from which to copy.
 *  \param result The beginning of the sequence into which to copy.
 *  \param pred The predicate to test on every value of the range <tt>[first, last)</tt>.
 *  \return <tt>result + n</tt>, where \c n is equal to the number of times \p pred
 *          evaluated to \c true in the range <tt>[first, last)</tt>.
 *
 *  \tparam InputIterator is a model of <a href="http://www.sgi.com/tech/stl/InputIterator.html">Input Iterator</a>,
 *                        and \p InputIterator's \c value_type is convertible to \p Predicate's \c argument_type.
 *  \tparam OutputIterator is a model of <a href="http://www.sgi.com/tech/stl/OutputIterator.html">Output Iterator</a>.
 *  \tparam Predicate is a model of <a href="http://www.sgi.com/tech/stl/Predicate.html">Predicate</a>.
 *
 *  \pre The ranges <tt>[first, last)</tt> and <tt>[result, result + (last - first))</tt> shall not overlap.
 *
 *  The following code snippet demonstrates how to use \p copy_if to perform stream compaction
 *
 *  \code
 *  #include <bolt/amp/copy.h>
 *  ...
 *  struct is_even
 *  {
 *    bool operator()(const int x)
 *    {
 *      return (x % 2) == 0;
 *    }
 *  };
 *  ...
 *  const int N = 6;
 *  int V[N] = {-2, 0, -1, 0, 1, 2};
 *  int result[4];
 *
 *  bolt::amp::copy_if(V, V + N, result, is_even());
 *
 *  // V remains {-2, 0, -1, 0, 1, 2}
 *  // result is now {-2, 0, 0, 2}
 *  \endcode
 *
 *  \see \c remove_copy_if
 */
template<typename DerivedPolicy, typename InputIterator, typename OutputIterator, typename Predicate>
OutputIterator copy_if(const bolt::amp::control &ctl,
                         InputIterator first,
                         InputIterator last,
                         OutputIterator result,
                         Predicate pred);

template<typename InputIterator,
         typename OutputIterator,
         typename Predicate>
  OutputIterator copy_if(InputIterator first,
                         InputIterator last,
                         OutputIterator result,
                         Predicate pred);


/*! This version of \p copy_if copies elements from the range <tt>[first,last)</tt>
 *  to a range beginning at \p result, except that any element whose corresponding stencil
 *  element causes \p pred to be \c false is not copied.
 *
 *  More precisely, for every integer \c n such that <tt>0 <= n < last-first</tt>,
 *  \p copy_if performs the assignment <tt>*result = *(first+n)</tt> and \p result
 *  is advanced one position if <tt>pred(*(stencil+n))</tt>. Otherwise, no assignment
 *  occurs and \p result is not advanced.
 *
 *  The algorithm's execution is parallelized as determined by \p exec.
 *
 *  \param ctl \b Optional Control structure to control accelerator, debug, tuning, etc.See bolt::amp::control.
 *  \param first The beginning of the sequence from which to copy.
 *  \param last  The end of the sequence from which to copy.
 *  \param stencil The beginning of the stencil sequence.
 *  \param result The beginning of the sequence into which to copy.
 *  \param pred   The predicate to test on every value of the range <tt>[stencil, stencil + (last-first))</tt>.
 *  \return <tt>result + n</tt>, where \c n is equal to the number of times \p pred
 *          evaluated to \c true in the range <tt>[stencil, stencil + (last-first))</tt>.
 *
 *  \tparam DerivedPolicy The name of the derived execution policy.
 *  \tparam InputIterator1 is a model of <a href="http://www.sgi.com/tech/stl/InputIterator.html">Input Iterator</a>.
 *  \tparam InputIterator2 is a model of <a href="http://www.sgi.com/tech/stl/InputIterator.html">Input Iterator</a>,
 *                         and \p InputIterator2's \c value_type is convertible to \p Predicate's \c argument_type.
 *  \tparam OutputIterator is a model of <a href="http://www.sgi.com/tech/stl/OutputIterator">Output Iterator</a>.
 *  \tparam Predicate is a model of <a href="http://www.sgi.com/tech/stl/Predicate.html">Predicate</a>.
 *
 *  \pre The ranges <tt>[first, last)</tt> and <tt>[result, result + (last - first))</tt> shall not overlap.
 *  \pre The ranges <tt>[stencil, stencil + (last - first))</tt> and <tt>[result, result + (last - first))</tt> shall not overlap.
 *
 *
 *  \code
 *  #include <bolt/amp/copy.h>
 *  #include <bolt/amp/execution_policy.h>
 *  ...
 *  struct is_even
 *  {
 *    bool operator()(const int x)
 *    {
 *      return (x % 2) == 0;
 *    }
 *  };
 *  
 *  int N = 6;
 *  int data[N]    = { 0, 1,  2, 3, 4, 5};
 *  int stencil[N] = {-2, 0, -1, 0, 1, 2};
 *  int result[4];
 *
 *  bolt::amp::copy_if(data, data + N, stencil, result, is_even());
 *
 *  // data remains    = { 0, 1,  2, 3, 4, 5};
 *  // stencil remains = {-2, 0, -1, 0, 1, 2};
 *  // result is now     { 0, 1,  3, 5}
 *  \endcode
 *
 *  \see \c remove_copy_if
 */

template<typename DerivedPolicy, typename InputIterator1, typename InputIterator2, typename OutputIterator, typename Predicate>
OutputIterator copy_if(const bolt::amp::control &ctl,
                         InputIterator1 first,
                         InputIterator1 last,
                         InputIterator2 stencil,
                         OutputIterator result,
                         Predicate pred);

template<typename InputIterator1,
         typename InputIterator2,
         typename OutputIterator,
         typename Predicate>
  OutputIterator copy_if(InputIterator1 first,
                         InputIterator1 last,
                         InputIterator2 stencil,
                         OutputIterator result,
                         Predicate pred);

/*! \} // end stream_compaction
 */
    };
};

#include <bolt/amp/detail/copy.inl>
#endif