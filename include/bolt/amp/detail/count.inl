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

#if !defined( BOLT_AMP_COUNT_INL )
#define BOLT_AMP_COUNT_INL
#pragma once

#include <algorithm>

#include "bolt/amp/bolt.h"
#include "bolt/amp/device_vector.h"
#include "bolt/amp/iterator/iterator_traits.h"
#ifdef ENABLE_TBB
//TBB Includes
#include "bolt/btbb/count.h"
#endif

#define _COUNT_REDUCE_STEP(_LENGTH, _IDX, _W) \
    if ((_IDX < _W) && ((_IDX + _W) < _LENGTH)) {\
      scratch_count[_IDX] =  scratch_count[_IDX] + scratch_count[_IDX + _W];\
    }\
    t_idx.barrier.wait();

#define COUNT_WAVEFRONT_SIZE 256

namespace bolt {
namespace amp {
namespace detail {

namespace serial{

    template<typename InputIterator, typename Predicate>
    typename bolt::amp::iterator_traits<InputIterator>::difference_type
        count(bolt::amp::control &ctl,
        const InputIterator& first,
        const InputIterator& last,
        const Predicate& predicate,
		std::random_access_iterator_tag)
    {

		return std::count_if(first,last,predicate);

	}

	//TODO use create_mapped_itr for mapping/unmapping 
	template<typename InputIterator, typename Predicate>
    typename bolt::amp::iterator_traits<InputIterator>::difference_type
        count(bolt::amp::control &ctl,
        const InputIterator& first,
        const InputIterator& last,
        const Predicate& predicate,
		bolt::amp::device_vector_tag)
    {

		size_t n = (last - first);

        typedef typename std::iterator_traits< InputIterator >::value_type iType;
		typedef typename bolt::amp::iterator_traits<InputIterator>::difference_type rType;

	    typename bolt::amp::device_vector< iType >::pointer countInputBuffer = first.getContainer( ).data( );
        return  (rType) std::count_if(&countInputBuffer[first.m_Index],
                       &countInputBuffer[first.m_Index + n], predicate) ;

	}


	template<typename InputIterator, typename Predicate>
    typename bolt::amp::iterator_traits<InputIterator>::difference_type
        count(bolt::amp::control &ctl,
        const InputIterator& first,
        const InputIterator& last,
        const Predicate& predicate,
		bolt::amp::fancy_iterator_tag)
    {

		return std::count_if(first,last,predicate);

	}

} // end of namespace serial

#ifdef ENABLE_TBB
namespace btbb{

    template<typename InputIterator, typename Predicate>
    typename bolt::amp::iterator_traits<InputIterator>::difference_type
        count(bolt::amp::control &ctl,
        const InputIterator& first,
        const InputIterator& last,
        const Predicate& predicate,
		std::random_access_iterator_tag)
    {

		return  bolt::btbb::count_if(first, last,predicate);

	}


	template<typename InputIterator, typename Predicate>
    typename bolt::amp::iterator_traits<InputIterator>::difference_type
        count(bolt::amp::control &ctl,
        const InputIterator& first,
        const InputIterator& last,
        const Predicate& predicate,
		bolt::amp::device_vector_tag)
    {

		size_t n = (last - first);

        typedef typename std::iterator_traits< InputIterator >::value_type iType;

	    typename bolt::amp::device_vector< iType >::pointer countInputBuffer = first.getContainer( ).data( );
        return  bolt::btbb::count_if(&countInputBuffer[first.m_Index],
                         &countInputBuffer[first.m_Index + n] ,predicate);

	}


	template<typename InputIterator, typename Predicate>
    typename bolt::amp::iterator_traits<InputIterator>::difference_type
        count(bolt::amp::control &ctl,
        const InputIterator& first,
        const InputIterator& last,
        const Predicate& predicate,
		bolt::amp::fancy_iterator_tag)
    {

		return  bolt::btbb::count_if(first, last,predicate);

	}

} // end of namespace btbb
#endif

namespace amp{
	//----
    // This is the base implementation of reduction that is called by all of the convenience wrappers below.
    // first and last must be iterators from a DeviceVector
    template<typename DVInputIterator, typename Predicate>
    typename bolt::amp::iterator_traits<DVInputIterator>::difference_type
	count(bolt::amp::control &ctl,
        const DVInputIterator& first,
        const DVInputIterator& last,
        const Predicate& predicate,
		bolt::amp::device_vector_tag)
    {
		typedef typename std::iterator_traits< DVInputIterator >::value_type iType;				
		const int szElements = static_cast< int >(std::distance(first, last));

		int max_ComputeUnits = 32;
		int numTiles = max_ComputeUnits*32;	/* Max no. of WG for Tahiti(32 compute Units) and 32 is the tuning factor that gives good performance*/
		int length = (COUNT_WAVEFRONT_SIZE * numTiles);
		length = szElements < length ? szElements : length;
		unsigned int residual = length % COUNT_WAVEFRONT_SIZE;
		length = residual ? (length + COUNT_WAVEFRONT_SIZE - residual): length ;
		numTiles = static_cast< int >((szElements/COUNT_WAVEFRONT_SIZE)>= numTiles?(numTiles):
							(std::ceil( static_cast< float >( szElements ) / COUNT_WAVEFRONT_SIZE) ));
				
		concurrency::array<unsigned int, 1> result(numTiles);
		concurrency::extent< 1 > inputExtent(length);
		concurrency::tiled_extent< COUNT_WAVEFRONT_SIZE > tiledExtentReduce = inputExtent.tile< COUNT_WAVEFRONT_SIZE >();

        try
        {
			concurrency::parallel_for_each(ctl.getAccelerator().default_view,
                                            tiledExtentReduce,
                                            [ first,
                                                szElements,
												length,
                                                &result,
                                                predicate ]
			(concurrency::tiled_index<COUNT_WAVEFRONT_SIZE> t_idx) restrict(amp)
            {
				int gx = t_idx.global[0];
				int gloId = gx;
				unsigned int tileIndex = t_idx.local[0];
                //  Initialize local data store
                bool stat;
				unsigned int count = 0;

				tile_static unsigned int scratch_count[COUNT_WAVEFRONT_SIZE];

                //  Abort threads that are passed the end of the input vector
				if (gloId < szElements)
                {
                //  Initialize the accumulator private variable with data from the input array
                //  This essentially unrolls the loop below at least once
					iType accumulator = first[gloId];
                stat =  predicate(accumulator);
				scratch_count[tileIndex]  = stat ? ++count : count;
				gx += length;
                }
                t_idx.barrier.wait();


				// Loop sequentially over chunks of input vector, reducing an arbitrary size input
				// length into a length related to the number of workgroups
				while (gx < szElements)
				{
					iType element = first[gx];
					stat = predicate(element);
					scratch_count[tileIndex] = stat ? ++scratch_count[tileIndex] : scratch_count[tileIndex];
					gx += length;
				}

				t_idx.barrier.wait();

                //  Tail stops the last workgroup from reading past the end of the input vector
				unsigned int tail = szElements - (t_idx.tile[0] * t_idx.tile_dim0);
                // Parallel reduction within a given workgroup using local data store
                // to share values between workitems

				_COUNT_REDUCE_STEP(tail, tileIndex, 128);
				_COUNT_REDUCE_STEP(tail, tileIndex, 64);
                _COUNT_REDUCE_STEP(tail, tileIndex, 32);
                _COUNT_REDUCE_STEP(tail, tileIndex, 16);
                _COUNT_REDUCE_STEP(tail, tileIndex,  8);
                _COUNT_REDUCE_STEP(tail, tileIndex,  4);
                _COUNT_REDUCE_STEP(tail, tileIndex,  2);
                _COUNT_REDUCE_STEP(tail, tileIndex,  1);


				//  Abort threads that are passed the end of the input vector
				if (gloId >= szElements)
					return;

                //  Write only the single reduced value for the entire workgroup
                if (tileIndex == 0)
                {
                    result[t_idx.tile[ 0 ]] = scratch_count[0];
                }


            });

			std::vector<unsigned int> *cpuPointerReduce = new std::vector<unsigned int>(numTiles);
			concurrency::copy(result, (*cpuPointerReduce).begin());                  
			unsigned int count = (*cpuPointerReduce)[0];
			for (int i = 1; i < numTiles; ++i)
			{
                count +=  (*cpuPointerReduce)[i];
            }
			delete cpuPointerReduce;

            return count;
        }
        catch(std::exception &e)
        {

                std::cout << "Exception while calling bolt::amp::count parallel_for_each"<<e.what()<<std::endl;

                return 0;
        }
    }

	
	template<typename InputIterator, typename Predicate>
    typename bolt::amp::iterator_traits<InputIterator>::difference_type
        count(bolt::amp::control &ctl,
        const InputIterator& first,
        const InputIterator& last,
        const Predicate& predicate,
		std::random_access_iterator_tag)
    {

		 int sz = static_cast<int>(last - first);

         typedef typename std::iterator_traits<InputIterator>::value_type  iType;
       	 
         device_vector< iType, concurrency::array_view > dvInput( first, last, false, ctl );
         return count( ctl, dvInput.begin(), dvInput.end(), predicate, bolt::amp::device_vector_tag() );

	}

	//TODO won't work for transform iterator with std::vectors...come back and fix it
	template<typename InputIterator, typename Predicate>
    typename bolt::amp::iterator_traits<InputIterator>::difference_type
        count(bolt::amp::control &ctl,
        const InputIterator& first,
        const InputIterator& last,
        const Predicate& predicate,
		bolt::amp::fancy_iterator_tag)
    {
		 return count(ctl, first, last, predicate, bolt::amp::device_vector_tag());

	}
			
} // end of namespace amp

    template<typename InputIterator, typename Predicate>
    typename std::enable_if< 
           !(std::is_same< typename std::iterator_traits< InputIterator>::iterator_category, 
                         std::input_iterator_tag 
                       >::value), typename bolt::amp::iterator_traits<InputIterator>::difference_type
                       >::type
    count( bolt::amp::control &ctl,
                const InputIterator& first,
                const InputIterator& last,
                const Predicate& predicate)
    {

        typedef typename bolt::amp::iterator_traits<InputIterator>::difference_type rType;
	    
        size_t szElements = (size_t)(last - first);
        if (szElements == 0)
            return 0;
	    
        bolt::amp::control::e_RunMode runMode = ctl.getForceRunMode();  // could be dynamic choice some day.
        if(runMode == bolt::amp::control::Automatic)
        {
            runMode = ctl.getDefaultPathToRun();
        }
        switch(runMode)
        {	    
        case bolt::amp::control::MultiCoreCpu:
        #ifdef ENABLE_TBB
        {
              return btbb::count( ctl, first, last,  predicate, 
				  typename std::iterator_traits< InputIterator >::iterator_category( ) );
	    
        }
        #else
        {
              throw std::runtime_error( "The MultiCoreCpu version of reduce is not enabled to be built! \n" );
        }
        #endif
	    
        case bolt::amp::control::SerialCpu:
        {
              return  serial::count( ctl, first, last,  predicate, 
				  typename std::iterator_traits< InputIterator >::iterator_category( ) );
	    
        }

        default: /* Incase of runMode not set/corrupted */
        {           
             return  amp::count( ctl, first, last,  predicate, 
				  typename std::iterator_traits< InputIterator >::iterator_category( ) );
        }
	   
       }

    }

    template<typename InputIterator, typename Predicate>
    typename std::enable_if< 
           (std::is_same< typename std::iterator_traits< InputIterator>::iterator_category, 
                         std::input_iterator_tag 
                       >::value), typename bolt::amp::iterator_traits<InputIterator>::difference_type
                       >::type
    count( bolt::amp::control &ctl,
                const InputIterator& first,
                const InputIterator& last,
                const Predicate& predicate)
    {
		      //  TODO:  It should be possible to support non-random_access_iterator_tag iterators, if we copied
              //   the data to a temporary buffer.  Should we?
              static_assert( std::is_same< typename std::iterator_traits< InputIterator>::iterator_category, 
                                           std::input_iterator_tag >::value , 
                             "Input vector cannot be of the type input_iterator_tag" );
    }


} //end of detail


template<typename InputIterator, typename Predicate>
typename bolt::amp::iterator_traits<InputIterator>::difference_type
    count_if(control& ctl, InputIterator first,
    InputIterator last,
    Predicate predicate)
{
      return detail::count(ctl, first, last, predicate);
}

template<typename InputIterator, typename Predicate>
typename bolt::amp::iterator_traits<InputIterator>::difference_type
    count_if( InputIterator first,
    InputIterator last,
    Predicate predicate)
{

  return count_if(bolt::amp::control::getDefault(), first, last, predicate);

}

} //end of amp

}//end of bolt

#endif //COUNT_INL
