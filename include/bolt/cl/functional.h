/***************************************************************************                                                                                     
*   Copyright 2012 Advanced Micro Devices, Inc.                                     
*                                                                                    
*   Licensed under the Apache License, Version 2.0 (the "License");   
*   you may not use this file except in compliance with the License.                 
*   You may obtain a copy of the License at                                          
*                                                                                    
*       http://www.apache.org/licenses/LICENSE-2.0                      
*                                                                                    
*   Unless required by applicable law or agreed to in writing, software              
*   distributed under the License is distributed on an "AS IS" BASIS,              
*   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.         
*   See the License for the specific language governing permissions and              
*   limitations under the License.                                                   
***************************************************************************/                                                                                     

#pragma once

#include <bolt/cl/bolt.h>

#define BOLT_CREATE_STD_TYPENAMES(OPERATOR) \
    BOLT_CREATE_TYPENAME(OPERATOR<float>); \
    BOLT_CREATE_TYPENAME(OPERATOR<double>); \
    BOLT_CREATE_TYPENAME(OPERATOR<int>); \
    BOLT_CREATE_TYPENAME(OPERATOR<unsigned int>);

#define BOLT_CREATE_STD_CLCODE(OPERATOR,CODE_STRING) \
    BOLT_CREATE_CLCODE(OPERATOR<float>, CODE_STRING); \
    BOLT_CREATE_CLCODE(OPERATOR<double>, CODE_STRING); \
    BOLT_CREATE_CLCODE(OPERATOR<int>, CODE_STRING); \
    BOLT_CREATE_CLCODE(OPERATOR<unsigned int>, CODE_STRING); 
#define BOLT_CREATE_TYPE_TYPENAMES(OPERATOR, TYPE) \
    BOLT_CREATE_TYPENAME(OPERATOR<TYPE>);

#define BOLT_CREATE_TYPE_CLCODE(OPERATOR, TYPE, CODE_STRING) \
    BOLT_CREATE_CLCODE(OPERATOR<TYPE>, CODE_STRING);


// macro for creating a host-side routine and an OCL string (in the bolcl::clcode:: namespace).
// Also defines the typename trait automatically.
#define CREATE_BOLT_FUNCTIONAL(OPERATOR, ... ) \
    __VA_ARGS__; \
    BOLT_CREATE_STD_TYPENAMES(OPERATOR); \
    BOLT_CREATE_STD_CLCODE(OPERATOR, #__VA_ARGS__);

#define CREATE_BOLT_TYPE_FUNCTIONAL(OPERATOR, TYPE, ... ) \
    __VA_ARGS__; \
    BOLT_CREATE_TYPE_TYPENAMES(OPERATOR, TYPE); \
    BOLT_CREATE_TYPE_CLCODE(OPERATOR, TYPE, #__VA_ARGS__);


namespace bolt {
namespace cl {


/******************************************************************************
 * Unary Operators
 *****************************************************************************/

CREATE_BOLT_FUNCTIONAL(square,
template <typename T>
struct square
{
    T operator() (const T& x)  const { return x * x; }
};
);

CREATE_BOLT_FUNCTIONAL(cube,
template <typename T>
struct cube
{
    T operator() (const T& x)  const { return x * x * x; }
};
);

CREATE_BOLT_FUNCTIONAL(negate,
template<typename T>
struct negate 
{
    T operator()(const T& x) const {return -x;}
}; 
);


/******************************************************************************
 * Binary Operators
 *****************************************************************************/

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

CREATE_BOLT_FUNCTIONAL(divides, 
template<typename T>
struct divides
{
    T operator()(const T &lhs, const T &rhs) const {return lhs / rhs;}
}; 
);

CREATE_BOLT_FUNCTIONAL(modulus, 
template<typename T>
struct modulus
{
    T operator()(const T &lhs, const T &rhs) const {return lhs % rhs;}
}; 
);

CREATE_BOLT_FUNCTIONAL(maximum, 
template<typename T>
struct maximum 
{
    T operator()(const T &lhs, const T &rhs) const  {return (lhs > rhs) ? lhs:rhs;}
}; 
);

CREATE_BOLT_FUNCTIONAL(minimum,
template<typename T>
struct minimum
{
    T operator()(const T &lhs, const T &rhs) const  {return (lhs < rhs) ? lhs:rhs;}
}; 
);

CREATE_BOLT_FUNCTIONAL(bit_and,
template<typename T>
struct bit_and
{
    T operator()(const T &lhs, const T &rhs) const  {return lhs & rhs;}
}; 
);

CREATE_BOLT_FUNCTIONAL(bit_or,
template<typename T>
struct bit_or
{
    T operator()(const T &lhs, const T &rhs) const  {return lhs | rhs;}
}; 
);

CREATE_BOLT_FUNCTIONAL(bit_xor,
template<typename T>
struct bit_xor
{
    T operator()(const T &lhs, const T &rhs) const  {return lhs ^ rhs;}
}; 
);


/******************************************************************************
 * Unary Predicates
 *****************************************************************************/

CREATE_BOLT_FUNCTIONAL(logical_not,
template<typename T>
struct logical_not
{
    bool operator()(const T &x) const  {return !x;}
}; 
);


/******************************************************************************
 * Binary Predicates
 *****************************************************************************/

CREATE_BOLT_FUNCTIONAL(equal_to,
template<typename T>
struct equal_to
{
    bool operator()(const T &lhs, const T &rhs) const  {return lhs == rhs;}
}; 
);

CREATE_BOLT_FUNCTIONAL(not_equal_to,
template<typename T>
struct not_equal_to
{
    bool operator()(const T &lhs, const T &rhs) const  {return lhs != rhs;}
}; 
);

CREATE_BOLT_FUNCTIONAL(greater,
template<typename T>
struct greater
{
    bool operator()(const T &lhs, const T &rhs) const  {return lhs > rhs;}
}; 
);

CREATE_BOLT_FUNCTIONAL(less,
template<typename T>
struct less
{
    bool operator()(const T &lhs, const T &rhs) const  {return lhs < rhs;}
}; 
);

CREATE_BOLT_FUNCTIONAL(greater_equal,
template<typename T>
struct greater_equal
{
    bool operator()(const T &lhs, const T &rhs) const  {return lhs >= rhs;}
}; 
);

CREATE_BOLT_FUNCTIONAL(less_equal,
template<typename T>
struct less_equal
{
    bool operator()(const T &lhs, const T &rhs) const  {return lhs <= rhs;}
}; 
);

CREATE_BOLT_FUNCTIONAL(logical_and,
template<typename T>
struct logical_and
{
    bool operator()(const T &lhs, const T &rhs) const  {return lhs && rhs;}
}; 
);

CREATE_BOLT_FUNCTIONAL(logical_or,
template<typename T>
struct logical_or
{
    bool operator()(const T &lhs, const T &rhs) const  {return lhs || rhs;}
}; 
);

}; // namespace cl
}; // namespace bolt

