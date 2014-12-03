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
/*THIS FILE HAS BEEN MODIFIED BY AMD*/ 
/*
 *  Copyright 2008-2012 NVIDIA Corporation
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#if !defined( BOLT_BTBB_COPY_INL)
#define BOLT_BTBB_COPY_INL
#pragma once

#include "tbb/task_scheduler_init.h"
#include "tbb/parallel_for.h"
#include "tbb/parallel_scan.h"
#include "tbb/blocked_range.h"
#include <iterator>

namespace bolt{
    namespace btbb {

             template<typename InputIterator, typename Size, typename OutputIterator>
             struct Copy_n {

               Copy_n () {}

				void operator()( InputIterator first, Size n, OutputIterator result)
                {
                    tbb::parallel_for(  tbb::blocked_range<int>(0, (int) n) ,
                        [&] (const tbb::blocked_range<int> &r) -> void
                        {
                              
                              for(int i = r.begin(); i!=r.end(); i++)
                              {
                                 
                                   *(result+i) = *(first+i);
                              }   
                          
                        });

                }


            };


            template<typename InputIterator, typename Size, typename OutputIterator>
            OutputIterator copy_n(InputIterator first, Size n, OutputIterator result)
            {
               //Gets the number of concurrent threads supported by the underlying platform
               //unsigned int concurentThreadsSupported = std::thread::hardware_concurrency();

               //This allows TBB to choose the number of threads to spawn.
               tbb::task_scheduler_init initialize(tbb::task_scheduler_init::automatic);

               //Explicitly setting the number of threads to spawn
               //tbb::task_scheduler_init((int) concurentThreadsSupported);

               Copy_n <InputIterator, Size, OutputIterator> copy_op;
               copy_op(first, n, result);

               return result;
            }


		template<typename InputIterator1,
				 typename InputIterator2,
				 typename OutputIterator,
				 typename Predicate,
				 typename Size>
		struct body
		{

		  InputIterator1 first;
		  InputIterator2 stencil;
		  OutputIterator result;
		  Predicate pred;
		  Size sum;

		  body(InputIterator1 first, InputIterator2 stencil, OutputIterator result, Predicate pred)
			: first(first), stencil(stencil), result(result), pred(pred), sum(0)
		  {}

		  body(body& b, tbb::split)
			: first(b.first), stencil(b.stencil), result(b.result), pred(b.pred), sum(0)
		  {}

		  void operator()(const tbb::blocked_range<Size>& r, tbb::pre_scan_tag)
		  {
			InputIterator2 iter = stencil + r.begin();

			for (Size i = r.begin(); i != r.end(); ++i, ++iter)
			{
			  if (pred(*iter))
				++sum;
			}
		  }
  
		  void operator()(const tbb::blocked_range<Size>& r, tbb::final_scan_tag)
		  {
			InputIterator1  iter1 = first   + r.begin();
			InputIterator2  iter2 = stencil + r.begin();
			OutputIterator  iter3 = result  + sum;
      
			for (Size i = r.begin(); i != r.end(); ++i, ++iter1, ++iter2)
			{
			  if (pred(*iter2))
			  {
				*iter3 = *iter1;
				++sum;
				++iter3;
			  }
			}
		  }

		  void reverse_join(body& b)
		  {
			sum = b.sum + sum;
		  } 

		  void assign(body& b)
		  {
			sum = b.sum;
		  } 
		}; // end body


		template<typename InputIterator1, typename InputIterator2, typename OutputIterator, typename Predicate>
		struct Copy_If {

				   Copy_If () {}
		       
				   OutputIterator operator()( InputIterator1 first, InputIterator1 last,
								 InputIterator2 stencil, OutputIterator result, Predicate pred)
				   {  
		       			   typedef int Size;
							  typedef  body<InputIterator1,InputIterator2,OutputIterator,Predicate,Size> Body;
							  Size n = std::distance(first, last);
	                     
							  if (n != 0)
							  {
								  Body body(first, stencil, result, pred);
								  tbb::parallel_scan(tbb::blocked_range<Size>(0,n), body);
		       					  result = result + body.sum;
		       					  return result;
							  }
	                     
		       			   return result;
				   }

			};	


			template<typename InputIterator1, typename InputIterator2, typename OutputIterator, typename Predicate>
			OutputIterator copy_if(InputIterator1 first, InputIterator1 last,
						  InputIterator2 stencil, OutputIterator result, Predicate pred)
			{
				   //Gets the number of concurrent threads supported by the underlying platform
				   //unsigned int concurentThreadsSupported = std::thread::hardware_concurrency();

				   //This allows TBB to choose the number of threads to spawn.
				   tbb::task_scheduler_init initialize(tbb::task_scheduler_init::automatic);

				   //Explicitly setting the number of threads to spawn
				   //tbb::task_scheduler_init((int) concurentThreadsSupported);

				   Copy_If <InputIterator1, InputIterator2, OutputIterator, Predicate> copy_op;
				   return copy_op(first, last, stencil, result, pred);

			}


    } //btbb
} // bolt

#endif //BTBB_COPY_INL