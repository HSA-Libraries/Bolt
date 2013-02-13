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

namespace bolt {
namespace amp {
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


    template<typename T>
    struct greater
    {
        bool operator()(const T &lhs, const T &rhs) const  restrict(cpu,amp) {return lhs > rhs;}
    }; 



    template<typename T>
    struct less
    {
        bool operator()(const T &lhs, const T &rhs) const  restrict(cpu,amp) {return lhs < rhs;}
    }; 

};//End of amp namespace
};//End of bolt namespace
