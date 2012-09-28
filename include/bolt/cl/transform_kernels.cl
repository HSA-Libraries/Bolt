template <typename iType, typename oType, typename binary_function>
kernel
void transformTemplate (global iType* A,
			global iType* B,
			global oType* Z,
			const int length,
			global binary_function *userFunctor)
{
	int gx = get_global_id (0);

	if (gx >= length)
		return;
	else
	{
		iType aa = A[gx];
		iType bb = B[gx];
		Z[gx] = (*userFunctor)(aa, bb);
	}
}


template <typename iType, typename oType, typename unary_function>
kernel
void unaryTransformTemplate(global iType* A,
			global oType* Z,
			const int length,
			global unary_function *userFunctor)
{
	int gx = get_global_id (0);

	if (gx >= length)
		return;
	else
	{ 
		iType aa = A[gx];
		Z[gx] = (*userFunctor)(aa); 
	}
}
