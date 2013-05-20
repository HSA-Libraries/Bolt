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

#pragma once
#if !defined( BOLT_AMP_TRANSFORM_REDUCE_INL )
#define BOLT_AMP_TRANSFORM_REDUCE_INL

#include <string>
#include <iostream>
#include <numeric>
#ifdef ENABLE_TBB
//TBB Includes
#include "bolt/btbb/transform_reduce.h"

#endif


#define WAVEFRONT_SIZE 64

#define _REDUCE_STEP(_LENGTH, _IDX, _W) \
    if ((_IDX < _W) && ((_IDX + _W) < _LENGTH)) {\
      iType mine = scratch[_IDX];\
      iType other = scratch[_IDX + _W];\
      scratch[_IDX] = reduce_op(mine, other); \
    }\
    t_idx.barrier.wait();

namespace bolt {
    namespace amp {

        // The following two functions are visible in .h file
        // Wrapper that user passes a control class
        template< typename InputIterator,
                  typename UnaryFunction,
                  typename T,
                  typename BinaryFunction >
        T transform_reduce( control& ctl,
                            InputIterator first,
                            InputIterator last,
                            UnaryFunction transform_op,
                            T init,
                            BinaryFunction reduce_op )
        {
            return detail::transform_reduce_detect_random_access( ctl, first, last, transform_op, init, reduce_op,
                std::iterator_traits< InputIterator >::iterator_category( ) );
        };

        // Wrapper that generates default control class
        template< typename InputIterator,
                  typename UnaryFunction,
                  typename T,
                  typename BinaryFunction >
        T transform_reduce( InputIterator first,
                            InputIterator last,
                            UnaryFunction transform_op,
                            T init,
                            BinaryFunction reduce_op )
        {
            return detail::transform_reduce_detect_random_access( control::getDefault(), first, last, transform_op,
                                                                                                    init, reduce_op,
                std::iterator_traits< InputIterator >::iterator_category( ) );
        };
    }; //end of namespace amp
}; //end of namespace bolt

namespace bolt {

  namespace amp {

    namespace  detail {

        //  The following two functions disallow non-random access functions
        // Wrapper that uses default control class, iterator interface
        template< typename InputIterator,
                  typename UnaryFunction,
                  typename T,
                  typename BinaryFunction >
        T transform_reduce_detect_random_access( control &ctl,
                                                 const InputIterator& first,
                                                 const InputIterator& last,
                                                 const UnaryFunction& transform_op,
                                                 const T& init,
                                                 const BinaryFunction& reduce_op,
                                                 std::input_iterator_tag )
        {
            //  TODO:  It should be possible to support non-random_access_iterator_tag iterators,if we copied the data
            //  to a temporary buffer.  Should we?
            static_assert( false, "Bolt only supports random access iterator types" );
        };

        // Wrapper that uses default control class, iterator interface
        template< typename InputIterator,
                  typename UnaryFunction,
                  typename T,
                  typename BinaryFunction >
        T transform_reduce_detect_random_access( control& ctl,
                                                 const InputIterator& first,
                                                 const InputIterator& last,
                                                 const UnaryFunction& transform_op,
                                                 const T& init,
                                                 const BinaryFunction& reduce_op,
                                                 std::random_access_iterator_tag )
        {
            return transform_reduce_pick_iterator( ctl, first, last, transform_op, init, reduce_op );
        };


        // This template is called by the non-detail versions of transform_reduce, it already assumes
        // random access iterators
        // This is called strictly for any non-device_vector iterator
        template< typename InputIterator,
                  typename UnaryFunction,
                  typename oType,
                  typename BinaryFunction >
        typename std::enable_if< !std::is_base_of<typename device_vector<typename std::iterator_traits<InputIterator>::value_type>::iterator,InputIterator>::value, oType >::type
        transform_reduce_pick_iterator(
            control &c,
            const InputIterator& first,
            const InputIterator& last,
            const UnaryFunction& transform_op,
            const oType& init,
            const BinaryFunction& reduce_op )
        {
            typedef std::iterator_traits<InputIterator>::value_type iType;
            size_t szElements = (last - first);
            if (szElements == 0)
                    return init;

            const bolt::amp::control::e_RunMode runMode = c.getForceRunMode();  // could be dynamic choice some day.
            if (runMode == bolt::amp::control::SerialCpu)
            {
                //Create a temporary array to store the transform result;
                //throw std::exception( "transform_reduce device_vector CPU device not implemented" );
                std::vector<oType> output(szElements);
                std::transform(first, last, output.begin(),transform_op);
                return std::accumulate(output.begin(), output.end(), init, reduce_op);
            }
            else if (runMode == bolt::amp::control::MultiCoreCpu)
            {
#ifdef ENABLE_TBB

                    return bolt::btbb::transform_reduce(first,last,transform_op,init,reduce_op);
#else
                    throw std::exception(  "The MultiCoreCpu version of transform_reduce is not enabled to be built.");
                    return init;
#endif
                 }
            else
            {
                // Map the input iterator to a device_vector
                device_vector< iType, concurrency::array_view > dvInput( first, last, false, c );

                return  transform_reduce_enqueue( c, dvInput.begin( ), dvInput.end( ), transform_op, init, reduce_op );
            }
        };

        // This template is called by the non-detail versions of transform_reduce,
        // it already assumes random access iterators
        // This is called strictly for iterators that are derived from device_vector< T >::iterator
        template< typename DVInputIterator,
                  typename UnaryFunction,
                  typename oType,
                  typename BinaryFunction >
        typename std::enable_if< std::is_base_of<typename device_vector<typename std::iterator_traits<DVInputIterator>::value_type>::iterator,DVInputIterator>::value, oType >::type
        transform_reduce_pick_iterator(
            control &c,
            const DVInputIterator& first,
            const DVInputIterator& last,
            const UnaryFunction& transform_op,
            const oType& init,
            const BinaryFunction& reduce_op )
        {
            typedef std::iterator_traits<DVInputIterator>::value_type iType;
            size_t szElements = (last - first);
            if (szElements == 0)
                    return init;

            const bolt::amp::control::e_RunMode runMode = c.getForceRunMode();  // could be dynamic choice some day.
            if (runMode == bolt::amp::control::SerialCpu)
            {
               std::vector<iType> InputBuffer(szElements);
               for(unsigned int index=0; index<szElements; index++){
                   InputBuffer[index] = first.getContainer().getBuffer()[index];
               }
               std::vector<oType> output(szElements);
               std::transform(InputBuffer.begin(), InputBuffer.end(), output.begin(),transform_op);

               return std::accumulate(output.begin(), output.end(), init, reduce_op);

            }
            else if (runMode == bolt::amp::control::MultiCoreCpu)
            {

#ifdef ENABLE_TBB
               std::vector<iType> InputBuffer(szElements);
               for(unsigned int index=0; index<szElements; index++)
               {
                   InputBuffer[index] = first.getContainer().getBuffer()[index];
               }
            return bolt::btbb::transform_reduce(InputBuffer.begin(),InputBuffer.end(),transform_op,init,reduce_op);

#else
               throw std::exception(  "The MultiCoreCpu version of transform_reduce is not enabled to be built." );
               return init;
#endif
            }
            else
            {
            return  transform_reduce_enqueue( c, first, last, transform_op, init, reduce_op );
            }
        };

        template<typename DVInputIterator, typename UnaryFunction, typename oType, typename BinaryFunction>
        oType transform_reduce_enqueue( control& ctl,
                                        const DVInputIterator& first,
                                        const DVInputIterator& last,
                                        const UnaryFunction& transform_op,
                                        const oType& init,
                                        const BinaryFunction& reduce_op )
        {
            typedef typename std::iterator_traits< DVInputIterator  >::value_type iType;

            const int szElements = static_cast< unsigned int >( std::distance( first, last ) );
            const unsigned int tileSize = WAVEFRONT_SIZE;
            unsigned int numTiles = (szElements/tileSize);
            const unsigned int ceilNumTiles = static_cast< size_t >( std::ceil( static_cast< float >( szElements )
                                                                                                    / tileSize) );
            unsigned int ceilNumElements = tileSize * ceilNumTiles;

            concurrency::array_view< iType, 1 > inputV (first.getContainer().getBuffer());

            //Now create a staging array ; May support zero-copy in the future?!
           concurrency::accelerator cpuAccelerator=concurrency::accelerator(concurrency::accelerator::cpu_accelerator);
            concurrency::accelerator_view cpuAcceleratorView = cpuAccelerator.default_view;
            concurrency::array< iType, 1 > resultArray ( szElements, ctl.getAccelerator().default_view,
                                                                                    cpuAcceleratorView);

            concurrency::array_view<iType, 1> result ( resultArray );
            result.discard_data();

            concurrency::extent< 1 > inputExtent( ceilNumElements );
            concurrency::tiled_extent< tileSize > tiledExtentTransformReduce = inputExtent.tile< tileSize >();

            // Algorithm is different from cl::transform_reduce. We launch worksize = number of elements here.
            // AMP doesn't have APIs to get CU capacity. Launchable size is great though.

            try
            {
               concurrency::parallel_for_each(ctl.getAccelerator().default_view,
                                              tiledExtentTransformReduce, [=]

                                              //todo: Capturing data by value is giving an unexpected compile error
                                              // Using [=] as default capture mode

                                              //[ inputV,
                                              //  szElements,
                                              //  result,
                                              //  transform_op,
                                              //  reduce_op ]
                                              ( concurrency::tiled_index<tileSize> t_idx ) mutable restrict(amp)
               {
                 int globalId = t_idx.global[ 0 ];
                 int tileIndex = t_idx.local[ 0 ];
                 //  Initialize local data store
                 tile_static iType scratch [WAVEFRONT_SIZE] ;


                 //  Abort threads that are passed the end of the input vector
                 if( t_idx.global[ 0 ] < szElements )
                 {

                  //  Initialize the accumulator private variable with data from the input array
                  //  This essentially unrolls the loop below at least once
                  iType accumulator = transform_op ( inputV[globalId] );
                  scratch[tileIndex] = accumulator;

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
                     result[t_idx.tile[ 0 ]] = scratch[0];
                 }


               });

               oType *cpuPointerReduce =  result.data();

               int numTailReduce = ceilNumTiles;
               oType acc = static_cast< oType >( init );
               for(int i = 0; i < numTailReduce; ++i)
               {
                   acc = reduce_op( acc, result[ i ] );
               }

               return acc ;


            }
            catch(std::exception &e)
            {
              std::cout<<"Exception while calling bolt::amp::transform_reduce parallel_for_each!\n"
                       <<std::endl<<e.what();
              throw std::exception();
            }

        };

    };// end of namespace detail
  };// end of namespace cl
};// end of namespace bolt

#endif
