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

#if !defined( REDUCE_H )
#define REDUCE_H
#pragma once

#include <bolt/cl/bolt.h>
#include <bolt/cl/functional.h>
#include <bolt/cl/device_vector.h>

#include <string>
#include <iostream>

namespace bolt {
    namespace cl {

        /*! \addtogroup algorithms
         */

        /*! \addtogroup reductions
        *   \ingroup algorithms
        *   Family of operations for reductions for boiling data down to a small set by summation, counting, finding min/max, and more.
        */

        /*! \addtogroup reduce
        *   \ingroup reductions
        *   \{
        *   \todo Document wg-per-compute unit flags for reduce
        */

        /*! \brief reduce returns the result of combining all the elements in the specified range using the specified binary_op.  
        * The classic example is a summation, where the binary_op is the plus operator.  By default, the initial value is "0" 
        * and the binary operator is "plus<>()".
        *
        * \p reduce requires that the binary reduction op ("binary_op") is cummutative.  The order in which \p reduce applies the binary_op
        * is not deterministic.
        *
        * The \p reduce operation is similar the std::accumulate function
        *
        * \param first The first position in the sequence to be reduced.
        * \param last  The last position in the sequence to be reduced.
        * \param cl_code Optional OpenCL(TM) code to be passed to the OpenCL compiler. The cl_code is inserted first in the generated code, before the cl_code trait.
        * \tparam InputIterator An iterator that may be dereferenced for an object, and may be incremented to get to the next element in a sequence
        * \tparam T The type of the result
        * \return The result of the reduction.
        * \sa http://www.sgi.com/tech/stl/accumulate.html
        *
        * The following code example shows the use of \p reduce to sum 10 numbers, using the default plus operator.
        * \code
        * #include <bolt/cl/reduce.h>
        *
        * int a[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        *
        * int sum = bolt::cl::reduce(a, a+10, 0);
        * // sum = 55
        *  \endcode
        *
        */
        template<typename InputIterator, typename T> 
        typename std::iterator_traits<InputIterator>::value_type
            reduce(InputIterator first, 
            InputIterator last, 
            T init,
            const std::string& cl_code="");

        /*! \brief reduce returns the result of combining all the elements in the specified range using the specified binary_op.  
        * The classic example is a summation, where the binary_op is the plus operator.  By default, 
        * the binary operator is "plus<>()".
        *
        * \p reduce requires that the binary reduction op ("binary_op") is cummutative.  The order in which \p reduce applies the binary_op
        * is not deterministic.
        *
        * The \p reduce operation is similar the std::accumulate function
        *
        * \param first The first position in the sequence to be reduced.
        * \param last  The last position in the sequence to be reduced.
        * \param init  The initial value for the accumulator.
        * \param binary_op  The binary operation used to combine two values.   By default, the binary operation is plus<>().
        * \param cl_code Optional OpenCL(TM) code to be passed to the OpenCL compiler. The cl_code is inserted first in the generated code, before the cl_code trait.
        * \tparam InputIterator An iterator that may be dereferenced for an object, and may be incremented to get to the next element in a sequence
        * \tparam T The type of the result
        * \tparam BinaryFunction A function object defining an operation that will be applied to consecutive elements in the sequence
        * \return The result of the reduction.
        * \sa http://www.sgi.com/tech/stl/accumulate.html
        *
        * The following code example shows the use of \p reduce to sum 10 numbers plus 100, using the default plus operator.
        * \code
        * #include <bolt/cl/reduce.h>
        *
        * int a[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        *
        * int sum = bolt::cl::reduce(a, a+10, 100);
        * // sum = 155
        *  \endcode
        *
        * The following code example shows the use of \p reduce to find the max of 10 numbers:
        * \code
        * #include <bolt/cl/reduce.h>
        *
        * int a[10] = {2, 9, 3, 7, 5, 6, 3, 8, 3, 4};
        *
        * int max = bolt::cl::reduce(a, a+10, -1, bolt::cl:maximum<int>());
        * // max = 9
        *  \endcode
        */
        template<typename InputIterator, typename T, typename BinaryFunction> 
        T reduce(InputIterator first, 
            InputIterator last,  
            T init,
            BinaryFunction binary_op, 
            const std::string& cl_code="")  ;

        /*! \p reduce returns the result of combining all the elements in the specified range using the specified binary_op.  
        * The classic example is a summation, where the binary_op is the plus operator.  By default, the initial value is "0" 
        * and the binary operator is "plus<>()".
        *
        * \p reduce requires that the binary reduction op ("binary_op") is cummutative.  The order in which \p reduce applies the binary_op
        * is not deterministic.
        *
        * The \p reduce operation is similar the std::accumulate function
        *
        * \param ctl Control structure to control command-queue, debug, tuning. See FIXME.
        * \param first The first position in the sequence to be reduced.
        * \param last  The last position in the sequence to be reduced.
        * \param cl_code Optional OpenCL(TM) code to be passed to the OpenCL compiler. The cl_code is inserted first in the generated code, before the cl_code trait.
        * \tparam InputIterator An iterator that may be dereferenced for an object, and may be incremented to get to the next element in a sequence
        * \tparam T The type of the result
        * \return The result of the reduction.
        * \sa http://www.sgi.com/tech/stl/accumulate.html
        *
        * The following code example shows the use of \p reduce to sum 10 numbers, using the default plus operator.
        * \code
        * #include <bolt/cl/reduce.h>
        *
        * int a[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        *
        * cl::CommandQueue myCommandQueue = ...
        *
        * bolt::cl::control ctl(myCommandQueue); // specify an OpenCL(TM) command queue to use for executing the reduce.
        * ctl.debug(bolt::cl::control::debug::SaveCompilerTemps); // save IL and ISA files for generated kernel
        *
        * int sum = bolt::cl::reduce(ctl, a, a+10, 0);
        * // sum = 55
        *  \endcode
        */
        template<typename InputIterator, typename T> 
        typename std::iterator_traits<InputIterator>::value_type
            reduce(const bolt::cl::control &ctl,
            InputIterator first, 
            InputIterator last, 
            T init,
            const std::string& cl_code="");

        /*! \brief reduce returns the result of combining all the elements in the specified range using the specified binary_op.  
        * The classic example is a summation, where the binary_op is the plus operator.  By default, 
        * the binary operator is "plus<>()".  The version takes a bolt::cl::control structure as a first argument.
        *
        * \p reduce requires that the binary reduction op ("binary_op") is cummutative.  The order in which \p reduce applies the binary_op
        * is not deterministic.
        *
        * The \p reduce operation is similar the std::accumulate function
        *
        * \param ctl Control structure to control command-queue, debug, tuning, etc.  See control.
        * \param first The first position in the sequence to be reduced.
        * \param last  The last position in the sequence to be reduced.
        * \param init  The initial value for the accumulator.
        * \param binary_op  The binary operation used to combine two values.   By default, the binary operation is plus<>().
        * \param cl_code Optional OpenCL(TM) code to be passed to the OpenCL compiler. The cl_code is inserted first in the generated code, before the cl_code trait.
        * \tparam InputIterator An iterator that may be dereferenced for an object, and may be incremented to get to the next element in a sequence
        * \tparam BinaryFunction A function object defining an operation that will be applied to consecutive elements in the sequence
        * \return The result of the reduction.
        * \sa http://www.sgi.com/tech/stl/accumulate.html
        *
        * The following code example shows the use of \p reduce to find the max of 10 numbers, 
        * specifying a specific command-queue and enabling debug messages.
        \code
        #include <bolt/cl/reduce.h>
        
        int a[10] = {2, 9, 3, 7, 5, 6, 3, 8, 3, 4};
        
        cl::CommandQueue myCommandQueue = ...
        
        bolt::cl::control ctl(myCommandQueue); // specify an OpenCL(TM) command queue to use for executing the reduce.
        ctl.debug(bolt::cl::control::debug::SaveCompilerTemps); // save IL and ISA files for generated kernel
        
        int max = bolt::cl::reduce(ctl, a, a+10, -1, bolt::cl:maximum<int>());
        // max = 9
        \endcode
        */
        template<typename InputIterator, typename T, typename BinaryFunction> 
        T reduce(const bolt::cl::control &ctl,
            InputIterator first, 
            InputIterator last,  
            T init,
            BinaryFunction binary_op=bolt::cl::plus<T>(), 
            const std::string& cl_code="")  ;

        /*!   \}  */

    };
};

#include <bolt/cl/detail/reduce.inl>
#endif