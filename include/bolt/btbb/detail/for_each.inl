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

/*! \file bolt/amp/for_each.h
	\brief  Iterates over the range of input elements and applies the unary function specified over them.
*/

#if !defined( BOLT_BTBB_FOR_EACH_INL)
#define BOLT_BTBB_FOR_EACH_INL
#pragma once

#include "tbb/task_scheduler_init.h"
#include "tbb/parallel_for.h"
#include "tbb/blocked_range.h"

namespace bolt{
    namespace btbb {

             template<typename InputIterator, typename  UnaryFunction>
             struct ForEach
             {
                ForEach () {}

                void operator()(InputIterator first,  InputIterator last, UnaryFunction f)
                {

                    tbb::parallel_for(  tbb::blocked_range<InputIterator>(first, last) ,
                        [=] (const tbb::blocked_range<InputIterator> &r) -> void
                        {
                              for(InputIterator a = r.begin(); a!=r.end(); a++)
                                 f(*a);
                        });
                }

            };
      

			template<typename InputIterator, typename Size, typename  UnaryFunction>
            struct ForEach_n {

                ForEach_n () {}

				void operator()( InputIterator first, Size n, UnaryFunction f)
                {
                    tbb::parallel_for(  tbb::blocked_range<int>(0, (int) n) ,
                        [&] (const tbb::blocked_range<int> &r) -> void
                        {
                              
                              for(int i = r.begin(); i!=r.end(); i++)
                              {
                                 
                                   f(*(first+i));
                              }   
                          
                        });

                }


            };

           
		    template<typename InputIterator , typename UnaryFunction >   
            void for_each (InputIterator first, InputIterator last, UnaryFunction f)
		    {
		    	 //This allows TBB to choose the number of threads to spawn.
                 tbb::task_scheduler_init initialize(tbb::task_scheduler_init::automatic);
		    
                 ForEach <InputIterator, UnaryFunction> for_each_op;
                 for_each_op(first, last, f);
		    }
		    
		    template<typename InputIterator , typename Size , typename UnaryFunction >  
            void for_each_n  ( InputIterator  first,  Size  n,  UnaryFunction  f)
		    {

				//This allows TBB to choose the number of threads to spawn.
                 tbb::task_scheduler_init initialize(tbb::task_scheduler_init::automatic);
		    
                 ForEach_n <InputIterator, Size, UnaryFunction> for_each_op;
                 for_each_op(first,  n, f);
		    }
       
    } //tbb
} // bolt

#endif //BTBB_FOR_EACH_INL