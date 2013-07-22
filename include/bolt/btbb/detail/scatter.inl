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

/*! \file bolt/btbb/scatter.h
*/


#if !defined( BOLT_BTBB_SCATTER_INL )
#define BOLT_BTBB_SCATTER_INL
#pragma once

#include "tbb/task_scheduler_init.h"
#include "tbb/parallel_for.h"
#include "tbb/blocked_range.h"
#include <thread>

namespace bolt 
{
    namespace btbb
    {

template<typename InputIterator1,
         typename InputIterator2, 
         typename OutputIterator>

void scatter(InputIterator1 first1, 
             InputIterator1 last1,
             InputIterator2 map, 
             OutputIterator result)
             { 
                // std::cout<<"TBB code...\n";
                 size_t numElements = static_cast< unsigned int >( std::distance( first1, last1 ) );
                 //Gets the number of concurrent threads supported by the underlying platform
                unsigned int concurentThreadsSupported = std::thread::hardware_concurrency();

               //This allows TBB to choose the number of threads to spawn.
               //tbb::task_scheduler_init initialize(tbb::task_scheduler_init::automatic);

               //Explicitly setting the number of threads to spawn
                 tbb::task_scheduler_init((int) concurentThreadsSupported);
                 tbb::parallel_for (tbb::blocked_range<size_t>(0,numElements),[&](const tbb::blocked_range<size_t>& r)
                 {
                    for(size_t iter = r.begin(); iter!=r.end(); iter++)
                             result[*(map+(int)iter)] = first1[(int)iter];
                 });
             }

template<typename InputIterator1,
         typename InputIterator2,
         typename InputIterator3,
         typename OutputIterator>

 void scatter_if( InputIterator1 first1,
                  InputIterator1 last1,
                  InputIterator2 map,
                  InputIterator3 stencil,
                  OutputIterator result)
        {
               //  std::cout<<"TBB code...\n";
                 size_t numElements = static_cast< unsigned int >( std::distance( first1, last1 ) );
                 //Gets the number of concurrent threads supported by the underlying platform
                unsigned int concurentThreadsSupported = std::thread::hardware_concurrency();

               //This allows TBB to choose the number of threads to spawn.
               //tbb::task_scheduler_init initialize(tbb::task_scheduler_init::automatic);

               //Explicitly setting the number of threads to spawn
                 tbb::task_scheduler_init((int) concurentThreadsSupported);
                 tbb::parallel_for (tbb::blocked_range<size_t>(0,numElements),[&](const tbb::blocked_range<size_t>& r)
                 {
                    for(size_t iter = r.begin(); iter!=r.end(); iter++)
                    {
                        if(stencil[iter] == 1)
                            result[*(map+(int)iter)] = first1[(int)iter];
                    }                            
                 });
        }


template<typename InputIterator1,
         typename InputIterator2,
         typename InputIterator3,
         typename OutputIterator,
         typename BinaryPredicate>

 void scatter_if( InputIterator1 first1,
                  InputIterator1 last1,
                  InputIterator2 map,
                  InputIterator3 stencil,
                  OutputIterator result,
                  BinaryPredicate pred)
        {
                // std::cout<<"TBB code...\n";
                 size_t numElements = static_cast< unsigned int >( std::distance( first1, last1 ) );
                 //Gets the number of concurrent threads supported by the underlying platform
                unsigned int concurentThreadsSupported = std::thread::hardware_concurrency();

               //This allows TBB to choose the number of threads to spawn.
               //tbb::task_scheduler_init initialize(tbb::task_scheduler_init::automatic);

               //Explicitly setting the number of threads to spawn
                 tbb::task_scheduler_init((int) concurentThreadsSupported);
                 tbb::parallel_for (tbb::blocked_range<size_t>(0,numElements),[&](const tbb::blocked_range<size_t>& r)
                 {
                    for(size_t iter = r.begin(); iter!=r.end(); iter++)
                    {
                       if(pred(stencil[(int)iter]))
                            result[*(map+((int)iter))] = first1[(int)iter];
                    }                            
                 });
        }

    }
}

#endif // TBB_SCATTER_INL


