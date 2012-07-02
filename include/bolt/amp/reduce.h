#pragma once

#include <iostream>  // FIXME, remove as this is only here for debug output
#include <vector>
#include <array>
#include <amp.h>
#include <numeric>
#include <bolt/AMP/functional.h>



namespace bolt {

#define BARRIER(W)  // FIXME - placeholder for future barrier insertions

#define REDUCE_STEP(_IDX, _W) \
	if (_IDX < _W) results0[_IDX] = binary_op(results0[_IDX], results0[_IDX+_W]); \
	BARRIER(_W)

    const int reduceMultiCpuThreshold = 2; // FIXME, artificially low to force use of GPU
    const int reduceGpuThreshold = 4; // FIXME, artificially low to force use of GPU

	template<typename InputIterator, typename T, typename BinaryFunction> 
	typename std::iterator_traits<InputIterator>::value_type 
		reduce(concurrency::accelerator_view av, 
		InputIterator begin, InputIterator end, 
		T init,
		BinaryFunction binary_op)  
	{
		//	NOTE: This subtraction of iterators implies that the memory is sequential, which implies a vector iterator
		// int sz = end - begin;
		auto sz = std::distance( begin, end );

		if (sz < reduceMultiCpuThreshold) {
			//serial CPU implementation
			return std::accumulate(begin, end, init, binary_op);
		} else if (sz < reduceGpuThreshold) {
			// multi-core CPU implementation:
			concurrency::combinable<T> results1;
			concurrency::parallel_for_each(begin, end, [&results1, &binary_op] (T i) {
				results1.local() = binary_op(results1.local(), i);
			});
			return results1.combine(binary_op);

		} else {
			int computeUnits     = 10; // FIXME - determine from HSA Runtime

			// FIXME - determine from HSA Runtime 
			// - based on est of how many threads needed to hide memory latency.
			int wgPerComputeUnit =  6; 
			int resultCnt = computeUnits * wgPerComputeUnit;
			static const int waveSize  = 64; // FIXME, read from device attributes.


			typedef std::iterator_traits<InputIterator>::value_type T;
			concurrency::array_view<T,1> A( static_cast< int >( sz ), &*begin);

			concurrency::extent<1> launchExt(resultCnt*waveSize);
			int iterationsPerWg = A.extent[0]/launchExt[0];

			concurrency::array<T,1> results1(resultCnt, av);  // Output after reducing through LDS.

			//std::cout << "ArrayDim=" << A.extent[0] << "  launchExt=" << launchExt[0] << "  iters=" << iterationsPerWg << std::endl;

			// FIXME - support actual BARRIER operations.
			// FIXME - support checks on local memory usage
			// FIXME - reduce size of work for small problems.
			concurrency::parallel_for_each(av,  launchExt.tile<waveSize>(), [=,&results1](concurrency::tiled_index<waveSize> idx) mutable restrict(amp)
			{
				tile_static T results0[waveSize];  // Could cause a problem for non-POD types in LDS?

				T sum = init; // FIXME - .  Don't use Init here, peel out first assignment.
				// FIXME - need to unroll loop to expose a bit more compute density
				for (int i=idx.global[0]; i<A.extent[0]; i+=launchExt[0]) {
					T val = A[i];
					sum = binary_op(sum, val);
				};


				//---
				// Reduce through LDS across wavefront:
				results0[idx.local[0]] = sum; 
				BARRIER(waveSize);

				REDUCE_STEP(idx.local[0], 32);
				REDUCE_STEP(idx.local[0], 16);
				REDUCE_STEP(idx.local[0], 8);
				REDUCE_STEP(idx.local[0], 4);
				REDUCE_STEP(idx.local[0], 2);
				REDUCE_STEP(idx.local[0], 1);

				//---
				// Save result of this tile to global mem
				if (idx.local[0] == 0) {
					results1[idx.tile] = results0[0];
				};

			} );  //end parallel_for_each
			// results1[] now contains intermediate results which need to be combined together.

			//---
			//Copy partial array back to host
			// FIXME - we'd really like to use ZC memory for this final step 
			std::vector<T> h_data(resultCnt);
			h_data = results1; 

#if 0
			for (int i=0; i<results1.extent[0]; i++) {
				std::cout << i << "] -> ";
				T r = h_data[i];
				std::cout << r << std::endl;
			};
#endif	

			T finalReduction = init;
			for (int i=0; i<results1.extent[0]; i++) {
				finalReduction = binary_op(finalReduction, h_data[i]);
			};

			return finalReduction;
		};
	};

	/*
	* This version of reduce defaults to disallow the use of iterators, unless a specialization exists below
	*/
	template<typename InputIterator, typename T, typename BinaryFunction> 
	typename std::iterator_traits<InputIterator>::value_type
		reduce(InputIterator begin, InputIterator end, 
		T init,
		BinaryFunction binary_op, std::input_iterator_tag )
	{
		return std::iterator_traits<InputIterator>::value_type( );
	};

	/*
	* Partial specialization of reduce which allows the use of naked pointer types
	*/
	template< typename T, typename BinaryFunction >
	typename std::iterator_traits< T* >::value_type
		reduce( T* begin, typename T* end, 
		T init,
		BinaryFunction binary_op, std::random_access_iterator_tag )
	{
		return reduce( concurrency::accelerator().default_view, begin, end, init, binary_op);
	};

	/*
	* Partial specialization of reduce which allows the use of constant naked pointer types
	*/
	template< typename T, typename BinaryFunction >
	typename std::iterator_traits< const T* >::value_type
		reduce( const T* begin, const T* end, 
		T init,
		BinaryFunction binary_op, std::random_access_iterator_tag )
	{
		return reduce( concurrency::accelerator().default_view, begin, end, init, binary_op);
	};

	/*
	* Partial specialization of reduce which allows the use of std::vector< T >::iterator types
	*/
	template< typename T, typename BinaryFunction >
	typename std::iterator_traits< typename std::vector< T >::iterator >::value_type
		reduce(typename std::vector< T >::iterator begin, typename std::vector< T >::iterator end, 
		T init,
		BinaryFunction binary_op, std::random_access_iterator_tag )
	{
		return reduce (concurrency::accelerator().default_view, begin, end, init, binary_op);
	};

	/*
	* Partial specialization of reduce which allows the use of std::vector< T >::const_iterator types
	*/
	template< typename T, typename BinaryFunction >
	typename std::iterator_traits< typename std::vector< T >::const_iterator >::value_type
		reduce(typename std::vector< T >::const_iterator begin, typename std::vector< T >::const_iterator end, 
		T init,
		BinaryFunction binary_op, std::random_access_iterator_tag )
	{
		return reduce (concurrency::accelerator().default_view, begin, end, init, binary_op);
	};

	/*
	* Partial specialization of reduce which disallows the use of std::vector< bool >::iterator types
	*/
	template< typename BinaryFunction >
	typename std::iterator_traits< typename std::vector< bool >::iterator >::value_type
		reduce(typename std::vector< bool >::iterator begin, typename std::vector< bool >::iterator end, 
		bool init,
		BinaryFunction binary_op, std::random_access_iterator_tag )
	{
		return std::iterator_traits<typename std::vector< bool >::iterator >::value_type( );
	};

	/*
	* Partial specialization of reduce which disallows the use of std::vector< bool >::const_iterator types
	*/
	template< typename BinaryFunction >
	typename std::iterator_traits< typename std::vector< bool >::const_iterator >::value_type
		reduce(typename std::vector< bool >::const_iterator begin, typename std::vector< bool >::const_iterator end, 
		bool init,
		BinaryFunction binary_op, std::random_access_iterator_tag )
	{
		return std::iterator_traits< typename std::vector< bool >::const_iterator >::value_type( );
	};

	/*
	* Partial specialization of reduce which allows the use of std::array< T, N >::iterator types
	*/
	template< typename T, size_t N, typename BinaryFunction >
	typename std::iterator_traits< typename std::_Array_iterator< T, N > >::value_type
		reduce(typename std::_Array_iterator< T, N > begin, typename std::_Array_iterator< T, N > end, 
		T init,
		BinaryFunction binary_op, std::random_access_iterator_tag )
	{
		return reduce (concurrency::accelerator().default_view, begin, end, init, binary_op);
	};

	/*
	* Partial specialization of reduce which allows the use of std::array< T, N >::const_iterator types
	*/
	template< typename T, size_t N, typename BinaryFunction >
	typename std::iterator_traits< typename std::_Array_const_iterator< T, N > >::value_type
		reduce(typename std::_Array_const_iterator< T, N > begin, typename std::_Array_const_iterator< T, N > end, 
		T init,
		BinaryFunction binary_op, std::random_access_iterator_tag )
	{
		return reduce (concurrency::accelerator().default_view, begin, end, init, binary_op);
	};

	/*
	* This version of reduce uses default accelerator
	*/
	template<typename InputIterator, typename T, typename BinaryFunction> 
	typename std::iterator_traits<InputIterator>::value_type
		reduce(InputIterator begin, InputIterator end, 
		T init,
		BinaryFunction binary_op)  
	{

		return reduce ( begin, end, init, binary_op, std::iterator_traits< InputIterator >::iterator_category( ) );
	};


	/*
	* This version of reduce uses 'plus' binary operation by default
	*/
	template<typename InputIterator, typename T> 
	typename std::iterator_traits<InputIterator>::value_type
		reduce(InputIterator begin, InputIterator end, 
		T init)
	{

		return reduce (begin, end, init, bolt::plus<T>(), std::iterator_traits< InputIterator >::iterator_category( ) );
	};


	/*
	* This version of reduce uses a default init value of 0 and plus<> as default argument.
	*/
	template<typename InputIterator> 
	typename std::iterator_traits<InputIterator>::value_type
		reduce(InputIterator begin, InputIterator end)
	{
		typedef std::iterator_traits<InputIterator>::value_type T;

		return reduce (begin, end, T(0), bolt::plus<T>(), std::iterator_traits< InputIterator >::iterator_category( ) );
	};


	// still need more versions that take accelerator as first argument.


}; // end namespace bolt
