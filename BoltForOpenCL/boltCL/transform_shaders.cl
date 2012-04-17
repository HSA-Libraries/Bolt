    template <typename T,  typename binary_function>
	kernel
		void transformTemplate(
		global T* A, 
		global T* B,
		global T* Z,

		const int length,
		global binary_function *userFunctor
		)
	{
		int gx = get_global_id (0);

		// FIXME - maybe add a for-loop over the assignment to reduce overhead?

		T aa = A[gx];
		T bb = B[gx];
		Z[gx] = (*userFunctor)(aa, bb); 
	};

#if 0
	//----
	// The instantiation would be generated on the host, replacing T with the type of the 
	template __attribute__((mangled_name(transformInstantiated))) 
		kernel void transformTemplate(
		global float* A, 
		global float* B,
		global float* Z,

		const int length,
		global SaxpyFunctor *userFunctor) ;
#endif
