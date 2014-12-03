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
#if !defined( BOLT_AMP_LOGICAL_INL )
#define BOLT_AMP_LOGICAL_INL

#include "bolt/amp/find.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace bolt
{
namespace amp
{

	    template <typename Predicate>
        struct unary_negate
        {
            typedef bool result_type;
        
            Predicate pred;
        
            unary_negate(const Predicate& pred) : pred(pred) {}
        
            template <typename T>
            bool operator()(const T& x) const restrict(amp,cpu)
            {
                return !bool(pred(x));
            }
        };
       
	    template<typename Predicate>
        bolt::amp::unary_negate<Predicate> not1(const Predicate &pred)
        {
            return bolt::amp::unary_negate<Predicate>(pred);
        }

		template<typename InputIterator, typename Predicate>
        bool all_of(bolt::amp::control &ctl, InputIterator first, InputIterator last, Predicate pred)
		{
			return bolt::amp::find_if(ctl, first, last, bolt::amp::unary_negate<Predicate>(pred)) == last;
		}

		template<typename InputIterator, typename Predicate>
        bool all_of(InputIterator first, InputIterator last, Predicate pred)
		{
			return all_of(control::getDefault(), first, last, pred);
		}


		template<typename InputIterator, typename Predicate>
        bool any_of(bolt::amp::control &ctl, InputIterator first, InputIterator last, Predicate pred)
		{
			return bolt::amp::find_if(ctl, first, last, pred) != last;
		}

		template<typename InputIterator, typename Predicate>
        bool any_of(InputIterator first, InputIterator last, Predicate pred)
		{
			return any_of(control::getDefault(), first, last, pred);
		}

		template<typename InputIterator, typename Predicate>
        bool none_of(bolt::amp::control &ctl, InputIterator first, InputIterator last, Predicate pred)
		{
		    return !any_of(ctl, first, last, pred);
		}

		template<typename InputIterator, typename Predicate>
        bool none_of(InputIterator first, InputIterator last, Predicate pred)
		{
		    return none_of(control::getDefault(), first, last, pred);
		}

    } //end of namespace amp
} //end of namespace bolt

#endif // AMP_LOGICAL_INL