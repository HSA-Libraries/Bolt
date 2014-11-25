/***************************************************************************                                                                                      
   Copyright 2012 - 2013 Advanced Micro Devices, Inc.                                      
                                                                                     
   Licensed under the Apache License, Version 2.0 (the "License");    
   you may not use this file except in compliance with the License.                  
   You may obtain a copy of the License at                                           
                                                                                     
       http://www.apache.org/licenses/LICENSE-2.0                       
                                                                                     
*   Unless required by applicable law or agreed to in writing, software               
*   distributed under the License is distributed on an "AS IS" BASIS,               
*   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.          
*   See the License for the specific language governing permissions and               
*   limitations under the License.                                                    


***************************************************************************/                                                                                      


#if !defined( BOLT_AMP_FOR_EACH_H ) 
#define BOLT_AMP_FOR_EACH_H 
#pragma once 


#include <bolt/amp/bolt.h> 


/*! \file bolt/amp/for_each.h 
    \brief  for_each applies the function object f to each element in the range [first, last); f's return value, if any, is ignored.  
	\brief Unlike the C++ Standard Template Library function std::for_each, this version offers no guarantee on order of execution.  
	\brief For this reason, this version of for_each does not return a copy of the function object.. 
*/ 
namespace bolt 
{ 
namespace amp 
{ 


/*! \addtogroup algorithms 
 */ 


/*! \addtogroup transformations
*   \ingroup algorithms 
*\p  Iterates over a range and applies function specified on each element in range.
*/  

/*! \addtogroup AMP-for_each
*   \ingroup transformations
*   \{
*/

/*! \brief  ForEach applies the unary function \c f to each element in the range [first,last] without modifying the input sequence.
*
*  \param ctl      \b Optional Control structure to control accelerator, debug, tuning, etc.See bolt::amp::control.
*  \param first    The first element in the range of interest.
*  \param last     The last element in the range of interest.
*  \param f        Unary Function to apply to elements in the range [first,last].
*
*  \tparam InputIterator is a model of \c InputIterator is mutable.

* \details The following code snippet demonstrates how to  use for_each with a device_vector..
*
*  \code
*  #include <bolt/amp/for_each.h>
*  #include <bolt/amp/device_vector.h>
*  #include <stdlib.h>
*  ...
*  template< typename T >
*  struct unary_op
*  {
*  
*  void operator()( T &x ) const restrict(amp, cpu)
*  {
*  	   x+=2;
*  
*  }
*  };
*  std::vector<int> sv(10, rand()%10);
*  bolt::amp::device_vector<int> v(sv.begin(), sv.end());
*
*  bolt::amp::for_each(v.begin(), v.end(), unary_op<int>());
*
*  // the elements of v are printed.
*  \endcode
*
*  \sa http://www.sgi.com/tech/stl/for_each.html
*/



template<typename InputIterator , typename UnaryFunction >   
InputIterator for_each (InputIterator first, InputIterator last, UnaryFunction f); 


template<typename InputIterator , typename UnaryFunction >   
InputIterator  for_each (control &ctl, InputIterator first, InputIterator last, UnaryFunction f); 




/*! \brief  ForEach_n applies the unary function \c f to each element in the range [first,first+n] without modifying the input sequence.
*
*  \param ctl      \b Optional Control structure to control accelerator, debug, tuning, etc.See bolt::amp::control.
*  \param first    The first element in the range of interest.
*  \param n        The size of the range of interest.
*  \param f        Unary Function to apply to elements in the range [first,first+n].
*
*  \tparam InputIterator is a model of \c InputIterator is mutable.

* \details The following code snippet demonstrates how to  use for_each_n with a device_vector..
*
*  \code
*  #include <bolt/amp/for_each.h>
*  #include <bolt/amp/device_vector.h>
*  #include <stdlib.h>
*  ...
*  template< typename T >
*  struct unary_op
*  {
*  
*  void operator()( T &x ) const restrict(amp, cpu)
*  {
*  	 x+=2;
*  
*  }
*  };
*  std::vector<int> sv(10, rand()%10);
*  bolt::amp::device_vector<float> v(sv.begin(), sv.end());
*  int n = 5;
*  bolt::amp::for_each_n(v.begin(), n, unary_op<int>());
*
*  // First 5 elements of v are printed.
*  \endcode
*
*  \sa http://www.sgi.com/tech/stl/for_each.html
*/

template<typename InputIterator , typename Size , typename UnaryFunction >  
InputIterator for_each_n  ( InputIterator  first,  Size  n,  UnaryFunction  f);   


 

 template<typename InputIterator , typename Size , typename UnaryFunction >  
 InputIterator for_each_n  ( control &ctl, InputIterator  first,  Size  n,  UnaryFunction  f);   
 

 /*!   \}  */ 
 }// end of bolt::amp namespace 
 }// end of bolt namespace 
 

 #include <bolt/amp/detail/for_each.inl> 
 

 #endif 
