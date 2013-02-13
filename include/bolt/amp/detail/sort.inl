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

#define WGSIZE 64

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
    //size_t szElements = (size_t)(last - first);
    unsigned int szElements = static_cast< unsigned int >( std::distance( first, last ) );
    if(szElements > (1<<31))
        throw std::exception( "AMP device vectors shall support only upto 2 power 31 elements" );
    if (szElements == 0 )
        return;
    const bolt::amp::control::e_RunMode runMode = ctl.forceRunMode();  // could be dynamic choice some day.
    if (runMode == bolt::amp::control::SerialCpu) {
        //  \TODO:  Need access to the device_vector .data method to get a host pointer
        throw ::std::exception( "Sort of device_vector Serial CPU run Mode is not implemented" );
        return;
    } else if (runMode == bolt::cl::control::MultiCoreCpu) {
		// \TODO - Find out what is the best way to report error std::cout should be removed
        std::cout << "The MultiCoreCpu version of sort on device_vector is not supported." << std ::endl;
        throw std::exception( "The MultiCoreCpu version of reduce on device_vector is not supported." );
        return;
    } else {
        sort_enqueue(ctl,first,last,comp);
    }
    return;
}

#if 0
template<typename DVRandomAccessIterator, typename StrictWeakOrdering>
void sort_pick_iterator( control &ctl,
						 const DVRandomAccessIterator& first, const DVRandomAccessIterator& last,
						 const StrictWeakOrdering& comp, bolt::cl::fancy_iterator_tag )
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
	if ((runMode == bolt::amp::control::SerialCpu) || (szElements < (2*WGSIZE))) {
		std::sort(first, last, comp);
		return;
	} else if (runMode == bolt::amp::control::MultiCoreCpu) {
#ifdef ENABLE_TBB
        std::cout << "The MultiCoreCpu version of sort is enabled with TBB. " << std ::endl;
        tbb::task_scheduler_init initialize(tbb::task_scheduler_init::automatic);
        tbb::parallel_sort(first,last, comp);
#else
        std::cout << "The MultiCoreCpu version of sort is not enabled. " << std ::endl;
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
    const int RADICES = 1<<N;
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
    concurrency::extent< 1 > globalSizeK0( szElements/RADICES );
    concurrency::tiled_extent< RADICES > tileK0 = globalSizeK0.tile< RADICES >();
    concurrency::accelerator_view av = ctl.getAccelerator().default_view;
	concurrency::parallel_for_each( av, tileK0, 
	[
		unsortedData,
		scanedBuckets,
        shiftCount,
        sortedData
	] 
    ( concurrency::tiled_index< RADICES > t_idx ) restrict(amp)
	{
        const int RADIX_T     = N;
        const int RADICES_T   = (1 << RADIX_T);
        const int MASK_T      = (1<<RADIX_T)  -1;

        size_t groupId   = get_group_id(0);
        size_t localId   = get_local_id(0);
        size_t globalId  = get_global_id(0);
        size_t groupSize = get_local_size(0);
        uiunsigned intnt bucketPos   = groupId * RADICES_T * groupSize;

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



#define BOLT_DEBUG 1
template<typename DVRandomAccessIterator, typename StrictWeakOrdering> 
typename std::enable_if< std::is_same< typename std::iterator_traits<DVRandomAccessIterator >::value_type, unsigned int >::value >::type
sort_enqueue(bolt::amp::control &ctl, 
             DVRandomAccessIterator &first, DVRandomAccessIterator &last,
             StrictWeakOrdering comp)
{
    typedef typename std::iterator_traits< DVRandomAccessIterator >::value_type T;
    const int RADIX = 4;
    const int RADICES = (1 << RADIX);	//Values handeled by each work-item?
    unsigned int orig_szElements = static_cast<unsigned int>(std::distance(first, last));
    unsigned int szElements = orig_szElements;
    bool  newBuffer = false;

    unsigned int groupSize;

    groupSize = RADICES;
    const int NUM_OF_ELEMENTS_PER_WORK_ITEM = RADICES;
    unsigned int num_of_elems_per_group = RADICES  * groupSize;
    unsigned int mulFactor = groupSize * RADICES;
    unsigned int numGroups = orig_szElements / mulFactor;
    
    if(orig_szElements%mulFactor != 0)
    {
        szElements  = ((orig_szElements + mulFactor) /mulFactor) * mulFactor;
        newBuffer = true;
    }
    device_vector< T, concurrency::array > dvInputData(static_cast<size_t>(szElements), 0);

    concurrency::extent<1> ext( static_cast< int >( orig_szElements ) );
    concurrency::array_view<T> dest = dvInputData.begin( )->getBuffer( ).section( ext );
    first->getBuffer( ).copy_to( dest );
    dest.synchronize( );
    printf("\ndest.get_extent()[0]=%d     first->getBuffer( ).get_extent()[0] = %d\n",dest.get_extent()[0], first->getBuffer( ).get_extent()[0]); 
    
    //dest.
    //printf("");
    device_vector< T, concurrency::array > dvSwapInputData(static_cast<size_t>(sizeof(T)*szElements), 0);
    device_vector< T, concurrency::array > dvHistogramBins(static_cast<size_t>(sizeof(T)*numGroups* groupSize * RADICES), 0);
    device_vector< T, concurrency::array > dvHistogramScanBuffer(static_cast<size_t>(sizeof(T)*(numGroups* RADICES + 10)), 0 );

    auto& clInputData = dvInputData.begin( )->getBuffer( );
    auto& clSwapData = dvSwapInputData.begin( )->getBuffer( );
    auto& clHistData = dvHistogramBins.begin( )->getBuffer( );
    auto& clHistScanData = dvHistogramScanBuffer.begin( )->getBuffer( );

    if(comp(2,3))
    {
        /*Ascending Sort*/
        if(newBuffer == true)
        {
            /*cl_buffer_region clBR;
            clBR.origin = (last - first)* sizeof(T);
            clBR.size  = (szElements * sizeof(T)) - (last - first)* sizeof(T);
            ctl.commandQueue().enqueueFillBuffer(clInputData, BOLT_UINT_MAX, clBR.origin, clBR.size, NULL, NULL);*/
            ;
        }
        int swap = 0;
        for(int bits = 0; bits < (sizeof(T) * 8)/*Bits per Byte*/; bits += RADIX)
        {
            if (swap == 0)
                AMP_RadixSortHistogramAscendingKernel<T, 4>(ctl,
                                                      clInputData, /*this can be either array_view or array*/
                                                      clHistData,
                                                      clHistScanData,
                                                      bits,
                                                      szElements,
                                                      groupSize); // \TODO - i don't need gropu size
            else
                AMP_RadixSortHistogramAscendingKernel<T, 4>(ctl,
                                                      clSwapData, /*this can be either array_view or array*/
                                                      clHistData,
                                                      clHistScanData,
                                                      bits,
                                                      szElements,
                                                      groupSize); // \TODO - i don't need gropu size
#if BOLT_DEBUG

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
            detail::scan_enqueue(ctl, dvHistogramScanBuffer.begin(),dvHistogramScanBuffer.end(),dvHistogramScanBuffer.begin(), 0, plus< T >( ));
#if BOLT_DEBUG
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
            AMP_scanLocalTemplate<T, 4>(ctl,
                                        clHistData,
                                        clHistScanData,
                                        szElements);
#if BOLT_DEBUG
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
                AMP_permuteAscendingRadixNTemplate<T, 4>( ctl,
                                                        clInputData,
                                                        clHistData,
                                                        bits,
                                                        clSwapData,
                                                        szElements);
            else
                AMP_permuteAscendingRadixNTemplate<T, 4>( ctl,
                                                        clSwapData,
                                                        clHistData,
                                                        bits,
                                                        clInputData,
                                                        szElements);
#if BOLT_DEBUG
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
        std::cout << "Unsigned int descending sort is called\n%%%%%";
#if 0
        /*Descending Sort*/
        histKernel = radixSortUintKernels[1];
        scanLocalKernel = radixSortUintKernels[2];
        permuteKernel = radixSortUintKernels[4];
        if(newBuffer == true)
        {
            cl_buffer_region clBR;
            clBR.origin = (last - first)* sizeof(T);
            clBR.size  = (szElements * sizeof(T)) - (last - first)* sizeof(T);
            ctl.commandQueue().enqueueFillBuffer(clInputData, BOLT_UINT_MIN, clBR.origin, clBR.size, NULL, NULL);
        }
#endif
    }
    //if(newBuffer == true)
    //{
        //::cl::copy(clInputData, first, last);
        //ctl.commandQueue().enqueueCopyBuffer( dvInputData.begin( ).getBuffer( ), first.getBuffer( ), 0, 0, sizeof(T)*(last-first), NULL, NULL );
    //concurrency::extent<1> ext( static_cast< int >( orig_szElements ) );
    dest = dvInputData.begin( )->getBuffer( ).section( ext );
    first->getBuffer( ).copy_to( dest );
    dest.copy_to( first->getBuffer( ) );
    first->getBuffer( ).synchronize( );

        dvInputData.begin( )->getBuffer( ).section( first->getBuffer( ).extent ).copy_to( first->getBuffer( ) );
        first->getBuffer( ).synchronize( );
    //}
#if 0
    //::cl::LocalSpaceArg localScanArray;
    //localScanArray.size_ = 2*RADICES* sizeof(cl_uint);
    int swap = 0;
    for(int bits = 0; bits < (sizeof(T) * 8)/*Bits per Byte*/; bits += RADIX)
    {
        //Do a histogram pass locally
        if (swap == 0)
            V_OPENCL( histKernel.setArg(0, clInputData), "Error setting a kernel argument" );
        else
            V_OPENCL( histKernel.setArg(0, clSwapData), "Error setting a kernel argument" );
        
        V_OPENCL( histKernel.setArg(1, clHistData), "Error setting a kernel argument" );
        V_OPENCL( histKernel.setArg(2, clHistScanData), "Error setting a kernel argument" );
        V_OPENCL( histKernel.setArg(3, bits), "Error setting a kernel argument" );
        //V_OPENCL( histKernel.setArg(4, loc), "Error setting kernel argument" );

        l_Error = ctl.commandQueue().enqueueNDRangeKernel(
                            histKernel,
                            ::cl::NullRange,
                            ::cl::NDRange(szElements/RADICES),
                            ::cl::NDRange(groupSize),
                            NULL,
                            NULL);
        //V_OPENCL( ctl.commandQueue().finish(), "Error calling finish on the command queue" );
        
#if (BOLT_DEBUG==1)
        //This map is required since the data is not available to the host when scanning.
        //Create local device_vector's 
        T *histBuffer;// = (T*)malloc(numGroups* groupSize * RADICES * sizeof(T));
        T *histScanBuffer;// = (T*)calloc(1, numGroups* RADICES * sizeof(T));
        ::cl::Event l_histEvent;
        histBuffer = (T*)ctl.commandQueue().enqueueMapBuffer(clHistData, false, CL_MAP_READ|CL_MAP_WRITE, 0, sizeof(T) * numGroups* groupSize * RADICES, NULL, &l_histEvent, &l_Error );
        bolt::cl::wait(ctl, l_histEvent);
        V_OPENCL( l_Error, "Error calling map on the result buffer" );
        printf("\n\n\n\n\nBITS = %d\nAfter Histogram", bits);
        for (unsigned int ng=0; ng<numGroups; ng++)
        { printf ("\nGroup-Block =%d",ng);
            for(unsigned int gS=0;gS<groupSize; gS++)
            { printf ("\nGroup =%d\n",gS);
                for(int i=0; i<RADICES;i++)
                {
                    size_t index = ng * groupSize * RADICES + gS * RADICES + i;
                    int value = histBuffer[ index ];
                    printf("%x %x, ",index, value);
                }
            }
        }
        ctl.commandQueue().enqueueUnmapMemObject(clHistData, histBuffer);

        ::cl::Event l_histScanBufferEvent;
        histScanBuffer = (T*) ctl.commandQueue().enqueueMapBuffer(clHistScanData, false, CL_MAP_READ|CL_MAP_WRITE, 0, sizeof(T) * numGroups * RADICES, NULL, &l_histScanBufferEvent, &l_Error );
        bolt::cl::wait(ctl, l_histScanBufferEvent);
        V_OPENCL( l_Error, "Error calling map on the result buffer" );
        int temp = 0;
        for(int i=0; i<RADICES;i++)
        { 
            printf ("\nRadix = %d\n",i);
            for (unsigned int ng=0; ng<numGroups; ng++)
            {
                printf ("%x, ",histScanBuffer[i*numGroups + ng]);
            }
            for (unsigned int ng=0; ng<numGroups; ng++)
            {
                temp += histScanBuffer[i*numGroups + ng];
                printf ("%x, ",temp);
            }
        }
        ctl.commandQueue().enqueueUnmapMemObject(clHistScanData, histScanBuffer);
#endif

        //Perform a global scan 
        detail::scan_enqueue(ctl, dvHistogramScanBuffer.begin(),dvHistogramScanBuffer.end(),dvHistogramScanBuffer.begin(), 0, plus< T >( ));
#if (BOLT_DEBUG==1)
        ::cl::Event l_histScanBufferEvent1;
        histScanBuffer = (T*) ctl.commandQueue().enqueueMapBuffer(clHistScanData, false, CL_MAP_READ|CL_MAP_WRITE, 0, sizeof(T) * numGroups * RADICES, NULL, &l_histScanBufferEvent1, &l_Error );
        bolt::cl::wait(ctl, l_histScanBufferEvent1);
        V_OPENCL( l_Error, "Error calling map on the result buffer" );
        for(int i=0; i<RADICES;i++)
        { 
            printf ("\nRadix = %d\n",i);
            for (int ng=0; ng<numGroups; ng++)
            {
                printf ("%x, ",histScanBuffer[i*numGroups + ng]);
            }
        }
        ctl.commandQueue().enqueueUnmapMemObject(clHistScanData, histScanBuffer);

#endif 

        //Add the results of the global scan to the local scan buffers
        V_OPENCL( scanLocalKernel.setArg(0, clHistData), "Error setting a kernel argument" );
        V_OPENCL( scanLocalKernel.setArg(1, clHistScanData), "Error setting a kernel argument" );
        V_OPENCL( scanLocalKernel.setArg(2, localScanArray), "Error setting a kernel argument" );
        l_Error = ctl.commandQueue().enqueueNDRangeKernel(
                            scanLocalKernel,
                            ::cl::NullRange,
                            ::cl::NDRange(szElements/RADICES),
                            ::cl::NDRange(groupSize),
                            NULL,
                            NULL);
        //V_OPENCL( ctl.commandQueue().finish(), "Error calling finish on the command queue" );

#if (BOLT_DEBUG==1)
        //This map is required since the data is not available to the host when scanning.
        ::cl::Event l_histEvent1;
        T *histBuffer1;
        histBuffer1 = (T*)ctl.commandQueue().enqueueMapBuffer(clHistData, false, CL_MAP_READ|CL_MAP_WRITE, 0, sizeof(T) * numGroups* groupSize * RADICES, NULL, &l_histEvent1, &l_Error );
        bolt::cl::wait(ctl, l_histEvent1);
        V_OPENCL( l_Error, "Error calling map on the result buffer" );

        printf("\n\nAfter Scan bits = %d", bits);
        for (int ng=0; ng<numGroups; ng++)
        { printf ("\nGroup-Block =%d",ng);
            for(int gS=0;gS<groupSize; gS++)
            { printf ("\nGroup =%d\n",gS);
                for(int i=0; i<RADICES;i++)
                {
                    size_t index = ng * groupSize * RADICES + gS * RADICES + i;
                    int value = histBuffer1[ index ];
                    printf("%x %x, ",index, value);
                }
            }
        }
        ctl.commandQueue().enqueueUnmapMemObject(clHistData, histBuffer1);
#endif 
        if (swap == 0)
            V_OPENCL( permuteKernel.setArg(0, clInputData), "Error setting kernel argument" );
        else
            V_OPENCL( permuteKernel.setArg(0, clSwapData), "Error setting kernel argument" );
        V_OPENCL( permuteKernel.setArg(1, clHistData), "Error setting a kernel argument" );
        V_OPENCL( permuteKernel.setArg(2, bits), "Error setting a kernel argument" );
        //V_OPENCL( permuteKernel.setArg(3, loc), "Error setting kernel argument" );

        if (swap == 0)
            V_OPENCL( permuteKernel.setArg(3, clSwapData), "Error setting kernel argument" );
        else
            V_OPENCL( permuteKernel.setArg(3, clInputData), "Error setting kernel argument" );
        l_Error = ctl.commandQueue().enqueueNDRangeKernel(
                            permuteKernel,
                            ::cl::NullRange,
                            ::cl::NDRange(szElements/RADICES),
                            ::cl::NDRange(groupSize),
                            NULL,
                            NULL);
        //V_OPENCL( ctl.commandQueue().finish(), "Error calling finish on the command queue" );
#if (BOLT_DEBUG==1)
            T *swapBuffer;// = (T*)malloc(szElements * sizeof(T));
            ::cl::Event l_swapEvent;
            if(bits==0 || bits==16)
            //if(bits==0 || bits==8 || bits==16 || bits==24)
            {
                swapBuffer = (T*)ctl.commandQueue().enqueueMapBuffer(clSwapData, false, CL_MAP_READ, 0, sizeof(T) * szElements, NULL, &l_swapEvent, &l_Error );
                V_OPENCL( l_Error, "Error calling map on the result buffer" );
                bolt::cl::wait(ctl, l_swapEvent);
                printf("\n Printing swap data\n");
                for(int i=0; i<szElements;i+= RADICES)
                {
                    for(int j =0;j< RADICES;j++)
                        printf("%x %x, ",i+j,swapBuffer[i+j]);
                    printf("\n");
                }
                ctl.commandQueue().enqueueUnmapMemObject(clSwapData, swapBuffer);
            }
            else
            {
                swapBuffer = (T*)ctl.commandQueue().enqueueMapBuffer(clInputData, false, CL_MAP_READ, 0, sizeof(T) * szElements, NULL, &l_swapEvent, &l_Error );
                V_OPENCL( l_Error, "Error calling map on the result buffer" );
                bolt::cl::wait(ctl, l_swapEvent);
                printf("\n Printing swap data\n");
                for(int i=0; i<szElements;i+= RADICES)
                {
                    for(int j =0;j< RADICES;j++)
                        printf("%x %x, ",i+j,swapBuffer[i+j]);
                    printf("\n");
                }
                ctl.commandQueue().enqueueUnmapMemObject(clInputData, swapBuffer);
            }


#endif
        if(swap==0)
            swap = 1;
        else
            swap = 0;
    }
    if(newBuffer == true)
    {
        //::cl::copy(clInputData, first, last);
        ctl.commandQueue().enqueueCopyBuffer( dvInputData.begin( ).getBuffer( ), first.getBuffer( ), 0, 0, sizeof(T)*(last-first), NULL, NULL );
    }
#endif
    return;
}
#if 0
template<typename DVRandomAccessIterator, typename StrictWeakOrdering> 
typename std::enable_if< 
    std::is_same< typename std::iterator_traits<DVRandomAccessIterator >::value_type, int >::value 
                       >::type
sort_enqueue(bolt::amp::control &ctl, 
             DVRandomAccessIterator first, DVRandomAccessIterator last,
             StrictWeakOrdering comp)
{
    typedef typename std::iterator_traits< DVRandomAccessIterator >::value_type T;
    const int RADIX = 4;
    const int RADICES = (1 << RADIX);	//Values handeled by each work-item?
    unsigned int szElements = (last - first);
    device_vector< T > dvInputData;//(sizeof(T), 0);
    bool  newBuffer = false;
    //std::cout << "Calling unsigned int sort_enqueue sizeof T = "<< sizeof(T) << "\n";
    int computeUnits     = ctl.device().getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>();
    int wgPerComputeUnit =  ctl.wgPerComputeUnit(); 
    //std::cout << "CU = " << computeUnits << "wgPerComputeUnit = "<< wgPerComputeUnit << "\n";
    cl_int l_Error = CL_SUCCESS;

    static  boost::once_flag initOnlyOnce;
    static std::vector< ::cl::Kernel > radixSortIntKernels;
                    
    //Power of 2 buffer size
    // For user-defined types, the user must create a TypeName trait which returns the name of the class - 
    // note use of TypeName<>::get to retreive the name here.
    boost::call_once( initOnlyOnce, boost::bind( CallCompiler_Sort::constructAndCompileRadixSortInt, 
                                                 &radixSortIntKernels, 
                                                 cl_code +ClCode<T>::get(), "", 
                                                 TypeName<StrictWeakOrdering>::get(), &ctl) );
    unsigned int groupSize  = 
        (unsigned int)radixSortIntKernels[0].getWorkGroupInfo< CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE >( ctl.device( ), &l_Error );
    V_OPENCL( l_Error, "Error querying kernel for CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE" );
    groupSize = RADICES;
    const int NUM_OF_ELEMENTS_PER_WORK_ITEM = RADICES;
    unsigned int num_of_elems_per_group = RADICES  * groupSize;

    int i = 0;
    unsigned int mulFactor = groupSize * RADICES;
    unsigned int numGroups = szElements / mulFactor;
                    
    if(szElements%mulFactor != 0)
    {
        szElements  = ((szElements + mulFactor) /mulFactor) * mulFactor;
        //the size is zero and not a multiple of mulFactor. So we need to resize the device_vector.
        dvInputData.resize(sizeof(T)*szElements);
        ctl.commandQueue().enqueueCopyBuffer( first.getBuffer( ), 
                                              dvInputData.begin( ).getBuffer( ), 
                                              0, 0, 
                                              sizeof(T)*(last-first), NULL, NULL );
        newBuffer = true;
    }
    else
    {
        dvInputData = device_vector< T >(first.getBuffer( ), ctl);
        newBuffer = false;
    }

    device_vector< T > dvSwapInputData( sizeof(T)*szElements, 0);
    device_vector< T > dvHistogramBins( sizeof(T)*(numGroups* groupSize * RADICES), 0);
    device_vector< T > dvHistogramScanBuffer( sizeof(T)*(numGroups* RADICES + 10), 0);

    ALIGNED( 256 ) StrictWeakOrdering aligned_comp( comp );
                    
    control::buffPointer userFunctor = ctl.acquireBuffer( sizeof( aligned_comp ), 
                                                          CL_MEM_USE_HOST_PTR|CL_MEM_READ_ONLY, 
                                                          &aligned_comp );
    ::cl::Buffer clInputData = dvInputData.begin( ).getBuffer( );
    ::cl::Buffer clSwapData = dvSwapInputData.begin( ).getBuffer( );
    ::cl::Buffer clHistData = dvHistogramBins.begin( ).getBuffer( );
    ::cl::Buffer clHistScanData = dvHistogramScanBuffer.begin( ).getBuffer( );
                    
    ::cl::Kernel histKernel;
    ::cl::Kernel permuteKernel;
    ::cl::Kernel scanLocalKernel;
    if(comp(2,3))
    {
        /*Ascending Sort*/
        histKernel = radixSortIntKernels[0];
        scanLocalKernel = radixSortIntKernels[2];
        permuteKernel = radixSortIntKernels[3];
        if(newBuffer == true)
        {
            cl_buffer_region clBR;
            clBR.origin = (last - first)* sizeof(T);
            clBR.size  = (szElements * sizeof(T)) - (last - first)* sizeof(T);
            ctl.commandQueue().enqueueFillBuffer(clInputData, BOLT_INT_MAX, clBR.origin, clBR.size, NULL, NULL);
        }
    }
    else
    {
        /*Descending Sort*/
        histKernel = radixSortIntKernels[1];
        scanLocalKernel = radixSortIntKernels[2];
        permuteKernel = radixSortIntKernels[4];
        if(newBuffer == true)
        {
            cl_buffer_region clBR;
            clBR.origin = (last - first)* sizeof(T);
            clBR.size  = (szElements * sizeof(T)) - (last - first)* sizeof(T);
            ctl.commandQueue().enqueueFillBuffer(clInputData, BOLT_INT_MIN, clBR.origin, clBR.size, NULL, NULL);
        }
    }
    //std::cout << "szElements " << szElements << "\n";
    ::cl::LocalSpaceArg localScanArray;
    localScanArray.size_ = 2*RADICES* sizeof(cl_uint);
    int swap = 0;
    int mask = (1<<RADIX) - 1;
    int bits = 0;
    for(bits = 0; bits < (sizeof(T) * 8)/*Bits per Byte*/; bits += RADIX)
    {
        //Do a histogram pass locally
        V_OPENCL( histKernel.setArg(0, RADIX), "Error setting a kernel argument" );
        V_OPENCL( histKernel.setArg(1, mask), "Error setting a kernel argument" );
        if (swap == 0)
            V_OPENCL( histKernel.setArg(2, clInputData), "Error setting a kernel argument" );
        else
            V_OPENCL( histKernel.setArg(2, clSwapData), "Error setting a kernel argument" );
        V_OPENCL( histKernel.setArg(3, clHistData), "Error setting a kernel argument" );
        V_OPENCL( histKernel.setArg(4, clHistScanData), "Error setting a kernel argument" );
        V_OPENCL( histKernel.setArg(5, bits), "Error setting a kernel argument" );
        //V_OPENCL( histKernel.setArg(4, loc), "Error setting kernel argument" );

        l_Error = ctl.commandQueue().enqueueNDRangeKernel(
                            histKernel,
                            ::cl::NullRange,
                            ::cl::NDRange(szElements/RADICES),
                            ::cl::NDRange(groupSize),
                            NULL,
                            NULL);
        //V_OPENCL( ctl.commandQueue().finish(), "Error calling finish on the command queue" );

#if (BOLT_DEBUG==1)
        //This map is required since the data is not available to the host when scanning.
        //Create local device_vector's 
        T *histBuffer;// = (T*)malloc(numGroups* groupSize * RADICES * sizeof(T));
        T *histScanBuffer;// = (T*)calloc(1, numGroups* RADICES * sizeof(T));
        ::cl::Event l_histEvent;
        histBuffer = (T*)ctl.commandQueue().enqueueMapBuffer(clHistData, false, CL_MAP_READ|CL_MAP_WRITE, 0, sizeof(T) * numGroups* groupSize * RADICES, NULL, &l_histEvent, &l_Error );
        bolt::cl::wait(ctl, l_histEvent);
        V_OPENCL( l_Error, "Error calling map on the result buffer" );
        printf("\n\n\n\n\nBITS = %d\nAfter Histogram", bits);
        for (unsigned int ng=0; ng<numGroups; ng++)
        { printf ("\nGroup-Block =%d",ng);
            for(unsigned int gS=0;gS<groupSize; gS++)
            { printf ("\nGroup =%d\n",gS);
                for(int i=0; i<RADICES;i++)
                {
                    size_t index = ng * groupSize * RADICES + gS * RADICES + i;
                    int value = histBuffer[ index ];
                    printf("%x %x, ",index, value);
                }
            }
        }
        ctl.commandQueue().enqueueUnmapMemObject(clHistData, histBuffer);

        ::cl::Event l_histScanBufferEvent;
        histScanBuffer = (T*) ctl.commandQueue().enqueueMapBuffer(clHistScanData, false, CL_MAP_READ|CL_MAP_WRITE, 0, sizeof(T) * numGroups * RADICES, NULL, &l_histScanBufferEvent, &l_Error );
        bolt::cl::wait(ctl, l_histScanBufferEvent);
        V_OPENCL( l_Error, "Error calling map on the result buffer" );
        int temp = 0;
        for(int i=0; i<RADICES;i++)
        { 
            printf ("\nRadix = %d\n",i);
            for (unsigned int ng=0; ng<numGroups; ng++)
            {
                printf ("%x, ",histScanBuffer[i*numGroups + ng]);
            }
            for (unsigned int ng=0; ng<numGroups; ng++)
            {
                temp += histScanBuffer[i*numGroups + ng];
                printf ("%x, ",temp);
            }
        }
        ctl.commandQueue().enqueueUnmapMemObject(clHistScanData, histScanBuffer);
#endif

        //Perform a global scan 
        detail::scan_enqueue(ctl, dvHistogramScanBuffer.begin(),dvHistogramScanBuffer.end(),dvHistogramScanBuffer.begin(), 0, plus< T >( ));
#if (BOLT_DEBUG==1)
        ::cl::Event l_histScanBufferEvent1;
        histScanBuffer = (T*) ctl.commandQueue().enqueueMapBuffer(clHistScanData, false, CL_MAP_READ|CL_MAP_WRITE, 0, sizeof(T) * numGroups * RADICES, NULL, &l_histScanBufferEvent1, &l_Error );
        bolt::cl::wait(ctl, l_histScanBufferEvent1);
        V_OPENCL( l_Error, "Error calling map on the result buffer" );
        for(int i=0; i<RADICES;i++)
        { 
            printf ("\nRadix = %d\n",i);
            for (int ng=0; ng<numGroups; ng++)
            {
                printf ("%x, ",histScanBuffer[i*numGroups + ng]);
            }
        }
        ctl.commandQueue().enqueueUnmapMemObject(clHistScanData, histScanBuffer);

#endif 

        //Add the results of the global scan to the local scan buffers
        V_OPENCL( scanLocalKernel.setArg(0, RADIX), "Error setting a kernel argument" );
        V_OPENCL( scanLocalKernel.setArg(1, clHistData), "Error setting a kernel argument" );
        V_OPENCL( scanLocalKernel.setArg(2, clHistScanData), "Error setting a kernel argument" );
        V_OPENCL( scanLocalKernel.setArg(3, localScanArray), "Error setting a kernel argument" );
        l_Error = ctl.commandQueue().enqueueNDRangeKernel(
                            scanLocalKernel,
                            ::cl::NullRange,
                            ::cl::NDRange(szElements/RADICES),
                            ::cl::NDRange(groupSize),
                            NULL,
                            NULL);
        //V_OPENCL( ctl.commandQueue().finish(), "Error calling finish on the command queue" );

#if (BOLT_DEBUG==1)
        //This map is required since the data is not available to the host when scanning.
        ::cl::Event l_histEvent1;
        T *histBuffer1;
        histBuffer1 = (T*)ctl.commandQueue().enqueueMapBuffer(clHistData, false, CL_MAP_READ|CL_MAP_WRITE, 0, sizeof(T) * numGroups* groupSize * RADICES, NULL, &l_histEvent1, &l_Error );
        bolt::cl::wait(ctl, l_histEvent1);
        V_OPENCL( l_Error, "Error calling map on the result buffer" );

        printf("\n\nAfter Scan bits = %d", bits);
        for (int ng=0; ng<numGroups; ng++)
        { printf ("\nGroup-Block =%d",ng);
            for(int gS=0;gS<groupSize; gS++)
            { printf ("\nGroup =%d\n",gS);
                for(int i=0; i<RADICES;i++)
                {
                    size_t index = ng * groupSize * RADICES + gS * RADICES + i;
                    int value = histBuffer1[ index ];
                    printf("%x %x, ",index, value);
                }
            }
        }
        ctl.commandQueue().enqueueUnmapMemObject(clHistData, histBuffer1);
#endif 
        V_OPENCL( permuteKernel.setArg(0, RADIX), "Error setting a kernel argument" );
        V_OPENCL( permuteKernel.setArg(1, mask),  "Error setting a kernel argument" );
        if (swap == 0)
            V_OPENCL( permuteKernel.setArg(2, clInputData), "Error setting kernel argument" );
        else
            V_OPENCL( permuteKernel.setArg(2, clSwapData), "Error setting kernel argument" );
        V_OPENCL( permuteKernel.setArg(3, clHistData), "Error setting a kernel argument" );
        V_OPENCL( permuteKernel.setArg(4, bits), "Error setting a kernel argument" );
        //V_OPENCL( permuteKernel.setArg(3, loc), "Error setting kernel argument" );

        if (swap == 0)
            V_OPENCL( permuteKernel.setArg(5, clSwapData), "Error setting kernel argument" );
        else
            V_OPENCL( permuteKernel.setArg(5, clInputData), "Error setting kernel argument" );
        l_Error = ctl.commandQueue().enqueueNDRangeKernel(
                            permuteKernel,
                            ::cl::NullRange,
                            ::cl::NDRange(szElements/RADICES),
                            ::cl::NDRange(groupSize),
                            NULL,
                            NULL);
        //V_OPENCL( ctl.commandQueue().finish(), "Error calling finish on the command queue" );
#if (BOLT_DEBUG==1)
            T *swapBuffer;// = (T*)malloc(szElements * sizeof(T));
            ::cl::Event l_swapEvent;
            if(bits==0 || bits==16)
            //if(bits==0 || bits==8 || bits==16 || bits==24)
            {
                swapBuffer = (T*)ctl.commandQueue().enqueueMapBuffer(clSwapData, false, CL_MAP_READ, 0, sizeof(T) * szElements, NULL, &l_swapEvent, &l_Error );
                V_OPENCL( l_Error, "Error calling map on the result buffer" );
                bolt::cl::wait(ctl, l_swapEvent);
                printf("\n Printing swap data\n");
                for(int i=0; i<szElements;i+= RADICES)
                {
                    for(int j =0;j< RADICES;j++)
                        printf("%x %x, ",i+j,swapBuffer[i+j]);
                    printf("\n");
                }
                ctl.commandQueue().enqueueUnmapMemObject(clSwapData, swapBuffer);
            }
            else
            {
                swapBuffer = (T*)ctl.commandQueue().enqueueMapBuffer(clInputData, false, CL_MAP_READ, 0, sizeof(T) * szElements, NULL, &l_swapEvent, &l_Error );
                V_OPENCL( l_Error, "Error calling map on the result buffer" );
                bolt::cl::wait(ctl, l_swapEvent);
                printf("\n Printing swap data\n");
                for(int i=0; i<szElements;i+= RADICES)
                {
                    for(int j =0;j< RADICES;j++)
                        printf("%x %x, ",i+j,swapBuffer[i+j]);
                    printf("\n");
                }
                ctl.commandQueue().enqueueUnmapMemObject(clInputData, swapBuffer);
            }


#endif
        if(swap==0)
            swap = 1;
        else
            swap = 0;
        //For the last Iteration ignore the sign bit and perform radix sort
        if(bits == ((sizeof(T) * 8) - 2*RADIX))
            mask = (1 << (RADIX-1) ) - 1;
    }
#define CPU_CODE 1
#if (CPU_CODE==1)
    {
        //This block is necessary because the mapped buffer pointer will get unmapped at the exit of this block.
        //This is needed when the input buffer size is not a multiple of mulFactor
        std::vector<T> cpuBuffer(szElements);
        boost::shared_array< T > ptr = dvInputData.data();
        memcpy_s(cpuBuffer.data(), szElements*sizeof(T), ptr.get(), szElements*sizeof(T));
        if( comp(2,3) )
        {
            int index=0;
            for( i=0; i<(last-first); i++)
            {
                if(cpuBuffer[i] < 0)
                {
                    ptr[index] = cpuBuffer[i];
                    index++;
                }
            }

            //int positiveIndex=0;
            for( i=0; i<(last-first); i++)
            {
                if(cpuBuffer[i] >= 0)
                {
                    ptr[index] = cpuBuffer[i];
                    index++;
                }
            }
        }
        else
        {
            int index=0;
            for( i=0; i<(last-first); i++)
            {
                if(cpuBuffer[i] >= 0)
                {
                    ptr[index] = cpuBuffer[i];
                    index++;
                }
            }
            //int positiveIndex=0;
            for( i=0; i<(last-first); i++)
            {
                if(cpuBuffer[i] < 0)
                {
                    ptr[index] = cpuBuffer[i];
                    index++;
                }
            }
        }
    }// End of the block the boost shared ptr gets freed here. 
    //This calls the unmap buffer Functor in the 
#endif // end of CPU code


#if 0
                    //Below code was not working for sorting the sign bit in the reverse order. 
                    //Hence wrote the CPU code path above.
                        if(comp(2,3))
                        {
                            /*Descending Sort*/
                            histKernel = radixSortIntKernels[1];
                            scanLocalKernel = radixSortIntKernels[2];
                            permuteKernel = radixSortIntKernels[4];
                        }
                        else
                        {
                            /*Ascending Sort*/
                            histKernel = radixSortIntKernels[0];
                            scanLocalKernel = radixSortIntKernels[2];
                            permuteKernel = radixSortIntKernels[3];
                        }
                        mask = 1;//(1<<(RADIX-1)); //only the sign bit needs to be set 
                        bits = sizeof(T) - 1; // We take the MSB byte or nibble
                        //Do a histogram pass locally
                        V_OPENCL( histKernel.setArg(0, RADIX), "Error setting a kernel argument" );
                        V_OPENCL( histKernel.setArg(1, mask), "Error setting a kernel argument" );
                            V_OPENCL( histKernel.setArg(2, clInputData), "Error setting a kernel argument" );
                        V_OPENCL( histKernel.setArg(3, clHistData), "Error setting a kernel argument" );
                        V_OPENCL( histKernel.setArg(4, clHistScanData), "Error setting a kernel argument" );
                        V_OPENCL( histKernel.setArg(5, bits), "Error setting a kernel argument" );
                        //V_OPENCL( histKernel.setArg(4, loc), "Error setting kernel argument" );

                        l_Error = ctl.commandQueue().enqueueNDRangeKernel(
                                            histKernel,
                                            ::cl::NullRange,
                                            ::cl::NDRange(szElements/RADICES),
                                            ::cl::NDRange(groupSize),
                                            NULL,
                                            NULL);
                        //V_OPENCL( ctl.commandQueue().finish(), "Error calling finish on the command queue" );

                        //Perform a global scan 
                        detail::scan_enqueue(ctl, dvHistogramScanBuffer.begin(),dvHistogramScanBuffer.end(),dvHistogramScanBuffer.begin(), 0, plus< T >( ));

                        //Add the results of the global scan to the local scan buffers
                        V_OPENCL( scanLocalKernel.setArg(0, RADIX), "Error setting a kernel argument" );
                        V_OPENCL( scanLocalKernel.setArg(1, clHistData), "Error setting a kernel argument" );
                        V_OPENCL( scanLocalKernel.setArg(2, clHistScanData), "Error setting a kernel argument" );
                        V_OPENCL( scanLocalKernel.setArg(3, localScanArray), "Error setting a kernel argument" );
                        l_Error = ctl.commandQueue().enqueueNDRangeKernel(
                                            scanLocalKernel,
                                            ::cl::NullRange,
                                            ::cl::NDRange(szElements/RADICES),
                                            ::cl::NDRange(groupSize),
                                            NULL,
                                            NULL);
                        //V_OPENCL( ctl.commandQueue().finish(), "Error calling finish on the command queue" );


                        V_OPENCL( permuteKernel.setArg(0, RADIX), "Error setting a kernel argument radix" );
                        V_OPENCL( permuteKernel.setArg(1, mask),  "Error setting a kernel argument mask" );
                            V_OPENCL( permuteKernel.setArg(2, clInputData), "Error setting kernel argument" );
                        V_OPENCL( permuteKernel.setArg(3, clHistData), "Error setting a kernel argument" );
                        V_OPENCL( permuteKernel.setArg(4, bits), "Error setting a kernel argument bits" );
                        //V_OPENCL( permuteKernel.setArg(3, loc), "Error setting kernel argument" );
                            V_OPENCL( permuteKernel.setArg(5, clSwapData), "Error setting kernel argument" );
                        l_Error = ctl.commandQueue().enqueueNDRangeKernel(
                                            permuteKernel,
                                            ::cl::NullRange,
                                            ::cl::NDRange(szElements/RADICES),
                                            ::cl::NDRange(groupSize),
                                            NULL,
                                            NULL);
                        //V_OPENCL( ctl.commandQueue().finish(), "Error calling finish on the command queue" );
                    ::cl::Event copy_event;
                    ctl.commandQueue().enqueueCopyBuffer(clSwapData, clInputData, 0, 0, szElements*sizeof(T), NULL, &copy_event);
                    copy_event.wait();
                    /**************************************/
#endif
    if(newBuffer == true)
    {
        //::cl::copy(clInputData, first, last);
        ctl.commandQueue().enqueueCopyBuffer( dvInputData.begin( ).getBuffer( ), 
                                              first.getBuffer( ), 
                                              0, 0, 
                                              sizeof(T)*(last-first), NULL, NULL );
    }

    return;
}
#endif

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
    concurrency::tiled_extent< WGSIZE > tileK0 = globalSizeK0.tile< WGSIZE >();
    concurrency::accelerator_view av = ctl.getAccelerator().default_view;
	concurrency::parallel_for_each( av, tileK0, 
	[
		A,
		passOfStage,
		stage,
        comp
	]
    ( concurrency::tiled_index< WGSIZE > t_idx ) restrict(amp)
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
    !(std::is_same< typename std::iterator_traits<DVRandomAccessIterator >::value_type, unsigned int >::value)
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

	auto&  A = first->getBuffer(); //( numElements, av );

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
