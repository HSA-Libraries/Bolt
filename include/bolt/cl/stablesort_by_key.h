/*************************************************************************** 
*   Copyright 2012 Advanced Micro Devices, Inc.                              
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

#pragma once
#if !defined( BOLT_CL_STABLESORT_BY_KEY_H )
#define BOLT_CL_STABLESORT_BY_KEY_H

#include "bolt/cl/bolt.h"
#include "bolt/cl/functional.h"
#include <string>
#include <iostream>

namespace bolt {
namespace cl {
    /*! \addtogroup algorithms
        */

    /*! \addtogroup sorting
    *   \ingroup algorithms
    *   An Algorithm for sorting the given InputIterator. 
    *   It is capable of sorting the arithmetic data types, or the user-defined data types. For common code between 
    *   the hostand device, take a look at the ClCode and TypeName implementations.  
    *   See the Bolt Tools for Split-Source for a detailed description. 
    */ 

    /*! \addtogroup stable_sort_by_key
    *   \ingroup sorting
    *   \{
    *   \todo Prove the performance of the stable_sort routines using a benchmark program that can 
    *   show decent results across a range of values (a graph).
    */

    /*! \p This version of stable_sort returns the sorted result of all the elements in the \p RandomAccessIterator  
    * between the the first and last elements. The routine arranges the elements in ascending order. 
    * \p RandomAccessIterator's value_type must provide operator < overload. 
    *
    * The \p stable_sort operation is analogus to the std::stable_sort function.  See http://www.sgi.com/tech/stl/stable_sort.html.
    *  \tparam RandomAccessIterator Is a model of http://www.sgi.com/tech/stl/RandomAccessIterator.html, \n
    *          \p RandomAccessIterator is mutable, \n
    *          \p RandomAccessIterator's \c value_type is convertible to \p StrictWeakOrdering's \n
    *          \p RandomAccessIterator's \c value_type is \p LessThanComparable http://www.sgi.com/tech/stl/LessThanComparable.html; i.e., the value _type must provide operator < overloaded. \n

    * \param first The first position in the sequence to be sorted.
    * \param last  The last position in the sequence to be sorted.
    * \param cl_code Optional OpenCL(TM) code to be passed to the OpenCL compiler. The cl_code is inserted first in the generated code, before the cl_code traits. 
    *  This can be used for any extra cl code to be passed when compiling the OpenCl Kernel.
    * \return The sorted data that is available in place.
    *
    * The following code example shows the use of \p stable_sort to sort the elements in the ascending order.
    * \code
    * #include <bolt/cl/stablesort.h>
    * 
    * int a[8] = {2, 9, 3, 7, 5, 6, 3, 8};
    * 
    * // for arranging the elements in descending order, use bolt::cl::greater<int>()
    * bolt::cl::stable_sort(ctl, a, a+10);
    * 
    *  \endcode
    */
    template< typename RandomAccessIterator1, typename RandomAccessIterator2 > 
    void stable_sort_by_key( RandomAccessIterator1 keys_first, RandomAccessIterator1 keys_last, 
        RandomAccessIterator2 values_first, const std::string& cl_code="" );

    /*! \p stable_sort returns the sorted result of all the elements in the inputIterator between the the first and last elements using the specified binary_op.  
    * You can arrange the elements in ascending order, where the binary_op is the less<>() operator. 
    * This version of \p stable_sort does not take a bolt::cl::control structure. The Bolt library provides a default command Queue for a device. The 
    * comparison object \c functor object is defined by \p StrictWeakOrdering.

    *
    * The \p stable_sort operation is analogus to the std::sort function.  See http://www.sgi.com/tech/stl/stable_sort.html.
    *  \tparam RandomAccessIterator is a model of http://www.sgi.com/tech/stl/RandomAccessIterator.html, \n
    *          \p RandomAccessIterator is mutable, \n
    *          \p RandomAccessIterator's \c value_type is convertible to \p StrictWeakOrdering's \n
    *          \p RandomAccessIterator's \c value_type is \p LessThanComparable http://www.sgi.com/tech/stl/LessThanComparable.html i.e the value _type should provide operator < overloaded. \n
    *  \tparam StrictWeakOrdering Is a model of http://www.sgi.com/tech/stl/StrictWeakOrdering.html. \n

    * \param first The first position in the sequence to be sorted.
    * \param last  The last position in the sequence to be sorted.
    * \param comp  The comparison operation used to compare two values. 
    * \param cl_code Optional OpenCL(TM) code to be passed to the OpenCL compiler. The cl_code is inserted first in the generated code, before the cl_code traits. 
    *  This can be used for any extra cl code that is to be passed when compiling the OpenCl Kernel.
    * \return The sorted data that is available in place.
    *
    * The following code example shows the use of \p stable_sort to sort the elements in descending order.
    * \code
    * #include <bolt/cl/stablesort.h>
    * #include <bolt/cl/functional.h>
    * 
    * int a[8] = {2, 9, 3, 7, 5, 6, 3, 8};
    *
    * // for arranging the elements in descending order, use bolt::cl::greater<int>()
    * bolt::cl::stable_sort(a, a+10, bolt::cl::greater<int>());
    * 
    *  \endcode
    */
    template< typename RandomAccessIterator1, typename RandomAccessIterator2, typename StrictWeakOrdering> 
    void stable_sort_by_key( RandomAccessIterator1 keys_first, RandomAccessIterator1 keys_last, RandomAccessIterator2 values_first, 
        StrictWeakOrdering comp, const std::string& cl_code="");

    /*! \p This version of stable_sort returns the sorted result of all the elements in the \p RandomAccessIterator between the the first and last elements.  
    * The routine arranges the elements in an ascending order. \p RandomAccessIterator's value_type must provide operator < overload. 

    *
    * The \p stable_sort operation is analogus to the std::stable_sort function.  See http://www.sgi.com/tech/stl/stable_sort.html
    *  \tparam RandomAccessIterator Is a model of http://www.sgi.com/tech/stl/RandomAccessIterator.html, \n
    *          \p RandomAccessIterator is mutable, \n
    *          \p RandomAccessIterator's \c value_type is convertible to \p StrictWeakOrdering's \n
    *          \p RandomAccessIterator's \c value_type is \p LessThanComparable http://www.sgi.com/tech/stl/LessThanComparable.html; i.e., the value _type must provide operator < overloaded. \n

    * \param ctl Control structure to control command-queue, debug, tuning, etc.  See bolt::cl::control.
    * \param first The first position in the sequence to be sorted.
    * \param last  The last position in the sequence to be sorted.
    * \param cl_code Optional OpenCL(TM) code to be passed to the OpenCL compiler. The cl_code is inserted first in the generated code, before the cl_code traits. 
    *  This can be used for any extra cl code that is to be passed when compiling the OpenCl Kernel.
    * \return The sorted data that is available in place.
    *
    * The following code example shows the use of \p stable_sort to sort the elements in the ascending order, 
    * specifying a specific command-queue.
    * \code
    * #include <bolt/cl/stablesort.h>
    * 
    * int a[8] = {2, 9, 3, 7, 5, 6, 3, 8};
    *
    * cl::CommandQueue myCommandQueue = ...
    * bolt::cl::control ctl(myCommandQueue); // specify an OpenCL(TM) command queue.
    * // for arranging the elements in descending order, use bolt::cl::greater<int>()
    * bolt::cl::stable_sort(ctl, a, a+10);
    * 
    *  \endcode
    */
    template< typename RandomAccessIterator1, typename RandomAccessIterator2 >
    void stable_sort_by_key( bolt::cl::control &ctl, RandomAccessIterator1 keys_first, RandomAccessIterator1 keys_last, 
        RandomAccessIterator2 values_first, const std::string& cl_code="");

    /*! \p stable_sort returns the sorted result of all the elements in the inputIterator between the the first and last elements using the specified binary_op.  
    * You can arrange the elements in an ascending order, where the binary_op is the less<>() operator. 
    * This version of \p stable_sort takes a bolt::cl::control structure as a first argument and compares objects using \c functor object defined by \p StrictWeakOrdering.

    *
    * The \p stable_sort operation is analogus to the std::stable_sort function.  See http://www.sgi.com/tech/stl/stable_sort.html.
    *  \tparam RandomAccessIterator Is a model of http://www.sgi.com/tech/stl/RandomAccessIterator.html, \n
    *          \p RandomAccessIterator is mutable, \n
    *          \p RandomAccessIterator's \c value_type is convertible to \p StrictWeakOrdering's \n
    *          \p RandomAccessIterator's \c value_type is \p LessThanComparable http://www.sgi.com/tech/stl/LessThanComparable.html i.e the value _type should provide operator < overloaded. \n
    *  \tparam StrictWeakOrdering Is a model of http://www.sgi.com/tech/stl/StrictWeakOrdering.html. \n

    * \param ctl Control structure to control command-queue, debug, tuning, etc.  See bolt::cl::control.
    * \param first The first position in the sequence to be sorted.
    * \param last  The last position in the sequence to be sorted.
    * \param comp  The comparison operation used to compare two values. 
    * \param cl_code Optional OpenCL(TM) code to be passed to the OpenCL compiler. The cl_code is inserted first in the generated code, before the cl_code traits. 
    *  This can be used for any extra cl code that is to be passed when compiling the OpenCl Kernel.
    * \return The sorted data that is available in place.
    *
    * The following code example shows the use of \p stable_sort to sort the elements in the descending order, 
    * specifying a specific command-queue.
    * \code
    * #include <bolt/cl/stablesort.h>
    * #include <bolt/cl/functional.h>
    * 
    * int a[8] = {2, 9, 3, 7, 5, 6, 3, 8};
    *
    * cl::CommandQueue myCommandQueue = ...
    * bolt::cl::control ctl(myCommandQueue); // specify an OpenCL(TM) command queue.
    * // for arranging the elements in descending order, use bolt::cl::greater<int>()
    * bolt::cl::stable_sort(ctl, a, a+10, bolt::cl::greater<int>());
    * 
    *  \endcode
    */

    template< typename RandomAccessIterator1, typename RandomAccessIterator2, typename StrictWeakOrdering> 
    void stable_sort_by_key( bolt::cl::control &ctl, RandomAccessIterator1 keys_first, RandomAccessIterator1 keys_last, 
        RandomAccessIterator2 values_first, StrictWeakOrdering comp, const std::string& cl_code="");

    /*!   \}  */

}// end of bolt::cl namespace
}// end of bolt namespace

#include "bolt/cl/detail/stablesort_by_key.inl"
#endif