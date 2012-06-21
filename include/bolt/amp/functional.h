#pragma once



namespace bolt {
	template<typename Argument1,
		typename Result>
	struct unary_function
		: public std::unary_function<Argument1, Result>
	{
	};

	template<typename Argument1,
		typename Argument2,
		typename Result>
	struct binary_function
		: public std::binary_function<Argument1, Argument2, Result>
	{
	};

	template<typename T>
	struct plus : public binary_function<T,T,T>  
	{
		T operator()(const T &lhs, const T &rhs) const restrict(cpu,amp) {return lhs + rhs;}
	}; 


	template<typename T>
	struct maximum : public binary_function<T,T,T>  
	{
		T operator()(const T &lhs, const T &rhs) const restrict(cpu,amp) {return rhs > lhs ? rhs:lhs;}
	}; 

	template<typename T>
	struct minimum : public binary_function<T,T,T>  
	{
		T operator()(const T &lhs, const T &rhs) const restrict(cpu,amp) {return rhs < lhs ? rhs:lhs;}
	}; 


	template <typename T>
	struct square
	{
		T& operator() (const T& x)  const restrict(amp) {
			return x * x;
		}
	};


	template<typename T>
	struct negate : public unary_function<T,T>  
	{
		T operator()(const T &__x) const restrict(amp) {return -__x;}
	}; 


};

