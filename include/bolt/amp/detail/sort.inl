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

#if !defined( AMP_SORT_INL )
#define AMP_SORT_INL
#pragma once

#include <algorithm>
#include <type_traits>

//#include <boost/bind.hpp>
//#include <boost/thread/once.hpp>
//#include <boost/shared_array.hpp>

#include "bolt/amp/bolt.h"
#include "bolt/amp/scan.h"
#include "bolt/amp/functional.h"
#include "bolt/amp/device_vector.h"
#include <amp.h>

#ifdef ENABLE_TBB
#include "tbb/parallel_sort.h"
#include "tbb/task_scheduler_init.h"
#endif


#define BOLT_UINT_MAX 0xFFFFFFFFU
#define BOLT_UINT_MIN 0x0U
#define BOLT_INT_MAX 0x7FFFFFFFU
#define BOLT_INT_MIN 0x80000000U

#define BITONIC_SORT_WGSIZE 64
/* \brief - SORT_CPU_THRESHOLD should be atleast 2 times the BITONIC_SORT_WGSIZE*/
#define SORT_CPU_THRESHOLD 128 

namespace bolt {
namespace amp {
template<typename RandomAccessIterator>
void sort(RandomAccessIterator first,
          RandomAccessIterator last)
{
    typedef std::iterator_traits< RandomAccessIterator >::value_type T;

    detail::sort_detect_random_access( bolt::amp::control::getDefault( ),
                                       first, last, less< T >( ),
                                       std::iterator_traits< RandomAccessIterator >::iterator_category( ) );
    return;
}

template<typename RandomAccessIterator, typename StrictWeakOrdering>
void sort(RandomAccessIterator first,
            RandomAccessIterator last,
            StrictWeakOrdering comp)
{
    detail::sort_detect_random_access( bolt::amp::control::getDefault( ),
                                       first, last, comp,
                                       std::iterator_traits< RandomAccessIterator >::iterator_category( ) );
    return;
}

template<typename RandomAccessIterator>
void sort(bolt::amp::control &ctl,
            RandomAccessIterator first,
            RandomAccessIterator last)
{
    typedef std::iterator_traits< RandomAccessIterator >::value_type T;
    detail::sort_detect_random_access(ctl,
                                        first, last, less< T >( ),
                                        std::iterator_traits< RandomAccessIterator >::iterator_category( ) );
    return;
}

template<typename RandomAccessIterator, typename StrictWeakOrdering>
void sort(bolt::amp::control &ctl,
            RandomAccessIterator first,
            RandomAccessIterator last,
            StrictWeakOrdering comp)
{
    detail::sort_detect_random_access(ctl,
                                        first, last, comp,
                                        std::iterator_traits< RandomAccessIterator >::iterator_category( ) );
    return;
}
}// End of amp namespace
};// End of bolt namespace


namespace bolt {
namespace amp {
namespace detail {

// Wrapper that uses default control class, iterator interface
template<typename RandomAccessIterator, typename StrictWeakOrdering>
void sort_detect_random_access( bolt::amp::control &ctl,
                                const RandomAccessIterator& first, const RandomAccessIterator& last,
                                const StrictWeakOrdering& comp, std::input_iterator_tag )
{
    //  \TODO:  It should be possible to support non-random_access_iterator_tag iterators, if we copied the data
    //  to a temporary buffer.  Should we?
    static_assert( false, "Bolt only supports random access iterator types" );
};

template<typename RandomAccessIterator, typename StrictWeakOrdering>
void sort_detect_random_access( bolt::amp::control &ctl,
                                const RandomAccessIterator& first, const RandomAccessIterator& last,
                                const StrictWeakOrdering& comp, std::random_access_iterator_tag )
{
    return sort_pick_iterator(ctl, first, last, comp,
                              std::iterator_traits< RandomAccessIterator >::iterator_category( ) );
};

//Device Vector specialization
template<typename DVRandomAccessIterator, typename StrictWeakOrdering>
void sort_pick_iterator( bolt::amp::control &ctl,
                         const DVRandomAccessIterator& first, const DVRandomAccessIterator& last,
                         const StrictWeakOrdering& comp, bolt::amp::device_vector_tag )
{
    // User defined Data types are not supported with device_vector. Hence we have a static assert here.
    // The code here should be in compliant with the routine following this routine.
    typedef typename std::iterator_traits<DVRandomAccessIterator>::value_type T;
    unsigned int szElements = static_cast< unsigned int >( std::distance( first, last ) );
    if(szElements > (1<<31))
  {
        throw std::exception( "AMP device_vector shall support only upto 2 power 31 elements" );
    return;
  }
    if (szElements == 0 )
        return;
    const bolt::amp::control::e_RunMode runMode = ctl.getForceRunMode();  // could be dynamic choice some day.
    if ((runMode == bolt::amp::control::SerialCpu) || (szElements < SORT_CPU_THRESHOLD)) {
        std::vector<T> localBuffer(szElements);
        //Copy the device_vector buffer to a CPU buffer
        for(unsigned int index=0; index<szElements; index++)
            localBuffer[index] = first.getBuffer()[index];
        //Compute sort using std::sort
        std::sort(localBuffer.begin(), localBuffer.end(), comp);
        //Copy the CPU buffer back to device_vector 
        for(unsigned int index=0; index<szElements; index++)
            first.getBuffer()[index] = localBuffer[index];
        return;
    } else if (runMode == bolt::amp::control::MultiCoreCpu) {
#ifdef ENABLE_TBB
        std::vector<T> localBuffer(szElements);
        //Copy the device_vector buffer to a CPU buffer
        for(unsigned int index=0; index<szElements; index++)
            localBuffer[index] = first.getBuffer()[index];
        tbb::task_scheduler_init initialize(tbb::task_scheduler_init::automatic);
        tbb::parallel_sort(localBuffer.begin(), localBuffer.end(), comp);

        std::sort(localBuffer.begin(), localBuffer.end(), comp);

        //Copy the CPU buffer back to device_vector 
        for(unsigned int index=0; index<szElements; index++)
            first.getBuffer()[index] = localBuffer[index];
        return;
        
#else
 //       std::cout << "The MultiCoreCpu version of sort is not enabled. " << std ::endl;
        throw std::exception( "The MultiCoreCpu version of sort is not enabled to be built." ); 
        return;
#endif
    } else {
        sort_enqueue(ctl,first,last,comp);
    }
    return;
}

#if 0
template<typename DVRandomAccessIterator, typename StrictWeakOrdering>
void sort_pick_iterator( control &ctl,
                         const DVRandomAccessIterator& first, const DVRandomAccessIterator& last,
                         const StrictWeakOrdering& comp, bolt::amp::fancy_iterator_tag )
{
    static_assert( false, "It is not possible to sort fancy iterators. They are not mutable" );
}
#endif

//Non Device Vector specialization.
//This implementation creates a cl::Buffer and passes the cl buffer to the sort specialization whichtakes
//the cl buffer as a parameter. In the future, Each input buffer should be mapped to the device_vector
//and the specialization specific to device_vector should be called.
template<typename RandomAccessIterator, typename StrictWeakOrdering>
void sort_pick_iterator( bolt::amp::control &ctl,
                         const RandomAccessIterator& first, const RandomAccessIterator& last,
                         const StrictWeakOrdering& comp, std::random_access_iterator_tag )
{
    typedef typename std::iterator_traits<RandomAccessIterator>::value_type T;
    unsigned int szElements = static_cast< unsigned int >( std::distance( first, last ) );
    if(szElements > (1<<31))
        throw std::exception( "AMP device vectors shall support only upto 2 power 31 elements" );
    if (szElements == 0)
        return;

    const bolt::amp::control::e_RunMode runMode = ctl.getForceRunMode();  // could be dynamic choice some day.
    if ((runMode == bolt::amp::control::SerialCpu) || (szElements < SORT_CPU_THRESHOLD)) {
        std::sort(first, last, comp);
        return;
    } else if (runMode == bolt::amp::control::MultiCoreCpu) {
#ifdef ENABLE_TBB
        tbb::task_scheduler_init initialize(tbb::task_scheduler_init::automatic);
        tbb::parallel_sort(first,last, comp);
#else
//        std::cout << "The MultiCoreCpu version of sort is not enabled. " << std ::endl;
        throw std::exception( "The MultiCoreCpu version of sort is not enabled to be built." ); 
        return;
#endif
    } else {
        device_vector< T, concurrency::array_view > dvInputOutput( first, last, false, ctl );
        //Now call the actual amp algorithm
        sort_enqueue(ctl,dvInputOutput.begin(),dvInputOutput.end(),comp);
        //Map the buffer back to the host
        dvInputOutput.data( );
        return;
    }
}

/****** sort_enqueue specailization for unsigned int data types. ******
 * THE FOLLOWING CODE IMPLEMENTS THE RADIX SORT ALGORITHM FOR sort()
 *********************************************************************/

/*AMP Kernels for unsigned integer sorting*/
template <typename T, int N, typename Container>
void AMP_RadixSortHistogramAscendingKernel(bolt::amp::control &ctl,
                                           Container &unsortedData, /*this can be either array_view or array*/
                                           Container &buckets,
                                           Container &histScanBuckets,
                                           unsigned int shiftCount,
                                           unsigned int szElements,
                                           const unsigned int groupSize)
{
    const int RADICES = 1 << N;
    concurrency::extent< 1 > globalSizeK0( szElements/RADICES );

    concurrency::tiled_extent< RADICES > tileK0 = globalSizeK0.tile< RADICES >();
    concurrency::accelerator_view av = ctl.getAccelerator().default_view;
    printf("globalSizeK0 = %d   tileK0.tile_dim0 = %d    tileK0[0]=%d\n",globalSizeK0, tileK0.tile_dim0, tileK0[0]);
    concurrency::parallel_for_each( av, tileK0, 
    [
        unsortedData,
        buckets,
        histScanBuckets,
        shiftCount,
        tileK0
    ] 
    ( concurrency::tiled_index< RADICES > t_idx ) restrict(amp)
    {
        const int RADIX_T     = N;
        const int RADICES_T   = (1 << RADIX_T);
        const int NUM_OF_ELEMENTS_PER_WORK_ITEM_T = RADICES_T; 
        const int MASK_T      = (1<<RADIX_T)  - 1;

        int localId     = t_idx.local[ 0 ];
        int globalId    = t_idx.global[ 0 ];
        int groupId     = t_idx.tile[ 0 ];
        int groupSize   = tileK0.tile_dim0;
        //tileK0[ 0 ] gives the global size 
        //tileK0.tile_dim0 gives the number of threads in a tile
        //\TODO - find a API to get the total number of numOfGroups
        int numOfGroups = tileK0[ 0 ]/tileK0.tile_dim0;//get_num_groups(0);
        unsigned int bucketPos   = groupId * RADICES_T * groupSize;
        for(int i = 0; i < RADICES_T; ++i)
        {
            buckets[bucketPos + localId * RADICES_T + i] = 0;
        }
        //barrier(CLK_GLOBAL_MEM_FENCE)
        t_idx.barrier.wait_with_all_memory_fence();

        /* Calculate thread-histograms */
        for(int i = 0; i < NUM_OF_ELEMENTS_PER_WORK_ITEM_T; ++i)
        {
            unsigned int value = unsortedData[globalId * NUM_OF_ELEMENTS_PER_WORK_ITEM_T + i];
            value = (value >> shiftCount) & MASK_T;
            buckets[bucketPos + localId * RADICES_T + value]++;
        }

        //barrier(CLK_GLOBAL_MEM_FENCE)
        t_idx.barrier.wait_with_all_memory_fence();

        //Start First step to scan
        int sum =0;
        for(int i = 0; i < groupSize; i++)
        {
            sum = sum + buckets[bucketPos + localId + groupSize*i];
        }
        histScanBuckets[localId*numOfGroups + groupId + 1] = sum;

    } );// end of concurrency::parallel_for_each
}

template <typename T, int N, typename Container>
void AMP_RadixSortHistogramDescendingKernel(bolt::amp::control &ctl,
                                           Container &unsortedData, /*this can be either array_view or array*/
                                           Container &buckets,
                                           Container &histScanBuckets,
                                           unsigned int shiftCount,
                                           unsigned int szElements,
                                           const unsigned int groupSize)
{
    const int RADICES = 1 << N;
    concurrency::extent< 1 > globalSizeK0( szElements/RADICES );
    concurrency::tiled_extent< RADICES > tileK0 = globalSizeK0.tile< RADICES >();
    concurrency::accelerator_view av = ctl.getAccelerator().default_view;
    concurrency::parallel_for_each( av, tileK0, 
    [
        unsortedData,
        buckets,
        histScanBuckets,
        shiftCount,
        tileK0
    ] 
    ( concurrency::tiled_index< RADICES > t_idx ) restrict(amp)
    {
        const int RADIX_T     = N;
        const int RADICES_T   = (1 << RADIX_T);
        const int NUM_OF_ELEMENTS_PER_WORK_ITEM_T = RADICES_T; 
        const int MASK_T      = (1<<RADIX_T)  -1;
        int localId     = t_idx.local[ 0 ];
        int globalId    = t_idx.global[ 0 ];
        int groupId     = t_idx.tile[ 0 ];
        int groupSize   = tileK0.tile_dim0;
        //tileK0[ 0 ] gives the global size 
        //tileK0.tile_dim0 gives the number of threads in a tile
        //\TODO - find a API to get the total number of numOfGroups
        int numOfGroups = tileK0[ 0 ]/tileK0.tile_dim0;
        /*size_t localId     = get_local_id(0);
        size_t globalId    = get_global_id(0);
        size_t groupId     = get_group_id(0);
        size_t groupSize   = get_local_size(0);
        size_t numOfGroups = get_num_groups(0);*/
        unsigned int bucketPos   = groupId * RADICES_T * groupSize;

        for(int i = 0; i < RADICES_T; ++i)
        {
            buckets[bucketPos + localId * RADICES_T + i] = 0;
        }
        //barrier(CLK_GLOBAL_MEM_FENCE)
        t_idx.barrier.wait_with_all_memory_fence();

        /* Calculate thread-histograms */
        for(int i = 0; i < NUM_OF_ELEMENTS_PER_WORK_ITEM_T; ++i)
        {
            unsigned int value = unsortedData[globalId * NUM_OF_ELEMENTS_PER_WORK_ITEM_T + i];
            value = (value >> shiftCount) & MASK_T;
            buckets[bucketPos + localId * RADICES_T + (RADICES_T - value -1)]++;
        }

        //barrier(CLK_GLOBAL_MEM_FENCE)
        t_idx.barrier.wait_with_all_memory_fence();
        //Start First step to scan
        int sum = 0;
        for(int i = 0; i < groupSize; i++)
            sum = sum + buckets[bucketPos + localId + groupSize*i];
        histScanBuckets[localId*numOfGroups + groupId + 1] = sum;
    } );// end of concurrency::parallel_for_each
}

template <typename T, int N, typename Container>
void AMP_scanLocalTemplate(bolt::amp::control &ctl,
                           Container &buckets,
                           Container &histScanBuckets,
                           unsigned int szElements)
{
    const int RADICES = 1<<N;
    concurrency::extent< 1 > globalSizeK0( szElements/RADICES );
    concurrency::tiled_extent< RADICES > tileK0 = globalSizeK0.tile< RADICES >();
    concurrency::accelerator_view av = ctl.getAccelerator().default_view;
    concurrency::parallel_for_each( av, tileK0, 
    [
        buckets,
        histScanBuckets,
        tileK0
    ] 
    ( concurrency::tiled_index< RADICES > t_idx ) restrict(amp)
    {
        const int RADIX_T     = N;
        const int RADICES_T   = (1 << RADIX_T);
        //create tiled_static for localScanArray
        tile_static T localScanArray[2*RADICES_T];
        /*size_t localId     = get_local_id(0); 
        size_t numOfGroups = get_num_groups(0);
        size_t groupId     = get_group_id(0);
        size_t groupSize   = get_local_size(0);*/
        int localId     = t_idx.local[ 0 ];
        int groupId     = t_idx.tile[ 0 ];
        int groupSize   = tileK0.tile_dim0;
        //\TODO - find a API to get the total number of numOfGroups
        int numOfGroups = tileK0[ 0 ]/tileK0.tile_dim0;

        localScanArray[localId] = histScanBuckets[localId*numOfGroups + groupId];
        localScanArray[RADICES_T+localId] = 0;

        //barrier(CLK_LOCAL_MEM_FENCE);
        t_idx.barrier.wait();
        for(int i = 0; i < RADICES_T; ++i)
        {
            unsigned int bucketPos = groupId * RADICES_T * groupSize + i * RADICES_T + localId;
            unsigned int temp = buckets[bucketPos];
            buckets[bucketPos] = localScanArray[RADICES_T+localId] + localScanArray[localId];
            localScanArray[RADICES_T+localId] += temp;
        }
    });
}

template <typename T, int N, typename Container>
void AMP_permuteAscendingRadixNTemplate(bolt::amp::control &ctl,
                                    Container &unsortedData,
                                    Container &scanedBuckets,
                                    unsigned int shiftCount,
                                    Container &sortedData,
                                    unsigned int szElements)
{
    const int RADICES = 1 << N;
    concurrency::extent< 1 > globalSizeK0( szElements/RADICES );
    concurrency::tiled_extent< RADICES > tileK0 = globalSizeK0.tile< RADICES >();
    concurrency::accelerator_view av = ctl.getAccelerator().default_view;
    concurrency::parallel_for_each( av, tileK0, 
    [
        unsortedData,
        scanedBuckets,
        shiftCount,
        sortedData,
        tileK0
    ] 
    ( concurrency::tiled_index< RADICES > t_idx ) restrict(amp)
    {
        const int RADIX_T     = N;
        const int RADICES_T   = (1 << RADIX_T);
        //const int NUM_OF_ELEMENTS_PER_WORK_ITEM_T = RADICES_T; 
        const int MASK_T      = (1<<RADIX_T)  -1;
        /*size_t groupId   = get_group_id(0);
        size_t localId   = get_local_id(0);
        size_t globalId  = get_global_id(0);
        size_t groupSize = get_local_size(0);*/
        int localId     = t_idx.local[ 0 ];
        int globalId    = t_idx.global[ 0 ];
        int groupId     = t_idx.tile[ 0 ];
        int groupSize   = tileK0.tile_dim0;

        unsigned int bucketPos   = groupId * RADICES_T * groupSize;

        /* Premute elements to appropriate location */
        for(int i = 0; i < RADICES_T; ++i)
        {
            unsigned int value = unsortedData[globalId * RADICES_T + i];
            value = (value >> shiftCount) & MASK_T;
            unsigned int index = scanedBuckets[bucketPos+localId * RADICES_T + value];
            sortedData[index] = unsortedData[globalId * RADICES_T + i];
            scanedBuckets[bucketPos+localId * RADICES_T + value] = index + 1;
            //barrier(CLK_LOCAL_MEM_FENCE);
            t_idx.barrier.wait();
        }
    } );
}

template <typename T, int N, typename Container>
void AMP_permuteDescendingRadixNTemplate(bolt::amp::control &ctl,
                                    Container &unsortedData,
                                    Container &scanedBuckets,
                                    unsigned int shiftCount,
                                    Container &sortedData,
                                    unsigned int szElements)
{
    const int RADICES = 1 << N;
    concurrency::extent< 1 > globalSizeK0( szElements/RADICES );
    concurrency::tiled_extent< RADICES > tileK0 = globalSizeK0.tile< RADICES >();
    concurrency::accelerator_view av = ctl.getAccelerator().default_view;
    concurrency::parallel_for_each( av, tileK0, 
    [
        unsortedData,
        scanedBuckets,
        shiftCount,
        sortedData,
        tileK0
    ] 
    ( concurrency::tiled_index< RADICES > t_idx ) restrict(amp)
    {
        const int RADIX_T     = N;
        const int RADICES_T   = (1 << RADIX_T);
        const int MASK_T      = (1<<RADIX_T)  -1;
        int localId     = t_idx.local[ 0 ];
        int globalId    = t_idx.global[ 0 ];
        int groupId     = t_idx.tile[ 0 ];
        int groupSize   = tileK0.tile_dim0;
        /*size_t groupId   = get_group_id(0);
        size_t localId   = get_local_id(0);
        size_t globalId  = get_global_id(0);
        size_t groupSize = get_local_size(0);*/
        unsigned int bucketPos   = groupId * RADICES_T * groupSize;

        /* Premute elements to appropriate location */
        for(int i = 0; i < RADICES_T; ++i)
        {
            unsigned int value = unsortedData[globalId * RADICES_T + i];
            value = (value >> shiftCount) & MASK_T;
            unsigned int index = scanedBuckets[bucketPos+localId * RADICES_T + (RADICES_T -value-1)];
            sortedData[index] = unsortedData[globalId * RADICES_T + i];
            scanedBuckets[bucketPos+localId * RADICES_T + (RADICES_T -value-1)] = index + 1;
            //barrier(CLK_LOCAL_MEM_FENCE);
            t_idx.barrier.wait();
        }
    } );
}

/*AMP Kernels for Signed integer sorting*/
/*! 
* \brief This template function is a permutation Algorithm for one work item. 
*        This is called by the the sort_enqueue for input buffers with siigned numbers routine for other than int's and unsigned int's
*        
* \details
*         The idea behind sorting signed numbers is that when we sort the MSB which contains the sign bit. We should 
*         sort in the signed bit in the descending order. For example in a radix 4 sort of signed numbers if we are 
*         doing ascending sort. Then the bits 28:31 should be sorted in an descending order, after doing the following 
*         transformation of the input bits. 
*         Shift bits 31:28 ->  3:0
*               value = (value >> shiftCount);
*         Retain the sign bit in the bit location 3. 
*               signBit = value & (1<<(RADIX_T-1));
*         XOR bits 2:1  with 111 
*               value = ( ( ( value & MASK_T ) ^ MASK_T ) & MASK_T )
*         Or the sign bit with the xor'ed bit. This forms your index.
*               value = value | signBitl
*
*       As an Example for Ascending order sort of 4 bit signed numbers.
*       ________________________________
*       -Flip the three bits other than the sign bit
*       -sort the resultant array considering them as unsigned numbers
*       -map the corresponding flipped numbers with the corresponding values.
*       1st Example
*       1111     1000      0000     0111
*       0111     0000      0101     0010     
*       1000 ->  1111 ->   0111 ->  0000     
*       1010     1101      1000     1111     
*       0000     0111      1101     1010     
*       0010     0101      1111     1000
*       
*       2nd Example 
*       1111     1000     1111      1000
*       0111     0000     1101      1010
*       1101     1010     1010      1101
*       1000     1111     1000      1111
*       1010 ->  1101 ->  0111 ->   0000
*       0000     0111     0101      0010
*       0010     0101     0010      0101
*       0101     0010     0001      0110
*       0110     0001     0000      0111
*
*/
/*Descending*/
template <typename T, int N, typename Container>
void AMP_RadixSortHistogramSignedAscendingKernel(bolt::amp::control &ctl,
                                           Container &unsortedData, /*this can be either array_view or array*/
                                           Container &buckets,
                                           Container &histScanBuckets,
                                           unsigned int shiftCount,
                                           unsigned int szElements,
                                           const unsigned int groupSize)
{
    const int RADICES = 1 << N;
    concurrency::extent< 1 > globalSizeK0( szElements/RADICES );

    concurrency::tiled_extent< RADICES > tileK0 = globalSizeK0.tile< RADICES >();
    concurrency::accelerator_view av = ctl.getAccelerator().default_view;
    printf("globalSizeK0 = %d   tileK0.tile_dim0 = %d    tileK0[0]=%d\n",globalSizeK0, tileK0.tile_dim0, tileK0[0]);
    concurrency::parallel_for_each( av, tileK0, 
    [
        unsortedData,
        buckets,
        histScanBuckets,
        shiftCount,
        tileK0
    ] 
    ( concurrency::tiled_index< RADICES > t_idx ) restrict(amp)
    {
        const int RADIX_T     = N;
        const int RADICES_T   = (1 << RADIX_T);
        const int NUM_OF_ELEMENTS_PER_WORK_ITEM_T = RADICES_T; 
        const int MASK_T      = ( 1 << ( RADIX_T - 1 ) ) - 1;

        int localId     = t_idx.local[ 0 ];
        int globalId    = t_idx.global[ 0 ];
        int groupId     = t_idx.tile[ 0 ];
        int groupSize   = tileK0.tile_dim0;
        //tileK0[ 0 ] gives the global size 
        //tileK0.tile_dim0 gives the number of threads in a tile
        //\TODO - find a API to get the total number of numOfGroups
        int numOfGroups = tileK0[ 0 ]/tileK0.tile_dim0;//get_num_groups(0);
        unsigned int bucketPos   = groupId * RADICES_T * groupSize;
        for(int i = 0; i < RADICES_T; ++i)
        {
            buckets[bucketPos + localId * RADICES_T + i] = 0;
        }
        //barrier(CLK_GLOBAL_MEM_FENCE)
        t_idx.barrier.wait_with_all_memory_fence();

        /* Calculate thread-histograms */
        for(int i = 0; i < NUM_OF_ELEMENTS_PER_WORK_ITEM_T; ++i)
        {
            unsigned int value = unsortedData[globalId * NUM_OF_ELEMENTS_PER_WORK_ITEM_T + i];
            value = (value >> shiftCount);
            unsigned int signBit = value & (1<<(RADIX_T-1));
            value = ( ( ( value & MASK_T ) ^ MASK_T ) & MASK_T ) | signBit;
            buckets[bucketPos + localId * RADICES_T + value]++;
        }

        //barrier(CLK_GLOBAL_MEM_FENCE)
        t_idx.barrier.wait_with_all_memory_fence();

        //Start First step to scan
        int sum =0;
        for(int i = 0; i < groupSize; i++)
        {
            sum = sum + buckets[bucketPos + localId + groupSize*i];
        }
        histScanBuckets[localId*numOfGroups + groupId + 1] = sum;

    } );// end of concurrency::parallel_for_each
}

template <typename T, int N, typename Container>
void AMP_RadixSortHistogramSignedDescendingKernel(bolt::amp::control &ctl,
                                           Container &unsortedData, /*this can be either array_view or array*/
                                           Container &buckets,
                                           Container &histScanBuckets,
                                           unsigned int shiftCount,
                                           unsigned int szElements,
                                           const unsigned int groupSize)
{
    const int RADICES = 1 << N;
    concurrency::extent< 1 > globalSizeK0( szElements/RADICES );
    concurrency::tiled_extent< RADICES > tileK0 = globalSizeK0.tile< RADICES >();
    concurrency::accelerator_view av = ctl.getAccelerator().default_view;
    concurrency::parallel_for_each( av, tileK0, 
    [
        unsortedData,
        buckets,
        histScanBuckets,
        shiftCount,
        tileK0
    ] 
    ( concurrency::tiled_index< RADICES > t_idx ) restrict(amp)
    {
        const int RADIX_T     = N;
        const int RADICES_T   = (1 << RADIX_T);
        const int NUM_OF_ELEMENTS_PER_WORK_ITEM_T = RADICES_T; 
        const int MASK_T      = ( 1 << ( RADIX_T - 1 ) ) - 1;
        int localId     = t_idx.local[ 0 ];
        int globalId    = t_idx.global[ 0 ];
        int groupId     = t_idx.tile[ 0 ];
        int groupSize   = tileK0.tile_dim0;
        //tileK0[ 0 ] gives the global size 
        //tileK0.tile_dim0 gives the number of threads in a tile
        //\TODO - find a API to get the total number of numOfGroups
        int numOfGroups = tileK0[ 0 ]/tileK0.tile_dim0;
        /*size_t localId     = get_local_id(0);
        size_t globalId    = get_global_id(0);
        size_t groupId     = get_group_id(0);
        size_t groupSize   = get_local_size(0);
        size_t numOfGroups = get_num_groups(0);*/
        unsigned int bucketPos   = groupId * RADICES_T * groupSize;

        for(int i = 0; i < RADICES_T; ++i)
        {
            buckets[bucketPos + localId * RADICES_T + i] = 0;
        }
        //barrier(CLK_GLOBAL_MEM_FENCE)
        t_idx.barrier.wait_with_all_memory_fence();

        /* Calculate thread-histograms */
        for(int i = 0; i < NUM_OF_ELEMENTS_PER_WORK_ITEM_T; ++i)
        {
            unsigned int value = unsortedData[globalId * NUM_OF_ELEMENTS_PER_WORK_ITEM_T + i];
            value = (value >> shiftCount);
            unsigned int signBit = value & (1<<(RADIX_T-1));
            value = ( ( ( ( value & MASK_T ) ^ MASK_T ) & MASK_T ) | signBit );
            buckets[bucketPos + localId * RADICES_T + (RADICES_T - value -1)]++;
        }

        //barrier(CLK_GLOBAL_MEM_FENCE)
        t_idx.barrier.wait_with_all_memory_fence();
        //Start First step to scan
        int sum = 0;
        for(int i = 0; i < groupSize; i++)
            sum = sum + buckets[bucketPos + localId + groupSize*i];
        histScanBuckets[localId*numOfGroups + groupId + 1] = sum;
    } );// end of concurrency::parallel_for_each
}

template <typename T, int N, typename Container>
void AMP_permuteSignedAscendingRadixNTemplate(bolt::amp::control &ctl,
                                    Container &unsortedData,
                                    Container &scanedBuckets,
                                    unsigned int shiftCount,
                                    Container &sortedData,
                                    unsigned int szElements)
{
    const int RADICES = 1 << N;
    concurrency::extent< 1 > globalSizeK0( szElements/RADICES );
    concurrency::tiled_extent< RADICES > tileK0 = globalSizeK0.tile< RADICES >();
    concurrency::accelerator_view av = ctl.getAccelerator().default_view;
    concurrency::parallel_for_each( av, tileK0, 
    [
        unsortedData,
        scanedBuckets,
        shiftCount,
        sortedData,
        tileK0
    ] 
    ( concurrency::tiled_index< RADICES > t_idx ) restrict(amp)
    {
        const int RADIX_T     = N;
        const int RADICES_T   = (1 << RADIX_T);
        //const int NUM_OF_ELEMENTS_PER_WORK_ITEM_T = RADICES_T; 
        const int MASK_T      = ( 1 << ( RADIX_T - 1 ) )  -1;
        /*size_t groupId   = get_group_id(0);
        size_t localId   = get_local_id(0);
        size_t globalId  = get_global_id(0);
        size_t groupSize = get_local_size(0);*/
        int localId     = t_idx.local[ 0 ];
        int globalId    = t_idx.global[ 0 ];
        int groupId     = t_idx.tile[ 0 ];
        int groupSize   = tileK0.tile_dim0;

        unsigned int bucketPos   = groupId * RADICES_T * groupSize;

        /* Premute elements to appropriate location */
        for(int i = 0; i < RADICES_T; ++i)
        {
            unsigned int value = unsortedData[globalId * RADICES_T + i];
            unsigned int resultValue = value;
            value = (value >> shiftCount);
            unsigned int signBit = value & (1<<(RADIX_T-1));
            value = ( ( ( ( value & MASK_T ) ^ MASK_T ) & MASK_T ) | signBit );

            unsigned int index = scanedBuckets[bucketPos+localId * RADICES_T + value];
            sortedData[index] = unsortedData[globalId * RADICES_T + i];
            scanedBuckets[bucketPos+localId * RADICES_T + value] = index + 1;
            //barrier(CLK_LOCAL_MEM_FENCE);
            t_idx.barrier.wait();
        }
    } );
}

template <typename T, int N, typename Container>
void AMP_permuteSignedDescendingRadixNTemplate(bolt::amp::control &ctl,
                                    Container &unsortedData,
                                    Container &scanedBuckets,
                                    unsigned int shiftCount,
                                    Container &sortedData,
                                    unsigned int szElements)
{
    const int RADICES = 1 << N;
    concurrency::extent< 1 > globalSizeK0( szElements/RADICES );
    concurrency::tiled_extent< RADICES > tileK0 = globalSizeK0.tile< RADICES >();
    concurrency::accelerator_view av = ctl.getAccelerator().default_view;
    concurrency::parallel_for_each( av, tileK0, 
    [
        unsortedData,
        scanedBuckets,
        shiftCount,
        sortedData,
        tileK0
    ] 
    ( concurrency::tiled_index< RADICES > t_idx ) restrict(amp)
    {
        const int RADIX_T     = N;
        const int RADICES_T   = (1 << RADIX_T);
        const int MASK_T      = ( 1 << ( RADIX_T - 1 ) )  -1;
        int localId     = t_idx.local[ 0 ];
        int globalId    = t_idx.global[ 0 ];
        int groupId     = t_idx.tile[ 0 ];
        int groupSize   = tileK0.tile_dim0;
        /*size_t groupId   = get_group_id(0);
        size_t localId   = get_local_id(0);
        size_t globalId  = get_global_id(0);
        size_t groupSize = get_local_size(0);*/
        unsigned int bucketPos   = groupId * RADICES_T * groupSize;

        /* Premute elements to appropriate location */
        for(int i = 0; i < RADICES_T; ++i)
        {
            unsigned int value = unsortedData[globalId * RADICES_T + i];
            //value = (value >> shiftCount) & MASK_T;
            value = (value >> shiftCount);
            unsigned int signBit = value & (1<<(RADIX_T-1));
            value = ( ( ( ( value & MASK_T ) ^ MASK_T ) & MASK_T ) | signBit );
            unsigned int index = scanedBuckets[bucketPos+localId * RADICES_T + (RADICES_T -value-1)];
            sortedData[index] = unsortedData[globalId * RADICES_T + i];
            scanedBuckets[bucketPos+localId * RADICES_T + (RADICES_T -value-1)] = index + 1;
            //barrier(CLK_LOCAL_MEM_FENCE);
            t_idx.barrier.wait();
        }
    } );
}



#define BOLT_SORT_INL_DEBUG 0
template<typename DVRandomAccessIterator, typename StrictWeakOrdering> 
typename std::enable_if< std::is_same< typename std::iterator_traits<DVRandomAccessIterator >::value_type, unsigned int >::value >::type
sort_enqueue(bolt::amp::control &ctl, 
             DVRandomAccessIterator &first, DVRandomAccessIterator &last,
             StrictWeakOrdering comp)
{
    typedef typename std::iterator_traits< DVRandomAccessIterator >::value_type T;
    const int RADIX = 8;
    const int RADICES = (1 << RADIX);
    unsigned int orig_szElements = static_cast<unsigned int>(std::distance(first, last));
    unsigned int szElements = orig_szElements;
    bool  newBuffer = false;
    concurrency::array_view<T> *pLocalArrayView = NULL;
    concurrency::array<T>      *pLocalArray = NULL;
    unsigned int groupSize = RADICES;
    unsigned int mulFactor = groupSize * RADICES;
    concurrency::extent<1> ext( static_cast< int >( orig_szElements ) );

    if(orig_szElements%mulFactor != 0)
    {
        szElements  = ((orig_szElements + mulFactor) /mulFactor) * mulFactor;
        concurrency::extent<1> modified_ext( static_cast< int >( szElements ) );
        pLocalArray     = new concurrency::array<T>( modified_ext );
        pLocalArrayView = new concurrency::array_view<T>(pLocalArray->view_as(modified_ext));
        concurrency::array_view<T> dest = pLocalArrayView->section( ext );
        first.getBuffer( ).copy_to( dest );
        dest.synchronize( );
        newBuffer = true;
    }
    else
    {
        pLocalArrayView = new concurrency::array_view<T>( first.getBuffer( ) );
    }

    unsigned int numGroups = szElements / mulFactor;
    concurrency::extent<1> modified_ext( static_cast< int >( szElements ) );
    
    device_vector< T, concurrency::array > dvSwapInputData(static_cast<size_t>(szElements), 0);
    device_vector< T, concurrency::array > dvHistogramBins(static_cast<size_t>(numGroups* groupSize * RADICES), 0);
    device_vector< T, concurrency::array > dvHistogramScanBuffer(static_cast<size_t>(numGroups* RADICES + 10), 0 );

    auto& clInputData = *pLocalArrayView;
    auto& clSwapData = dvSwapInputData.begin( ).getBuffer( );
    auto& clHistData = dvHistogramBins.begin( ).getBuffer( );
    auto& clHistScanData = dvHistogramScanBuffer.begin( ).getBuffer( );
    int swap = 0;
    if(comp(2,3))
    {
        /*If the buffer is a local buffer which is more than the usual buffer size then */
        if(newBuffer == true)
        {
            concurrency::index<1> origin(orig_szElements);
            concurrency::array_view<T> dest = pLocalArrayView->section( origin, modified_ext - ext);
            //arrayview_type m_devMemoryAV( *m_devMemory );
            Concurrency::parallel_for_each( dest.extent, [dest]
                (Concurrency::index<1> idx) restrict(amp)
                {
                    dest[idx] = BOLT_UINT_MAX;
                }
            );
        }
        /*Ascending Sort*/
        for(int bits = 0; bits < (sizeof(T) * 8)/*Bits per Byte*/; bits += RADIX)
        {
            if (swap == 0)
                AMP_RadixSortHistogramAscendingKernel<T, RADIX>(ctl,
                                                      clInputData, /*this can be either array_view or array*/
                                                      clHistData,
                                                      clHistScanData,
                                                      bits,
                                                      szElements,
                                                      groupSize); // \TODO - i don't need gropu size
            else
                AMP_RadixSortHistogramAscendingKernel<T, RADIX>(ctl,
                                                      clSwapData, /*this can be either array_view or array*/
                                                      clHistData,
                                                      clHistScanData,
                                                      bits,
                                                      szElements,
                                                      groupSize); // \TODO - i don't need gropu size
#if BOLT_SORT_INL_DEBUG

printf("\n\n\n\n\nBITS = %d\nAfter Histogram", bits);
for (unsigned int ng=0; ng<numGroups; ng++)
{ printf ("\nGroup-Block =%d",ng);
    for(unsigned int gS=0;gS<groupSize; gS++)
    { printf ("\nGroup =%d\n",gS);
        for(int i=0; i<RADICES;i++)
        {
            int index = ng * groupSize * RADICES + gS * RADICES + i;
            int value = clHistData[ index ];
            printf("%2x %2x, ",index, value);
        }
    }
}
int temp = 0;
printf("\n Printing Histogram scan SUM\n");
for(int i=0; i<RADICES;i++)
{ 
    printf ("\nRadix = %d\n",i);
    for (unsigned int ng=0; ng<numGroups; ng++)
    {
        printf ("%4x, ",clHistScanData[i*numGroups + ng]);
    }
}
#endif
            detail::scan_enqueue( ctl, dvHistogramScanBuffer.begin( ), dvHistogramScanBuffer.end( ),dvHistogramScanBuffer.begin( ), 0, plus< T >( ) );
#if BOLT_SORT_INL_DEBUG
printf("\nprinting scan_enqueue SUM\n");        
        for(int i=0; i<RADICES;i++)
        { 
            printf ("\nRadix = %d\n",i);
            for (unsigned int ng=0; ng<numGroups; ng++)
            {
                printf ("%4x, ",clHistScanData[i*numGroups + ng]);
            }
        }
#endif
            AMP_scanLocalTemplate<T, RADIX>(ctl,
                                        clHistData,
                                        clHistScanData,
                                        szElements);
#if BOLT_SORT_INL_DEBUG
        printf("\n\nAfter Scan bits = %d", bits);
        for (unsigned int ng=0; ng<numGroups; ng++)
        { printf ("\nGroup-Block =%d",ng);
            for(unsigned int gS=0;gS<groupSize; gS++)
            { printf ("\nGroup =%d\n",gS);
                for(int i=0; i<RADICES;i++)
                {
                    int index = ng * groupSize * RADICES + gS * RADICES + i;
                    int value = clHistData[ index ];
                    printf("%4x %4x, ",index, value);
                }
            }
        }
#endif
            if (swap == 0)
                AMP_permuteAscendingRadixNTemplate<T, RADIX>( ctl,
                                                        clInputData,
                                                        clHistData,
                                                        bits,
                                                        clSwapData,
                                                        szElements);
            else
                AMP_permuteAscendingRadixNTemplate<T, RADIX>( ctl,
                                                        clSwapData,
                                                        clHistData,
                                                        bits,
                                                        clInputData,
                                                        szElements);
#if BOLT_SORT_INL_DEBUG
if (swap == 0) 
{
        printf("\n Printing swap data\n");
        for(unsigned int i=0; i<szElements;i+= RADICES)
        {
            for(int j =0;j< RADICES;j++)
                printf("%8x %8x, ",i+j,clSwapData[i+j]);
            printf("\n");
        }
}
else
{
        printf("\n Printing swap data\n");
        for(unsigned int i=0; i<szElements;i+= RADICES)
        {
            for(int j =0;j< RADICES;j++)
                printf("%8x %8x, ",i+j,clInputData[i+j]);
            printf("\n");
        }
}
#endif
            if(swap==0)
                swap = 1;
            else
                swap = 0;
        }
    }
    else
    {
        if(newBuffer == true)
        {
            concurrency::index<1> origin(orig_szElements);
            concurrency::array_view<T> dest = pLocalArrayView->section( origin, modified_ext - ext);
            //arrayview_type m_devMemoryAV( *m_devMemory );
            Concurrency::parallel_for_each( dest.extent, [dest]
                (Concurrency::index<1> idx) restrict(amp)
                {
                    dest[idx] = BOLT_UINT_MIN;
                }
            );
        }
        /*Ascending Sort*/
        for(int bits = 0; bits < (sizeof(T) * 8)/*Bits per Byte*/; bits += RADIX)
        {
            if (swap == 0)
                AMP_RadixSortHistogramDescendingKernel<T, RADIX>(ctl,
                                                      clInputData, /*this can be either array_view or array*/
                                                      clHistData,
                                                      clHistScanData,
                                                      bits,
                                                      szElements,
                                                      groupSize); // \TODO - i don't need gropu size
            else
                AMP_RadixSortHistogramDescendingKernel<T, RADIX>(ctl,
                                                      clSwapData, /*this can be either array_view or array*/
                                                      clHistData,
                                                      clHistScanData,
                                                      bits,
                                                      szElements,
                                                      groupSize); // \TODO - i don't need gropu size

            detail::scan_enqueue( ctl, dvHistogramScanBuffer.begin( ), dvHistogramScanBuffer.end( ),dvHistogramScanBuffer.begin( ), 0, plus< T >( ) );

            AMP_scanLocalTemplate<T, RADIX>(ctl,
                                        clHistData,
                                        clHistScanData,
                                        szElements);

            if (swap == 0)
                AMP_permuteDescendingRadixNTemplate<T, RADIX>( ctl,
                                                        clInputData,
                                                        clHistData,
                                                        bits,
                                                        clSwapData,
                                                        szElements);
            else
                AMP_permuteDescendingRadixNTemplate<T, RADIX>( ctl,
                                                        clSwapData,
                                                        clHistData,
                                                        bits,
                                                        clInputData,
                                                        szElements);
            /*For swapping the buffers*/
            swap = swap? 0: 1;
        }//End of For loop
    }
    if(newBuffer == true)
    {
        std::cout << "New buffer was allocated So copying back the buffer\n";
        //dest = clInputData.section( ext );
        clInputData.section( ext ).copy_to( first.getBuffer( ) );
        first.getBuffer( ).synchronize( );
        delete pLocalArray;
    }
    delete pLocalArrayView;
    return;
}


/*
 *
 */
template<typename DVRandomAccessIterator, typename StrictWeakOrdering> 
typename std::enable_if< std::is_same< typename std::iterator_traits<DVRandomAccessIterator >::value_type,          int >::value >::type
sort_enqueue(bolt::amp::control &ctl, 
             DVRandomAccessIterator &first, DVRandomAccessIterator &last,
             StrictWeakOrdering comp)
{
    typedef typename std::iterator_traits< DVRandomAccessIterator >::value_type T;
    const int RADIX = 4;
    const int RADICES = (1 << RADIX);
    unsigned int orig_szElements = static_cast<unsigned int>(std::distance(first, last));
    unsigned int szElements = orig_szElements;
    bool  newBuffer = false;
    concurrency::array_view<T> *pLocalArrayView = NULL;
    concurrency::array<T>      *pLocalArray = NULL;
    unsigned int groupSize = RADICES;
    unsigned int mulFactor = groupSize * RADICES;
    concurrency::extent<1> ext( static_cast< int >( orig_szElements ) );

    if(orig_szElements%mulFactor != 0)
    {
        szElements  = ((orig_szElements + mulFactor) /mulFactor) * mulFactor;
        concurrency::extent<1> modified_ext( static_cast< int >( szElements ) );
        pLocalArray     = new concurrency::array<T>( modified_ext );
        pLocalArrayView = new concurrency::array_view<T>(pLocalArray->view_as( modified_ext ) );
        concurrency::array_view<T> dest = pLocalArrayView->section( ext );
        first.getBuffer( ).copy_to( dest );
        dest.synchronize( );
        newBuffer = true;
    }
    else
    {
        pLocalArrayView = new concurrency::array_view<T>( first.getBuffer( ) );
    }

    unsigned int numGroups = szElements / mulFactor;
    concurrency::extent<1> modified_ext( static_cast< int >( szElements ) );
    
    device_vector< T, concurrency::array > dvSwapInputData(static_cast<size_t>(szElements), 0);
    device_vector< T, concurrency::array > dvHistogramBins(static_cast<size_t>(numGroups* groupSize * RADICES), 0);
    device_vector< T, concurrency::array > dvHistogramScanBuffer(static_cast<size_t>(numGroups* RADICES + 10), 0 );

    auto& clInputData = *pLocalArrayView;
    auto& clSwapData = dvSwapInputData.begin( ).getBuffer( );
    auto& clHistData = dvHistogramBins.begin( ).getBuffer( );
    auto& clHistScanData = dvHistogramScanBuffer.begin( ).getBuffer( );
    int swap = 0;
    if(comp(2,3))
    {
        /*If the buffer is a local buffer which is more than the usual buffer size then */
        if(newBuffer == true)
        {
            concurrency::index<1> origin(orig_szElements);
            concurrency::array_view<T> dest = pLocalArrayView->section( origin, modified_ext - ext);
            //arrayview_type m_devMemoryAV( *m_devMemory );
            Concurrency::parallel_for_each( dest.extent, [dest]
                (Concurrency::index<1> idx) restrict(amp)
                {
                    dest[idx] = BOLT_INT_MAX;
                }
            );
        }
        /*Ascending Sort*/
        int bits = 0;
        for(bits = 0; bits < ((sizeof(T) * 8) - RADIX); bits += RADIX)
        {
            if (swap == 0)
                AMP_RadixSortHistogramAscendingKernel<T, RADIX>(ctl,
                                                      clInputData, /*this can be either array_view or array*/
                                                      clHistData,
                                                      clHistScanData,
                                                      bits,
                                                      szElements,
                                                      groupSize); // \TODO - i don't need group size
            else
                AMP_RadixSortHistogramAscendingKernel<T, RADIX>(ctl,
                                                      clSwapData, /*this can be either array_view or array*/
                                                      clHistData,
                                                      clHistScanData,
                                                      bits,
                                                      szElements,
                                                      groupSize); // \TODO - i don't need group size
#if BOLT_SORT_INL_DEBUG

printf("\n\n\n\n\nBITS = %d\nAfter Histogram", bits);
for (unsigned int ng=0; ng<numGroups; ng++)
{ printf ("\nGroup-Block =%d",ng);
    for(unsigned int gS=0;gS<groupSize; gS++)
    { printf ("\nGroup =%d\n",gS);
        for(int i=0; i<RADICES;i++)
        {
            int index = ng * groupSize * RADICES + gS * RADICES + i;
            int value = clHistData[ index ];
            printf("%2x %2x, ",index, value);
        }
    }
}
int temp = 0;
printf("\n Printing Histogram scan SUM\n");
for(int i=0; i<RADICES;i++)
{ 
    printf ("\nRadix = %d\n",i);
    for (unsigned int ng=0; ng<numGroups; ng++)
    {
        printf ("%4x, ",clHistScanData[i*numGroups + ng]);
    }
}
#endif
            detail::scan_enqueue( ctl, dvHistogramScanBuffer.begin( ), dvHistogramScanBuffer.end( ),dvHistogramScanBuffer.begin( ), 0, plus< T >( ) );
#if BOLT_SORT_INL_DEBUG
printf("\nprinting scan_enqueue SUM\n");        
        for(int i=0; i<RADICES;i++)
        { 
            printf ("\nRadix = %d\n",i);
            for (unsigned int ng=0; ng<numGroups; ng++)
            {
                printf ("%4x, ",clHistScanData[i*numGroups + ng]);
            }
        }
#endif
            AMP_scanLocalTemplate<T, RADIX>(ctl,
                                        clHistData,
                                        clHistScanData,
                                        szElements);
#if BOLT_SORT_INL_DEBUG
        printf("\n\nAfter Scan bits = %d", bits);
        for (unsigned int ng=0; ng<numGroups; ng++)
        { printf ("\nGroup-Block =%d",ng);
            for(unsigned int gS=0;gS<groupSize; gS++)
            { printf ("\nGroup =%d\n",gS);
                for(int i=0; i<RADICES;i++)
                {
                    int index = ng * groupSize * RADICES + gS * RADICES + i;
                    int value = clHistData[ index ];
                    printf("%4x %4x, ",index, value);
                }
            }
        }
#endif
            if (swap == 0)
                AMP_permuteAscendingRadixNTemplate<T, RADIX>( ctl,
                                                        clInputData,
                                                        clHistData,
                                                        bits,
                                                        clSwapData,
                                                        szElements);
            else
                AMP_permuteAscendingRadixNTemplate<T, RADIX>( ctl,
                                                        clSwapData,
                                                        clHistData,
                                                        bits,
                                                        clInputData,
                                                        szElements);
#if BOLT_SORT_INL_DEBUG
if (swap == 0) 
{
        printf("\n Printing swap data\n");
        for(unsigned int i=0; i<szElements;i+= RADICES)
        {
            for(int j =0;j< RADICES;j++)
                printf("%8x %8x, ",i+j,clSwapData[i+j]);
            printf("\n");
        }
}
else
{
        printf("\n Printing swap data\n");
        for(unsigned int i=0; i<szElements;i+= RADICES)
        {
            for(int j =0;j< RADICES;j++)
                printf("%8x %8x, ",i+j,clInputData[i+j]);
            printf("\n");
        }
}
#endif
            /*For swapping the buffers*/
            swap = swap? 0: 1;
        }
            /* Do descending for the signed bit 
             * IN the case of radix 4 bits = 28 and in the case of radix 8 bits = 24 
             */
             AMP_RadixSortHistogramSignedDescendingKernel<T, RADIX>(ctl,
                                                      clSwapData, /*this can be either array_view or array*/
                                                      clHistData,
                                                      clHistScanData,
                                                      bits,
                                                      szElements,
                                                      groupSize); // \TODO - i don't need gropu size

            detail::scan_enqueue( ctl, dvHistogramScanBuffer.begin( ), dvHistogramScanBuffer.end( ),dvHistogramScanBuffer.begin( ), 0, plus< T >( ) );

            AMP_scanLocalTemplate<T, RADIX>(ctl,
                                        clHistData,
                                        clHistScanData,
                                        szElements);

            AMP_permuteSignedDescendingRadixNTemplate<T, RADIX>( ctl,
                                                        clSwapData,
                                                        clHistData,
                                                        bits,
                                                        clInputData,
                                                        szElements);
            /*End of Ascending for the signed bit */
    }
    else
    {
        if(newBuffer == true)
        {
            concurrency::index<1> origin(orig_szElements);
            concurrency::array_view<T> dest = pLocalArrayView->section( origin, modified_ext - ext);
            //arrayview_type m_devMemoryAV( *m_devMemory );
            Concurrency::parallel_for_each( dest.extent, [dest]
                (Concurrency::index<1> idx) restrict(amp)
                {
                    dest[idx] = BOLT_INT_MIN;
                }
            );
        }
        /* Descending Sort */
        int bits=0;
        for(bits = 0; bits < ((sizeof(T) * 8) - RADIX); bits += RADIX)
        {
            if (swap == 0)
                AMP_RadixSortHistogramDescendingKernel<T, RADIX>(ctl,
                                                      clInputData, /*this can be either array_view or array*/
                                                      clHistData,
                                                      clHistScanData,
                                                      bits,
                                                      szElements,
                                                      groupSize); // \TODO - i don't need gropu size
            else
                AMP_RadixSortHistogramDescendingKernel<T, RADIX>(ctl,
                                                      clSwapData, /*this can be either array_view or array*/
                                                      clHistData,
                                                      clHistScanData,
                                                      bits,
                                                      szElements,
                                                      groupSize); // \TODO - i don't need gropu size

            detail::scan_enqueue( ctl, dvHistogramScanBuffer.begin( ), 
                                  dvHistogramScanBuffer.end( ),dvHistogramScanBuffer.begin( ), 
                                  0, plus< T >( ) );

            AMP_scanLocalTemplate<T, RADIX>(ctl,
                                        clHistData,
                                        clHistScanData,
                                        szElements);

            if (swap == 0)
                AMP_permuteDescendingRadixNTemplate<T, RADIX>( ctl,
                                                        clInputData,
                                                        clHistData,
                                                        bits,
                                                        clSwapData,
                                                        szElements);
            else
                AMP_permuteDescendingRadixNTemplate<T, RADIX>( ctl,
                                                        clSwapData,
                                                        clHistData,
                                                        bits,
                                                        clInputData,
                                                        szElements);
            /*For swapping the buffers*/
            swap = swap? 0: 1;
        }//End of For loop
            /* Do ascending for the signed bit 
             * IN the case of radix 4 bits = 28 and in the case of radix 8 bits = 24 
             */
             AMP_RadixSortHistogramSignedAscendingKernel<T, RADIX>(ctl,
                                                      clSwapData, /*this can be either array_view or array*/
                                                      clHistData,
                                                      clHistScanData,
                                                      bits,
                                                      szElements,
                                                      groupSize); // \TODO - i don't need gropu size

            detail::scan_enqueue( ctl, dvHistogramScanBuffer.begin( ), 
                                  dvHistogramScanBuffer.end( ),dvHistogramScanBuffer.begin( ), 
                                  0, plus< T >( ) );

            AMP_scanLocalTemplate<T, RADIX>(ctl,
                                        clHistData,
                                        clHistScanData,
                                        szElements);

            AMP_permuteSignedAscendingRadixNTemplate<T, RADIX>( ctl,
                                                        clSwapData,
                                                        clHistData,
                                                        bits,
                                                        clInputData,
                                                        szElements);
            /*End of Ascending for the signed bit */

    }
    if(newBuffer == true)
    {
        std::cout << "New buffer was allocated So copying back the buffer\n";
        //dest = clInputData.section( ext );
        clInputData.section( ext ).copy_to( first.getBuffer( ) );
        first.getBuffer( ).synchronize( );
        delete pLocalArray;
    }
    delete pLocalArrayView;
    return;
}

/*! 
* \brief This template function is a Bitonic Algorithm for one work item. 
*        This is called by the the sort_enqueue routine for other than int's and unsigned int's
* \details Container - can be either array or array_view object and is dependant on the 
*                      device_vector of the calling routine
*/
template <typename T, typename Container, typename StrictWeakOrdering>
void AMP_BitonicSortKernel(bolt::amp::control &ctl,
                           Container &A, /*this can be either array_view or array*/
                           int szElements,
                           unsigned int stage,
                           unsigned int passOfStage,
                           StrictWeakOrdering comp)
{
    concurrency::extent< 1 > globalSizeK0( szElements/2 );
    concurrency::tiled_extent< BITONIC_SORT_WGSIZE > tileK0 = globalSizeK0.tile< BITONIC_SORT_WGSIZE >();
    concurrency::accelerator_view av = ctl.getAccelerator().default_view;
    concurrency::parallel_for_each( av, tileK0, 
    [
        A,
        passOfStage,
        stage,
        comp
    ]
    ( concurrency::tiled_index< BITONIC_SORT_WGSIZE > t_idx ) restrict(amp)
    {
        unsigned int  threadId = t_idx.global[ 0 ];
        unsigned int  pairDistance = 1 << (stage - passOfStage);
        unsigned int  blockWidth   = 2 * pairDistance;
        unsigned int  temp;
        unsigned int  leftId = (threadId % pairDistance) 
                            + (threadId / pairDistance) * blockWidth;
        bool compareResult;
    
        unsigned int  rightId = leftId + pairDistance;

        T greater, lesser;
        T leftElement = A[leftId];
        T rightElement = A[rightId];

        unsigned int sameDirectionBlockWidth = 1 << stage;
    
        if((threadId/sameDirectionBlockWidth) % 2 == 1)
        {
            temp = rightId;
            rightId = leftId;
            leftId = temp;
        }

        compareResult = comp(leftElement, rightElement);

        if(compareResult)
        {
            greater = rightElement;
            lesser  = leftElement;
        }
        else
        {
            greater = leftElement;
            lesser  = rightElement;
        }
        A[leftId]  = lesser;
        A[rightId] = greater;
    } );// end of concurrency::parallel_for_each
}

template<typename DVRandomAccessIterator, typename StrictWeakOrdering> 
typename std::enable_if<
    !(std::is_same< typename std::iterator_traits<DVRandomAccessIterator >::value_type, unsigned int >::value || 
      std::is_same< typename std::iterator_traits<DVRandomAccessIterator >::value_type,          int >::value)
                       >::type
sort_enqueue(bolt::amp::control &ctl, const DVRandomAccessIterator& first, const DVRandomAccessIterator& last,
const StrictWeakOrdering& comp)
{
    unsigned int numStages,stage,passOfStage;
    size_t  temp;
    typedef typename std::iterator_traits< DVRandomAccessIterator >::value_type T;

    int szElements = static_cast<int>( std::distance(first, last) );

    if(((szElements-1) & (szElements)) != 0)
    {
        sort_enqueue_non_powerOf2(ctl,first,last,comp);
        return;
    }
    /*if((szElements/2) < BITONIC_SORT_WGSIZE)
    {
        wgSize = (int)szElements/2;
    }*/
    auto&  A = first.getBuffer(); //( numElements, av );

    numStages = 0;
    for(temp = szElements; temp > 1; temp >>= 1)
        ++numStages;
    
    for(stage = 0; stage < numStages; ++stage)
    {
        for(passOfStage = 0; passOfStage < stage + 1; ++passOfStage) 
        {
            //\TODO - can we remove this <T> specialization by getting the data type type from the container.
            AMP_BitonicSortKernel<T>(  ctl,
                                    A, 
                                    szElements,
                                    stage, 
                                    passOfStage, 
                                    comp );
        }//end of for passStage = 0:stage-1
    }//end of for stage = 0:numStage-1

    return;
}// END of sort_enqueue

template<typename DVRandomAccessIterator, typename StrictWeakOrdering>
void sort_enqueue_non_powerOf2(control &ctl, const DVRandomAccessIterator& first, const DVRandomAccessIterator& last,
const StrictWeakOrdering& comp)
{
#if 0
    //std::cout << "The BOLT sort routine does not support non power of 2 buffer size. Falling back to CPU std::sort" << std ::endl;
    typedef typename std::iterator_traits< DVRandomAccessIterator >::value_type T;
    static boost::once_flag initOnlyOnce;
    size_t szElements = (size_t)(last - first);

    //Power of 2 buffer size
    // For user-defined types, the user must create a TypeName trait which returns the name of the class - note use of TypeName<>::get to retreive the name here.
    static std::vector< ::cl::Kernel > sortKernels;

    kernelParamsSort args( TypeName< T >::get( ), TypeName< DVRandomAccessIterator >::get( ), TypeName< T >::get( ),
    TypeName< StrictWeakOrdering >::get( ) );

    std::string typeDefinitions = cl_code + ClCode< T >::get( ) + ClCode< DVRandomAccessIterator >::get( );
    if( !boost::is_same< T, StrictWeakOrdering >::value)
    {
        typeDefinitions += ClCode< StrictWeakOrdering >::get( );
    }

    //Power of 2 buffer size
// For user-defined types, the user must create a TypeName trait which returns the name of the class
//  - note use of TypeName<>::get to retreive the name here.
    static std::vector< ::cl::Kernel > sortKernels;
        boost::call_once( initOnlyOnce, boost::bind( CallCompiler_Sort::constructAndCompileSelectionSort, &sortKernels,
            typeDefinitions, &args, &ctl) );

    size_t wgSize  = sortKernels[0].getWorkGroupInfo< CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE >( ctl.device( ), &l_Error );

    size_t totalWorkGroups = (szElements + wgSize)/wgSize;
    size_t globalSize = totalWorkGroups * wgSize;
    V_OPENCL( l_Error, "Error querying kernel for CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE" );

const ::cl::Buffer& in = first.getBuffer( );
    // ::cl::Buffer out(ctl.context(), CL_MEM_READ_WRITE, sizeof(T)*szElements);
    control::buffPointer out = ctl.acquireBuffer( sizeof(T)*szElements );

    ALIGNED( 256 ) StrictWeakOrdering aligned_comp( comp );
// ::cl::Buffer userFunctor(ctl.context(), CL_MEM_USE_HOST_PTR, sizeof( aligned_comp ), &aligned_comp );
control::buffPointer userFunctor = ctl.acquireBuffer( sizeof( aligned_comp ),
                                    CL_MEM_USE_HOST_PTR|CL_MEM_READ_ONLY, &aligned_comp );

    ::cl::LocalSpaceArg loc;
    loc.size_ = wgSize*sizeof(T);

    V_OPENCL( sortKernels[0].setArg(0, in), "Error setting a kernel argument in" );
V_OPENCL( sortKernels[0].setArg(1, first.gpuPayloadSize( ), &first.gpuPayload( ) ), "Error setting a kernel argument" );
V_OPENCL( sortKernels[0].setArg(2, *out), "Error setting a kernel argument out" );
V_OPENCL( sortKernels[0].setArg(3, *userFunctor), "Error setting a kernel argument userFunctor" );
V_OPENCL( sortKernels[0].setArg(4, loc), "Error setting kernel argument loc" );
V_OPENCL( sortKernels[0].setArg(5, static_cast<cl_uint> (szElements)), "Error setting kernel argument szElements" );
    {
            l_Error = ctl.commandQueue().enqueueNDRangeKernel(
                    sortKernels[0],
                    ::cl::NullRange,
                    ::cl::NDRange(globalSize),
                    ::cl::NDRange(wgSize),
                    NULL,
                    NULL);
            V_OPENCL( l_Error, "enqueueNDRangeKernel() failed for sort() kernel" );
            V_OPENCL( ctl.commandQueue().finish(), "Error calling finish on the command queue" );
    }

    wgSize  = sortKernels[1].getWorkGroupInfo< CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE >( ctl.device( ), &l_Error );

    V_OPENCL( sortKernels[1].setArg(0, *out), "Error setting a kernel argument in" );
    V_OPENCL( sortKernels[1].setArg(1, in), "Error setting a kernel argument out" );
V_OPENCL( sortKernels[1].setArg(2, first.gpuPayloadSize( ), &first.gpuPayload( ) ), "Error setting a kernel argument" );
V_OPENCL( sortKernels[1].setArg(3, *userFunctor), "Error setting a kernel argument userFunctor" );
V_OPENCL( sortKernels[1].setArg(4, loc), "Error setting kernel argument loc" );
V_OPENCL( sortKernels[1].setArg(5, static_cast<cl_uint> (szElements)), "Error setting kernel argument szElements" );
    {
            l_Error = ctl.commandQueue().enqueueNDRangeKernel(
                    sortKernels[1],
                    ::cl::NullRange,
                    ::cl::NDRange(globalSize),
                    ::cl::NDRange(wgSize),
                    NULL,
                    NULL);
            V_OPENCL( l_Error, "enqueueNDRangeKernel() failed for sort() kernel" );
            V_OPENCL( ctl.commandQueue().finish(), "Error calling finish on the command queue" );
    }
    // Map the buffer back to the host
    ctl.commandQueue().enqueueMapBuffer(in, true, CL_MAP_READ | CL_MAP_WRITE, 0/*offset*/, sizeof(T) * szElements, NULL, NULL, &l_Error );
    V_OPENCL( l_Error, "Error calling map on the result buffer" );
#endif
    return;
}// END of sort_enqueue_non_powerOf2

}//namespace bolt::cl::detail
}//namespace bolt::cl
}//namespace bolt

#endif
