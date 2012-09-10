#pragma once

#include <bolt/cl/bolt.h>
#include <bolt/cl/device_vector.h>

#include <string>
#include <iostream>

namespace bolt {
    namespace cl {


        /*! \brief transform_reduce
         *  \todo Document transform_reduce
         */
        template<typename T, typename InputIterator, typename UnaryFunction, typename BinaryFunction> 
        T transform_reduce(const control &c, InputIterator first1, InputIterator last1,  
            UnaryFunction transform_op, 
            T init,  BinaryFunction reduce_op, const std::string user_code="" );

        /*! \brief transform_reduce
         *  \todo Document transform_reduce
         */
        template<typename T, typename InputIterator, typename UnaryFunction, typename BinaryFunction> 
        T transform_reduce(InputIterator first1, InputIterator last1,  
            UnaryFunction transform_op, 
            T init,  BinaryFunction reduce_op, const std::string user_code="" );
    };
};

#include <bolt/cl/detail/transform_reduce.inl>

// FIXME -review use of string vs const string.  Should TypeName<> return a std::string?
// FIXME - add line numbers to pretty-print kernel log file.
// FIXME - experiment with passing functors as objects rather than as parameters.  (Args can't return state to host, but OK?)
