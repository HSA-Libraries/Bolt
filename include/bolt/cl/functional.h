#pragma once

#include <bolt/cl/bolt.h>

#define BOLT_CREATE_STD_TYPENAMES(OPERATOR) \
	BOLT_CREATE_TYPENAME(OPERATOR<float>); \
	BOLT_CREATE_TYPENAME(OPERATOR<double>); \
	BOLT_CREATE_TYPENAME(OPERATOR<int>);

#define BOLT_CREATE_STD_CLCODE(OPERATOR,CODE_STRING) \
	BOLT_CREATE_CLCODE(OPERATOR<float>, CODE_STRING); \
	BOLT_CREATE_CLCODE(OPERATOR<double>, CODE_STRING); \
	BOLT_CREATE_CLCODE(OPERATOR<int>, CODE_STRING); 


// macro for creating a host-side routine and an OCL string (in the bolcl::clcode:: namespace).  Also defines the typename trait automatically.
#define CREATE_BOLT_FUNCTIONAL(OPERATOR, F)  F; BOLT_CREATE_STD_TYPENAMES(OPERATOR); BOLT_CREATE_STD_CLCODE(OPERATOR, #F);


namespace bolt {
    namespace cl {

	CREATE_BOLT_FUNCTIONAL(plus, 
	template<typename T>
	struct plus
	{
		T operator()(const T &lhs, const T &rhs) const {return lhs + rhs;}
	}; 
	);

	CREATE_BOLT_FUNCTIONAL(minus, 
	template<typename T>
	struct minus
	{
		T operator()(const T &lhs, const T &rhs) const {return lhs - rhs;}
	}; 
	);

	CREATE_BOLT_FUNCTIONAL(multiplies, 
	template<typename T>
	struct multiplies
	{
		T operator()(const T &lhs, const T &rhs) const {return lhs * rhs;}
	}; 
	);

	CREATE_BOLT_FUNCTIONAL(less, 
	template<typename T>
	struct less 
	{
		bool operator()(const T &lhs, const T &rhs) const  {return lhs < rhs ? true: false;}
 	};
	);
	
	CREATE_BOLT_FUNCTIONAL(greater, 
	template<typename T>
	struct greater 
	{
		bool operator()(const T &lhs, const T &rhs) const  {return lhs > rhs ? true: false;}
	}; 
	);


    CREATE_BOLT_FUNCTIONAL(maximum, 
	template<typename T>
	struct maximum 
	{
		T operator()(const T &lhs, const T &rhs) const  {return rhs > lhs ? rhs:lhs;}
	}; 
	);


	CREATE_BOLT_FUNCTIONAL(minimum,
	template<typename T>
	struct minimum
	{
		T operator()(const T &lhs, const T &rhs) const  {return rhs < lhs ? rhs:lhs;}
	}; 
	);


	CREATE_BOLT_FUNCTIONAL(square,
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
	CREATE_BOLT_FUNCTIONAL(negate,
	template<typename T>
	struct negate 
	{
		T operator()(const T &__x) const {return -__x;}
	}; 
	);

};
};

