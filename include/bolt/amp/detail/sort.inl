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

#if !defined( BOLT_AMP_SORT_INL )
#define BOLT_AMP_SORT_INL
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
#include "bolt/btbb/sort.h"

#endif


#define BOLT_UINT_MAX 0xFFFFFFFFU
#define BOLT_UINT_MIN 0x0U
#define BOLT_INT_MAX 0x7FFFFFFFU
#define BOLT_INT_MIN 0x80000000U

#define BITONIC_SORT_WGSIZE 64
/* \brief - SORT_CPU_THRESHOLD should be atleast 2 times the BITONIC_SORT_WGSIZE*/
#define SORT_CPU_THRESHOLD 128
#define MERGE_SORT_WAVESIZE 64
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
        bolt::amp::device_vector< T >::pointer firstPtr =  first.getContainer( ).data( );
        std::sort(&firstPtr[ first.m_Index ], &firstPtr[ last.m_Index ], comp);
    } else if (runMode == bolt::amp::control::MultiCoreCpu) {
#ifdef ENABLE_TBB
        bolt::amp::device_vector< T >::pointer firstPtr =  first.getContainer( ).data( );
        bolt::btbb::sort(&firstPtr[ first.m_Index ], &firstPtr[ last.m_Index ], comp);
#else
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
        bolt::btbb::sort(first,last, comp);
#else
//      std::cout << "The MultiCoreCpu version of sort is not enabled. " << std ::endl;
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
    //printf("globalSizeK0 = %d   tileK0.tile_dim0 = %d    tileK0[0]=%d\n",globalSizeK0, tileK0.tile_dim0, tileK0[0]);
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
    //printf("globalSizeK0 = %d   tileK0.tile_dim0 = %d    tileK0[0]=%d\n",globalSizeK0, tileK0.tile_dim0, tileK0[0]);
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
        first.getContainer().getBuffer().copy_to( dest );
        dest.synchronize( );
        newBuffer = true;
    }
    else
    {
        pLocalArrayView = new concurrency::array_view<T>( first.getContainer().getBuffer() );
    }

    unsigned int numGroups = szElements / mulFactor;
    concurrency::extent<1> modified_ext( static_cast< int >( szElements ) );

    device_vector< T, concurrency::array > dvSwapInputData(static_cast<size_t>(szElements), 0);
    device_vector< T, concurrency::array > dvHistogramBins(static_cast<size_t>(numGroups* groupSize * RADICES), 0);
    device_vector< T, concurrency::array > dvHistogramScanBuffer(static_cast<size_t>(numGroups* RADICES + 10), 0 );

    auto& clInputData = *pLocalArrayView;
    auto& clSwapData = dvSwapInputData.begin( ).getContainer().getBuffer();
    auto& clHistData = dvHistogramBins.begin( ).getContainer().getBuffer();
    auto& clHistScanData = dvHistogramScanBuffer.begin( ).getContainer().getBuffer();
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
        //std::cout << "New buffer was allocated So copying back the buffer\n";
        //dest = clInputData.section( ext );
        clInputData.section( ext ).copy_to( first.getContainer().getBuffer() );
        first.getContainer().getBuffer().synchronize( );
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
        first.getContainer().getBuffer().copy_to( dest );
        dest.synchronize( );
        newBuffer = true;
    }
    else
    {
        pLocalArrayView = new concurrency::array_view<T>( first.getContainer().getBuffer() );
    }

    unsigned int numGroups = szElements / mulFactor;
    concurrency::extent<1> modified_ext( static_cast< int >( szElements ) );

    device_vector< T, concurrency::array > dvSwapInputData(static_cast<size_t>(szElements), 0);
    device_vector< T, concurrency::array > dvHistogramBins(static_cast<size_t>(numGroups* groupSize * RADICES), 0);
    device_vector< T, concurrency::array > dvHistogramScanBuffer(static_cast<size_t>(numGroups* RADICES + 10), 0 );

    auto& clInputData = *pLocalArrayView;
    auto& clSwapData = dvSwapInputData.begin( ).getContainer().getBuffer();
    auto& clHistData = dvHistogramBins.begin( ).getContainer().getBuffer();
    auto& clHistScanData = dvHistogramScanBuffer.begin( ).getContainer().getBuffer();
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
        //std::cout << "New buffer was allocated So copying back the buffer\n";
        //dest = clInputData.section( ext );
        clInputData.section( ext ).copy_to( first.getContainer().getBuffer() );
        first.getContainer().getBuffer().synchronize( );
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
        //sort_enqueue_non_powerOf2(ctl,first,last,comp);
        stablesort_enqueue(ctl,first,last,comp);
        return;
    }
    /*if((szElements/2) < BITONIC_SORT_WGSIZE)
    {
        wgSize = (int)szElements/2;
    }*/
    auto&  A = first.getContainer().getBuffer(); //( numElements, av );

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

#define max(a,b)    (((a) > (b)) ? (a) : (b))
#define min(a,b)    (((a) < (b)) ? (a) : (b))

template< typename sType, typename Container, typename StrictWeakOrdering >
unsigned int lowerBoundBinary( Container& data, int left, int right, sType searchVal, StrictWeakOrdering& lessOp ) restrict(amp)
{
    //  The values firstIndex and lastIndex get modified within the loop, narrowing down the potential sequence
    int firstIndex = left;
    int lastIndex = right;
    
    //  This loops through [firstIndex, lastIndex)
    //  Since firstIndex and lastIndex will be different for every thread depending on the nested branch,
    //  this while loop will be divergent within a wavefront
    while( firstIndex < lastIndex )
    {
        //  midIndex is the average of first and last, rounded down
        unsigned int midIndex = ( firstIndex + lastIndex ) / 2;
        sType midValue = data[ midIndex ];
        
        //  This branch will create divergent wavefronts
        if( lessOp( midValue, searchVal ) )
        {
            firstIndex = midIndex+1;
             //printf( "lowerBound: lastIndex[ %i ]=%i\n", get_local_id( 0 ), lastIndex );
        }
        else
        {
            lastIndex = midIndex;
             //printf( "lowerBound: firstIndex[ %i ]=%i\n", get_local_id( 0 ), firstIndex );
        }
    }
    //printf("lowerBoundBinary: left=%d, right=%d, firstIndex=%d\n", left, right, firstIndex);
    return firstIndex;
}

//  This implements a binary search routine to look for an 'insertion point' in a sequence, denoted
//  by a base pointer and left and right index for a particular candidate value.  The comparison operator is 
//  passed as a functor parameter lessOp
//  This function returns an index that is the first index whos value would be greater than the searched value
//  If the search value is not found in the sequence, upperbound returns the same result as lowerbound
template< typename sType, typename Container, typename StrictWeakOrdering >
unsigned int  upperBoundBinary( Container& data, unsigned int left, unsigned int right, sType searchVal, StrictWeakOrdering& lessOp ) restrict(amp)
{
    unsigned int upperBound = lowerBoundBinary( data, left, right, searchVal, lessOp );
    
     //printf( "start of upperBoundBinary: upperBound, left, right = [%d, %d, %d]\n", upperBound, left, right );
    //  upperBound is always between left and right or equal to right
    //  If upperBound == right, then  searchVal was not found in the sequence.  Just return.
    if( upperBound != right )
    {
        //  While the values are equal i.e. !(x < y) && !(y < x) increment the index
        int mid = 0;
        sType upperValue = data[ upperBound ];
        //This loop is a kind of a specialized binary search. 
        //This will find the first index location which is not equal to searchVal.
        while( !lessOp( upperValue, searchVal ) && !lessOp( searchVal, upperValue) && (upperBound < right))
        {
            mid = (upperBound + right)/2;
            sType midValue = data[mid];
            if( !lessOp( midValue, searchVal ) && !lessOp( searchVal, midValue) )
            {
                upperBound = mid + 1;
            }   
            else
            {
                right = mid;
                upperBound++;
            }
            upperValue = data[ upperBound ];
            //printf( "upperBoundBinary: upperBound, left, right = [%d, %d, %d]\n", upperBound, left, right);
        }
    }
    //printf( "end of upperBoundBinary: upperBound, left, right = [%d, %d, %d]\n", upperBound, left, right);
    return upperBound;
}

//  This kernel implements merging of blocks of sorted data.  The input to this kernel most likely is
//  the output of blockInsertionSortTemplate.  It is expected that the source array contains multiple
//  blocks, each block is independently sorted.  The goal is to write into the output buffer half as 
//  many blocks, of double the size.  The even and odd blocks are stably merged together to form
//  a new sorted block of twice the size.  The algorithm is out-of-place.
template< typename sPtrType, typename Container, typename StrictWeakOrdering >
void AMP_mergeTemplate( bolt::amp::control &ctl,
                Container & source_ptr,
                Container & result_ptr,
                const int srcVecSize,
                const int srcLogicalBlockSize,
                StrictWeakOrdering& lessOp,
                int globalRange
            )
{
    //size_t globalID     = get_global_id( 0 );
    //size_t groupID      = get_group_id( 0 );
    //size_t localID      = get_local_id( 0 );
    //size_t wgSize       = get_local_size( 0 );
    concurrency::extent< 1 > globalSizeK0( globalRange );
    concurrency::tiled_extent< MERGE_SORT_WAVESIZE > tileK0 = globalSizeK0.tile< MERGE_SORT_WAVESIZE >();
    concurrency::accelerator_view av = ctl.getAccelerator().default_view;
    concurrency::parallel_for_each(av, tileK0,
        [
            source_ptr,
            result_ptr,
            srcVecSize,
            srcLogicalBlockSize,
            tileK0,
            lessOp
        ]
    ( concurrency::tiled_index< MERGE_SORT_WAVESIZE > t_idx ) restrict (amp)
    {
        int globalID    = t_idx.global[ 0 ];
        int groupID     = t_idx.tile[ 0 ];
        int localID     = t_idx.local[ 0 ];
        int wgSize      = tileK0.tile_dim0;
        tile_static sPtrType lds[MERGE_SORT_WAVESIZE];

        //  Abort threads that are passed the end of the input vector
        if( globalID >= srcVecSize )
            return; // on SI this doesn't mess-up barriers

        //  For an element in sequence A, find the lowerbound index for it in sequence B
        int srcBlockNum = globalID / srcLogicalBlockSize;
        int srcBlockIndex = globalID % srcLogicalBlockSize;
    
        //printf( "mergeTemplate: srcBlockNum[%i]=%i\n", srcBlockNum, srcBlockIndex );

        //  Pairs of even-odd blocks will be merged together 
        //  An even block should search for an insertion point in the next odd block, 
        //  and the odd block should look for an insertion point in the corresponding previous even block
        int dstLogicalBlockSize = srcLogicalBlockSize<<1;
        int leftBlockIndex = globalID & ~(dstLogicalBlockSize - 1 );
        //printf("mergeTemplate: leftBlockIndex=%d\n", leftBlockIndex );
        leftBlockIndex += (srcBlockNum & 0x1) ? 0 : srcLogicalBlockSize;
        leftBlockIndex = min( leftBlockIndex, srcVecSize );
        int rightBlockIndex = min( leftBlockIndex + srcLogicalBlockSize, srcVecSize );
    
        //  For a particular element in the input array, find the lowerbound index for it in the search sequence given by leftBlockIndex & rightBlockIndex
        // uint insertionIndex = lowerBoundLinear( source_ptr, leftBlockIndex, rightBlockIndex, source_ptr[ globalID ], lessOp ) - leftBlockIndex;
        int insertionIndex = 0;
        if( (srcBlockNum & 0x1) == 0 )
        {
            insertionIndex = lowerBoundBinary( source_ptr, leftBlockIndex, rightBlockIndex, source_ptr[ globalID ], lessOp ) - leftBlockIndex;
        }
        else
        {
            insertionIndex = upperBoundBinary( source_ptr, leftBlockIndex, rightBlockIndex, source_ptr[ globalID ], lessOp ) - leftBlockIndex;
        }
    
        //  The index of an element in the result sequence is the summation of it's indixes in the two input 
        //  sequences
        int dstBlockIndex = srcBlockIndex + insertionIndex;
        int dstBlockNum = srcBlockNum/2;
    
        result_ptr[ (dstBlockNum*dstLogicalBlockSize)+dstBlockIndex ] = source_ptr[ globalID ];
    } );
}


template< typename T, typename StrictWeakOrdering, typename Container >
void AMP_BlockInsertionSortTemplate( bolt::amp::control &ctl,
                Container  &data_ptr,
                int vecSize,
                StrictWeakOrdering &lessOp,
                int globalRange,
                int localRange
            )
{
    const int BLOCK_SORT_WAVESIZE = 64;
    concurrency::extent< 1 > globalSizeK0( globalRange );
    concurrency::tiled_extent< BLOCK_SORT_WAVESIZE > tileK0 = globalSizeK0.tile< BLOCK_SORT_WAVESIZE >();
    concurrency::accelerator_view av = ctl.getAccelerator().default_view;
    concurrency::parallel_for_each(av, tileK0,
        [
            data_ptr,
            tileK0,
            lessOp,
            vecSize
        ]
    ( concurrency::tiled_index< BLOCK_SORT_WAVESIZE > t_idx ) restrict (amp)
    {
        int gloId    = t_idx.global[ 0 ];
        int groId     = t_idx.tile[ 0 ];
        int locId     = t_idx.local[ 0 ];
        int wgSize   = tileK0.tile_dim0;
        tile_static T lds[64];
        // Abort threads that are passed the end of the input vector

        if (gloId < vecSize) 
            lds[ locId ] = data_ptr[ gloId ];

        t_idx.barrier.wait();

        //  Sorts a workgroup using a naive insertion sort
        //  The sort uses one thread within a workgroup to sort the entire workgroup
        if( locId == 0 )
        {
            //  The last workgroup may have an irregular size, so we calculate a per-block endIndex
            //  endIndex is essentially emulating a mod operator with subtraction and multiply
            int endIndex = vecSize - ( groId * wgSize );
            endIndex = min( endIndex, wgSize );

            //  Indices are signed because the while loop will generate a -1 index inside of the max function
            for( int currIndex = 1; currIndex < endIndex; ++currIndex )
            {
                T val = lds[ currIndex ];
                int scanIndex = currIndex;
                T ldsVal = lds[scanIndex - 1];
                while( scanIndex > 0 && lessOp( val, ldsVal ) )
                {
                    lds[ scanIndex ] = ldsVal;
                    scanIndex = scanIndex - 1;
                    ldsVal = lds[ max(0, scanIndex - 1) ];  // scanIndex-1 may be -1
                }
                lds[ scanIndex ] = val;
            }
        }
        t_idx.barrier.wait();
        if(gloId < vecSize)
            data_ptr[ gloId ] = lds[ locId ];//In C++ AMP we don;t need to store in a local variable
    }
    );
}

template<typename DVRandomAccessIterator, typename StrictWeakOrdering> 
void stablesort_enqueue(control& ctrl, const DVRandomAccessIterator& first, const DVRandomAccessIterator& last,
             const StrictWeakOrdering& comp)
{
    const int STABLE_SORT_VECTOR_SIZE = 64;
    int vecSize = static_cast< int >( std::distance( first, last ) );
    typedef std::iterator_traits< DVRandomAccessIterator >::value_type iType;
    concurrency::extent<1> ext( vecSize );
    int localRange = STABLE_SORT_VECTOR_SIZE;
    //  Make sure that globalRange is a multiple of localRange
    int globalRange = vecSize;
    int modlocalRange = ( globalRange & ( localRange-1 ) );
    if( modlocalRange )
    {
        globalRange &= (~modlocalRange);
        globalRange += localRange;
    }
    unsigned int ldsSize  = static_cast< unsigned int >( localRange * sizeof( iType ) );

    auto&  inputBuffer = first.getContainer().getBuffer(); //( numElements, av );

    AMP_BlockInsertionSortTemplate<iType>( ctrl,
                inputBuffer,
                vecSize,
                comp,
                globalRange,
                localRange
            );


    //  Early exit for the case of no merge passes, values are already in destination vector
    if( vecSize <= localRange )
    {
        return;
    }

    //  An odd number of elements requires an extra merge pass to sort
    int numMerges = 0;

    //  Calculate the log2 of vecSize, taking into account our block size from kernel 1 is 64
    //  this is how many merge passes we want
    int log2BlockSize = vecSize >> 6;
    for( ; log2BlockSize > 1; log2BlockSize >>= 1 )
    {
        ++numMerges;
    }

    //  Check to see if the input vector size is a power of 2, if not we will need last merge pass
    int vecPow2 = (vecSize & (vecSize-1));
    numMerges += vecPow2? 1: 0;

    //  Allocate a flipflop buffer because the merge passes are out of place
    device_vector< iType, concurrency::array > tmpBufferDV(static_cast<size_t>(globalRange), 0);
    auto& tmpBuffer = tmpBufferDV.begin( ).getContainer().getBuffer();

    for( int pass = 1; pass <= numMerges; ++pass )
    {
        //  For each pass, flip the input-output buffers 
        int srcLogicalBlockSize =  localRange << (pass-1) ;
        if( pass & 0x1 )
        {   
            AMP_mergeTemplate<iType>( ctrl,
                first.getContainer().getBuffer(),
                tmpBuffer,
                vecSize,
                srcLogicalBlockSize,
                comp,
                globalRange
            );

        }
        else
        {
            AMP_mergeTemplate<iType>( ctrl,
                tmpBuffer,
                first.getContainer().getBuffer(),
                vecSize,
                srcLogicalBlockSize,
                comp,
                globalRange
            );

        }
        //std::cout << "\nPass Num"<< pass<< std::endl;
    }

    //  If there are an odd number of merges, then the output data is sitting in the temp buffer.  We need to copy
    //  the results back into the input array
    if( numMerges & 0x1 )
    {
        tmpBuffer.section( ext ).copy_to( first.getContainer().getBuffer() );
        first.getContainer().getBuffer().synchronize( );
    }

    //iType * temp = inputBuffer.data();
    //std::cout << "*********Final Sort data ********* vecSize = "<< vecSize << "\n";
    //for(int ii=0; ii<vecSize; ii++ )
    //{
    //    std::cout << " " <<temp[ii];
    //}

    return;
}// END of sort_enqueue



}//namespace bolt::cl::detail
}//namespace bolt::cl
}//namespace bolt

#endif
