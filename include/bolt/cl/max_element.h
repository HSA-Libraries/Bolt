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

        /*! \addtogroup max_elements
        *   \ingroup algorithms
        *    The max_element finds the location of the first smallest in the range [first, last]
        */

        /*! \addtogroup max_element
        *   \ingroup max_elements
        *   \{
        *   \todo Document wg-per-compute unit flags for max_element
        */

        /*! \brief The max_element returns the location of the first maximum element in the specified range.
        *
        *
        * \param first A forward iterator addressing the position of the first element in the range to be searched for the maximum element
        * \param last  A forward iterator addressing the position one past the final element in the range to be searched for the maximum element
        * \param cl_code Optional OpenCL(TM) code to be passed to the OpenCL compiler. The cl_code is inserted first in the generated code, before the cl_code trait.
        * \tparam ForwardIterator An iterator that can be dereferenced for an object, and can be incremented to get to the next element in a sequence.
        * \return The result of the max_element.
                *
        * The following code example shows the use of \p max_element of 10 numbers, using the default BinaryPredicate.
        * \code
        * #include <bolt/cl/max_element.h>
        *
        * int a[10] = {4, 8, 6, 1, 5, 3, 10, 2, 9, 7};
        *
        * int max_pos = bolt::cl::max_element(a, a+10);
        * // max_pos = 6
        *  \endcode
        *
        */
        template<typename ForwardIterator> 
        ForwardIterator max_element(ForwardIterator first, 
            ForwardIterator last, 
            const std::string& cl_code="");

        /*! \brief The max_element returns the location of the first maximum element in the specified range using the specified binary_op.    
        * \param first A forward iterator addressing the position of the first element in the range to be searched for the maximum element
        * \param last  A forward iterator addressing the position one past the final element in the range to be searched for the maximum element
        * \param binary_op  The binary operation used to combine two values.   By default, the binary operation is less<>().
        * \param cl_code Optional OpenCL(TM) code to be passed to the OpenCL compiler. The cl_code is inserted first in the generated code, before the cl_code trait.
        * \tparam InputIterator An iterator that can be dereferenced for an object, and can be incremented to get to the next element in a sequence.
        * \return The result of the max_element.
        *
        *
        * The following code example shows the use of \p max_element  10 numbers plus 100, using the default less operator.
        * \code
        * #include <bolt/cl/max_element.h>
        *
        * int a[10] = {4, 8, 6, 1, 5, 3, 10, 2, 9, 7};
        *
        * int max_pos = bolt::cl::max_element(a, a+10, bolt::cl::greater<T>());
        * // max_pos = 6
        *  \endcode
        *
        * The following code example shows the use of \p max_element to find the min of 10 numbers:
        * \code
        * #include <bolt/cl/max_element.h>
        *
        * int a[10] = {4, 8, 6, 1, 5, 3, 10, 2, 9, 7};
        *
        * int min_pos = bolt::cl::max_element(a, a+10, bolt::cl::less<T>());
        * // min_pos = 3
        *  \endcode
        */
        template<typename ForwardIterator, typename BinaryPredicate> 
        ForwardIterator max_element(ForwardIterator first, 
            ForwardIterator last,  
            BinaryPredicate binary_op, 
            const std::string& cl_code="")  ;


        /*! \brief The max_element returns the location of the first maximum element in the specified range.
        *
        *
        * \param ctl Control structure to control command-queue, debug, tuning.
        * \param first A forward iterator addressing the position of the first element in the range to be searched for the maximum element
        * \param last  A forward iterator addressing the position one past the final element in the range to be searched for the maximum element
        * \param cl_code Optional OpenCL(TM) code to be passed to the OpenCL compiler. The cl_code is inserted first in the generated code, before the cl_code trait.
        * \tparam ForwardIterator An iterator that can be dereferenced for an object, and can be incremented to get to the next element in a sequence.
        * \return The result of the max_element.
                *
        * The following code example shows the use of \p max_element of 10 numbers, using the default BinaryPredicate.
        * \code
        * #include <bolt/cl/max_element.h>
        *
        * int a[10] = {4, 8, 6, 1, 5, 3, 10, 2, 9, 7};
        *
        * cl::CommandQueue myCommandQueue = ...
        *
        * bolt::cl::control ctl(myCommandQueue); // specify an OpenCL(TM) command queue to use for executing the max_element.
        * ctl.debug(bolt::cl::control::debug::SaveCompilerTemps); // save IL and ISA files for generated kernel
        *
        * int max_pos = bolt::cl::max_element(a, a+10);
        * // max_pos = 6
        *  \endcode
        *
        */
          
        template<typename ForwardIterator> 
        ForwardIterator  max_element(bolt::cl::control &ctl,
            ForwardIterator first, 
            ForwardIterator last, 
            const std::string& cl_code="");

            
        /*! \brief The max_element returns the location of the first maximum element in the specified range using the specified binary_op.    
        *
        * \param ctl Control structure to control command-queue, debug, tuning, etc.  See control.
        * \param first A forward iterator addressing the position of the first element in the range to be searched for the maximum element
        * \param last  A forward iterator addressing the position one past the final element in the range to be searched for the maximum element
        * \param binary_op  The binary operation used to combine two values.   By default, the binary operation is less<>().
        * \param cl_code Optional OpenCL(TM) code to be passed to the OpenCL compiler. The cl_code is inserted first in the generated code, before the cl_code trait.
        * \tparam InputIterator An iterator that can be dereferenced for an object, and can be incremented to get to the next element in a sequence.
        * \return The result of the max_element.
        *
        *
        * The following code example shows the use of \p max_element  10 numbers plus 100, using the default less operator.
        * \code
        * #include <bolt/cl/max_element.h>
        *
        * int a[10] = {4, 8, 6, 1, 5, 3, 10, 2, 9, 7};
        *
        * cl::CommandQueue myCommandQueue = ...
        *
        * bolt::cl::control ctl(myCommandQueue); // specify an OpenCL(TM) command queue to use for executing the max_element.
        * ctl.debug(bolt::cl::control::debug::SaveCompilerTemps); // save IL and ISA files for generated kernel.
        *
        * int max_pos = bolt::cl::max_element(a, a+10, bolt::cl::greater<T>());
        * // max_pos = 6
        *  \endcode
        *
        * The following code example shows the use of \p max_element to find the min of 10 numbers:
        * \code
        * #include <bolt/cl/max_element.h>
        *
        * int a[10] = {4, 8, 6, 1, 5, 3, 10, 2, 9, 7};
        *
        * cl::CommandQueue myCommandQueue = ...
        *
        * bolt::cl::control ctl(myCommandQueue); // specify an OpenCL(TM) command queue to use for executing the max_element.
        * ctl.debug(bolt::cl::control::debug::SaveCompilerTemps); // save IL and ISA files for generated kernel.
        *
        * int min_pos = bolt::cl::max_element(a, a+10, bolt::cl::less<T>());
        *
        * // min_pos = 3
        *  \endcode
        */

        template<typename ForwardIterator, typename BinaryPredicate> 
        ForwardIterator max_element(bolt::cl::control &ctl,
            ForwardIterator first, 
            ForwardIterator last,  
            BinaryPredicate binary_op, 
            const std::string& cl_code="")  ;

        /*!   \}  */

    };
};

#include <bolt/cl/detail/min_element.inl>
#endif