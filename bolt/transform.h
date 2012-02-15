#pragma once

#include <amp.h>
#include <bolt/functional.h>


//#define RESTRICT_AMP restrict(direct3d) 


namespace bolt {

    const int transformThreshold = 4; // FIXME, threshold for using CPU or GPU

    /*! Comment and example here
     */
	template<typename InputIterator, typename OutputIterator, typename UnaryFunction> 
	void transform(InputIterator first, InputIterator last, OutputIterator result, UnaryFunction f)  {

        size_t sz = last - first;

        if (sz < transformThreshold) {
            // Run on CPU using serial implementation or concrt
		} else {
			concurrency::array_view<std::iterator_traits<InputIterator>::value_type,  1> inputV(sz, &*first);
			concurrency::array_view<std::iterator_traits<OutputIterator>::value_type, 1> resultV(sz, &*result);

			concurrency::parallel_for_each( inputV.grid, [=](concurrency::index<1> idx) mutable RESTRICT_AMP
			{
                resultV[idx.x] = f(inputV[idx.x]);
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

			concurrency::parallel_for_each(av, inputV.grid, [=](concurrency::index<1> idx) mutable RESTRICT_AMP
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

			concurrency::parallel_for_each( inputV1.grid, [=](concurrency::index<1> idx) mutable RESTRICT_AMP
			{
                resultV[idx.x] = f(inputV1[idx.x], inputV2[idx.x]);
			}
			);
		};
	};
};


// FIXME-TODO
// Add Doxygen-style comments:
