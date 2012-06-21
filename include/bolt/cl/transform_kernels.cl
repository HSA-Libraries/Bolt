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
