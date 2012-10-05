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
	 * This version of transform accepts an accelerator_view as an argument
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
