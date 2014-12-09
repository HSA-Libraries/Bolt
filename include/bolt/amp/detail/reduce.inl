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

///////////////////////////////////////////////////////////////////////////////
// AMP REDUCE
//////////////////////////////////////////////////////////////////////////////

#if !defined( BOLT_AMP_REDUCE_INL )
#define BOLT_AMP_REDUCE_INL
#define REDUCE_WAVEFRONT_SIZE 256 //64
#define _REDUCE_STEP(_LENGTH, _IDX, _W) \
if ((_IDX < _W) && ((_IDX + _W) < _LENGTH)) {\
	iType mine = scratch[_IDX]; \
	iType other = scratch[_IDX + _W]; \
	scratch[_IDX] = binary_op(mine, other); \
}\
    t_idx.barrier.wait();

#ifdef BOLT_ENABLE_PROFILING
#include "bolt/AsyncProfiler.h"
//AsyncProfiler aProfiler("transform");
#endif


#ifdef ENABLE_TBB
//TBB Includes
#include "bolt/btbb/reduce.h"

#endif

#include <algorithm>
#include <type_traits>
#include "bolt/amp/bolt.h"
#include "bolt/amp/device_vector.h"
#include "bolt/amp/iterator/iterator_traits.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace bolt{
namespace amp{
namespace detail{

namespace serial{
    template<typename T, typename InputIterator, typename BinaryFunction>
    T reduce(bolt::amp::control &ctl,
                const InputIterator& first,
                const InputIterator& last,
                const T& init,
                const BinaryFunction& binary_op,
				std::random_access_iterator_tag)
    {
		return std::accumulate(first, last, init, binary_op);
    }

	template<typename T, typename InputIterator, typename BinaryFunction>
    T reduce(bolt::amp::control &ctl,
                const InputIterator& first,
                const InputIterator& last,
                const T& init,
                const BinaryFunction& binary_op,
				bolt::amp::fancy_iterator_tag)
    {
		return std::accumulate(first, last, init, binary_op);
    }

	//std::accumulate also works fine with device_vector as input, but it does a map & unmap for every element.
	//Added device_vector specialization for reduce to overcome the above drawback of std::accumulate functions default behaviour, 
	// by doing map & unmap only once for all the elements.
	template<typename T, typename InputIterator, typename BinaryFunction>
    T reduce(bolt::amp::control &ctl,
                const InputIterator& first,
                const InputIterator& last,
                const T& init,
                const BinaryFunction& binary_op,
				bolt::amp::device_vector_tag)
    {

          typedef typename std::iterator_traits< InputIterator >::value_type iType;
		  typename bolt::amp::device_vector< iType >::pointer reduceInputBuffer =  first.getContainer( ).data( );
          return std::accumulate(  &reduceInputBuffer[first.m_Index], &reduceInputBuffer[ last.m_Index ],
                                               init, binary_op);
    }

} // end of namespace serial

#ifdef ENABLE_TBB
namespace btbb{

	template<typename T, typename InputIterator, typename BinaryFunction>
    T reduce(bolt::amp::control &ctl,
                const InputIterator& first,
                const InputIterator& last,
                const T& init,
                const BinaryFunction& binary_op,
				std::random_access_iterator_tag)
    {
		return bolt::btbb::reduce(first, last, init, binary_op);
    }

	template<typename T, typename InputIterator, typename BinaryFunction>
    T reduce(bolt::amp::control &ctl,
                const InputIterator& first,
                const InputIterator& last,
                const T& init,
                const BinaryFunction& binary_op,
				bolt::amp::fancy_iterator_tag)
    {
		return bolt::btbb::reduce(first, last, init, binary_op);
    }

	//btbb::reduce works fine with device_vector as input, but it does a map & unmap for every element. 
	//Added device_vector specialization for reduce to overcome the above drawback of std::accumulate functions default behaviour, 
	//by doing map & unmap only once for all the elements.
    template<typename T, typename InputIterator, typename BinaryFunction>
    T reduce(bolt::amp::control &ctl,
                const InputIterator& first,
                const InputIterator& last,
                const T& init,
                const BinaryFunction& binary_op,
				bolt::amp::device_vector_tag)
    {

        typedef typename std::iterator_traits< InputIterator >::value_type iType;
		typename bolt::amp::device_vector< iType >::pointer reduceInputBuffer =  first.getContainer( ).data( );
        return bolt::btbb::reduce(  &reduceInputBuffer[first.m_Index], &reduceInputBuffer[ last.m_Index ],
                                               init, binary_op);
	  
    }
} // end of namespace btbb
#endif

namespace amp{

    // This is the base implementation of reduction that is called by all of the convenience wrappers below.
    // first and last must be iterators from a DeviceVector
	template<typename T, typename DVInputIterator, typename BinaryFunction>
    T reduce_enqueue(bolt::amp::control &ctl,
        DVInputIterator& first,
        DVInputIterator& last,
        T& init,
        BinaryFunction& binary_op)
	{

		        typedef typename std::iterator_traits< DVInputIterator >::value_type iType;

        const int szElements = static_cast< int >( std::distance( first, last ) );

		int max_ComputeUnits = 32;
		int numTiles = max_ComputeUnits*32;			/* Max no. of WG for Tahiti(32 compute Units) and 32 is the tuning factor that gives good performance*/
		int length = (REDUCE_WAVEFRONT_SIZE*numTiles);	
		length = szElements < length ? szElements : length;
		unsigned int residual = length % REDUCE_WAVEFRONT_SIZE;
		length = residual ? (length + REDUCE_WAVEFRONT_SIZE - residual): length ;
		
		numTiles = static_cast< int >((szElements/REDUCE_WAVEFRONT_SIZE)>= numTiles?(numTiles):
							(std::ceil( static_cast< float >( szElements ) / REDUCE_WAVEFRONT_SIZE) ));

		concurrency::array<iType, 1> result(numTiles, ctl.getAccelerator().default_view);

		concurrency::extent< 1 > inputExtent(length);
        concurrency::tiled_extent< REDUCE_WAVEFRONT_SIZE > tiledExtentReduce = inputExtent.tile< REDUCE_WAVEFRONT_SIZE >();

        // Algorithm is different from cl::reduce. We launch worksize = number of elements here.
        // AMP doesn't have APIs to get CU capacity. Launchable size is great though.

        try
        {
                concurrency::parallel_for_each(ctl.getAccelerator().default_view,
                                               tiledExtentReduce,
                                               [ first,
											 szElements,
											 length,
                                                 &result,
                                                 binary_op ]
                                               ( concurrency::tiled_index<REDUCE_WAVEFRONT_SIZE> t_idx ) restrict(amp)
                {
				int gx = t_idx.global[0];
				int gloId = gx;
				tile_static iType scratch[REDUCE_WAVEFRONT_SIZE];
				//  Initialize local data store
				unsigned int tileIndex = t_idx.local[0];

				iType accumulator;
				if (gloId < szElements)
				{
					accumulator = first[gx];
					gx += length;
				}
				

				// Loop sequentially over chunks of input vector, reducing an arbitrary size input
				// length into a length related to the number of workgroups
				while (gx < szElements)
				{
					iType element = first[gx];
					accumulator = binary_op(accumulator, element);
					gx += length;
				}
                    
				scratch[tileIndex] = accumulator;
				t_idx.barrier.wait();

				unsigned int tail = szElements - (t_idx.tile[0] * REDUCE_WAVEFRONT_SIZE);

				_REDUCE_STEP(tail, tileIndex, 128);
				_REDUCE_STEP(tail, tileIndex, 64);
				_REDUCE_STEP(tail, tileIndex, 32);
				_REDUCE_STEP(tail, tileIndex, 16);
				_REDUCE_STEP(tail, tileIndex, 8);
				_REDUCE_STEP(tail, tileIndex, 4);
				_REDUCE_STEP(tail, tileIndex, 2);
				_REDUCE_STEP(tail, tileIndex, 1);


				//  Abort threads that are passed the end of the input vector
				if (gloId >= szElements)
					return;

                  //  Write only the single reduced value for the entire workgroup
                  if (tileIndex == 0)
                  {
                      result[t_idx.tile[ 0 ]] = scratch[0];
                  }

                });
                
			iType acc = static_cast<iType>(init);
			std::vector<iType> *cpuPointerReduce = new std::vector<iType>(numTiles);

			concurrency::copy(result, (*cpuPointerReduce).begin());
			for(int i = 0; i < numTiles; ++i)
			{
				acc = binary_op(acc, (*cpuPointerReduce)[i]);
			}
			delete cpuPointerReduce;
			return acc;
            }
            catch(std::exception &e)
            {
                  std::cout << "Exception while calling bolt::amp::reduce parallel_for_each " ;
                  std::cout<< e.what() << std::endl;
                  throw std::exception();
            }

	}

    template<typename T, typename DVInputIterator, typename BinaryFunction>
    T reduce(bolt::amp::control &ctl,
        DVInputIterator& first,
        DVInputIterator& last,
        T& init,
        BinaryFunction& binary_op,
	    bolt::amp::device_vector_tag)
    {
		return reduce_enqueue( ctl, first, last, init, binary_op);
    }

    
    /*! \brief This template function overload is used strictly std random access vectors and AMP implementations. 
        \detail 
    */
    template<typename T, typename InputIterator, typename BinaryFunction>
    T reduce(bolt::amp::control &ctl,
                InputIterator first,
                InputIterator last,
                T& init,
                BinaryFunction& binary_op,
                std::random_access_iterator_tag)
    {
        int sz = static_cast<int>(last - first);
        if (sz == 0)
            return init;
        typedef typename std::iterator_traits<InputIterator>::value_type  iType;

		device_vector< iType, concurrency::array_view > dvInput( first, last, false, ctl );
        return reduce_enqueue( ctl, dvInput.begin(), dvInput.end(), init, binary_op);
    }

    template<typename T, typename InputIterator, typename BinaryFunction>
    T reduce(bolt::amp::control &ctl,
                InputIterator first,
                InputIterator last,
                T init,
                BinaryFunction& binary_op,
                bolt::amp::fancy_iterator_tag)
    {
        return reduce_enqueue(ctl, first, last, init, binary_op);
    }

} //end of namespace amp


    /*! \brief This template function overload is used strictly for device vectors and std random access vectors. 
        \detail Here we branch out into the SerialCpu, MultiCore TBB or The AMP code paths. 
    */
    template<typename T, typename InputIterator, typename BinaryFunction>
    T reduce(bolt::amp::control &ctl,
                InputIterator first,
                InputIterator last,
                T init,
                BinaryFunction& binary_op)
    {
        int sz = static_cast<int>( std::distance(first, last ) );
        if (sz == 0)
            return init;

        bolt::amp::control::e_RunMode runMode = ctl.getForceRunMode();  // could be dynamic choice some day.
        if(runMode == bolt::amp::control::Automatic)
        {
           runMode = ctl.getDefaultPathToRun();
        }
    
        if( runMode == bolt::amp::control::SerialCpu )
        {
            return serial::reduce(ctl, first, last, init, binary_op, typename std::iterator_traits<InputIterator>::iterator_category());
        }
        else if( runMode == bolt::amp::control::MultiCoreCpu )
        {
#if defined( ENABLE_TBB )
            return btbb::reduce(ctl, first, last, init, binary_op, typename std::iterator_traits<InputIterator>::iterator_category() );
#else
            throw std::runtime_error( "The MultiCoreCpu version of transform is not enabled to be built! \n" );
#endif
        }
        else
        {
            return amp::reduce(ctl, first, last, init, binary_op, typename std::iterator_traits<InputIterator>::iterator_category() );
        }
        return init;
    }


};//end of namespace detail

        //////////////////////////////////////////
        //  Reduce overloads
        //////////////////////////////////////////
        // default control, two-input transform, std:: iterator

        template<typename InputIterator>
        typename std::iterator_traits<InputIterator>::value_type
            reduce(InputIterator first,
            InputIterator last)
        {
            typedef typename std::iterator_traits<InputIterator>::value_type iType;
            return reduce(bolt::amp::control::getDefault(), first, last, iType(), bolt::amp::plus<iType>());
        };

        template<typename InputIterator>
        typename std::iterator_traits<InputIterator>::value_type
            reduce(bolt::amp::control &ctl,
            InputIterator first,
            InputIterator last)
        {
            typedef typename std::iterator_traits<InputIterator>::value_type iType;
            return reduce(ctl, first, last, iType(), bolt::amp::plus< iType >( ));
        };

        template< typename InputIterator,
                 typename T >
        T reduce( InputIterator first,
                InputIterator last,
                T init )
        {
            typedef typename std::iterator_traits<InputIterator>::value_type iType;
            return reduce(bolt::amp::control::getDefault(), first, last, init, bolt::amp::plus<iType>());
        };

        template< typename InputIterator,
                  typename T >
        T reduce( bolt::amp::control &ctl,
                InputIterator first,
                InputIterator last,
                T init )
        {
            typedef typename std::iterator_traits<InputIterator>::value_type iType;
            return reduce(ctl, first, last, init, bolt::amp::plus< iType >( ));

        };

		template< typename InputIterator,
                  typename T,
                  typename BinaryFunction >
        T reduce( InputIterator first,
                  InputIterator last,
                  T init,
                  BinaryFunction binary_op )
        {
            return reduce(bolt::amp::control::getDefault(), first, last, init, binary_op);
        };


        // This template is called by all other "convenience" versions of reduce.
        // It also implements the CPU-side mappings of the algorithm for SerialCpu and MultiCoreCpu
        template< typename InputIterator,
                  typename T,
                  typename BinaryFunction >
        T reduce( bolt::amp::control &ctl,
                  InputIterator first,
                  InputIterator last,
                  T init,
                  BinaryFunction binary_op )
        {
               return detail::reduce(ctl, first, last, init, binary_op);

        };

    }; //end of namespace amp
}; //end of namespace bolt

#endif // AMP_REDUCE_INL