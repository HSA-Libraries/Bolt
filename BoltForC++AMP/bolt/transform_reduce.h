#pragma once

#include <iostream>  // FIXME, remove as this is only here for debug output
#include <vector>
#include <amp.h>
#include <bolt/functional.h>



namespace bolt {

#define BARRIER(W)  // FIXME - placeholder for future barrier insertions

#define REDUCE_STEP(_IDX, _W) \
	if (_IDX < _W) results0[_IDX] = reduce_op(results0[_IDX], results0[_IDX+_W]); \
	BARRIER(_W)

	const int reduceMultiCpuThreshold = 2; // FIXME, artificially low to force use of GPU
	const int reduceGpuThreshold = 4; // FIXME, artificially low to force use of GPU

	template<typename T, typename InputIterator, typename UnaryFunction, typename BinaryFunction> 
	T transform_reduce(concurrency::accelerator_view av, 
		InputIterator begin, InputIterator end,  
		UnaryFunction transform_op, 
		T init,  BinaryFunction reduce_op)

	{
		int sz = (int)(end - begin);  // FIXME- size_t

#if 0
		if (sz < reduceMultiCpuThreshold) {
			//serial CPU implementation
			std::transform
				return std::accumulate(begin, end, init, reduce_op);
		} else if (sz < reduceGpuThreshold) {
			// multi-core CPU implementation:
			concurrency::combinable<T> results1;
			concurrency::parallel_for_each(begin, end, [&results1, &reduce_op] (T i) {
				results1.local() = reduce_op(results1.local(), i);
			});
			return results1.combine(reduce_op);
		} else 
#endif
			if (1) {
				int computeUnits     = 10; // FIXME - determine from HSA Runtime

				// FIXME - determine from HSA Runtime 
				// - based on est of how many threads needed to hide memory latency.
				int wgPerComputeUnit =  6; 
				int resultCnt = computeUnits * wgPerComputeUnit;
				static const int waveSize  = 64; // FIXME, read from device attributes.


				typedef std::iterator_traits<InputIterator>::value_type T;
				concurrency::array_view<T,1> A(sz, &*begin);

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
						T val = transform_op(A[i]);
						sum = reduce_op(sum, val);
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
					finalReduction = reduce_op(finalReduction, h_data[i]);
				};

				return finalReduction;
			};
	};


	template<typename T, typename InputIterator, typename UnaryFunction, typename BinaryFunction> 
	T transform_reduce(
		InputIterator begin, InputIterator end,  
		UnaryFunction transform_op, 
		T init,  BinaryFunction reduce_op)

	{
		return transform_reduce(concurrency::accelerator().default_view, begin, end, transform_op, init, reduce_op);
	};

	// still need more versions that take accelerator as first argument.


}; // end namespace bolt
