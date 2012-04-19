#pragma once

#include <boltCL/bolt.h>

#define CREATE_STD_TYPENAMES(T) \
	CREATE_TYPENAME(T<float>);\
	CREATE_TYPENAME(T<int>);\
	CREATE_TYPENAME(T<double>);


// macro for creating a host-side routine and an OCL string (in the bolcl::oclcode:: namespace).  Also defines the typename trait automatically.
#define BOLT_FUNCTIONAL(NAME, F) namespace oclcode {std::string NAME=#F;}; F; CREATE_STD_TYPENAMES(NAME);


namespace boltcl {

	BOLT_FUNCTIONAL(plus, 
	template<typename T>
	struct plus
	{
		T operator()(const T &lhs, const T &rhs) const {return lhs + rhs;}
	}; 
	);

	BOLT_FUNCTIONAL(minus, 
	template<typename T>
	struct minus
	{
		T operator()(const T &lhs, const T &rhs) const {return lhs - rhs;}
	}; 
	);


	BOLT_FUNCTIONAL(maximum, 
	template<typename T>
	struct maximum 
	{
		T operator()(const T &lhs, const T &rhs) const  {return rhs > lhs ? rhs:lhs;}
	}; 
	);


	BOLT_FUNCTIONAL(minimum,
	template<typename T>
	struct minimum
	{
		T operator()(const T &lhs, const T &rhs) const  {return rhs < lhs ? rhs:lhs;}
	}; 
	);


	BOLT_FUNCTIONAL(square,
	template <typename T>
	struct square
	{
		T& operator() (const T& x)  const{
			return x * x;
		}
	};
	);

	//---
	// Unary operations:
	BOLT_FUNCTIONAL(negate,
	template<typename T>
	struct negate 
	{
		T operator()(const T &__x) const {return -__x;}
	}; 
	);

};

