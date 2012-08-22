#pragma once

#include <bolt/cl/bolt.h>
#include <bolt/cl/functional.h>
#include <mutex>
#include <string>
#include <iostream>

namespace bolt
{
    namespace cl
    {

        /*! \addtogroup algorithms
         */

        /*! \addtogroup PrefixSums Prefix Sums
        *   \ingroup algorithms
        *   The sorting Algorithm for sorting the given InputIterator.
        */ 

        /*! \addtogroup scan
        *   \ingroup PrefixSums
        *   \{
        */

        /*! \brief inclusive_scan calculates a running sum over a range of values, inclusive of the current value.
        *   The result value at iterator position \p i is the running sum of all values less than \p i in the input range
        *
        * \param first The first iterator in the input range to be scanned
        * \param last  The last iterator in the input range to be scanned
        * \param result  The first iterator in the output range
        * \return An iterator pointing at the end of the result range
        *
        * \code
        * #include <bolt/cl/scan.h>
        *
        * int a[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9};
        *
        * // Calculate the inclusive scan of an input range, modifying the values in-place
        * bolt::cl::inclusive_scan( a, a+10, a );
        * // a => {1, 3, 6, 10, 15, 21, 28, 36, 45}
        *  \endcode
        * \sa http://www.sgi.com/tech/stl/partial_sum.html
        */
        template< typename InputIterator, typename OutputIterator, typename BinaryFunction >
        OutputIterator
            inclusive_scan( const bolt::cl::control &ctl, InputIterator first, InputIterator last, 
            OutputIterator result, BinaryFunction binary_op );

        /*! \brief inclusive_scan calculates a running sum over a range of values, inclusive of the current value.
        *   The result value at iterator position \p i is the running sum of all values less than \p i in the input range
        *
        * \param first The first iterator in the input range to be scanned
        * \param last  The last iterator in the input range to be scanned
        * \param result  The first iterator in the output range
        * \return An iterator pointing at the end of the result range
        *
        * \code
        * #include <bolt/cl/scan.h>
        *
        * int a[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9};
        *
        * // Calculate the inclusive scan of an input range, modifying the values in-place
        * bolt::cl::inclusive_scan( a, a+10, a );
        * // a => {1, 3, 6, 10, 15, 21, 28, 36, 45}
        *  \endcode
        * \sa http://www.sgi.com/tech/stl/partial_sum.html
        */
        template< typename InputIterator, typename OutputIterator, typename BinaryFunction >
        OutputIterator 
            inclusive_scan( InputIterator begin, InputIterator end, OutputIterator result,
            BinaryFunction binary_op, std::input_iterator_tag );

        /*! \brief inclusive_scan calculates a running sum over a range of values, inclusive of the current value.
        *   The result value at iterator position \p i is the running sum of all values less than \p i in the input range
        *
        * \param first The first iterator in the input range to be scanned
        * \param last  The last iterator in the input range to be scanned
        * \param result  The first iterator in the output range
        * \return An iterator pointing at the end of the result range
        *
        * \code
        * #include <bolt/cl/scan.h>
        *
        * int a[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9};
        *
        * // Calculate the inclusive scan of an input range, modifying the values in-place
        * bolt::cl::inclusive_scan( a, a+10, a );
        * // a => {1, 3, 6, 10, 15, 21, 28, 36, 45}
        *  \endcode
        * \sa http://www.sgi.com/tech/stl/partial_sum.html
        */
        template< typename InputIterator, typename OutputIterator, typename BinaryFunction > 
        OutputIterator 
            inclusive_scan( InputIterator first, InputIterator last, OutputIterator result, BinaryFunction binary_op );

        /*! \brief inclusive_scan calculates a running sum over a range of values, inclusive of the current value.
        *   The result value at iterator position \p i is the running sum of all values less than \p i in the input range
        *
        * \param first The first iterator in the input range to be scanned
        * \param last  The last iterator in the input range to be scanned
        * \param result  The first iterator in the output range
        * \return An iterator pointing at the end of the result range
        *
        * \code
        * #include <bolt/cl/scan.h>
        *
        * int a[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9};
        *
        * // Calculate the inclusive scan of an input range, modifying the values in-place
        * bolt::cl::inclusive_scan( a, a+10, a );
        * // a => {1, 3, 6, 10, 15, 21, 28, 36, 45}
        *  \endcode
        * \sa http://www.sgi.com/tech/stl/partial_sum.html
        */
        template< typename InputIterator, typename OutputIterator >
        OutputIterator 
            inclusive_scan( InputIterator first, InputIterator last, OutputIterator result );

        /*!   \}  */

    }// end of bolt::cl namespace
}// end of bolt namespace

#include <bolt/cl/detail/scan.inl>

