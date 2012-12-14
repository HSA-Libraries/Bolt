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

#if !defined( INNERPRODUCT_H )
#define INNERPRODUCT_H
#pragma once

#include <bolt/cl/bolt.h>
#include <bolt/cl/device_vector.h>

#include <string>
#include <iostream>

namespace bolt {
    namespace cl {

        /*! \addtogroup algorithms
         */

        /*! \addtogroup reductions
        *   \ingroup algorithms
        */

        /*! \addtogroup inner_product
        *   \ingroup reductions
        *   \{
        */

        /*! \brief Inner Product returns the inner product of two iterators using user specified binary functors f1 and f2.  
        *
        * \p This is similar to calculating transform and then reducing the result. The functor f1 should be commutative.
        *
        * The \p inner_product operation is similar the std::inner_product function.
        *
        * \param first1 The beginning of input sequence.
        * \param last1  The end of input sequence.
		* \param first2 The beginning of the second input sequence.
		* \param init   The initial value for the accumulator.
        * \param cl_code Optional OpenCL(TM) code to be passed to the OpenCL compiler. The cl_code is inserted first in the generated code, before the cl_code trait.
        * \tparam InputIterator An iterator that can be dereferenced for an object, and can be incremented to get to the next element in a sequence.
        * \tparam OutputType The type of the result.
        * \return The result of the reduction.
        * \sa http://www.sgi.com/tech/stl/inner_product.html
        *
        * The following code example shows the use of \p inner_product to perform dot product on two vectors of size 10 , using the default multiplies and plus operator.
        * \code
        * #include <bolt/cl/inner_product.h>
        *
        * int a[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
		* int b[10] = {1, 1, 2, 3, 5, 8, 13, 21, 34, 55};
        *
        * int ip = bolt::cl::inner_product(a, a+10, b,0);
        * // sum = 1209
        *  \endcode
        *
        */
		template<typename InputIterator, typename OutputType> 
        OutputType inner_product( bolt::cl::control &ctl,  InputIterator first1, InputIterator last1, InputIterator first2, OutputType init, 
            const std::string& user_code="");
        /*! \p Inner Product returns the inner product of two iterators using user specified binary functors f1 and f2.  
        * \p This is similar to calculating transform and then reducing the result. The functor f1 should be commutative.
        *
        * The \p inner_product operation is similar the std::inner_product function.
        *
        * \param ctl Control structure to control command-queue, debug, tuning. See FIXME.
        * \param first1 The first position in the input sequence.
        * \param last1  The last position in the input sequence.
		* \param last2  The beginning of second input sequence.
		* \param f1		Binary functor for transformation.
		* \param f2     Binary functor for reduction.
        * \param cl_code Optional OpenCL(TM) code to be passed to the OpenCL compiler. The cl_code is inserted first in the generated code, before the cl_code trait.
        * \tparam InputIterator An iterator that can be dereferenced for an object, and can be incremented to get to the next element in a sequence.
        * \tparam OutputType The type of the result.
        * \return The result of the reduction.
        * \sa http://www.sgi.com/tech/stl/inner_product.html
        *
        * The following code example shows the use of \p inner_product on two vectors of size 10, using the user defined functors.
        * \code
        * #include <bolt/cl/inner_product.h>
        *
        * int a[10] = {-5,  0,  2,  3,  2,  4, -2,  1,  2,  3};
		* int b[10] = {-5,  0,  2,  3,  2,  4, -2,  1,  2,  3};
        *
        * cl::CommandQueue myCommandQueue = ...
        *
        * bolt::cl::control ctl(myCommandQueue); // specify an OpenCL(TM) command queue to use for executing the reduce.
        * ctl.debug(bolt::cl::control::debug::SaveCompilerTemps); // save IL and ISA files for generated kernel
        *
        * int ip = bolt::cl::inner_product(ctl, a, a+10, b, bolt::cl::plus<int>(),bolt::cl::multiplies<int>(),0);
        * // sum = 76
        *  \endcode
        */


        template<typename InputIterator, typename OutputType, typename BinaryFunction1, typename BinaryFunction2> 
        OutputType inner_product( InputIterator first1, InputIterator last1, InputIterator first2, OutputType init, 
            BinaryFunction1 f1, BinaryFunction2 f2, const std::string& user_code="");

        /*! \p Inner Product returns the inner product of two iterators.
        * \p This is similar to calculating transform and then reducing the result.
        *
        * The \p inner_product operation is similar the std::inner_product function.
        *        
        * \param first1 The first position in the input sequence.
        * \param last1  The last position in the input sequence.
		* \param last2  The beginning of second input sequence.
        * \param cl_code Optional OpenCL(TM) code to be passed to the OpenCL compiler. The cl_code is inserted first in the generated code, before the cl_code trait.
        * \tparam InputIterator An iterator that can be dereferenced for an object, and can be incremented to get to the next element in a sequence.
        * \tparam OutputType The type of the result.
        * \return The result of the reduction.
        * \sa http://www.sgi.com/tech/stl/inner_product.html
        *
        * The following code example shows the use of \p inner_product on two vectors of size 10, using the user defined functors.
        * \code
        * #include <bolt/cl/inner_product.h>
        *
        * int a[10] = {-5,  0,  2,  3,  2,  4, -2,  1,  2,  3};
		* int b[10] = {-5,  0,  2,  3,  2,  4, -2,  1,  2,  3};
        *
        * int ip = bolt::cl::inner_product(ctl, a, a+10, b, 1);
        * // sum = 77
        *  \endcode
        */


		template<typename InputIterator, typename OutputType> 
        OutputType inner_product( InputIterator first1, InputIterator last1, InputIterator first2, OutputType init, 
            const std::string& user_code="");

        /*! \p Inner Product returns the inner product of two iterators.
        * \p This is similar to calculating transform and then reducing the result.
        *
        * The \p inner_product operation is similar the std::inner_product function.
        *
        * \param ctl Control structure to control command-queue, debug, tuning. See FIXME.
        * \param first1 The first position in the input sequence.
        * \param last1  The last position in the input sequence.
		* \param last2  The beginning of second input sequence.
        * \param cl_code Optional OpenCL(TM) code to be passed to the OpenCL compiler. The cl_code is inserted first in the generated code, before the cl_code trait.
        * \tparam InputIterator An iterator that can be dereferenced for an object, and can be incremented to get to the next element in a sequence.
        * \tparam OutputType The type of the result.
        * \return The result of the reduction.
        * \sa http://www.sgi.com/tech/stl/inner_product.html
        *
        * The following code example shows the use of \p inner_product on two vectors of size 10, using the user defined functors.
        * \code
        * #include <bolt/cl/inner_product.h>
        *
        * int a[10] = {-5,  0,  2,  3,  2,  4, -2,  1,  2,  3};
		* int b[10] = {-5,  0,  2,  3,  2,  4, -2,  1,  2,  3};
        *
        * cl::CommandQueue myCommandQueue = ...
        *
        * bolt::cl::control ctl(myCommandQueue); // specify an OpenCL(TM) command queue to use for executing the reduce.
        * ctl.debug(bolt::cl::control::debug::SaveCompilerTemps); // save IL and ISA files for generated kernel
        *
        * int ip = bolt::cl::inner_product(ctl, a, a+10, b,0);
        * // sum = 76
        *  \endcode
        */


        template<typename InputIterator, typename OutputType, typename BinaryFunction1, typename BinaryFunction2> 
        OutputType inner_product( bolt::cl::control &ctl,  InputIterator first1, InputIterator last1, InputIterator first2, OutputType init, 
            BinaryFunction1 f1, BinaryFunction2 f2, const std::string& user_code="");

       
    };
};

#include <bolt/cl/detail/inner_product.inl>
#endif