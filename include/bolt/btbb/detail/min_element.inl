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

#if !defined( BOLT_BTBB_MIN_ELEMENT_INL)
#define BOLT_BTBB_MIN_ELEMENT_INL
#pragma once

#include "tbb/task_scheduler_init.h"
#include "tbb/parallel_for.h"
#include "tbb/blocked_range.h"
#include <thread>
#include <iterator>

#include<iostream>
namespace bolt{
    namespace btbb {

             template<typename ForwardIterator>
             struct Min_Element 
             {

               ForwardIterator result;

               Min_Element (): result(0) {}
               Min_Element (ForwardIterator _x ): result(_x) {}

               void operator()( ForwardIterator first, ForwardIterator last)
                {
                    if (first == last) 
                        result = last;
                    else
                    {   
                      result = first;

                      tbb::parallel_for(  tbb::blocked_range<ForwardIterator>(first, last) ,
                        [&] (const tbb::blocked_range<ForwardIterator> &r) -> void
                        {
                              for(ForwardIterator a = r.begin(); a!=r.end(); ++a)
                              {
                                   if (*a<*result)  
                                      result = a;

                              }   
                          
                        });
                    }
                }

            };


            template<typename ForwardIterator, typename BinaryPredicate>
            struct Min_Element_comp 
            {

               ForwardIterator result;

               Min_Element_comp (): result(0) {}
               Min_Element_comp (ForwardIterator _x ): result(_x) {}

               void operator()( ForwardIterator first, ForwardIterator last,BinaryPredicate comp )
                {
                    if (first == last) 
                        result = last;
                    else
                    {    
                      result = first;

                      tbb::parallel_for(  tbb::blocked_range<ForwardIterator>(first, last) ,
                        [&] (const tbb::blocked_range<ForwardIterator> &r) -> void
                        {
                              for(ForwardIterator a = r.begin(); a!=r.end(); ++a)
                              {
                                     if (comp (*a, *result)) 
                                       result = a;

                              }   
                          
                        });
                    }
                }

            };

            template<typename ForwardIterator>
            struct Max_Element 
            {

               ForwardIterator result;

               Max_Element (): result(0) {}
               Max_Element (ForwardIterator _x ): result(_x) {}

               void operator()( ForwardIterator first, ForwardIterator last)
                {
                    if (first == last) 
                        result = last;
                    else
                    {   
                      result = first;

                      tbb::parallel_for(  tbb::blocked_range<ForwardIterator>(first, last) ,
                        [&] (const tbb::blocked_range<ForwardIterator> &r) -> void
                        {
                              for(ForwardIterator a = r.begin(); a!=r.end(); ++a)
                              {
                                   if (*a>*result)  
                                      result = a;

                              }   
                          
                        });
                    }
                }

           };

           template<typename ForwardIterator>
           ForwardIterator min_element(ForwardIterator first, ForwardIterator last)
           {
             //Gets the number of concurrent threads supported by the underlying platform
             unsigned int concurentThreadsSupported = std::thread::hardware_concurrency();

             //This allows TBB to choose the number of threads to spawn.
             tbb::task_scheduler_init initialize(tbb::task_scheduler_init::automatic);

             //Explicitly setting the number of threads to spawn
             //tbb::task_scheduler_init((int) concurentThreadsSupported);

             Min_Element <ForwardIterator> min_element_op(first);
             min_element_op(first, last);

             return min_element_op.result;
          }


          template<typename ForwardIterator,typename BinaryPredicate>
          ForwardIterator min_element(ForwardIterator first, ForwardIterator last, BinaryPredicate binary_op)
          {
             //Gets the number of concurrent threads supported by the underlying platform
             unsigned int concurentThreadsSupported = std::thread::hardware_concurrency();

             //This allows TBB to choose the number of threads to spawn.
             tbb::task_scheduler_init initialize(tbb::task_scheduler_init::automatic);

             //Explicitly setting the number of threads to spawn
             //tbb::task_scheduler_init((int) concurentThreadsSupported);

             Min_Element_comp <ForwardIterator, BinaryPredicate> min_element_op(first);
             min_element_op(first, last, binary_op);

             return min_element_op.result;
          }

          template<typename ForwardIterator>
          ForwardIterator max_element(ForwardIterator first, ForwardIterator last)
          {
             //Gets the number of concurrent threads supported by the underlying platform
             unsigned int concurentThreadsSupported = std::thread::hardware_concurrency();

             //This allows TBB to choose the number of threads to spawn.
             tbb::task_scheduler_init initialize(tbb::task_scheduler_init::automatic);

             //Explicitly setting the number of threads to spawn
             //tbb::task_scheduler_init((int) concurentThreadsSupported);

             Max_Element <ForwardIterator> max_element_op(first);
             max_element_op(first, last);

             return max_element_op.result;
          }
 
    } //tbb
} // bolt

#endif //BTBB_MIN_ELEMENT_INL