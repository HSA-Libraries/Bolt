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

#include <amp.h>
#include <bolt/AMP/functional.h>


//#define RESTRICT_AMP restrict(direct3d) 

#define USE_PROTOTYPE 0

#if USE_PROTOTYPE
namespace bolt {

    const int transformThreshold = 4; // FIXME, threshold for using CPU or GPU

    /*! Comment and example here
     */
	template<typename InputIterator, typename OutputIterator, typename UnaryFunction> 
	void transform(InputIterator first, InputIterator last, OutputIterator result, UnaryFunction f)  {

        int sz = (int) (last - first);  //FIXME - size_t conversion.

        if (sz < transformThreshold) {
            // Run on CPU using serial implementation or concrt
		} else {
			concurrency::array_view<std::iterator_traits<InputIterator>::value_type,  1> inputV(sz, &*first);
			concurrency::array_view<std::iterator_traits<OutputIterator>::value_type, 1> resultV(sz, &*result);

			concurrency::parallel_for_each( inputV.extent, [=](concurrency::index<1> idx) mutable restrict(amp)
			{
                resultV[idx[0]] = f(inputV[idx[0]]);
			}
			);
		};
	};


    /*! 
	 * This version of transform accepts an accelerator_view as an argument.
     */
	template<typename InputIterator, typename OutputIterator, typename UnaryFunction> 
	void transform(concurrency::accelerator_view av, InputIterator first, InputIterator last, OutputIterator result, UnaryFunction f)  {
        size_t sz = last - first;

        if (sz < transformThreshold) {
            // Run on CPU using serial implementation or concrt
		} else {
			concurrency::array_view<iterator_traits<InputIterator>::value_type,  1> inputV(sz, &*first);
			concurrency::array_view<iterator_traits<OutputIterator>::value_type, 1> resultV(sz, &*result);

			concurrency::parallel_for_each(av, inputV.grid, [=](concurrency::index<1> idx) mutable restrict(amp)
			{
                resultV[idx.x] = f(inputV[idx.x]);
			}
			);
		};
	};



	template<typename InputIterator, typename OutputIterator, typename BinaryFunction> 
	void transform(InputIterator first1, InputIterator last1, InputIterator first2, OutputIterator result, BinaryFunction f)  {

        int sz = last1 - first1; // FIXME - use size_t

        if (sz < transformThreshold) {
#if 1
            // Run on CPU using serial implementation or concrt
            concurrency::parallel_for(0, sz, [&](int x)
            {
                //result[x] = f(first1[x], first2[x]);
                result[x] = (first1[x] + first2[x]);
            });
#endif
		} else {
			concurrency::array_view<std::iterator_traits<InputIterator>::value_type,  1> inputV1(sz, &*first1);
			concurrency::array_view<std::iterator_traits<InputIterator>::value_type,  1> inputV2(sz, &*first2);
			concurrency::array_view<std::iterator_traits<OutputIterator>::value_type, 1> resultV(sz, &*result);

			concurrency::parallel_for_each( inputV1.extent, [=](concurrency::index<1> idx) mutable restrict(amp)
			{
                resultV[idx[0]] = f(inputV1[idx[0]], inputV2[idx[0]]);
			}
			);
		};
	};
};


// FIXME-TODO
// Add Doxygen-style comments:
#endif

#if !defined( AMP_TRANSFORM_H )
#define AMP_TRANSFORM_H
#pragma once
#include <bolt/amp/bolt.h>
#include <string>
#include <assert.h>  



namespace bolt
{
	namespace amp
	{
        /*! \addtogroup algorithms
         */

        /*! \addtogroup transformations
        *   \ingroup algorithms
        *   \p transform applies a specific function object to each element pair in the specified input ranges, and writes the result
        *   into the specified output range. For common code between the host
        *   and device, one can take a look at the TypeName implementations. See Bolt Tools for Split-Source 
        *   for a detailed description. 
        */ 
        
        /*! \addtogroup transform
        *   \ingroup transformations
        *   \{
        */

        /*! This version of \p transform applies a binary function to each pair
         *  of elements from two input sequences and stores the result in the
         *  corresponding position in an output sequence.  
         *  The input and output sequences can coincide, resulting in an 
         *  in-place transformation.
         *    
         *  \param ctl Control structure to control accelerator, debug, tuning, etc.  See bolt::amp::control.
         *  \param first1 The beginning of the first input sequence.
         *  \param last1 The end of the first input sequence.
         *  \param first2 The beginning of the second input sequence.
         *  \param result The beginning of the output sequence.
         *  \param op The tranformation operation.
         *  \return The end of the output sequence.
         *
         *  \tparam InputIterator1 is a model of InputIterator
         *                        and \c InputIterator1's \c value_type is convertible to \c BinaryFunction's \c first_argument_type.
         *  \tparam InputIterator2 is a model of InputIterator
         *                        and \c InputIterator2's \c value_type is convertible to \c BinaryFunction's \c second_argument_type.
         *  \tparam OutputIterator is a model of OutputIterator
         *  \tparam BinaryFunction is a model of BinaryFunction
         *                              and \c BinaryFunction's \c result_type is convertible to \c OutputIterator's \c value_type.
         *
         *  The following code snippet demonstrates how to use \p transform.
         *
         *  \code
         *  #include <bolt/amp/transform.h>
         *  #include <bolt/amp/functional.h>
         *  
         *  int input1[10] = {-5,  0,  2,  3,  2,  4, -2,  1,  2,  3};
         *  int input2[10] = { 3,  6, -2,  1,  2,  3, -5,  0,  2,  3};
         *  int output[10];
         *
         *  //Create an AMP Control object using the default accelerator
         *  ::Concurrency::accelerator accel(::Concurrency::accelerator::default_accelerator);
         *  bolt::amp::control ctl(accel);
         *  bolt::plus<int> op;
         *  bolt::amp::transform(ctl, input1, input1 + 10, input2, output, op);
         *
         *  // output is now {-2,  6,  0,  4,  4,  7};
         *  \endcode
         *
         *  \sa http://www.sgi.com/tech/stl/transform.html
         *  \sa http://www.sgi.com/tech/stl/InputIterator.html
         *  \sa http://www.sgi.com/tech/stl/OutputIterator.html
         *  \sa http://www.sgi.com/tech/stl/UnaryFunction.html
         *  \sa http://www.sgi.com/tech/stl/BinaryFunction.html
         */
        template<typename InputIterator, typename OutputIterator, typename BinaryFunction> 
        void transform(control &ctl,
					   InputIterator first1,
					   InputIterator last1,
					   InputIterator first2,
					   OutputIterator result, 
					   BinaryFunction f);

       /*! This version of \p transform applies a binary function to each pair
         *  of elements from two input sequences and stores the result in the
         *  corresponding position in an output sequence.  
         *  The input and output sequences can coincide, resulting in an 
         *  in-place transformation.
         *    

         *  \param first1 The beginning of the first input sequence.
         *  \param last1 The end of the first input sequence.
         *  \param first2 The beginning of the second input sequence.
         *  \param result The beginning of the output sequence.
         *  \return The end of the output sequence.
         *
         *  \tparam InputIterator1 is a model of InputIterator
         *                        and \c InputIterator1's \c value_type is convertible to \c BinaryFunction's \c first_argument_type.
         *  \tparam InputIterator2 is a model of InputIterator
         *                        and \c InputIterator2's \c value_type is convertible to \c BinaryFunction's \c second_argument_type.
         *  \tparam OutputIterator is a model of OutputIterator
         *  \tparam BinaryFunction is a model of BinaryFunction
         *                              and \c BinaryFunction's \c result_type is convertible to \c OutputIterator's \c value_type.
         *
         *  The following code snippet demonstrates how to use \p transform
         *
         *  \code
         *  #include <bolt/amp/transform.h>
         *  #include <bolt/amp/functional.h>
         *  
         *  int input1[10] = {-5,  0,  2,  3,  2,  4, -2,  1,  2,  3};
         *  int input2[10] = { 3,  6, -2,  1,  2,  3, -5,  0,  2,  3};
         *  int output[10];
         * 
         *
         *  bolt::amp::transform(input1, input1 + 10, input2, output, op);
         *
         *  // output is now {-2,  6,  0,  4,  4,  7};
         *  \endcode
         *
         *  \sa http://www.sgi.com/tech/stl/transform.html
         *  \sa http://www.sgi.com/tech/stl/InputIterator.html
         *  \sa http://www.sgi.com/tech/stl/OutputIterator.html
         *  \sa http://www.sgi.com/tech/stl/UnaryFunction.html
         *  \sa http://www.sgi.com/tech/stl/BinaryFunction.html
         */
        template<typename InputIterator, typename OutputIterator, typename BinaryFunction> 
        void transform(InputIterator first1,
                       InputIterator last1,
                       InputIterator first2,
                       OutputIterator result,
                       BinaryFunction f);

        template<typename InputIterator, typename OutputIterator, typename UnaryFunction> 
        void transform(InputIterator first,
                       InputIterator last,
                       OutputIterator result,
                       UnaryFunction f);

        template<typename InputIterator, typename OutputIterator, typename UnaryFunction> 
        void transform(control &ctl,
                       InputIterator first,
                       InputIterator last,
                       OutputIterator result, 
                       UnaryFunction f);


	}//amp namespace ends
}//bolt namespace ends

#include <bolt/amp/detail/transform.inl>

#endif // AMP_TRANSFORM_H


