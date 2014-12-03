/***************************************************************************
*   © 2012,2014 Advanced Micro Devices, Inc. All rights reserved.
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

///////////////////////////////////////////////////////////////////////////////
// AMP Replace
//////////////////////////////////////////////////////////////////////////////

#pragma once
#if !defined( BOLT_AMP_REPLACE_INL )
#define BOLT_AMP_REPLACE_INL

#include <bolt/amp/transform.h>
//#include <bolt/amp/functional.h>
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace bolt
{
namespace amp
{
        //////////////////////////////////////////
        //  Replace-If overloads
        ///////////////////////////////////////// 

        template<typename T>
        struct constant_unary
        {
          constant_unary(T _c):c(_c){}
        
          template<typename U>
          T operator() (U &x) const restrict(amp,cpu)
          {
            return c;
          } // end operator()()
        
          T c;
        }; // end constant_unary

		
		// this functor receives x, and returns a new_value if predicate(x) is true; otherwise,
        // it returns x
        template<typename Predicate, typename NewType, typename OutputType>
        struct new_value_if
        {
          new_value_if(Predicate p, NewType nv):pred(p),new_value(nv){}
        
          template<typename InputType>
          OutputType operator()(const InputType x) const restrict(amp,cpu)
          {
            return pred(x) ? new_value : x;
          } // end operator()()
        
          // this version of operator()() works like the previous but
          // feeds its second argument to pred
          template<typename InputType, typename PredicateArgumentType>
          OutputType operator()(const InputType x, const PredicateArgumentType y) const restrict(amp,cpu)
          {
            return pred(y) ? new_value : x;
          } // end operator()()
          
          Predicate pred;
          NewType new_value;
        }; // end new_value_if

        // default control, two-input transform, std:: iterator
        template<typename InputIterator, typename Predicate, typename T>
        void replace_if( bolt::amp::control& ctl,
                        InputIterator first1,
                        InputIterator last1,
						Predicate  	pred,
						const T &new_value)
        {
             // bolt::amp::detail::replace_if( ctl, first1, last1, result, f, pred);
			constant_unary<T> f(new_value);
            bolt::amp::transform_if(ctl, first1, last1, first1, first1, f, pred);
        }

        // default control, two-input transform, std:: iterator
        template<typename InputIterator, typename Predicate, typename T>
        void replace_if( InputIterator first1,
                        InputIterator last1,
						Predicate  	pred,
						const T &new_value)
        {
              replace_if( control::getDefault(), first1, last1, pred, new_value );
        }


		template<typename InputIterator1, typename InputIterator2, typename Predicate, typename T>
		void replace_if (bolt::amp::control& ctl, InputIterator1 first, InputIterator1 last, InputIterator2 stencil, Predicate pred, const T &new_value)
		{
			// bolt::amp::detail::replace_if_stencil (ctl, first, last, stencil, pred, new_value);
			constant_unary<T> f(new_value);
			bolt::amp::transform_if(ctl, first, last, stencil, first, f, pred);
		}

		template<typename InputIterator1, typename InputIterator2, typename Predicate, typename T>
		void  replace_if (InputIterator1 first, InputIterator1 last, InputIterator2 stencil, Predicate pred, const T &new_value)
		{
			 replace_if (control::getDefault(), first, last, stencil, pred, new_value);
		}


		template<typename InputIterator, typename T>
        void replace( bolt::amp::control& ctl,
			   InputIterator first,
               InputIterator last,
               const T &old_value,
               const T &new_value)
		{
			bolt::amp::equal_to_value<T> pred(old_value);
            replace_if(ctl, first, last, pred, new_value);
		}

		template<typename InputIterator, typename T>
        void replace( InputIterator first,
               InputIterator last,
               const T &old_value,
               const T &new_value)
		{
            replace(control::getDefault(), first, last, old_value, new_value);
		}


		template< typename InputIterator, typename OutputIterator, typename Predicate, typename T>
        OutputIterator replace_copy_if(control &ctl,
                                 InputIterator first,
                                 InputIterator last,
                                 OutputIterator result,
                                 Predicate pred,
                                 const T &new_value)
		{
			typedef typename std::iterator_traits<OutputIterator>::value_type oType;
            new_value_if<Predicate,T,oType> op(pred,new_value);
            bolt::amp::transform(ctl, first, last, result, op);
			return result;
		}

		template< typename InputIterator, typename OutputIterator, typename Predicate, typename T>
        OutputIterator replace_copy_if(InputIterator first,
                                 InputIterator last,
                                 OutputIterator result,
                                 Predicate pred,
                                 const T &new_value)
		{
			return replace_copy_if(control::getDefault(), first, last,  result, pred, new_value);
		}

		template<typename InputIterator1, typename InputIterator2, typename OutputIterator, typename Predicate, typename T>
        OutputIterator replace_copy_if(control &ctl,
                                 InputIterator1 first,
                                 InputIterator1 last,
                                 InputIterator2 stencil,
                                 OutputIterator result,
                                 Predicate pred,
                                 const T &new_value)
		{
			typedef typename std::iterator_traits<OutputIterator>::value_type OutputType;
            new_value_if<Predicate,T,OutputType> op(pred,new_value);
            bolt::amp::transform(ctl, first, last, stencil, result, op);
			return result;
		}

		template<typename InputIterator1, typename InputIterator2, typename OutputIterator, typename Predicate, typename T>
        OutputIterator replace_copy_if(InputIterator1 first,
                                 InputIterator1 last,
                                 InputIterator2 stencil,
                                 OutputIterator result,
                                 Predicate pred,
                                 const T &new_value)
		{
			return replace_copy_if(control::getDefault(), first, last,  stencil, result, pred, new_value);
		}


		template< typename InputIterator, typename OutputIterator, typename T>
        OutputIterator replace_copy(control &ctl,
			                  InputIterator first,
                              InputIterator last,
                              OutputIterator result,
                              const T &old_value,
                              const T &new_value)
		{
			bolt::amp::equal_to_value<T> pred(old_value);
            return replace_copy_if(ctl, first, last, result, pred, new_value);
		}


		template<typename InputIterator, typename OutputIterator, typename T>
        OutputIterator replace_copy(InputIterator first,
                              InputIterator last,
                              OutputIterator result,
                              const T &old_value,
                              const T &new_value)
		{
			return replace_copy(control::getDefault(), first, last, result, old_value, new_value);
		}



    } //end of namespace amp
} //end of namespace bolt

#endif // AMP_REPLACE_INL