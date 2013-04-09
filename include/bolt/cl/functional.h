/***************************************************************************                                                                                     
*   Copyright 2012 - 2013 Advanced Micro Devices, Inc.                                     
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
#if !defined( BOLT_CL_FUNCTIONAL_H )
#define BOLT_CL_FUNCTIONAL_H

#include "bolt/cl/bolt.h"

/*! \file bolt/cl/functional.h
    \brief List all the unary and binary functions.
*/

namespace bolt {
namespace cl {

/******************************************************************************
 * Unary Operators
 *****************************************************************************/
static const std::string squareFunctor = BOLT_HOST_DEVICE_DEFINITION(
template< typename T >
struct square
{
    T operator() (const T& x)  const { return x * x; }
};
);

static const std::string cubeFunctor = BOLT_HOST_DEVICE_DEFINITION(
template< typename T >
struct cube
{
    T operator() (const T& x)  const { return x * x * x; }
};
);

static const std::string negateFunctor = BOLT_HOST_DEVICE_DEFINITION(
template< typename T >
struct negate 
{
    T operator()(const T& x) const {return -x;}
}; 
);

/******************************************************************************
 * Binary Operators
 *****************************************************************************/

static const std::string plusFunctor = BOLT_HOST_DEVICE_DEFINITION(
template<typename T>
struct plus
{
    T operator()(const T &lhs, const T &rhs) const {return lhs + rhs;}
}; 
);

static const std::string minusFunctor = BOLT_HOST_DEVICE_DEFINITION(
template<typename T>
struct minus
{
    T operator()(const T &lhs, const T &rhs) const {return lhs - rhs;}
}; 
);

static const std::string multipliesFunctor = BOLT_HOST_DEVICE_DEFINITION(
template<typename T>
struct multiplies
{
    T operator()(const T &lhs, const T &rhs) const {return lhs * rhs;}
}; 
);

static const std::string dividesFunctor = BOLT_HOST_DEVICE_DEFINITION(
template<typename T>
struct divides
{
    T operator()(const T &lhs, const T &rhs) const {return lhs / rhs;}
}; 
);

static const std::string modulusFunctor = BOLT_HOST_DEVICE_DEFINITION(
template<typename T>
struct modulus
{
    T operator()(const T &lhs, const T &rhs) const {return lhs % rhs;}
}; 
);

static const std::string maximumFunctor = BOLT_HOST_DEVICE_DEFINITION(
template<typename T>
struct maximum 
{
    T operator()(const T &lhs, const T &rhs) const  {return (lhs > rhs) ? lhs:rhs;}
}; 
);

static const std::string minimumFunctor = BOLT_HOST_DEVICE_DEFINITION(
template<typename T>
struct minimum
{
    T operator()(const T &lhs, const T &rhs) const  {return (lhs < rhs) ? lhs:rhs;}
}; 
);

static const std::string bit_andFunctor = BOLT_HOST_DEVICE_DEFINITION(
template<typename T>
struct bit_and
{
    T operator()(const T &lhs, const T &rhs) const  {return lhs & rhs;}
}; 
);

static const std::string bit_orFunctor = BOLT_HOST_DEVICE_DEFINITION(
template<typename T>
struct bit_or
{
    T operator()(const T &lhs, const T &rhs) const  {return lhs | rhs;}
}; 
);

static const std::string bit_xorFunctor = BOLT_HOST_DEVICE_DEFINITION(
template<typename T>
struct bit_xor
{
    T operator()(const T &lhs, const T &rhs) const  {return lhs ^ rhs;}
}; 
);


/******************************************************************************
 * Unary Predicates
 *****************************************************************************/

static const std::string logical_notFunctor = BOLT_HOST_DEVICE_DEFINITION(
template<typename T>
struct logical_not
{
    bool operator()(const T &x) const  {return !x;}
}; 
);


/******************************************************************************
 * Binary Predicates
 *****************************************************************************/

static const std::string equal_toFunctor = BOLT_HOST_DEVICE_DEFINITION(
template<typename T>
struct equal_to
{
    bool operator()(const T &lhs, const T &rhs) const  {return lhs == rhs;}
}; 
);

static const std::string not_equal_toFunctor = BOLT_HOST_DEVICE_DEFINITION(
template<typename T>
struct not_equal_to
{
    bool operator()(const T &lhs, const T &rhs) const  {return lhs != rhs;}
}; 
);

static const std::string greaterFunctor = BOLT_HOST_DEVICE_DEFINITION(
template<typename T>
struct greater
{
    bool operator()(const T &lhs, const T &rhs) const  {return lhs > rhs;}
}; 
);

static const std::string lessFunctor = BOLT_HOST_DEVICE_DEFINITION(
template<typename T>
struct less
{
    bool operator()(const T &lhs, const T &rhs) const  {return lhs < rhs;}
}; 
);

static const std::string greater_equalFunctor = BOLT_HOST_DEVICE_DEFINITION(
template<typename T>
struct greater_equal
{
    bool operator()(const T &lhs, const T &rhs) const  {return lhs >= rhs;}
}; 
);

static const std::string less_equalFunctor = BOLT_HOST_DEVICE_DEFINITION(
template<typename T>
struct less_equal
{
    bool operator()(const T &lhs, const T &rhs) const  {return lhs <= rhs;}
}; 
);

static const std::string logical_andFunctor = BOLT_HOST_DEVICE_DEFINITION(
template<typename T>
struct logical_and
{
    bool operator()(const T &lhs, const T &rhs) const  {return lhs && rhs;}
}; 
);

static const std::string logical_orFunctor = BOLT_HOST_DEVICE_DEFINITION(
template<typename T>
struct logical_or
{
    bool operator()(const T &lhs, const T &rhs) const  {return lhs || rhs;}
}; 
);

}; // namespace cl
}; // namespace bolt

BOLT_CREATE_TYPENAME( bolt::cl::square< int > );
BOLT_CREATE_CLCODE( bolt::cl::square< int >, bolt::cl::squareFunctor );

BOLT_CREATE_TYPENAME( bolt::cl::cube< int > );
BOLT_CREATE_CLCODE( bolt::cl::cube< int >, bolt::cl::cubeFunctor );

BOLT_CREATE_TYPENAME( bolt::cl::negate< int > );
BOLT_CREATE_CLCODE( bolt::cl::negate< int >, bolt::cl::negateFunctor );

BOLT_CREATE_TYPENAME( bolt::cl::plus< int > );
BOLT_CREATE_CLCODE( bolt::cl::plus< int >, bolt::cl::plusFunctor );

BOLT_CREATE_TYPENAME( bolt::cl::minus< int > );
BOLT_CREATE_CLCODE( bolt::cl::minus< int >, bolt::cl::minusFunctor );

BOLT_CREATE_TYPENAME( bolt::cl::multiplies< int > );
BOLT_CREATE_CLCODE( bolt::cl::multiplies< int >, bolt::cl::multipliesFunctor );

BOLT_CREATE_TYPENAME( bolt::cl::divides< int > );
BOLT_CREATE_CLCODE( bolt::cl::divides< int >, bolt::cl::dividesFunctor );

BOLT_CREATE_TYPENAME( bolt::cl::modulus< int > );
BOLT_CREATE_CLCODE( bolt::cl::modulus< int >, bolt::cl::modulusFunctor );

BOLT_CREATE_TYPENAME( bolt::cl::maximum< int > );
BOLT_CREATE_CLCODE( bolt::cl::maximum< int >, bolt::cl::maximumFunctor );

BOLT_CREATE_TYPENAME( bolt::cl::minimum< int > );
BOLT_CREATE_CLCODE( bolt::cl::minimum< int >, bolt::cl::minimumFunctor );

BOLT_CREATE_TYPENAME( bolt::cl::bit_and< int > );
BOLT_CREATE_CLCODE( bolt::cl::bit_and< int >, bolt::cl::bit_andFunctor );

BOLT_CREATE_TYPENAME( bolt::cl::bit_or< int > );
BOLT_CREATE_CLCODE( bolt::cl::bit_or< int >, bolt::cl::bit_orFunctor );

BOLT_CREATE_TYPENAME( bolt::cl::bit_xor< int > );
BOLT_CREATE_CLCODE( bolt::cl::bit_xor< int >, bolt::cl::bit_xorFunctor );

BOLT_CREATE_TYPENAME( bolt::cl::logical_not< int > );
BOLT_CREATE_CLCODE( bolt::cl::logical_not< int >, bolt::cl::logical_notFunctor );

BOLT_CREATE_TYPENAME( bolt::cl::equal_to< int > );
BOLT_CREATE_CLCODE( bolt::cl::equal_to< int >, bolt::cl::equal_toFunctor );

BOLT_CREATE_TYPENAME( bolt::cl::not_equal_to< int > );
BOLT_CREATE_CLCODE( bolt::cl::not_equal_to< int >, bolt::cl::not_equal_toFunctor );

BOLT_CREATE_TYPENAME( bolt::cl::greater< int > );
BOLT_CREATE_CLCODE( bolt::cl::greater< int >, bolt::cl::greaterFunctor );

BOLT_CREATE_TYPENAME( bolt::cl::less< int > );
BOLT_CREATE_CLCODE( bolt::cl::less< int >, bolt::cl::lessFunctor );

BOLT_CREATE_TYPENAME( bolt::cl::greater_equal< int > );
BOLT_CREATE_CLCODE( bolt::cl::greater_equal< int >, bolt::cl::greater_equalFunctor );

BOLT_CREATE_TYPENAME( bolt::cl::less_equal< int > );
BOLT_CREATE_CLCODE( bolt::cl::less_equal< int >, bolt::cl::less_equalFunctor );

BOLT_CREATE_TYPENAME( bolt::cl::logical_and< int > );
BOLT_CREATE_CLCODE( bolt::cl::logical_and< int >, bolt::cl::logical_andFunctor );

BOLT_CREATE_TYPENAME( bolt::cl::logical_or< int > );
BOLT_CREATE_CLCODE( bolt::cl::logical_or< int >, bolt::cl::logical_orFunctor );

BOLT_TEMPLATE_REGISTER_NEW_TYPE( bolt::cl::square, int, unsigned int );
BOLT_TEMPLATE_REGISTER_NEW_TYPE( bolt::cl::square, int, float );
BOLT_TEMPLATE_REGISTER_NEW_TYPE( bolt::cl::square, int, double );

BOLT_TEMPLATE_REGISTER_NEW_TYPE( bolt::cl::cube, int, unsigned int );
BOLT_TEMPLATE_REGISTER_NEW_TYPE( bolt::cl::cube, int, float );
BOLT_TEMPLATE_REGISTER_NEW_TYPE( bolt::cl::cube, int, double );

BOLT_TEMPLATE_REGISTER_NEW_TYPE( bolt::cl::negate, int, unsigned int );
BOLT_TEMPLATE_REGISTER_NEW_TYPE( bolt::cl::negate, int, float );
BOLT_TEMPLATE_REGISTER_NEW_TYPE( bolt::cl::negate, int, double );

BOLT_TEMPLATE_REGISTER_NEW_TYPE( bolt::cl::plus, int, unsigned int );
BOLT_TEMPLATE_REGISTER_NEW_TYPE( bolt::cl::plus, int, float );
BOLT_TEMPLATE_REGISTER_NEW_TYPE( bolt::cl::plus, int, double );

BOLT_TEMPLATE_REGISTER_NEW_TYPE( bolt::cl::minus, int, unsigned int );
BOLT_TEMPLATE_REGISTER_NEW_TYPE( bolt::cl::minus, int, float );
BOLT_TEMPLATE_REGISTER_NEW_TYPE( bolt::cl::minus, int, double );

BOLT_TEMPLATE_REGISTER_NEW_TYPE( bolt::cl::multiplies, int, unsigned int );
BOLT_TEMPLATE_REGISTER_NEW_TYPE( bolt::cl::multiplies, int, float );
BOLT_TEMPLATE_REGISTER_NEW_TYPE( bolt::cl::multiplies, int, double );

BOLT_TEMPLATE_REGISTER_NEW_TYPE( bolt::cl::divides, int, unsigned int );
BOLT_TEMPLATE_REGISTER_NEW_TYPE( bolt::cl::divides, int, float );
BOLT_TEMPLATE_REGISTER_NEW_TYPE( bolt::cl::divides, int, double );

BOLT_TEMPLATE_REGISTER_NEW_TYPE( bolt::cl::modulus, int, unsigned int );
BOLT_TEMPLATE_REGISTER_NEW_TYPE( bolt::cl::modulus, int, float );
BOLT_TEMPLATE_REGISTER_NEW_TYPE( bolt::cl::modulus, int, double );

BOLT_TEMPLATE_REGISTER_NEW_TYPE( bolt::cl::maximum, int, unsigned int );
BOLT_TEMPLATE_REGISTER_NEW_TYPE( bolt::cl::maximum, int, float );
BOLT_TEMPLATE_REGISTER_NEW_TYPE( bolt::cl::maximum, int, double );

BOLT_TEMPLATE_REGISTER_NEW_TYPE( bolt::cl::minimum, int, unsigned int );
BOLT_TEMPLATE_REGISTER_NEW_TYPE( bolt::cl::minimum, int, float );
BOLT_TEMPLATE_REGISTER_NEW_TYPE( bolt::cl::minimum, int, double );

BOLT_TEMPLATE_REGISTER_NEW_TYPE( bolt::cl::bit_and, int, unsigned int );
BOLT_TEMPLATE_REGISTER_NEW_TYPE( bolt::cl::bit_and, int, float );
BOLT_TEMPLATE_REGISTER_NEW_TYPE( bolt::cl::bit_and, int, double );

BOLT_TEMPLATE_REGISTER_NEW_TYPE( bolt::cl::bit_or, int, unsigned int );
BOLT_TEMPLATE_REGISTER_NEW_TYPE( bolt::cl::bit_or, int, float );
BOLT_TEMPLATE_REGISTER_NEW_TYPE( bolt::cl::bit_or, int, double );

BOLT_TEMPLATE_REGISTER_NEW_TYPE( bolt::cl::bit_xor, int, unsigned int );
BOLT_TEMPLATE_REGISTER_NEW_TYPE( bolt::cl::bit_xor, int, float );
BOLT_TEMPLATE_REGISTER_NEW_TYPE( bolt::cl::bit_xor, int, double );

BOLT_TEMPLATE_REGISTER_NEW_TYPE( bolt::cl::logical_not, int, unsigned int );
BOLT_TEMPLATE_REGISTER_NEW_TYPE( bolt::cl::logical_not, int, float );
BOLT_TEMPLATE_REGISTER_NEW_TYPE( bolt::cl::logical_not, int, double );

BOLT_TEMPLATE_REGISTER_NEW_TYPE( bolt::cl::equal_to, int, unsigned int );
BOLT_TEMPLATE_REGISTER_NEW_TYPE( bolt::cl::equal_to, int, float );
BOLT_TEMPLATE_REGISTER_NEW_TYPE( bolt::cl::equal_to, int, double );

BOLT_TEMPLATE_REGISTER_NEW_TYPE( bolt::cl::not_equal_to, int, unsigned int );
BOLT_TEMPLATE_REGISTER_NEW_TYPE( bolt::cl::not_equal_to, int, float );
BOLT_TEMPLATE_REGISTER_NEW_TYPE( bolt::cl::not_equal_to, int, double );

BOLT_TEMPLATE_REGISTER_NEW_TYPE( bolt::cl::greater, int, unsigned int );
BOLT_TEMPLATE_REGISTER_NEW_TYPE( bolt::cl::greater, int, float );
BOLT_TEMPLATE_REGISTER_NEW_TYPE( bolt::cl::greater, int, double );

BOLT_TEMPLATE_REGISTER_NEW_TYPE( bolt::cl::less, int, unsigned int );
BOLT_TEMPLATE_REGISTER_NEW_TYPE( bolt::cl::less, int, float );
BOLT_TEMPLATE_REGISTER_NEW_TYPE( bolt::cl::less, int, double );

BOLT_TEMPLATE_REGISTER_NEW_TYPE( bolt::cl::greater_equal, int, unsigned int );
BOLT_TEMPLATE_REGISTER_NEW_TYPE( bolt::cl::greater_equal, int, float );
BOLT_TEMPLATE_REGISTER_NEW_TYPE( bolt::cl::greater_equal, int, double );

BOLT_TEMPLATE_REGISTER_NEW_TYPE( bolt::cl::less_equal, int, unsigned int );
BOLT_TEMPLATE_REGISTER_NEW_TYPE( bolt::cl::less_equal, int, float );
BOLT_TEMPLATE_REGISTER_NEW_TYPE( bolt::cl::less_equal, int, double );

BOLT_TEMPLATE_REGISTER_NEW_TYPE( bolt::cl::logical_and, int, unsigned int );
BOLT_TEMPLATE_REGISTER_NEW_TYPE( bolt::cl::logical_and, int, float );
BOLT_TEMPLATE_REGISTER_NEW_TYPE( bolt::cl::logical_and, int, double );

BOLT_TEMPLATE_REGISTER_NEW_TYPE( bolt::cl::logical_or, int, unsigned int );
BOLT_TEMPLATE_REGISTER_NEW_TYPE( bolt::cl::logical_or, int, float );
BOLT_TEMPLATE_REGISTER_NEW_TYPE( bolt::cl::logical_or, int, double );

#endif