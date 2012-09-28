template <typename T,  typename binary_function>
kernel
void transformTemplate(global T* A,
			global T* B,
			global T* Z,
			const int length,
			global binary_function *userFunctor)
{
	int gx = get_global_id (0);

	if (gx >= length)
		return;
	else
	{
		T aa = A[gx];
		T bb = B[gx];
		Z[gx] = (*userFunctor)(aa, bb);
	}
}


template <typename T,  typename unary_function>
kernel
void unaryTransformTemplate(global T* A,
			global T* Z,
			const int length,
			global unary_function *userFunctor)
{
	int gx = get_global_id (0);

	if (gx >= length)
		return;
	else
	{
		T aa = A[gx];
		Z[gx] = (*userFunctor)(aa);
	}
}
