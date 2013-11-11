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

#if !defined( BOLT_BTBB_REDUCE_BY_KEY_INL)
#define BOLT_BTBB_REDUCE_BY_KEY_INL
#pragma once

#include "bolt/cl/scan.h"
#include "bolt/cl/scan_by_key.h"
#include "tbb/task_scheduler_init.h"
#include <iterator>
#include "tbb/blocked_range.h"
#include "tbb/parallel_for.h"
#include <iostream>
using namespace std;

namespace bolt
{
    namespace btbb 
    {
 
             template<
                 typename InputIterator1,
                 typename InputIterator2,
                 typename OutputIterator1,
                 typename OutputIterator2,
                 typename BinaryPredicate,
                 typename BinaryFunction>

           unsigned int reduce_by_key( 
                            InputIterator1 keys_first,
                            InputIterator1 keys_last,
                            InputIterator2 values_first,
                            OutputIterator1 keys_output,
                            OutputIterator2 values_output,
                            BinaryPredicate binary_pred,
                            BinaryFunction binary_op )
             { 
                unsigned int numElements = static_cast< int >( std::distance( keys_first, keys_last ));
                //This allows TBB to choose the number of threads to spawn.
                tbb::task_scheduler_init initialize(tbb::task_scheduler_init::automatic);

                typedef typename std::iterator_traits< OutputIterator2 >::value_type voType;   
              
                int *temKeyOutput = (int*)calloc(sizeof(int), numElements);  
                voType *temValueOutput = (voType*)calloc(sizeof(voType), numElements); 

                 tbb::parallel_for (tbb::blocked_range<size_t>(0,numElements),[&](const tbb::blocked_range<size_t>& r)
                  {
                    for(size_t iter = r.begin(); iter!=r.end(); iter++)
                        {   if(iter == 0)
                            {  
                                temKeyOutput[iter] = 0;

                            }
                            else if(binary_pred( keys_first[iter], keys_first[iter-1]))
                            
                                temKeyOutput[iter] = 0;
                            else 
                                temKeyOutput[iter] = 1;
                        }
                  }); 
       
                   bolt::cl::control ctl = bolt::cl::control::getDefault( );
                   ctl.setForceRunMode(bolt::cl::control::MultiCoreCpu); 

                   bolt::cl::inclusive_scan(ctl, temKeyOutput,  temKeyOutput  + numElements , temKeyOutput,
					                                                               bolt::cl::plus<int>() );

                   bolt::cl::inclusive_scan_by_key(ctl, temKeyOutput, temKeyOutput  + numElements,
					          values_first, temValueOutput, bolt::cl::equal_to<int>(), binary_op);

                 tbb::parallel_for (tbb::blocked_range<size_t>(0,numElements),[&](const tbb::blocked_range<size_t>& s)
                  {
                    for(size_t iter = s.begin(); iter!=s.end(); iter++)
                    {
                        if(temKeyOutput[iter] != temKeyOutput[iter+1])
                             values_output[temKeyOutput[iter]] = temValueOutput[iter];

                        keys_output[temKeyOutput[iter]] = keys_first[iter];
                    }
                    }); 

                   int count = temKeyOutput[numElements-1] + 1;

                    free(temKeyOutput);
                    free(temValueOutput);

              return (count) ;//count; 
           }       
    } //tbb
} // bolt

#endif //BTBB_REDUCE_BY_KEY_INL
