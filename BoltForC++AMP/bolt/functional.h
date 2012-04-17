#pragma once

#define RESTRICT_AMP restrict(amp) 
#define RESTRICT_AMP_CPU restrict(amp,cpu) 


namespace bolt {
	template<typename Argument1,
		typename Result>
	struct unary_function
		: public std::unary_function<Argument1, Result>
	{
        int _dummy;  // FIXME, workaround for beta C++AMP release.
	};

	template<typename Argument1,
		typename Argument2,
		typename Result>
	struct binary_function
		: public std::binary_function<Argument1, Argument2, Result>
	{
        int _dummy;  // FIXME, workaround for beta C++AMP release.
	};

	template<typename T>
	struct plus : public binary_function<T,T,T>  
	{
		T operator()(const T &lhs, const T &rhs) const RESTRICT_AMP_CPU {return lhs + rhs;}
	}; 


	template<typename T>
	struct maximum : public binary_function<T,T,T>  
	{
		T operator()(const T &lhs, const T &rhs) const RESTRICT_AMP_CPU {return rhs > lhs ? rhs:lhs;}
	}; 

	template<typename T>
	struct minimum : public binary_function<T,T,T>  
	{
		T operator()(const T &lhs, const T &rhs) const RESTRICT_AMP_CPU {return rhs < lhs ? rhs:lhs;}
	}; 


	template <typename T>
	struct square
	{
		T& operator() (const T& x)  const RESTRICT_AMP {
			return x * x;
		}
	};


	template<typename T>
	struct negate : public unary_function<T,T>  
	{
		T operator()(const T &__x) const RESTRICT_AMP {return -__x;}
	}; 


};

