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
#define BINARY_SEARCH_WAVEFRONT_SIZE 64

#include <type_traits>

#include "bolt/amp/bolt.h"
#include "bolt/amp/device_vector.h"
#include "bolt/amp/functional.h"
#include "bolt/amp/iterator/iterator_traits.h"
#include <amp.h>
//TBB Includes
#if defined(ENABLE_TBB)
#include "bolt/btbb/binary_search.h"
#endif

namespace bolt {
    namespace amp {

        namespace detail {

            /*****************************************************************************
             * BS Enqueue
             ****************************************************************************/
            template< typename DVForwardIterator, typename T , typename StrictWeakOrdering>
            bool binary_search_enqueue( bolt::amp::control &ctl, const DVForwardIterator &first,
                const DVForwardIterator &last, const T & val, StrictWeakOrdering comp)
            {
				concurrency::accelerator_view av = ctl.getAccelerator().default_view;

                // No. of I/P elements
                int numElementsProcessedperWI = 16; //For now choose 16
                int numOfWGs;
                unsigned int globalThreads;
                unsigned int localThreads;
                unsigned int residueGlobalThreads;
                unsigned int residueLocalThreads;
                int szElements = static_cast< unsigned int>( std::distance( first, last ) );

                if(szElements < (BINARY_SEARCH_WAVEFRONT_SIZE*numElementsProcessedperWI) )
                {
                    numOfWGs = 1;
                    residueGlobalThreads = (szElements/numElementsProcessedperWI);
                    residueGlobalThreads = residueGlobalThreads + ( (szElements & (numElementsProcessedperWI-1) )? 1: 0 );
                    residueLocalThreads = residueGlobalThreads; //Because only 1 WG will be spawned
                    globalThreads = 0;
                    localThreads = 0;
                }
                else
                {
                    //Here you will definitely spawn more than BINARY_SEARCH_WAVEFRONT_SIZE work items.
                    globalThreads = (szElements/numElementsProcessedperWI);
                    globalThreads = globalThreads + ( (szElements & (numElementsProcessedperWI-1) )? 1: 0 ); //Create One extra thread if some buffer residue is left.
                    localThreads = BINARY_SEARCH_WAVEFRONT_SIZE;
                    residueGlobalThreads = globalThreads % localThreads;
                    residueLocalThreads = residueGlobalThreads;
                    globalThreads = globalThreads - residueGlobalThreads; // This makes globalThreads multiple of BINARY_SEARCH_WAVEFRONT_SIZE
                }

				unsigned int totalThreads = globalThreads+residueGlobalThreads;

                /**********************************************************************************
                 * Type Names - used in KernelTemplateSpecializer
                 *********************************************************************************/
                typedef typename std::iterator_traits<DVForwardIterator>::value_type iType;

				//concurrency::array< int > result( szElements, av );
				std::vector<int> stdResBuffer(szElements);
                device_vector<int, concurrency::array_view > stdResVec(stdResBuffer.begin(), stdResBuffer.end(), true, ctl );
                auto&  result   =  stdResVec.begin().getContainer().getBuffer(stdResVec.begin()); 


                auto&  inputBuffer =  first.getContainer().getBuffer(first); 


                // Input buffer
                unsigned int startIndex = 0;
                unsigned int endIndex = static_cast< unsigned int >( numElementsProcessedperWI*globalThreads );
                   
                if(globalThreads != 0 )
                {
					     concurrency::extent< 1 > inputExtent( globalThreads);

	                     try
	                     {
                          concurrency::parallel_for_each( av, inputExtent,
                              [
                                 inputBuffer,
                                 val,
                                 numElementsProcessedperWI,
                                 comp,	
                                 result,
								 startIndex,
								 endIndex
                              ] ( concurrency::index<1> idx ) restrict(amp)
                           {

                                 unsigned int gloId = idx[0];
                                
								 unsigned int mid;
                                 unsigned int low = startIndex + gloId * numElementsProcessedperWI;
                                 unsigned int high = low + numElementsProcessedperWI;
                                 unsigned int resultIndex = startIndex/numElementsProcessedperWI;

                                 if(high > endIndex)
                                 high = endIndex;

                                 int found = 0;
    
                                 while(low < high)
                                 {	
                                     mid = (low + high) / 2;
        
                                     iType midVal = inputBuffer[mid];
                                     iType firstVal = inputBuffer[low];
   
                                     if( !(comp(midVal, val)) && !(comp(val, midVal)) )
                                     {
                                       found = 1;
                                       break;
                                     }
                                     else if ( comp(midVal, val) ) /*if true, midVal comes before val hence adjust low*/
                                       low = mid + 1;
                                     else	/*else val comes before midVal, hence adjust high*/
                                       high = mid;
                                }
    
                                result[resultIndex+gloId] = found;
						 });
						}

						 catch(std::exception &e)
                         {
                                  std::cout << "Exception while calling bolt::amp::binary_search parallel_for_each " ;
                                  std::cout<< e.what() << std::endl;
                                  throw std::exception();
                         }	
				    }

                    startIndex = globalThreads*numElementsProcessedperWI;
                    endIndex = szElements;
                    if(residueGlobalThreads !=0)
                    {
                            concurrency::extent< 1 > inputExtent( residueGlobalThreads);

	                        try
	                        {
                             concurrency::parallel_for_each( av, inputExtent,
                              [
                                 inputBuffer,
                                 val,
                                 numElementsProcessedperWI,
                                 comp,	
                                 result,
								 startIndex,
								 endIndex
                              ] ( concurrency::index<1> idx ) restrict(amp)
                             {

                                 unsigned int gloId = idx[0];
                                
								 unsigned int mid;
                                 unsigned int low = startIndex + gloId * numElementsProcessedperWI;
                                 unsigned int high = low + numElementsProcessedperWI;
                                 unsigned int resultIndex = startIndex/numElementsProcessedperWI;

                                 if(high > endIndex)
                                 high = endIndex;

                                 int found = 0;
    
                                 while(low < high)
                                 {	
                                     mid = (low + high) / 2;
        
                                     iType midVal = inputBuffer[mid];
                                     iType firstVal = inputBuffer[low];
   
                                     if( !(comp(midVal, val)) && !(comp(val, midVal)) )
                                     {
                                       found = 1;
                                       break;
                                     }
                                     else if ( comp(midVal, val) ) /*if true, midVal comes before val hence adjust low*/
                                       low = mid + 1;
                                     else	/*else val comes before midVal, hence adjust high*/
                                       high = mid;
                                }
    
                                result[resultIndex+gloId] = found;
						   });
							}
						   catch(std::exception &e)
                           {
                                  std::cout << "Exception while calling bolt::amp::binary_search parallel_for_each " ;
                                  std::cout<< e.what() << std::endl;
                                  throw std::exception();
                           }	
                    }
               

               
                  bool r = false;
                  for(unsigned int i=0; i<totalThreads; i++)
                  {
                    if(result[i] == 1)
                    {
                        r = true;
                        break;
                    }
                  }

                  return r;

            }; // end binary_search_enqueue


            /*****************************************************************************
             * Pick Iterator
             ****************************************************************************/

        /*! \brief This template function overload is used to seperate device_vector iterators from all other iterators
                \detail This template is called by the non-detail versions of binary_search, it already assumes random access
             * iterators. This overload is called strictly for non-device_vector iterators
        */

            template<typename ForwardIterator, typename T, typename StrictWeakOrdering>
            bool binary_search_pick_iterator( bolt::amp::control &ctl, const ForwardIterator &first,
                const ForwardIterator &last, const T & value, StrictWeakOrdering comp, 
                std::random_access_iterator_tag )
            {

                typedef typename std::iterator_traits<ForwardIterator>::value_type Type;
                size_t sz = (last - first);
                if (sz < 1)
                     return false;

                bolt::amp::control::e_RunMode runMode = ctl.getForceRunMode(); // could be dynamic choice some day.
                if(runMode == bolt::amp::control::Automatic)
                {
                     runMode = ctl.getDefaultPathToRun();
                }
	
				
                if( runMode == bolt::amp::control::SerialCpu )
                {
                     return std::binary_search(first, last, value, comp );
                }
                else if(runMode == bolt::amp::control::MultiCoreCpu)
                {
                    #ifdef ENABLE_TBB
                          return bolt::btbb::binary_search(first, last, value, comp);
                    #else
                          throw std::runtime_error("MultiCoreCPU Version of Binary Search not Enabled! \n");
                    #endif
                }
                else
                {
                        // Use host pointers memory since these arrays are only write once - no benefit to copying.
                        // Map the forward iterator to a device_vector
					    device_vector< Type, concurrency::array_view> range( first, sz, true, ctl );

                        return binary_search_enqueue( ctl, range.begin( ), range.end( ), value, comp );

                }

            }

            // This template is called by the non-detail versions of BS, it already assumes random access iterators
            // This is called strictly for iterators that are derived from device_vector< T >::iterator

            template<typename DVForwardIterator, typename T , typename StrictWeakOrdering>
            bool binary_search_pick_iterator(bolt::amp::control &ctl, const DVForwardIterator &first,
                const DVForwardIterator &last, const T & value, StrictWeakOrdering comp, 
                bolt::amp::device_vector_tag )
            {
                typedef typename std::iterator_traits<DVForwardIterator>::value_type iType;
                size_t szElements = static_cast<size_t>(std::distance(first, last) );
                bolt::amp::control::e_RunMode runMode = ctl.getForceRunMode(); // could be dynamic choice some day.
                if(runMode == bolt::amp::control::Automatic)
                {
                     runMode = ctl.getDefaultPathToRun();
                }
				
                if( runMode == bolt::amp::control::SerialCpu )
                {
                     typename bolt::amp::device_vector< iType >::pointer bsInputBuffer = first.getContainer( ).data( );
                     return std::binary_search(&bsInputBuffer[first.m_Index], &bsInputBuffer[last.m_Index], value, comp );
                }
                else if(runMode == bolt::amp::control::MultiCoreCpu)
                {
                    #ifdef ENABLE_TBB
                        typename bolt::amp::device_vector< iType >::pointer bsInputBuffer = first.getContainer( ).data( );
                        return bolt::btbb::binary_search(&bsInputBuffer[first.m_Index], &bsInputBuffer[last.m_Index], value, comp );
                    #else
                        throw std::runtime_error("MultiCoreCPU Version of Binary Search not Enabled! \n");
                    #endif
                }
                else
                {
                    return binary_search_enqueue( ctl, first, last, value, comp );
                }
            }

            // This template is called by the non-detail versions of binary_search, it already assumes random access iterators
            // This is called strictly for iterators that are derived from device_vector< T >::iterator

            template<typename DVForwardIterator, typename T, typename StrictWeakOrdering>
            bool binary_search_pick_iterator(bolt::amp::control &ctl, const DVForwardIterator &first,
                const DVForwardIterator &last, const T & value, StrictWeakOrdering comp, 
                bolt::amp::fancy_iterator_tag )
            {

                typedef typename std::iterator_traits<DVForwardIterator>::value_type iType;
                size_t szElements = static_cast<size_t>(std::distance(first, last) );
                if (szElements == 0)
                    return false;

                bolt::amp::control::e_RunMode runMode = ctl.getForceRunMode(); // could be dynamic choice some day.
                if(runMode == bolt::amp::control::Automatic)
                {
                    runMode = ctl.getDefaultPathToRun();
                }

                if( runMode == bolt::amp::control::SerialCpu )
                {
                     return std::binary_search(first, last, value, comp);
                }
                else if(runMode == bolt::amp::control::MultiCoreCpu)
                {		   
                    #ifdef ENABLE_TBB
                        return bolt::btbb::binary_search(first, last, value, comp);
                        //return std::binary_search(first, last, value, comp);
                    #else
                        throw std::runtime_error("MultiCoreCPU Version of Binary Search not Enabled! \n");
                    #endif
                }
                else
                {
                    return binary_search_enqueue( ctl, first, last, value, comp );
                }

            }

            /*****************************************************************************
             * Random Access
             ****************************************************************************/


            // Random-access
            template<typename ForwardIterator, typename T, typename StrictWeakOrdering>
            bool binary_search_detect_random_access( bolt::amp::control &ctl, ForwardIterator first,
                ForwardIterator last,
                const T & value, StrictWeakOrdering comp, std::random_access_iterator_tag )
            {
                 return binary_search_pick_iterator(ctl, first, last, value, comp, 
                 typename std::iterator_traits< ForwardIterator >::iterator_category( ) );
            }


            // No support for non random access iterators
            template<typename ForwardIterator, typename T, typename StrictWeakOrdering>
            bool binary_search_detect_random_access( bolt::amp::control &ctl, ForwardIterator first,
                ForwardIterator last,
                const T & value, StrictWeakOrdering comp, std::forward_iterator_tag )
            {
                static_assert( std::is_same< ForwardIterator, std::forward_iterator_tag   >::value, "Bolt only supports random access iterator types" );
            }

        }//End of detail namespace


        //Default control
        template<typename ForwardIterator, typename T>
        bool binary_search( ForwardIterator first,
            ForwardIterator last,
            const T & value)
        {
            return detail::binary_search_detect_random_access( bolt::amp::control::getDefault(), first, last, value,
                bolt::amp::less< T >( ),  typename std::iterator_traits< ForwardIterator >::iterator_category( ) );
        }

        //User specified control
        template< typename ForwardIterator, typename T >
        bool binary_search( bolt::amp::control &ctl,
            ForwardIterator first,
            ForwardIterator last,
            const T & value)
        {
            return detail::binary_search_detect_random_access( ctl, first, last, value, bolt::amp::less< T >( ), 
                typename std::iterator_traits< ForwardIterator >::iterator_category( ) );
        }

        //Default control
        template<typename ForwardIterator, typename T, typename StrictWeakOrdering>
        bool binary_search(ForwardIterator first,
            ForwardIterator last,
            const T & value,
            StrictWeakOrdering comp)
        {
            return detail::binary_search_detect_random_access( bolt::amp::control::getDefault(), first, last, value,
                comp,  typename std::iterator_traits< ForwardIterator >::iterator_category( ) );
        }

        //User specified control
        template<typename ForwardIterator, typename T, typename StrictWeakOrdering>
        bool binary_search(bolt::amp::control &ctl,
            ForwardIterator first,
            ForwardIterator last,
            const T & value,
            StrictWeakOrdering comp)
        {
            return detail::binary_search_detect_random_access( ctl, first, last, value, comp,
                typename std::iterator_traits< ForwardIterator >::iterator_category( ) );
        }

    }//end of amp namespace
};//end of bolt namespace



#endif
