#pragma once

#include <amp.h>


//#include <bolt/combineable_queue.h>

//#define RESTRICT_AMP restrict(direct3d) 


namespace bolt {

	/*! parallel_iteration1 : Slow implementation - just use a parallel_for for all devices.

	Init is the initial state for the Iteration type for every index.

	The user provides a functor which accepts two arguments:
	index<> - location in the extent that should be processed.
	IterType : User-supplied type that tracks the state of each iteration.
	The functor is responsible for :
	* Updating the state so that it points to the next 'iteration'.  The next "iteration" could be the next cascade stage 
	in a face-detection algorithm, or the next iteration of a loop.  It is critical to update to the next state to avoid
	an infinite loop.
	* Return true if the runtime should schedule the next iteration, or false if this iteration has been processed.
	*/
	template<typename IterType, typename Function> 
	void parallel_iteration(concurrency::extent<1> ext, IterType Init, Function f)  {

		using namespace concurrency;

		parallel_for_each(grid<1>(ext), [=] (index<1> idx) mutable restrict(direct3d) {
			IterType iter(Init);

			bool keepGoing = true;
			
			while (keepGoing) {
				keepGoing = f(idx, iter);
			};
		});

	};

	/*!
	*/
	template<typename IterType, typename Function> 
	void parallel_iteration_1(concurrency::extent<1> ext, IterType Init, Function f)  {

		using namespace concurrency;
		int splitPoint = 3;  // Iterations to run on the GPU:

		static const int maxQSz=1024;
		array<pair<index<1>, IterType> outQueue(maxQSz);
		array<int,1>  qPtr(1);

		parallel_for_each(grid<1>(ext), [=] (index<1> idx) mutable restrict(direct3d) {
			IterType iter(Init);
			int iterations = splitPoint;

			bool keepGoing = true;
			while (keepGoing && --iterations) {
				keepGoing = f(idx, iter);
			};
			if (keepGoing) {
				// Enqueue for processing belowv - save idx and state.  
				//; Horrible performance due to contended atomic operations, need to combine enqueue operations.
				int lPtr = atomic_fetch_add(&qPtr[0], 1);
				// FIXME - bounds check
				if (lPtr < maxQSz) {
					outQueue[lPtr].first = idx;
					outQueue[lPtr].second = iter;
				}
			};
		});
	};
};

// TODO: Need versions to handle more extent dimensions??

// FIXME-TODO
// Add Doxygen-style comments:
