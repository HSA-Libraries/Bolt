#pragma once

#include <iostream>  // FIXME, remove as this is only here for debug output
#include <vector>
#include <amp.h>
#include <bolt/functional.h>


#ifdef BOLT_POOL_ALLOC

// FIXME - hack for use in hessian project.
// Buffer pool accelerates performance by reducing dynamic alloc/dealloc, and automatically uses a staging buffer.
// Hack here since we hard-code the user of the HessianState type; we should do a generic pool based on the 
// size of the object?  Or separate pools for each call?
#include <bolt/pool_alloc.h>


// Hacky global variable.
extern bolt::ArrayPool<HessianState> arrayPool;


// Remove me or move to boltControl structure - this is just to provide control for experimentation:
extern int p_computeUnits;
extern int p_wgPerComputeUnit;

#endif


namespace bolt {

#define VW 1
#define BARRIER(W)  // FIXME - placeholder for future barrier insertions

#define REDUCE_STEP(_IDX, _W) \
	if (_IDX < _W) tiled_data[_IDX] = reduce_op(tiled_data[_IDX], tiled_data[_IDX+_W]); \
	BARRIER(_W)

	// Optimal values for pitcairn are 32/8, TN are 6/12
	static const int reduceMultiCpuThreshold = 2; // FIXME, artificially low to force use of GPU
	static const int reduceGpuThreshold = 4; // FIXME, artificially low to force use of GPU
	//static const int computeUnits     = 32; // FIXME - determine from HSA Runtime
	//static const int wgPerComputeUnit =  12; // Set high enough to hide memory latency, but low enough so that most reduction is done within each WG rather than between WG.
	static const int localW = 8;
	static const int localH = 8;
	static const int waveSize  = 64; // FIXME, read from device attributes.


	// First version accepts accelerator view and range of iterators.
	template<typename outputT, typename InputIterator, typename UnaryFunction, typename BinaryFunction> 
	outputT transform_reduce(concurrency::accelerator_view av, 
		InputIterator begin, InputIterator end,  
		UnaryFunction transform_op, 
		outputT init,  BinaryFunction reduce_op)

	{
		int computeUnits = 32;
		int wgPerComputeUnit = 8;
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

		int resultCnt = computeUnits * wgPerComputeUnit;
		static const int waveSize  = 64; // FIXME, read from device attributes.

		typedef std::iterator_traits<InputIterator>::value_type inputT;
		concurrency::array_view<inputT,1> A(sz, &*begin);

		concurrency::extent<1> launchExt(resultCnt*waveSize);

		concurrency::array<outputT,1> results1(resultCnt, av);  // Output after reducing through LDS.

		//std::cout << "ArrayDim=" << A.extent[0] << "  launchExt=" << launchExt[0] << "  iters=" << iterationsPerWg << std::endl;

		// FIXME - support actual BARRIER operations.
		// FIXME - support checks on local memory usage
		// FIXME - reduce size of work for small problems.
		concurrency::parallel_for_each(av,  launchExt.tile<waveSize>(), [=,&results1](concurrency::tiled_index<waveSize> idx) mutable restrict(amp)
		{
			tile_static outputT tiled_data[waveSize];

			// FIXME - need to unroll loop to expose a bit more compute density
			for (int i=idx.global[0]; i<A.extent[0]; i+=launchExt[0]) {
				outputT val = transform_op(A[i]);
				init = reduce_op(init, val);
			};

			//---
			// Reduce through LDS across wavefront:
			tiled_data[idx.local[0]] = init; 
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
				results1[idx.tile] = tiled_data[0];
			};

		} );  //end parallel_for_each
		// results1[] now contains intermediate results which need to be combined together.

		//---
		//Copy partial array back to host
		// FIXME - we'd really like to use ZC memory for this final step 
		std::vector<outputT> h_data(resultCnt);
		h_data = results1; 

#if 0
		for (int i=0; i<results1.extent[0]; i++) {
			std::cout << i << "] -> ";
			T r = h_data[i];
			std::cout << r << std::endl;
		};
#endif	

		outputT finalReduction = init;
		for (int i=0; i<results1.extent[0]; i++) {
			finalReduction = reduce_op(finalReduction, h_data[i]);
		};

		return finalReduction;

	};


	// This version takes a start index and extent as the range to iterate.  The tranform_op is called with an index<> for each point in the range.  Useful for indexing over all points in an image or array
	template<typename outputT, int Rank, typename UnaryFunction, typename BinaryFunction> 
	outputT transform_reduce(concurrency::accelerator_view av, 
		concurrency::index<Rank> origin, concurrency::extent<Rank> ext,
		UnaryFunction transform_op, 
		outputT init,  BinaryFunction reduce_op)
	{
		int wgPerComputeUnit = p_wgPerComputeUnit; // FIXME - remove me.
		int computeUnits     = p_computeUnits;     // FIXME - remove me.
		int resultCnt = computeUnits * wgPerComputeUnit;

		// FIXME: implement a more clever algorithm for setting the shape of the calculation.
		int globalH = wgPerComputeUnit * localH;
		int globalW = computeUnits * localW;

		globalH = (ext[0] < globalH) ? ext[0] : globalH; //FIXME, this is not a multiple of localSize.
		globalW = (ext[1] < globalW) ? ext[1] : globalW;


		concurrency::extent<2> launchExt(globalH, globalW);
#ifdef BOLT_POOL_ALLOC
		bolt::ArrayPool<outputT>::PoolEntry &entry = arrayPool.alloc(av, resultCnt);
		concurrency::array<outputT,1> &results1 = *(entry._dBuffer);
#else
		concurrency::array<outputT,1> results1(resultCnt, av);  // Output after reducing through LDS.
#endif

		// FIXME - support actual BARRIER operations.
		// FIXME - support checks on local memory usage
		// FIXME - reduce size of work for small problems.
		concurrency::parallel_for_each(av,  launchExt.tile<localH, localW>(), [=,&results1](concurrency::tiled_index<localH, localW> idx) mutable restrict(amp)
		{
			tile_static outputT tiled_data[waveSize];

			// FIXME - need to unroll loop to expose a bit more compute density
			// FIXME-hardcoded for size 2 right now.
			for (int y=origin[0]+idx.global[0]; y<origin[0]+ext[0]; y+=launchExt[0]) {
				for (int x=origin[1]+idx.global[1]*VW; x<origin[1]+ext[1]; x+=launchExt[1]*VW) {
					init = reduce_op(init, transform_op(concurrency::index<Rank>(y,x)));
					//init = reduce_op(init, transform_op(concurrency::index<Rank>(y,x+1)));
				};
			};

			//---
			// Reduce through LDS across wavefront:
			int lx = localW * idx.local[0] + idx.local[1];
			tiled_data[lx] = init; 
			BARRIER(waveSize);

			REDUCE_STEP(lx, 32);
			REDUCE_STEP(lx, 16);
			REDUCE_STEP(lx, 8);
			REDUCE_STEP(lx, 4);
			REDUCE_STEP(lx, 2);
			REDUCE_STEP(lx, 1);

			//---
			// Save result of this tile to global mem
			if (lx== 0) {
				results1[idx.tile[0]*computeUnits + idx.tile[1]] = tiled_data[0];
			};

		} );  //end parallel_for_each
		// results1[] now contains intermediate results which need to be combined together.

		//---
		//Copy partial array back to host
		// FIXME - we'd really like to use ZC memory for this final step 
		//std::vector<outputT> h_data(resultCnt);
		//h_data = results1; 
		concurrency::copy(*entry._dBuffer, *entry._stagingBuffer);
		
	

		outputT finalReduction = init;
		for (int i=0; i<results1.extent[0]; i++) {
			finalReduction = reduce_op(finalReduction, (*entry._stagingBuffer)[i]);
		};

#ifdef BOLT_POOL_ALLOC
		arrayPool.free(entry);
#endif

		return finalReduction;

	};


	template<typename outputT, int Rank, typename UnaryFunction, typename BinaryFunction> 
	outputT transform_reduce(
		concurrency::index<Rank> origin, concurrency::extent<Rank> ext,
		UnaryFunction transform_op, 
		outputT init,  BinaryFunction reduce_op) 
	{
		return transform_reduce(concurrency::accelerator().default_view, origin, ext, transform_op, init, reduce_op);
	};


	template<typename T, typename InputIterator, typename UnaryFunction, typename BinaryFunction> 
	T transform_reduce(
		InputIterator begin, InputIterator end,  
		UnaryFunction transform_op, 
		T init,  BinaryFunction reduce_op)

	{
		return transform_reduce(concurrency::accelerator().default_view, begin, end, transform_op, init, reduce_op);
	};

	// FIXME - still need more versions that take accelerator as first argument.


}; // end namespace bolt
