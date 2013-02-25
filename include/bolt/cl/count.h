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
#if !defined( BOLT_CL_COUNT_H )
#define BOLT_CL_COUNT_H

#include "bolt/cl/bolt.h"
#include "bolt/cl/functional.h"
#include "bolt/cl/transform_reduce.h"
#include "bolt/cl/iterator/iterator_traits.h"

namespace bolt {
    namespace cl {

        /*! \addtogroup algorithms
         */

        /*! \addtogroup reductions
        *   \ingroup algorithms

        /*! \addtogroup counting
        *  \ingroup reductions
        *  \{
        *  \bug Interfaces which accept a control structure need to be added
        *  \todo Better documentation and example code need to be added
        */

        namespace detail
        {
            static std::string CountIfEqual_OclCode = 
            BOLT_HOST_DEVICE_DEFINITION(
            template <typename T> 
            struct CountIfEqual {
                CountIfEqual(const T &targetValue)  : _targetValue(targetValue)
                { };
                CountIfEqual(){}
                bool operator() (const T &x) {
                    return x == _targetValue;
                };

            private:
                T _targetValue;
            };
            );
        }

        /*!
         * \p count counts the number of elements in the specified range which compare equal to the specified \p value.
         *
         *\code
        int a[14] = {0, 10, 42, 55, 13, 13, 42, 19, 42, 11, 42, 99, 13, 77};

        size_t countOf42 = bolt::cl::count (A, A+14, 42);
        // countOf42 contains 4.
         * \endcode
             *
         */
        template<typename InputIterator, typename EqualityComparable> 
        typename bolt::cl::iterator_traits<InputIterator>::difference_type
            count(InputIterator first, 
            InputIterator last, 
            const EqualityComparable &value,
            const std::string cl_code="")
        {
            typedef typename std::iterator_traits<InputIterator>::value_type T;
            return count_if(first, last, detail::CountIfEqual<T>(value), detail::CountIfEqual_OclCode + cl_code);
        };
        
        template<typename InputIterator, typename EqualityComparable> 
        typename bolt::cl::iterator_traits<InputIterator>::difference_type
            count(control& ctl, InputIterator first, 
            InputIterator last, 
            const EqualityComparable &value,
            const std::string cl_code="")
        {
            typedef typename std::iterator_traits<InputIterator>::value_type T;
            return count_if(ctl, first, last, detail::CountIfEqual<T>(value), detail::CountIfEqual_OclCode + cl_code);
        };

        

        /*!
        * \p count_if counts the number of elements in the specified range for which the specified \p predicate is \p true.  
        * 
        * \param first The first position in the sequence to be counted.
        * \param last The last position in the sequence to be counted.
        * \param predicate The predicate. The count is incremented for each element which returns true when passed to the predicate function.
        *
        * \returns: The number of elements for which \p predicate is true.
        */
        template<typename InputIterator, typename Predicate> 
        typename bolt::cl::iterator_traits<InputIterator>::difference_type
            count_if(InputIterator first, 
            InputIterator last, 
            Predicate predicate,
            const std::string cl_code="")
        {
            typedef typename bolt::cl::iterator_traits<InputIterator>::value_type CountType;
            /*Change result type from std::iterator_traits<InputIterator>::difference_type to size_t as
               with device vectorits comming int64 but with stdvector its __int64 which is not in openCL*/ 
            typedef int ResultType;
            ResultType result = static_cast< ResultType >( transform_reduce( first, last, 
                predicate,  static_cast< CountType >( 0 ), bolt::cl::plus< CountType >( ), cl_code ) );
            return result;
        };
        
        template<typename InputIterator, typename Predicate> 
        typename bolt::cl::iterator_traits<InputIterator>::difference_type
            count_if(control& ctl, InputIterator first, 
            InputIterator last, 
            Predicate predicate,
            const std::string cl_code="")
        {
            typedef typename bolt::cl::iterator_traits<InputIterator>::value_type CountType;
            typedef size_t ResultType;
            ResultType result = static_cast< ResultType >(  transform_reduce(ctl, first, last, 
                predicate, static_cast< CountType >( 0 ), bolt::cl::plus< CountType >( ), cl_code ) );
            return result;
        };
        
    };
};

BOLT_CREATE_TYPENAME( bolt::cl::detail::CountIfEqual< int > );
BOLT_CREATE_CLCODE( bolt::cl::detail::CountIfEqual< int >, bolt::cl::detail::CountIfEqual_OclCode );

BOLT_TEMPLATE_REGISTER_NEW_TYPE( bolt::cl::detail::CountIfEqual, int, unsigned int );
BOLT_TEMPLATE_REGISTER_NEW_TYPE( bolt::cl::detail::CountIfEqual, int, float );
BOLT_TEMPLATE_REGISTER_NEW_TYPE( bolt::cl::detail::CountIfEqual, int, double );

#endif