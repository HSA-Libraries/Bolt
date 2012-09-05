#pragma once

#include <bolt/cl/bolt.h>
#include <bolt/cl/functional.h>
#include <bolt/cl/transform_reduce.h>
#include <bolt/cl/iterator_traits.h>


namespace bolt {
	namespace cl {

        /*! \addtogroup algorithms
         */

		/*! \addtogroup reductions
        *   \ingroup algorithms

        /*! \addtogroup counting
        *  \ingroup reductions
		*  \{
        */


        std::string CountIfEqual_OclCode = 
		BOLT_CODE_STRING(
		template <typename T> 
		struct CountIfEqual {
			CountIfEqual(const T &targetValue)  : _targetValue(targetValue)
			{ };

			bool operator() (const T &x) {
				return x == _targetValue;
			};

		private:
			T _targetValue;
		};
        );

        BOLT_CREATE_STD_TYPENAMES(CountIfEqual); 



		/*!
         * \p count counts the number of elements in the specified range which compare equal to the specified \p value.
         *
         *\code
        int a[14] = {0, 10, 42, 55, 13, 13, 42, 19, 42, 11, 42, 99, 13, 77};

        size_t countOf42 = bolt::cl::count (A, A+14, 42);
        // countOf42 contains 4.
         * \endcode
             *
		* \todo More documentation needed
         */
		template<typename InputIterator, typename EqualityComparable> 
		typename std::iterator_traits<InputIterator>::difference_type
			count(InputIterator first, 
			InputIterator last, 
			const EqualityComparable &value,
			const std::string cl_code="")
		{
			typedef typename std::iterator_traits<InputIterator>::value_type T;

			return count_if(first, last, CountIfEqual<T>(value), bolt::cl::CountIfEqual_OclCode + cl_code);
		};


		/*!
		* \p count_if counts the number of elements in the specified range for which the specified \p predicate is \p true.  
        * 
		* \param first The first position in the sequence to be counted.
		* \param last The last position in the sequence to be counted.
		* \param predicate The predicate. The count is incremented for each element which returns true when passed to the predicate function.
        *
        * \returns: The number of elements for which \p predicate is true.
		* \todo more documentation needed and a code sample needed
		* \bug Failure for the test written for it
		*/
		template<typename InputIterator, typename Predicate> 
		typename std::iterator_traits<InputIterator>::difference_type
			count_if(InputIterator first, 
			InputIterator last, 
			Predicate predicate,
			const std::string cl_code="")
		{
			typedef typename bolt::cl::iterator_traits<InputIterator>::difference_type CountType;
            //typedef int CountType; // FIXME, need to create a bolt class that returns an ocl-supported typename.
			return transform_reduce(bolt::cl::control::getDefault(), first, last, 
				predicate,  // FIXME - need CountIfTransform here?
				CountType(0), bolt::cl::plus<CountType>(), cl_code);
		};


        // FIXME - add interfaces for ::cl::Buffer
        // FIXME - add interfaces which accept a control structure.
	};
};
