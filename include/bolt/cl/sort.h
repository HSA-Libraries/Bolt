#pragma once

#include <bolt/cl/bolt.h>
#include <bolt/cl/functional.h>
#include <mutex>
#include <string>
#include <iostream>

namespace bolt {
	namespace cl {
        

        /*! \addtogroup algorithms
         */

		/*! \addtogroup sorting
        *   \ingroup algorithms
        *   The sorting Algorithm for sorting the given InputIterator.
        */ 

		/*! \addtogroup sort
        *   \ingroup sorting
        *   \{
        */



		/*! \p sort returns the sorted result of all the elements in the inputIterator between the the first and last elements using the specified binary_op.  
		* You can arrange the elements in an ascending order, where the binary_op is the less<>() operator.  By default, 
		* the binary operator is "less<>()".  The version takes a bolt::cl::control structure as a first argument.

		*
		* The \p sort operation is similar the std::sort function.  See http://www.sgi.com/tech/stl/sort.html
		*
		* \param ctl Control structure to control command-queue, debug, tuning, etc.  See control.
		* \param first The first position in the sequence to be sorted.
		* \param last  The last position in the sequence to be sorted.
		* \param comp  The comparison operation used to compare two values.   By default, the compare operation is less<>(). i.e the elements are stored in ascending order. 
		* \param cl_code Optional OpenCL(TM) code to be passed to the OpenCL compiler. The cl_code is inserted first in the generated code, before the cl_code trait.
		* \return The sorted data which is available in place.
		*
		* The following code example shows the use of \p sort to sort the elements in the descending order, 
		* specifying a specific command-queue and enabling debug messages.
		* \code
		* #include <bolt/cl/sort.h>
		*
		* int a[10] = {2, 9, 3, 7, 5, 6, 3, 8, 3, 4};
		*
		* cl::CommandQueue myCommandQueue = ...
		*
		* bolt::cl::control ctl(myCommandQueue); // specify an OpenCL(TM) command queue to use for executing the sort routine.
		* ctl.debug(bolt::cl::control::debug::SaveCompilerTemps); // save IL and ISA files for generated kernel
		* // for arranging th eelements in the descending order, you can use bolt::cl::greater<int>()
		* bolt::cl::sort(ctl, a, a+10, bolt::cl::greater<int>());
		* // a => {9, 8, 7, 6, 5, 4, 3, 3, 3, 2}
		*  \endcode
		*/

		template<typename RandomAccessIterator, typename Compare> 
		void sort(const bolt::cl::control &ctl,
			RandomAccessIterator first, 
			RandomAccessIterator last,  
            Compare comp=bolt::cl::less<typename std::iterator_traits<RandomAccessIterator>::value_type>(), 
			const std::string cl_code="");


		/*! \p sort returns the sorted result of all the elements in the inputIterator between the the first and last elements using the specified binary_op.  
		* You can arrange the elements in an ascending order, where the binary_op is the less<>() operator.  By default, 
		* the binary operator is "less<>()".  The version takes a bolt::cl::control structure as a first argument.

		*
		* The \p sort operation is similar the std::sort function.  See http://www.sgi.com/tech/stl/sort.html
		*
		* \param ctl Control structure to control command-queue, debug, tuning, etc.  See control.
		* \param first The first position in the sequence to be sorted.
		* \param last  The last position in the sequence to be sorted.
		* \param cl_code Optional OpenCL(TM) code to be passed to the OpenCL compiler. The cl_code is inserted first in the generated code, before the cl_code trait.
		* \return The sorted data which is available in place and is in ascending order.
		*
		* Here by default the function does an ascending order sort. 
        * The following code example shows the use of \p sort to sort the elements in the ascending order, 
		* specifying a specific command-queue and enabling debug messages.
		* \code
		* #include <bolt/cl/sort.h>
		*
		* int a[10] = {2, 9, 3, 7, 5, 6, 3, 8, 3, 4};
		*
		* cl::CommandQueue myCommandQueue = ...
		*
		* bolt::cl::control ctl(myCommandQueue); // specify an OpenCL(TM) command queue to use for executing the sort routine.
		* ctl.debug(bolt::cl::control::debug::SaveCompilerTemps); // save IL and ISA files for generated kernel
		* // for arranging th eelements in the descending order, you can use bolt::cl::greater<int>()
		* bolt::cl::sort(ctl, a, a+10);
		* // a => {2, 3, 3, 3, 4, 5, 6, 7, 8, 9}
		*  \endcode
		*/
        template<typename RandomAccessIterator> 
		void sort(const bolt::cl::control &ctl,
			RandomAccessIterator first, 
			RandomAccessIterator last, 
			const std::string cl_code="");


		/*! \p sort returns the sorted result of all the elements in the inputIterator between the the first and last elements using the specified binary_op.  
		* You can arrange the elements in an ascending order, where the binary_op is the less<>() operator.  By default, 
		* the binary operator is "less<>()".  The version takes a bolt::cl::control structure as a first argument.

		*
		* The \p sort operation is similar the std::sort function.  See http://www.sgi.com/tech/stl/sort.html
		*
		* \param first The first position in the sequence to be sorted.
		* \param last  The last position in the sequence to be sorted.
		* \param cl_code Optional OpenCL(TM) code to be passed to the OpenCL compiler. The cl_code is inserted first in the generated code, before the cl_code trait.
		* \return The sorted data which is available in place and is in ascending order.
		*
		* Here also the elements are arranged in ascending order. The routine takes care of allocating the cl::CommandQueue an other data structures. 
        * The following code example shows the use of \p sort to sort the elements in the ascending order, 
		* specifying a specific command-queue and enabling debug messages.
		* \code
		* #include <bolt/cl/sort.h>
		*
		* int a[10] = {2, 9, 3, 7, 5, 6, 3, 8, 3, 4};
		*
		* // for arranging the elements in the descending order, you can use bolt::cl::greater<int>()
		* bolt::cl::sort(a, a+10);
		* // a => {2, 3, 3, 3, 4, 5, 6, 7, 8, 9}
		*  \endcode
		*/
        
        template<typename RandomAccessIterator, typename Compare> 
		void sort(RandomAccessIterator first, 
			RandomAccessIterator last,  
            Compare comp=bolt::cl::less<typename std::iterator_traits<RandomAccessIterator>::value_type>(), 
			const std::string cl_code="");


		/*! \p sort returns the sorted result of all the elements in the inputIterator between the the first and last elements using the specified binary_op.  
		* You can arrange the elements in an ascending order, where the binary_op is the less operator.  By default, 
		* the binary operator is "less<>()".  The version takes a bolt::cl::control structure as a first argument.

		*
		* The \p sort operation is similar the std::sort function.  See http://www.sgi.com/tech/stl/sort.html
		*
		* \param first The first position in the sequence to be sorted.
		* \param last  The last position in the sequence to be sorted.
		* \param cl_code Optional OpenCL(TM) code to be passed to the OpenCL compiler. The cl_code is inserted first in the generated code, before the cl_code trait.
		* \return The sorted data which is available in place and is in ascending order.
		*
		* Here also the elements are arranged in ascending order. The routine takes care of allocating the cl::CommandQueue an other data structures. 
        * The following code example shows the use of \p sort to sort the elements in the ascending order, 
		* specifying a specific command-queue and enabling debug messages.
		* \code
		* #include <bolt/cl/sort.h>
		*
		* int a[10] = {2, 9, 3, 7, 5, 6, 3, 8, 3, 4};
		*
		* // for arranging the elements in the descending order, you can use bolt::cl::greater<int>()
		* bolt::cl::sort(a, a+10);
		* // a => {2, 3, 3, 3, 4, 5, 6, 7, 8, 9}
		*  \endcode
		*/
		template<typename RandomAccessIterator> 
		void sort(RandomAccessIterator first, 
			RandomAccessIterator last, 
			const std::string cl_code="");
	}// end of bolt::cl namespace
}// end of bolt namespace

#include <bolt/cl/detail/sort.inl>

