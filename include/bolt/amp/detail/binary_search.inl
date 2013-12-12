/***************************************************************************
* Copyright 2012 - 2013 Advanced Micro Devices, Inc.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.

***************************************************************************/

#pragma once
#if !defined( BOLT_AMP_BINARY_SEARCH_INL )
#define BOLT_AMP_BINARY_SEARCH_INL
#define WAVEFRONT_SIZE 32
//#define BINARY_SEARCH_THRESHOLD 16
#include <boost/thread/once.hpp>
#include <boost/bind.hpp>
#include <type_traits>

#include "bolt/amp/bolt.h"
#include "bolt/amp/device_vector.h"
#include "bolt/amp/functional.h"
//TBB Includes
#if defined(ENABLE_TBB)
#include "bolt/btbb/binary_search.h"
#endif
using namespace concurrency;
namespace bolt {
    namespace amp {

        namespace detail {        

            /*****************************************************************************
             * BS Enqueue
             ****************************************************************************/
            template< typename DVForwardIterator, 
                      typename T , 
                      typename StrictWeakOrdering>
            bool binary_search_enqueue( bolt::amp::control &ctl, 
                                        const DVForwardIterator &first,                
                                        const DVForwardIterator &last, 
                                        const T & val, 
                                        StrictWeakOrdering comp)
            {
                typedef std::iterator_traits< DVForwardIterator >::value_type iType;

               const int arraySize =  static_cast< unsigned int >( std::distance( first, last ) );
               int wavefrontMultiple = arraySize;
               const int lowerBits = ( arraySize & ( WAVEFRONT_SIZE -1 ) );

               int boundsCheck = 0;  // size is not multiple of the WAVEFRONT_SIZE

               if( lowerBits ) // size is not multiple of the WAVEFRONT_SIZE
               {
                   wavefrontMultiple &= ~lowerBits;
                   wavefrontMultiple += WAVEFRONT_SIZE;
               }
               else  // size is multiple of the WAVEFRONT_SIZE
               boundsCheck = 1;

               concurrency::array_view<iType,1> inputV (first.getContainer().getBuffer());
               std::vector<int> v(1,0);
               concurrency::array_view<int,1> result(1,v);
               concurrency::extent< 1 > inputExtent( wavefrontMultiple );

               static const int TS = 16;
               
               // using automatic queuing mode
                accelerator_view myAv = accelerator(accelerator::direct3d_ref).default_view;

                // or if you want to try immediate_queuing_mode
                //accelerator_view myAv = accelerator(accelerator::direct3d_ref).create_view(queuing_mode_immediate);
               concurrency::parallel_for_each(/*ctl.getAccelerator().default_view*/myAv, inputExtent.tile<TS>(), [=](concurrency::tiled_index<TS> t_idx) restrict(amp)
               {
                   unsigned int globalId = t_idx.global[0];
                   /*if(boundsCheck == 0)
                   {
                     if( (globalId >= arraySize) )
                       return;
                   }*/

                    tile_static int t[TS];
                    t[t_idx.local[0]] = inputV[t_idx.global[0]];

                    t_idx.barrier.wait();
                    
                   if(t_idx.local == index<1>(0))
                  { 
                    int imin = t_idx.tile_origin[0] ;
                    int imax = imin+(TS-1);
                    int imid;
                    while(imax>=imin)
                    {
                        imid = (imax+imin)/2;
                        if(t[imid] == val )
                           result[0] = 1;
                        else if(t[imid]<val)
                            imin = imid+1;
                        else
                            imax = imid-1;
                     }
                  }	

               });

                if(result[0]==0)
                  return false;
                else
                  return true; 
            }; // end binary_search_enqueue


            /*****************************************************************************
             * Pick Iterator
             ****************************************************************************/

        /*! \brief This template function overload is used to seperate device_vector iterators from all other iterators
                \detail This template is called by the non-detail versions of binary_search, it already assumes random access
             * iterators. This overload is called strictly for non-device_vector iterators
        */

            template<typename ForwardIterator,
                     typename T, 
                     typename StrictWeakOrdering>
            bool binary_search_pick_iterator( bolt::amp::control &ctl,
                                              const ForwardIterator &first,
                                              const ForwardIterator &last,
                                              const T & value,
                                              StrictWeakOrdering comp)
            {

                typedef std::iterator_traits<ForwardIterator>::value_type Type;
                size_t sz = (last - first);
                if (sz < 1)
                     return false;

                const bolt::amp::control::e_RunMode runMode = ctl.getForceRunMode(); // could be dynamic choice some day.
                                
                if( runMode == bolt::amp::control::SerialCpu )
                {
                     return std::binary_search(first, last, value, comp );
                }
                else if(runMode == bolt::amp::control::MultiCoreCpu)
                {
                    #ifdef ENABLE_TBB

                          return bolt::btbb::binary_search(first, last, value, comp);
                    #else
                          throw std::exception("MultiCoreCPU Version of Binary Search not Enabled! \n");
                    #endif
                }
                else
                {
                        
                        // Use host pointers memory since these arrays are only write once - no benefit to copying.
                        // Map the forward iterator to a device_vector
                         device_vector< Type, concurrency::array_view > range( first, sz, true, ctl );
                        return binary_search_enqueue( ctl, range.begin( ), range.end( ), value, comp );

                }

            }

            // This template is called by the non-detail versions of BS, it already assumes random access iterators
            // This is called strictly for iterators that are derived from device_vector< T >::iterator  
        /*	

            template<typename DVForwardIterator, 
                     typename T , 
                     typename StrictWeakOrdering>
            bool binary_search_pick_iterator(bolt::amp::control &ctl, 
                                             const DVForwardIterator &first,                
                                             const DVForwardIterator &last, 
                                             const T & value, 
                                             StrictWeakOrdering comp)
            {
                typedef  std::iterator_traits<DVForwardIterator>::value_type iType;
                size_t sz = static_cast<size_t>(std::distance(first, last) );
                if (sz < 1)
                     return false;
                const bolt::amp::control::e_RunMode runMode = ctl.getForceRunMode(); // could be dynamic choice some day.
                
                
                if( runMode == bolt::amp::control::SerialCpu )
                {
                    
                     bolt::amp::device_vector< iType >::pointer bsInputBuffer = first.getContainer( ).data( );
                     return std::binary_search(&bsInputBuffer[first.m_Index], &bsInputBuffer[sz], value, comp );
                }
                else if(runMode == bolt::amp::control::MultiCoreCpu)
                {
                     #ifdef ENABLE_TBB
                        bolt::amp::device_vector< iType >::pointer bsInputBuffer = first.getContainer( ).data( );
                        return bolt::btbb::binary_search(&bsInputBuffer[first.m_Index], &bsInputBuffer[sz], value, comp );
                    #else
                        throw std::exception("MultiCoreCPU Version of Binary Search not Enabled! \n");
                    #endif
                }
                else
                {
                    return binary_search_enqueue( ctl, first, last, value, comp );
                }
            }
            */

            /*****************************************************************************
             * Random Access
             ****************************************************************************/

            // Random-access
            template<typename ForwardIterator, 
                     typename T, 
                     typename StrictWeakOrdering>
            bool binary_search_detect_random_access( bolt::amp::control &ctl, 
                                                     ForwardIterator first,
                                                     ForwardIterator last,
                                                     const T & value, 
                                                     StrictWeakOrdering comp,
                                                     std::random_access_iterator_tag)
            {
                 return binary_search_pick_iterator(ctl, first, last, value, comp);
            }


            // No support for non random access iterators
            template<typename ForwardIterator, 
                     typename T, 
                     typename StrictWeakOrdering>
            bool binary_search_detect_random_access( bolt::amp::control &ctl, 
                                                     ForwardIterator first,                
                                                     ForwardIterator last,                
                                                     const T & value, 
                                                     StrictWeakOrdering comp,
                                                     std::input_iterator_tag)
            {
                static_assert( false, "Bolt only supports random access iterator types" );
            }

        }//End of detail namespace


        //Default control
        template<typename ForwardIterator, 
                 typename T>
        bool binary_search( ForwardIterator first,
                            ForwardIterator last,                          
                            const T & value)
        {
            return detail::binary_search_detect_random_access( bolt::amp::control::getDefault(), first, last, value,
                            bolt::amp::less< T >( ), std::iterator_traits< ForwardIterator >::iterator_category( ) );
        }

        //User specified control
        template< typename ForwardIterator, 
                  typename T >
        bool binary_search( bolt::amp::control &ctl,            
                            ForwardIterator first,           
                            ForwardIterator last,            
                            const T & value)
        {
            return detail::binary_search_detect_random_access( ctl, first, last, value, bolt::amp::less< T >( ),
                                                 std::iterator_traits< ForwardIterator >::iterator_category( ));
        }

        //Default control
        template<typename ForwardIterator, 
                 typename T, 
                 typename StrictWeakOrdering>
        bool binary_search(ForwardIterator first,            
                           ForwardIterator last,            
                           const T & value,            
                           StrictWeakOrdering comp)
        {
            return detail::binary_search_detect_random_access( bolt::amp::control::getDefault(), first, last, value,
                                               comp, std::iterator_traits< ForwardIterator >::iterator_category( ));
        }

        //User specified control
        template<typename ForwardIterator, 
                 typename T, 			             
                 typename StrictWeakOrdering>
        bool binary_search(bolt::amp::control &ctl,            
                           ForwardIterator first,            
                           ForwardIterator last,            
                           const T & value,            
                           StrictWeakOrdering comp)
        {
            return detail::binary_search_detect_random_access( ctl, first, last, value, comp, 
                             std::iterator_traits< ForwardIterator >::iterator_category( ) );
        }

    }//end of amp namespace
};//end of bolt namespace



#endif
