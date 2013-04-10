/***************************************************************************                                                                                     
*   Copyright 2012 - 2013 Advanced Micro Devices, Inc.                                     
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

#if !defined( COUNT_INL )
#define COUNT_INL
#pragma once

#include <algorithm>

#include "bolt/amp/bolt.h"
#include "bolt/amp/functional.h"
#include "bolt/amp/device_vector.h"
#ifdef ENABLE_TBB
//TBB Includes
#include "tbb/parallel_reduce.h"
#include "tbb/blocked_range.h"
#include "tbb/task_scheduler_init.h"
#endif

#define _REDUCE_STEP(_LENGTH, _IDX, _W) \
    if ((_IDX < _W) && ((_IDX + _W) < _LENGTH)) {\
      scratch_count[_IDX] =  scratch_count[_IDX] + scratch_count[_IDX + _W];\
    }\
    t_idx.barrier.wait();

#define WAVEFRONT_SIZE 64

namespace bolt {
    namespace amp {

       template<typename InputIterator, typename Predicate> 
        typename bolt::amp::iterator_traits<InputIterator>::difference_type
            count_if(control& ctl, InputIterator first, 
            InputIterator last, 
            Predicate predicate)
        {
              return detail::count_detect_random_access(ctl, first, last, predicate,
                std::iterator_traits< InputIterator >::iterator_category( ) );

        }

       template<typename InputIterator, typename Predicate> 
        typename bolt::amp::iterator_traits<InputIterator>::difference_type
            count_if( InputIterator first, 
            InputIterator last, 
            Predicate predicate)
        {

         return count_if(bolt::amp::control::getDefault(), first, last, predicate);

        }


    }

};


namespace bolt {
    namespace amp {
        namespace detail {

#ifdef ENABLE_TBB
            /*For documentation on the reduce object see below link
             *http://threadingbuildingblocks.org/docs/help/reference/algorithms/parallel_reduce_func.htm
             *The imperative form of parallel_reduce is used. 
             *
            */
            template <typename T, typename Predicate>
            struct Count {
                size_t value;
                Predicate predicate;
               
                //TODO - Decide on how many threads to spawn? Usually it should be equal to th enumber of cores
                //You might need to look at the tbb::split and there there cousin's 
                //
                Count (const Predicate &_opt) : predicate(_opt),value(0){}                
                Count (): value(0) {}

                Count (Count & s, tbb::split ):predicate(s.predicate),value(0){}
                 void operator()( const tbb::blocked_range<T*>& r ) 
                 {

                    for( T* a=r.begin(); a!=r.end(); ++a ) 
                    {     
                      if(predicate(*a))
                      {
                        value++; 
                      }
                    }

                }
                 //Join is called by the parent thread after the child finishes to execute.
                void join(Count & rhs ) {
                    value = (value + rhs.value);
                }
            };
#endif



            //----
            // This is the base implementation of reduction that is called by all of the convenience wrappers below.
            // first and last must be iterators from a DeviceVector
            template<typename DVInputIterator, typename Predicate> 
            int count_enqueue(bolt::amp::control &ctl, 
                const DVInputIterator& first,
                const DVInputIterator& last, 
                const Predicate& predicate)
            {
                typedef typename std::iterator_traits< DVInputIterator >::value_type iType;

                const int szElements = static_cast< unsigned int >( std::distance( first, last ) );
                const unsigned int tileSize = WAVEFRONT_SIZE;
                unsigned int numTiles = (szElements/tileSize);
                const unsigned int ceilNumTiles = static_cast< size_t >( std::ceil( static_cast< float >( szElements ) / tileSize) );
                unsigned int ceilNumElements = tileSize * ceilNumTiles;


                concurrency::array_view< iType, 1 > inputV (first.getBuffer());

                //Now create a staging array ; May support zero-copy in the future?!
                concurrency::accelerator cpuAccelerator = concurrency::accelerator(concurrency::accelerator::cpu_accelerator);
                concurrency::accelerator_view cpuAcceleratorView = cpuAccelerator.default_view;
                concurrency::array< int, 1 > resultArray ( szElements, ctl.getAccelerator().default_view, cpuAcceleratorView);
                
                concurrency::array_view<int, 1> result ( resultArray );
                result.discard_data();
                concurrency::extent< 1 > inputExtent( ceilNumElements );
                concurrency::tiled_extent< tileSize > tiledExtentReduce = inputExtent.tile< tileSize >();

                // Algorithm is different from cl::reduce. We launch worksize = number of elements here.
                // AMP doesn't have APIs to get CU capacity. Launchable size is great though.

                try
                {
                    concurrency::parallel_for_each(ctl.getAccelerator().default_view,
                                                   tiledExtentReduce,
                                                   [ inputV,
                                                     szElements,
                                                     result,
                                                     predicate ]
                                                   ( concurrency::tiled_index<tileSize> t_idx ) restrict(amp)
                    {
                      int globalId = t_idx.global[ 0 ];
                      int tileIndex = t_idx.local[ 0 ];
                      //  Initialize local data store
                      bool stat;
                      int count = 0;

                      tile_static int scratch_count [WAVEFRONT_SIZE] ;

                      //  Abort threads that are passed the end of the input vector
                      if( t_idx.global[ 0 ] < szElements )
                      {
                       //  Initialize the accumulator private variable with data from the input array
                       //  This essentially unrolls the loop below at least once
                       iType accumulator = inputV[globalId];
                       stat =  predicate(accumulator);
                       count=  stat?++count:count;
                       scratch_count[tileIndex] = count;
                      }
                      t_idx.barrier.wait();                  
                      
                      //  Tail stops the last workgroup from reading past the end of the input vector
                      int tail = szElements - (t_idx.tile[ 0 ] * t_idx.tile_dim0);
                      // Parallel reduction within a given workgroup using local data store
                      // to share values between workitems
                      
                      _REDUCE_STEP(tail, tileIndex, 32);
                      _REDUCE_STEP(tail, tileIndex, 16);
                      _REDUCE_STEP(tail, tileIndex,  8);
                      _REDUCE_STEP(tail, tileIndex,  4);
                      _REDUCE_STEP(tail, tileIndex,  2);
                      _REDUCE_STEP(tail, tileIndex,  1);
                      
                      
                      //  Write only the single reduced value for the entire workgroup
                      if (tileIndex == 0)
                      {
                          result[t_idx.tile[ 0 ]] = scratch_count[0];
                      }
                     

                    });
                    
                    
                    int *cpuPointerReduce =  result.data();

                    int numTailReduce = (ceilNumTiles>numTiles)? ceilNumTiles : numTiles;

                    int count =  cpuPointerReduce[0] ;
                    for(int i = 1; i < numTailReduce; ++i)
                    {

                       count +=  cpuPointerReduce[i];              
                     
                    }
                    
                    return count;
                }
                catch(std::exception &e)
                {
                      std::cout << "Exception while calling bolt::amp::reduce parallel_for_each " << e.what() << std::endl;
                      return 0;
                }
            }

            template<typename InputIterator, typename Predicate> 
            int count_detect_random_access(bolt::amp::control &ctl, 
                const InputIterator& first,
                const InputIterator& last, 
                const Predicate& predicate,
                std::input_iterator_tag)  
            {
                //  TODO:  It should be possible to support non-random_access_iterator_tag iterators, if we copied the data 
                //  to a temporary buffer.  Should we?
                static_assert( false, "Bolt only supports random access iterator types" );
            }


            template<typename InputIterator, typename Predicate> 
            int count_detect_random_access(bolt::amp::control &ctl, 
                const InputIterator& first,
                const InputIterator& last, 
                const Predicate& predicate, 
                std::random_access_iterator_tag)  
            {
                return count_pick_iterator( ctl, first, last, predicate );
            }

            // This template is called after we detect random access iterators
            // This is called strictly for any non-device_vector iterator
            template<typename InputIterator, typename Predicate> 
            typename std::enable_if< !std::is_base_of<typename device_vector<typename std::iterator_traits<InputIterator>::value_type>::iterator,InputIterator>::value, int >::type
            count_pick_iterator(bolt::amp::control &ctl, 
                const InputIterator& first,
                const InputIterator& last, 
                const Predicate& predicate )
            {
                /*************/
                typedef typename std::iterator_traits<InputIterator>::value_type iType;
                size_t szElements = (size_t)(last - first); 
                if (szElements == 0)
                    return 0;
                /*TODO - probably the forceRunMode should be replaced by getRunMode and setRunMode*/
                // Its a dynamic choice. See the reduce Test Code
                // What should we do if the run mode is automatic. Currently it goes to the last else statement
                //How many threads we should spawn? 
                //Need to look at how to control the number of threads spawned.

                //
                bolt::amp::control::e_RunMode runMode = ctl.getForceRunMode();  // could be dynamic choice some day.
                if (runMode == bolt::amp::control::SerialCpu)
                {
                      return (int) std::count_if(first,last,predicate);
                }
                else if (runMode == bolt::amp::control::MultiCoreCpu)
                {
#ifdef ENABLE_TBB
                    tbb::task_scheduler_init initialize(tbb::task_scheduler_init::automatic);
                    Count<iType,Predicate> count_op(predicate);
                    tbb::parallel_reduce( tbb::blocked_range<iType*>( &*first, (iType*)&*(last-1) + 1), count_op );
                    return (int)count_op.value;
#else
                    std::cout << "The MultiCoreCpu version of count function is not enabled." << std ::endl;
                    throw std::exception( "The MultiCoreCpu version of count function is not enabled to be built." );
                    return 0;
#endif  
                }
                else 
                {
                    device_vector< iType, concurrency::array_view > dvInput( first, last, false, ctl );
                    return count_enqueue( ctl, dvInput.begin(), dvInput.end(), predicate );
                }
            };

            // This template is called after we detect random access iterators
            // This is called strictly for iterators that are derived from device_vector< T >::iterator
            template<typename DVInputIterator, typename Predicate>
            typename std::enable_if< std::is_base_of<typename device_vector<typename std::iterator_traits<DVInputIterator>::value_type>::iterator,DVInputIterator>::value, int >::type
            count_pick_iterator(bolt::amp::control &ctl, 
                const DVInputIterator& first,
                const DVInputIterator& last, 
                const Predicate& predicate )
            {

                typedef typename std::iterator_traits<DVInputIterator>::value_type iType;
                size_t szElements = (size_t) (last - first);
                if (szElements == 0)
                    return 0;

                bolt::amp::control::e_RunMode runMode = ctl.getForceRunMode();  // could be dynamic choice some day.
                if(runMode == bolt::amp::control::Automatic)
                {
                    //runMode = ctl.getDefaultPathToRun(); *Fixit*
                }
                if (runMode == bolt::amp::control::SerialCpu) {
                     std::vector<iType> InputBuffer(szElements);
                     for(unsigned int index=0; index<szElements; index++){
                         InputBuffer[index] = first.getBuffer()[index];
                     }
                     return (int) std::count_if(InputBuffer.begin(),InputBuffer.end() ,predicate);


                }

                else if (runMode == bolt::amp::control::MultiCoreCpu)
                {
#ifdef ENABLE_TBB
                    std::vector<iType> InputBuffer(szElements);
                    for(unsigned int index=0; index<szElements; index++){
                        InputBuffer[index] = first.getBuffer()[index];
                    }
                    tbb::task_scheduler_init initialize(tbb::task_scheduler_init::automatic);
                    Count<iType,Predicate> count_op(predicate);
                    tbb::parallel_reduce( tbb::blocked_range<iType*>( &*InputBuffer.begin(), (iType*)&*(InputBuffer.end()-1) + 1), count_op );
                    return (int)count_op.value;
#else
                    std::cout << "The MultiCoreCpu version of count function is not enabled." << std ::endl;
                    throw std::exception( "The MultiCoreCpu version of count function is not enabled to be built." );
                    return 0;
#endif  

                }
                else
                {
                  return  count_enqueue( ctl, first, last, predicate );
                }
            }


        }
    }
}

#endif //COUNT_INL
