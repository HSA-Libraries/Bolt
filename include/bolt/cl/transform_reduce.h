#pragma once

#include <bolt/cl/bolt.h>
#include <mutex>
#include <string>
#include <iostream>

namespace bolt {
	namespace cl {

		// I think we are not providing the cl::Buffer interfaces
		/*template<typename T, typename UnaryFunction, typename BinaryFunction> 
		T transform_reduce(const control c, ::cl::Buffer A, UnaryFunction transform_op,
			T init, BinaryFunction reduce_op, const std::string user_code="");*/

		template<typename T, typename InputIterator, typename UnaryFunction, typename BinaryFunction> 
		T transform_reduce(const control &c, InputIterator first1, InputIterator last1,  
			UnaryFunction transform_op, 
			T init,  BinaryFunction reduce_op, const std::string user_code="" );

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
