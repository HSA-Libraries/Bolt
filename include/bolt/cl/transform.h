#pragma once

#include <bolt/cl/bolt.h>
#include <mutex>
#include <string>
#include <iostream>

namespace bolt {
	namespace cl {


		// Function definition that accepts iterators and converts to buffers.
		template<typename InputIterator, typename OutputIterator, typename BinaryFunction> 
		void transform(const bolt::cl::control &c,  InputIterator first1, InputIterator last1, InputIterator first2, OutputIterator result, 
			BinaryFunction f, const std::string user_code="");

		// default control, two-input transform, std:: iterator
		template<typename InputIterator, typename OutputIterator, typename BinaryFunction> 
		void transform( InputIterator first1, InputIterator last1, InputIterator first2, OutputIterator result, 
			BinaryFunction f, const std::string user_code="");

		// default control, two-input transform, buffer inputs
		template<typename T, typename BinaryFunction> 
		void transform(::cl::Buffer A, ::cl::Buffer B, ::cl::Buffer Z, 
			BinaryFunction f, std::string user_code="");


	};
};


#include <bolt/cl/detail/transform.inl>