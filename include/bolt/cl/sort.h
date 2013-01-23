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

#if !defined( SORT_H )
#define SORT_H
#pragma once

#include <bolt/cl/bolt.h>
#include <bolt/cl/functional.h>
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

        /*! \addtogroup sort
        *   \ingroup sorting
        *   \{
        *   \todo Prove the performance of the Sort routines using a benchmark program that can 
        *   show decent results across a range of values (a graph).
        */

        /*! \p This version of sort returns the sorted result of all the elements in the \p RandomAccessIterator  
        * between the the first and last elements. The routine arranges the elements in ascending order. 
        * \p RandomAccessIterator's value_type must provide operator < overload. 
        *
        * The \p sort operation is analogus to the std::sort function.  See http://www.sgi.com/tech/stl/sort.html.
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
        * The following code example shows the use of \p sort to sort the elements in the ascending order.
        * \code
        * #include <bolt/cl/sort.h>
        * 
        * int a[8] = {2, 9, 3, 7, 5, 6, 3, 8};
        * 
        * // for arranging the elements in descending order, use bolt::cl::greater<int>()
        * bolt::cl::sort(ctl, a, a+10);
        * 
        *  \endcode
        */
        template<typename RandomAccessIterator> 
        void sort(RandomAccessIterator first, 
            RandomAccessIterator last, 
            const std::string& cl_code="");

        /*! \p sort returns the sorted result of all the elements in the inputIterator between the the first and last elements using the specified binary_op.  
        * You can arrange the elements in ascending order, where the binary_op is the less<>() operator. 
        * This version of \p sort does not take a bolt::cl::control structure. The Bolt library provides a default command Queue for a device. The 
        * comparison object \c functor object is defined by \p StrictWeakOrdering.

        *
        * The \p sort operation is analogus to the std::sort function.  See http://www.sgi.com/tech/stl/sort.html.
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
        * The following code example shows the use of \p sort to sort the elements in descending order.
        * \code
        * #include <bolt/cl/sort.h>
        * #include <bolt/cl/functional.h>
        * 
        * int a[8] = {2, 9, 3, 7, 5, 6, 3, 8};
        *
        * // for arranging the elements in descending order, use bolt::cl::greater<int>()
        * bolt::cl::sort(a, a+10, bolt::cl::greater<int>());
        * 
        *  \endcode
        */
        
        template<typename RandomAccessIterator, typename StrictWeakOrdering> 
        void sort(RandomAccessIterator first, 
            RandomAccessIterator last,  
            StrictWeakOrdering comp, 
            const std::string& cl_code="");

        /*! \p This version of sort returns the sorted result of all the elements in the \p RandomAccessIterator between the the first and last elements.  
        * The routine arranges the elements in an ascending order. \p RandomAccessIterator's value_type must provide operator < overload. 

        *
        * The \p sort operation is analogus to the std::sort function.  See http://www.sgi.com/tech/stl/sort.html
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
        * The following code example shows the use of \p sort to sort the elements in the ascending order, 
        * specifying a specific command-queue.
        * \code
        * #include <bolt/cl/sort.h>
        * 
        * int a[8] = {2, 9, 3, 7, 5, 6, 3, 8};
        *
        * cl::CommandQueue myCommandQueue = ...
        * bolt::cl::control ctl(myCommandQueue); // specify an OpenCL(TM) command queue.
        * // for arranging the elements in descending order, use bolt::cl::greater<int>()
        * bolt::cl::sort(ctl, a, a+10);
        * 
        *  \endcode
        */
        template<typename RandomAccessIterator> 
        void sort(bolt::cl::control &ctl,
            RandomAccessIterator first, 
            RandomAccessIterator last, 
            const std::string& cl_code="");

        /*! \p sort returns the sorted result of all the elements in the inputIterator between the the first and last elements using the specified binary_op.  
        * You can arrange the elements in an ascending order, where the binary_op is the less<>() operator. 
        * This version of \p sort takes a bolt::cl::control structure as a first argument and compares objects using \c functor object defined by \p StrictWeakOrdering.

        *
        * The \p sort operation is analogus to the std::sort function.  See http://www.sgi.com/tech/stl/sort.html.
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
        * The following code example shows the use of \p sort to sort the elements in the descending order, 
        * specifying a specific command-queue.
        * \code
        * #include <bolt/cl/sort.h>
        * #include <bolt/cl/functional.h>
        * 
        * int a[8] = {2, 9, 3, 7, 5, 6, 3, 8};
        *
        * cl::CommandQueue myCommandQueue = ...
        * bolt::cl::control ctl(myCommandQueue); // specify an OpenCL(TM) command queue.
        * // for arranging the elements in descending order, use bolt::cl::greater<int>()
        * bolt::cl::sort(ctl, a, a+10, bolt::cl::greater<int>());
        * 
        *  \endcode
        */

        template<typename RandomAccessIterator, typename StrictWeakOrdering> 
        void sort(bolt::cl::control &ctl,
            RandomAccessIterator first, 
            RandomAccessIterator last,  
            StrictWeakOrdering comp, 
            const std::string& cl_code="");

        /*!   \}  */

    }// end of bolt::cl namespace
}// end of bolt namespace

#include <bolt/cl/detail/sort.inl>
#endif