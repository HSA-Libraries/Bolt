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

#include "tbb/task_scheduler_init.h"
#include <iterator>
#include "tbb/blocked_range.h"
#include "tbb/parallel_for.h"

namespace bolt
{
    namespace btbb 
    {
                      
    template<
    typename InputIterator1,
    typename InputIterator2,
    typename OutputIterator1,
    typename OutputIterator2,
    typename voType,
    typename BinaryPredicate,
    typename BinaryFunction>

     struct ReduceByKey
      {
          typedef typename std::iterator_traits< OutputIterator2 >::value_type voType;          
          voType sum;          
          InputIterator1& first_key;
          InputIterator1& last_key;
          InputIterator2& first_value;
          OutputIterator1& keys_output;
          voType *values_output; 
          int *offset_array;
          unsigned int numElements, strt_indx, end_indx;
          const BinaryPredicate binary_pred;
          const BinaryFunction binary_op;        
          bool flag, pre_flag, next_flag;          
        
          public:
          ReduceByKey() : sum(voType()){}
          ReduceByKey( InputIterator1&  _first,
            InputIterator1&  _last,
            InputIterator2& first_val,
            OutputIterator1& _keys_output,
            voType *_values_output,
            int *_offset_array,
            unsigned int _numElements,
            const BinaryPredicate &_pred, 
            const BinaryFunction &_opr) : first_key(_first),last_key(_last), first_value(first_val), 
            keys_output(_keys_output),values_output(_values_output), offset_array(_offset_array),
            numElements(_numElements), binary_pred(_pred), binary_op(_opr), flag(FALSE), pre_flag(TRUE),
            next_flag(FALSE){}
          voType get_sum() const {return sum;}         
        
          template<typename Tag>
          void operator()( const tbb::blocked_range<unsigned int>& r, Tag ) 
          { 
              voType temp = sum;              
              next_flag = flag = FALSE;               
              unsigned int i;
               strt_indx = r.begin();
               end_indx = r.end();
              for( i=r.begin(); i<r.end(); ++i ) 
              {
                 if( Tag::is_final_scan() )
                 {
                    if(i == 0 )
                        temp = *(first_value+i);                      
                    else if(binary_pred(*(first_key+i), *(first_key +i- 1)))
                        temp = binary_op(temp, *(first_value+i));
                    else
                    {  
                        offset_array[i-1]  = 1;
                        *(values_output + (i-1)) = temp;                     
                        temp = *(first_value+i);   
                        flag = TRUE;                       
                     }                     
                     if(i==numElements-1)
                     {                  
                         offset_array[i]  = 1;
                         *(values_output + i) = temp;  
                     }
                 }
                 else if(pre_flag)
                 {
                    temp = *(first_value+i);
                    pre_flag = FALSE;
                 }
                 else if(binary_pred(*(first_key+i), *(first_key +i - 1)))
                         temp = binary_op(temp, *(first_value+i));
                 else
                 { 
                     temp = *(first_value+i);
                     flag = TRUE;
                 }
             }
               if(i<numElements && !binary_pred(*(first_key+i-1), *(first_key +i )))
                   next_flag = TRUE; // this will check the key change at boundaries
               sum = temp;
          }
          ReduceByKey( ReduceByKey& b, tbb::split): first_key(b.first_key),last_key(b.last_key), first_value(b.first_value),
                                                    keys_output(b.keys_output),values_output(b.values_output),
                                                    offset_array(b.offset_array),numElements(b.numElements),sum(voType()),
                                                    pre_flag(TRUE){}
         void reverse_join( ReduceByKey& a )
         {
             if(!a.next_flag && !flag && binary_pred(*(a.first_key +  a.end_indx),*(first_key+strt_indx))) 
                  sum = binary_op(a.sum,sum);
         }

          void assign( ReduceByKey& b) 
          {
             sum = b.sum;           
          }
      };

             template<
                 typename InputIterator1,
                 typename InputIterator2,
                 typename OutputIterator1,
                 typename OutputIterator2,
                 typename BinaryPredicate,
                 typename BinaryFunction>

           unsigned int reduce_by_key( InputIterator1 keys_first,
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
                voType *temValOutput = (voType*)calloc(sizeof(voType), numElements);
                 
                int *offset_array = (int*)calloc(sizeof(int), numElements);

                ReduceByKey<InputIterator1,InputIterator2, OutputIterator1, OutputIterator2, voType, BinaryPredicate,BinaryFunction>
                reduce_by_key_op((InputIterator1 &)keys_first, (InputIterator1 &)keys_last, (InputIterator2 &)values_first,
                (OutputIterator1 &)keys_output, temValOutput, offset_array, numElements, binary_pred, binary_op);
                
                tbb::parallel_scan( tbb::blocked_range<unsigned int>(  0, static_cast< int >( std::distance( keys_first, keys_last ))),reduce_by_key_op, tbb::auto_partitioner());             
                unsigned int count = 0;               
                unsigned int j;

              for(unsigned int i = 0; i < numElements; i++)
              {
                  if(offset_array[i] != 0)
                  {
                    *(values_output+count) = temValOutput[i];            
                    count++;
                  }
                  j = i+1;
                  if(j<numElements)
                  {
                      if(!binary_pred(*(keys_first+j-1),*(keys_first+j)))
                     {
                        *(keys_output) = *(keys_first+j-1);                 
                        keys_output++;
                        *(keys_output) = *(keys_first+j);
                     }
                    if(j==numElements-1)
                        *(keys_output)   = *(keys_first +j); 
                 }
              }            
              free(temValOutput);
              free(offset_array);
              return count; 
           }       
    } //tbb
} // bolt

#endif //BTBB_REDUCE_BY_KEY_INL