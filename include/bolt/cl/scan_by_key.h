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

#if !defined( BOLT_CL_SCAN_BY_KEY_H )
#define BOLT_CL_SCAN_BY_KEY_H
#pragma once

#include <bolt/cl/bolt.h>
#include <bolt/cl/functional.h>
#include <bolt/cl/device_vector.h>

/*! \file bolt/cl/scan_by_key.h
    \brief Performs, on a sequence, scan of each sub-sequence as defined by equivalent keys inclusive or exclusive.
*/
namespace bolt
{
namespace cl
{

/*! \addtogroup algorithms
 */

/*! \addtogroup PrefixSums Prefix Sums
 *   \ingroup algorithms
 */

/*! \addtogroup SegmentedPrefixSums CL-Segmented Prefix Sums
 *   \ingroup PrefixSums
 *   \{
 */


/*! \brief \p inclusive_scan_by_key performs, on a sequence,
 * an inclusive scan of each sub-sequence as defined by equivalent keys;
 * the BinaryFunction in this version is plus(), and the BinaryPredicate is equal_to().
 * inclusive_scan_by_key uses the associative operator binary_op to perform the parallel segmented prefix sum.
 *
 * \param ctl          \b Optional Control structure to control command-queue, debug, tuning, etc.See bolt::cl::control.
 * \param first1       The first element of the key sequence.
 * \param last1        The last  element of the key sequence.
 * \param first2       The first element of the value sequence.
 * \param result       The first element of the output sequence.
 * \param user_code    A user-specified string that is preppended to the generated OpenCL kernel.
 *
 * \tparam InputIterator1   is a model of Input Iterator.
 * \tparam InputIterator2   is a model of Input Iterator.
 * \tparam OutputIterator   is a model of Output Iterator.
 *
 * \return result+(last1-first1).
 *
 * \details Example
 * \code
 * #include "bolt/cl/scan_by_key.h"
 * ...
 *
 * int keys[11] = { 7, 0, 0, 3, 3, 3, -5, -5, -5, -5, 3 };
 * int vals[11] = { 1, 1, 1, 1, 1, 1,  1,  1,  1,  1, 1 };
 * int out[11];
 *
 * bolt::cl::control ctrl = control::getDefault();
 *
 * bolt::cl::inclusive_scan_by_key( ctrl, keys, keys+11, vals, out );
 * // out => { 1, 1, 2, 1, 2, 3, 1, 2, 3, 4, 1 }
 *  \endcode
 *
 * \sa inclusive_scan
 * \sa http://www.sgi.com/tech/stl/partial_sum.html
 * \sa http://www.sgi.com/tech/stl/InputIterator.html
 * \sa http://www.sgi.com/tech/stl/OutputIterator.html
 * \sa http://www.sgi.com/tech/stl/BinaryPredicate.html
 * \sa http://www.sgi.com/tech/stl/BinaryFunction.html
 */
template<
    typename InputIterator1,
    typename InputIterator2,
    typename OutputIterator>
OutputIterator
inclusive_scan_by_key(
    control &ctl,
    InputIterator1  first1,
    InputIterator1  last1,
    InputIterator2  first2,
    OutputIterator  result,
    const std::string& user_code="" );

template<
    typename InputIterator1,
    typename InputIterator2,
    typename OutputIterator>
OutputIterator
inclusive_scan_by_key(
    InputIterator1  first1,
    InputIterator1  last1,
    InputIterator2  first2,
    OutputIterator  result,
    const std::string& user_code="" );


/*! \brief \p inclusive_scan_by_key performs, on a sequence,
 * an inclusive scan of each sub-sequence as defined by equivalent keys;
 * the BinaryFunction in this version is plus().
 * inclusive_scan_by_key uses the associative operator binary_op to perform the parallel segmented prefix sum.
 *
 * \param ctl        \b Optional Control structure to control command-queue, debug, tuning, etc. See bolt::cl::control.
 * \param first1      The first element of the key sequence.
 * \param last1       The last  element of the key sequence.
 * \param first2      The first element of the value sequence.
 * \param result      The first element of the output sequence.
 * \param binary_pred Binary predicate which determines if two keys are equal.
 * \param user_code   A user-specified string that is preppended to the generated OpenCL kernel.
 *
 * \tparam InputIterator1   is a model of Input Iterator.
 * \tparam InputIterator2   is a model of Input Iterator.
 * \tparam OutputIterator   is a model of Output Iterator.
 * \tparam BinaryPredicate  is a model of Binary Predicate.
 *
 * \return result+(last1-first1).
 *
 * \code
 * #include "bolt/cl/scan_by_key.h"
 * ...
 *
 * int keys[11] = { 7, 0, 0, 3, 3, 3, -5, -5, -5, -5, 3 };
 * int vals[11] = { 1, 1, 1, 1, 1, 1,  1,  1,  1,  1, 1 };
 * int out[11];
 *
 * bolt::cl::equal_to<int> eq;
 * bolt::cl::control ctrl = control::getDefault();
 *
 * bolt::cl::inclusive_scan_by_key( ctrl, keys, keys+11, vals, out, eq );
 * // out => { 1, 1, 2, 1, 2, 3, 1, 2, 3, 4, 1 }
 *  \endcode
 *
 * \sa inclusive_scan
 * \sa http://www.sgi.com/tech/stl/partial_sum.html
 * \sa http://www.sgi.com/tech/stl/InputIterator.html
 * \sa http://www.sgi.com/tech/stl/OutputIterator.html
 * \sa http://www.sgi.com/tech/stl/BinaryPredicate.html
 * \sa http://www.sgi.com/tech/stl/BinaryFunction.html
 */
template<
    typename InputIterator1,
    typename InputIterator2,
    typename OutputIterator,
    typename BinaryPredicate>
OutputIterator
inclusive_scan_by_key(
    control &ctl,
    InputIterator1  first1,
    InputIterator1  last1,
    InputIterator2  first2,
    OutputIterator  result,
    BinaryPredicate binary_pred,
    const std::string& user_code="" );

template<
    typename InputIterator1,
    typename InputIterator2,
    typename OutputIterator,
    typename BinaryPredicate>
OutputIterator
inclusive_scan_by_key(
    InputIterator1  first1,
    InputIterator1  last1,
    InputIterator2  first2,
    OutputIterator  result,
    BinaryPredicate binary_pred,
    const std::string& user_code="" );



/*! \brief \p inclusive_scan_by_key performs, on a sequence,
 * an inclusive scan of each sub-sequence as defined by equivalent keys.
 * inclusive_scan_by_key uses the associative operator binary_op to perform the parallel segmented prefix sum.
 *
 * \param ctl           \b Optional Control structure to control command-queue, debug, tuning, etc.  See bolt::cl::control.
 * \param first1        The first element of the key sequence.
 * \param last1         The last  element of the key sequence.
 * \param first2        The first element of the value sequence.
 * \param result        The first element of the output sequence.
 * \param binary_pred   Binary predicate which determines if two keys are equal.
 * \param binary_funct  Binary function for scanning transformed elements.
 * \param user_code     A user-specified string that is preppended to the generated OpenCL kernel.
 *
 * \tparam InputIterator1   is a model of Input Iterator.
 * \tparam InputIterator2   is a model of Input Iterator.
 * \tparam OutputIterator   is a model of Output Iterator.
 * \tparam BinaryPredicate  is a model of Binary Predicate.
 * \tparam BinaryFunction   is a model of Binary Function whose return type
 *                          is convertible to \c OutputIterator's  \c value_type.
 *
 * \return result+(last1-first1).
 *
 * \code
 * #include "bolt/cl/scan_by_key.h"
 * ...
 *
 * int keys[11] = { 7, 0, 0, 3, 3, 3, -5, -5, -5, -5, 3 };
 * int vals[11] = { 2, 2, 2, 2, 2, 2,  2,  2,  2,  2, 2 };
 * int out[11];
 *
 * bolt::cl::equal_to<int> eq;
 * bolt::cl::multiplies<int> mult;
 * bolt::cl::control ctrl = control::getDefault();
 *
 * bolt::cl::inclusive_scan_by_key( ctrl, keys, keys+11, vals, out, eq, mult );
 * // out => { 2, 2, 4, 2, 4, 8, 2, 4, 8, 16, 2 }
 *  \endcode
 *
 * \sa inclusive_scan
 * \sa http://www.sgi.com/tech/stl/partial_sum.html
 * \sa http://www.sgi.com/tech/stl/InputIterator.html
 * \sa http://www.sgi.com/tech/stl/OutputIterator.html
 * \sa http://www.sgi.com/tech/stl/BinaryPredicate.html
 * \sa http://www.sgi.com/tech/stl/BinaryFunction.html
 */
template<
    typename InputIterator1,
    typename InputIterator2,
    typename OutputIterator,
    typename BinaryPredicate,
    typename BinaryFunction>
OutputIterator
inclusive_scan_by_key(
    control &ctl,
    InputIterator1  first1,
    InputIterator1  last1,
    InputIterator2  first2,
    OutputIterator  result,
    BinaryPredicate binary_pred,
    BinaryFunction  binary_funct,
    const std::string& user_code="" );


template<
    typename InputIterator1,
    typename InputIterator2,
    typename OutputIterator,
    typename BinaryPredicate,
    typename BinaryFunction>
OutputIterator
inclusive_scan_by_key(
    InputIterator1  first1,
    InputIterator1  last1,
    InputIterator2  first2,
    OutputIterator  result,
    BinaryPredicate binary_pred,
    BinaryFunction  binary_funct,
    const std::string& user_code="" );


/***********************************************************************************************************************
 * Exclusive Segmented Scan
 **********************************************************************************************************************/

/*! \brief \p exclusive_scan_by_key performs, on a sequence,
 * an exclusive scan of each sub-sequence as defined by equivalent keys;
 * the BinaryFunction in this version is plus(), the BinaryPredicate is equal_to(), and init is 0.
 * exclusive_scan_by_key uses the associative operator binary_op to perform the parallel segmented prefix sum.
 *
 * \param ctl           \b Optional Control structure to control command-queue, debug, tuning, etc.  See bolt::cl::control.
 * \param first1        The first element of the key sequence.
 * \param last1         The last  element of the key sequence.
 * \param first2        The first element of the value sequence.
 * \param result        The first element of the output sequence.
 * \param user_code     A user-specified string that is preppended to the generated OpenCL kernel.
 *
 * \tparam InputIterator1   is a model of Input Iterator.
 * \tparam InputIterator2   is a model of Input Iterator.
 * \tparam OutputIterator   is a model of Output Iterator.
 *
 * \return result+(last1-first1).
 *
 * \code
 * #include "bolt/cl/scan_by_key.h"
 * ...
 *
 * int keys[11] = { 7, 0, 0, 3, 3, 3, -5, -5, -5, -5, 3 };
 * int vals[11] = { 1, 1, 1, 1, 1, 1,  1,  1,  1,  1, 1 };
 * int out[11];
 *
 * bolt::cl::control ctrl = control::getDefault();
 *
 * bolt::cl::exclusive_scan_by_key( ctrl, keys, keys+11, vals, out );
 * // out => { 0, 0, 1, 0, 1, 2, 0, 1, 2, 3, 0 }
 *  \endcode
 *
 * \sa exclusive_scan
 * \sa http://www.sgi.com/tech/stl/partial_sum.html
 * \sa http://www.sgi.com/tech/stl/InputIterator.html
 * \sa http://www.sgi.com/tech/stl/OutputIterator.html
 * \sa http://www.sgi.com/tech/stl/BinaryPredicate.html
 * \sa http://www.sgi.com/tech/stl/BinaryFunction.html
 */
template<
    typename InputIterator1,
    typename InputIterator2,
    typename OutputIterator>
OutputIterator
exclusive_scan_by_key(
    control &ctl,
    InputIterator1  first1,
    InputIterator1  last1,
    InputIterator2  first2,
    OutputIterator  result,
    const std::string& user_code="" );

template<
    typename InputIterator1,
    typename InputIterator2,
    typename OutputIterator>
OutputIterator
exclusive_scan_by_key(
    InputIterator1  first1,
    InputIterator1  last1,
    InputIterator2  first2,
    OutputIterator  result,
    const std::string& user_code="" );



/*! \brief \p exclusive_scan_by_key performs, on a sequence,
 * an exclusive scan of each sub-sequence as defined by equivalent keys;
 * the BinaryFunction in this version is plus(), and the BinaryPredicate is equal_to().
 * exclusive_scan_by_key uses the associative operator binary_op to perform the parallel segmented prefix sum.
 *
 * \param ctl           \b Optional Control structure to control command-queue, debug, tuning, etc.  See bolt::cl::control.
 * \param first1        The first element of the key sequence.
 * \param last1         The last  element of the key sequence.
 * \param first2        The first element of the value sequence.
 * \param result        The first element of the output sequence.
 * \param init          The value used to initialize the output scan sequence.
 * \param user_code     A user-specified string that is preppended to the generated OpenCL kernel.
 *
 * \tparam InputIterator1   is a model of Input Iterator.
 * \tparam InputIterator2   is a model of Input Iterator.
 * \tparam OutputIterator   is a model of Output Iterator.
 * \tparam T                is convertible to \c OutputIterator's value_type.
 *
 * \return result+(last1-first1).
 *
 * \code
 * #include "bolt/cl/scan_by_key.h"
 * ...
 *
 * int keys[11] = { 7, 0, 0, 3, 3, 3, -5, -5, -5, -5, 3 };
 * int vals[11] = { 1, 1, 1, 1, 1, 1,  1,  1,  1,  1, 1 };
 * int out[11];
 *
 * bolt::cl::control ctrl = control::getDefault();
 *
 * bolt::cl::exclusive_scan_by_key( ctrl, keys, keys+11, vals, out, 0 );
 * // out => { 0, 0, 1, 0, 1, 2, 0, 1, 2, 3, 0 }
 *  \endcode
 *
 * \sa exclusive_scan
 * \sa http://www.sgi.com/tech/stl/partial_sum.html
 * \sa http://www.sgi.com/tech/stl/InputIterator.html
 * \sa http://www.sgi.com/tech/stl/OutputIterator.html
 * \sa http://www.sgi.com/tech/stl/BinaryPredicate.html
 * \sa http://www.sgi.com/tech/stl/BinaryFunction.html
 */
template<
    typename InputIterator1,
    typename InputIterator2,
    typename OutputIterator,
    typename T>
OutputIterator
exclusive_scan_by_key(
    control &ctl,
    InputIterator1  first1,
    InputIterator1  last1,
    InputIterator2  first2,
    OutputIterator  result,
    T               init,
    const std::string& user_code="" );

template<
    typename InputIterator1,
    typename InputIterator2,
    typename OutputIterator,
    typename T>
OutputIterator
exclusive_scan_by_key(
    InputIterator1  first1,
    InputIterator1  last1,
    InputIterator2  first2,
    OutputIterator  result,
    T               init,
    const std::string& user_code="" );



/*! \brief \p exclusive_scan_by_key performs, on a sequence,
 * an exclusive scan of each sub-sequence as defined by equivalent keys;
 * the BinaryFunction in this version is plus().
 * exclusive_scan_by_key uses the associative operator binary_op to perform the parallel segmented prefix sum.
 *
 * \param ctl           \b Optional Control structure to control command-queue, debug, tuning, etc.  See bolt::cl::control.
 * \param first1        The first element of the key sequence.
 * \param last1         The last  element of the key sequence.
 * \param first2        The first element of the value sequence.
 * \param result        The first element of the output sequence.
 * \param init          The value used to initialize the output scan sequence.
 * \param binary_pred   Binary predicate which determines if two keys are equal.
 * \param user_code     A user-specified string that is preppended to the generated OpenCL kernel.
 *
 * \tparam InputIterator1   is a model of Input Iterator.
 * \tparam InputIterator2   is a model of Input Iterator.
 * \tparam OutputIterator   is a model of Output Iterator.
 * \tparam T                is convertible to \c OutputIterator's value_type.
 * \tparam BinaryPredicate  is a model of Binary Predicate.
 *
 * \return result+(last1-first1).
 *
 * \code
 * #include "bolt/cl/scan_by_key.h"
 * ...
 *
 * int keys[11] = { 7, 0, 0, 3, 3, 3, -5, -5, -5, -5, 3 };
 * int vals[11] = { 1, 1, 1, 1, 1, 1,  1,  1,  1,  1, 1 };
 * int out[11];
 *
 * bolt::cl::equal_to<int> eq;
 * bolt::cl::control ctrl = control::getDefault();
 *
 * bolt::cl::exclusive_scan_by_key( ctrl, keys, keys+11, vals, out, 1, eq );
 * // out => { 1, 1, 2, 1, 2, 3, 1, 2, 3, 4, 1 }
 *  \endcode
 *
 * \sa exclusive_scan
 * \sa http://www.sgi.com/tech/stl/partial_sum.html
 * \sa http://www.sgi.com/tech/stl/InputIterator.html
 * \sa http://www.sgi.com/tech/stl/OutputIterator.html
 * \sa http://www.sgi.com/tech/stl/BinaryPredicate.html
 * \sa http://www.sgi.com/tech/stl/BinaryFunction.html
 */
template<
    typename InputIterator1,
    typename InputIterator2,
    typename OutputIterator,
    typename T,
    typename BinaryPredicate>
OutputIterator
exclusive_scan_by_key(
    control &ctl,
    InputIterator1  first1,
    InputIterator1  last1,
    InputIterator2  first2,
    OutputIterator  result,
    T               init,
    BinaryPredicate binary_pred,
    const std::string& user_code="" );

template<
    typename InputIterator1,
    typename InputIterator2,
    typename OutputIterator,
    typename T,
    typename BinaryPredicate>
OutputIterator
exclusive_scan_by_key(
    InputIterator1  first1,
    InputIterator1  last1,
    InputIterator2  first2,
    OutputIterator  result,
    T               init,
    BinaryPredicate binary_pred,
    const std::string& user_code="" );


/*! \brief \p exclusive_scan_by_key performs, on a sequence,
 * an exclusive scan of each sub-sequence as defined by equivalent keys.
 * exclusive_scan_by_key uses the associative operator binary_op to perform the parallel segmented prefix sum.
 *
 * \param ctl           \b Optional Control structure to control command-queue, debug, tuning, etc.  See bolt::cl::control.
 * \param first1        The first element of the key sequence.
 * \param last1         The last  element of the key sequence.
 * \param first2        The first element of the value sequence.
 * \param result        The first element of the output sequence.
 * \param init          The value used to initialize the output scan sequence.
 * \param binary_pred   Binary predicate which determines if two keys are equal.
 * \param binary_funct  Binary function for scanning transformed elements.
 * \param user_code     A user-specified string that is preppended to the generated OpenCL kernel.
 *
 * \tparam InputIterator1   is a model of Input Iterator.
 * \tparam InputIterator2   is a model of Input Iterator.
 * \tparam OutputIterator   is a model of Output Iterator.
 * \tparam T                is convertible to \c OutputIterator's value_type.
 * \tparam BinaryPredicate  is a model of Binary Predicate.
 * \tparam BinaryFunction   is a model of Binary Function whose return type
 *                          is convertible to \c OutputIterator's  \c value_type.
 *
 * \return result+(last1-first1).
 *
 * \code
 * #include "bolt/cl/scan_by_key.h"
 * ...
 *
 * int keys[11] = { 7, 0, 0, 3, 3, 3, -5, -5, -5, -5, 3 };
 * int vals[11] = { 2, 2, 2, 2, 2, 2,  2,  2,  2,  2, 2 };
 * int out[11];
 *
 * bolt::cl::equal_to<int> eq;
 * bolt::cl::multiplies<int> mult;
 * bolt::cl::control ctrl = control::getDefault();
 *
 * bolt::cl::exclusive_scan_by_key( ctrl, keys, keys+11, vals, out, 1, eq, mult );
 * // out => { 1, 1, 2, 1, 2, 4, 1, 2, 4, 8, 1 }
 *  \endcode
 *
 * \sa exclusive_scan
 * \sa http://www.sgi.com/tech/stl/partial_sum.html
 * \sa http://www.sgi.com/tech/stl/InputIterator.html
 * \sa http://www.sgi.com/tech/stl/OutputIterator.html
 * \sa http://www.sgi.com/tech/stl/BinaryPredicate.html
 * \sa http://www.sgi.com/tech/stl/BinaryFunction.html
 */
template<
    typename InputIterator1,
    typename InputIterator2,
    typename OutputIterator,
    typename T,
    typename BinaryPredicate,
    typename BinaryFunction>
OutputIterator
exclusive_scan_by_key(
    control &ctl,
    InputIterator1  first1,
    InputIterator1  last1,
    InputIterator2  first2,
    OutputIterator  result,
    T               init,
    BinaryPredicate binary_pred,
    BinaryFunction  binary_funct,
    const std::string& user_code="" );

template<
    typename InputIterator1,
    typename InputIterator2,
    typename OutputIterator,
    typename T,
    typename BinaryPredicate,
    typename BinaryFunction>
OutputIterator
exclusive_scan_by_key(
    InputIterator1  first1,
    InputIterator1  last1,
    InputIterator2  first2,
    OutputIterator  result,
    T               init,
    BinaryPredicate binary_pred,
    BinaryFunction  binary_funct,
    const std::string& user_code="" );


/*!   \}  */
}// end of bolt::cl namespace
}// end of bolt namespace

#include <bolt/cl/detail/scan_by_key.inl>

#endif
