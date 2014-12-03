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

#if !defined( BOLT_BTBB_FIND_INL)
#define BOLT_BTBB_FIND_INL
#pragma once

#include "tbb/task_scheduler_init.h"
#include "tbb/parallel_for.h"
#include "tbb/blocked_range.h"
#include <iterator>
#include <bolt/btbb/min_element.h>
#include <bolt/amp/functional.h>

namespace bolt{
    namespace btbb {

             template<typename InputIterator, typename Predicate>
             struct find
			 {
               find () {}
				void operator()( InputIterator& first, int n, unsigned int* result, Predicate& pred)
                {
				    
                      tbb::parallel_for(  tbb::blocked_range<int>(0, (int) n) ,
                        [&] (const tbb::blocked_range<int> &r) -> void
                        {
                              
                              for(int i = r.begin(); i!=r.end(); i++)
                              {
							     if(pred(*(first+i))) 
									*(result+i) = i;
								 else
                                   *(result+i) = n;
							   }
                          
                        });

                }
            };

            template<typename InputIterator, typename Predicate>
			InputIterator find_if( InputIterator first,
								   InputIterator last,
								   Predicate& pred
							     )
            {
               
               //This allows TBB to choose the number of threads to spawn.
               tbb::task_scheduler_init initialize(tbb::task_scheduler_init::automatic);
			   int szElements = static_cast< int >( std::distance( first, last ) );
			   std::vector<unsigned int> index(szElements);

               find<InputIterator, Predicate> find_op;
               find_op(first, szElements, &index[0], pred);

			   std::vector<unsigned int>::iterator itr = bolt::btbb::min_element( index.begin(), index.end(), bolt::amp::less<unsigned int>());
			   return first + itr[0];
            }

       
    } //tbb
} // bolt

#endif //BTBB_FIND_INL