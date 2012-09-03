#pragma once

#include <bolt/cl/bolt.h>
#include <bolt/cl/device_vector.h>

#include <mutex>
#include <string>
#include <iostream>

namespace bolt {
    namespace cl {

        /*! \brief transform applies a specific function object to each element pair in the specified input ranges, and writes the result
        *   into the specified output range
        * \bug Transform only works if the input buffer is a multiple of workgroup size, currently set to 256
        */
        template<typename InputIterator, typename OutputIterator, typename BinaryFunction> 
        void transform(const bolt::cl::control &c,  InputIterator first1, InputIterator last1, InputIterator first2, OutputIterator result, 
            BinaryFunction f, const std::string user_code="");

        /*! \brief transform applies a specific function object to each element pair in the specified input ranges, and writes the result
        *   into the specified output range
        * \bug Transform only works if the input buffer is a multiple of workgroup size, currently set to 256
        */
        template<typename InputIterator, typename OutputIterator, typename BinaryFunction> 
        void transform( InputIterator first1, InputIterator last1, InputIterator first2, OutputIterator result, 
            BinaryFunction f, const std::string user_code="");
    };
};

#include <bolt/cl/detail/transform.inl>