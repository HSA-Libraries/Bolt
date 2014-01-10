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
#if !defined( BOLT_AMP_STABLESORT_INL )
#define BOLT_AMP_STABLESORT_INL
#define BUFFER_SIZE 64
#include <algorithm>
#include <type_traits>

#include "bolt/amp/bolt.h"
#include "bolt/amp/functional.h"
#include "bolt/amp/device_vector.h"
#include "bolt/amp/iterator/iterator_traits.h"
#include <amp.h>

#include "bolt/amp/detail/sort.inl"
#ifdef ENABLE_TBB
//TBB Includes
#include "bolt/btbb/stable_sort.h"
#endif

#define BOLT_AMP_STABLESORT_CPU_THRESHOLD 64

namespace bolt {
namespace amp {

namespace detail
{

template< typename DVRandomAccessIterator, typename StrictWeakOrdering >
typename std::enable_if< std::is_same< typename std::iterator_traits<DVRandomAccessIterator >::value_type,
                                       unsigned int
                                     >::value
                       >::type  /*If enabled then this typename will be evaluated to void*/
stablesort_enqueue(control &ctl,
             DVRandomAccessIterator first, DVRandomAccessIterator last,
             StrictWeakOrdering comp)
{
    bolt::amp::detail::sort_enqueue(ctl, first, last, comp);
    return;
}

template<typename DVRandomAccessIterator, typename StrictWeakOrdering>
typename std::enable_if< std::is_same< typename std::iterator_traits<DVRandomAccessIterator >::value_type,
                                       int
                                     >::value
                       >::type  /*If enabled then this typename will be evaluated to void*/
stablesort_enqueue(control &ctl,
             DVRandomAccessIterator first, DVRandomAccessIterator last,
             StrictWeakOrdering comp)
{
    bolt::amp::detail::sort_enqueue(ctl, first, last, comp);
    return;
}

template< typename sType, typename StrictWeakOrdering >
unsigned int lowerBoundBinary( sType* data, unsigned int left, unsigned int right, sType searchVal, const StrictWeakOrdering& lessOp )
{
    //  The values firstIndex and lastIndex get modified within the loop, narrowing down the potential sequence
    unsigned int firstIndex = left;
    unsigned int lastIndex = right;
    
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
        }
        else
        {
            lastIndex = midIndex;
        }
    }
    //printf("lowerBoundBinary: left=%d, right=%d, firstIndex=%d\n", left, right, firstIndex);
    return firstIndex;
}

template< typename sType, typename StrictWeakOrdering >
unsigned int upperBoundBinary(sType* data, unsigned int left, unsigned int right, sType searchVal, const StrictWeakOrdering& lessOp )
{
    uint upperBound = lowerBoundBinary( data, left, right, searchVal, lessOp );
    
     //printf( "start of upperBoundBinary: upperBound, left, right = [%d, %d, %d]\n", upperBound, left, right );
    //  upperBound is always between left and right or equal to right
    //  If upperBound == right, then  searchVal was not found in the sequence.  Just return.
    if( upperBound != right )
    {
        //  While the values are equal i.e. !(x < y) && !(y < x) increment the index
        uint mid = 0;
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

#define max(a,b)    (((a) > (b)) ? (a) : (b))
#define min(a,b)    (((a) < (b)) ? (a) : (b))

template<typename DVRandomAccessIterator, typename StrictWeakOrdering>
typename std::enable_if<
    !(std::is_same< typename std::iterator_traits<DVRandomAccessIterator >::value_type, unsigned int >::value || 
      std::is_same< typename std::iterator_traits<DVRandomAccessIterator >::value_type, int >::value  )
                       >::type
stablesort_enqueue(control& ctrl, const DVRandomAccessIterator& first, const DVRandomAccessIterator& last,
             const StrictWeakOrdering& comp)
{

    concurrency::accelerator_view av = ctrl.getAccelerator().default_view;
    unsigned int vecSize = static_cast< unsigned int >( std::distance( first, last ) );

    /**********************************************************************************
     * Type Names - used in KernelTemplateSpecializer
     *********************************************************************************/
    typedef typename std::iterator_traits< DVRandomAccessIterator >::value_type iType;

    const unsigned int localRange=BUFFER_SIZE;

    //  Make sure that globalRange is a multiple of localRange
    unsigned int globalRange = vecSize/**localRange*/;
    unsigned int  modlocalRange = ( globalRange & ( localRange-1 ) );
    if( modlocalRange )
    {
        globalRange &= ~modlocalRange;
        globalRange += localRange;
    }

    auto&  inputBuffer =  first.getContainer().getBuffer(first); 

    /**********************************************************************************
     *  Kernel 0
     *********************************************************************************/

    concurrency::extent< 1 > globalSizeK0( globalRange );
    concurrency::tiled_extent< localRange > tileK0 = globalSizeK0.tile< localRange >();

    try
    {

    concurrency::parallel_for_each( av, tileK0,
        [
            inputBuffer,
            vecSize,
            comp,
            localRange
        ] ( concurrency::tiled_index< localRange > t_idx ) restrict(amp)
  {

    unsigned int gloId = t_idx.global[ 0 ];
    unsigned int groId = t_idx.tile[ 0 ];
    unsigned int locId = t_idx.local[ 0 ];
    unsigned int wgSize = localRange;

    tile_static iType lds[BUFFER_SIZE]; 

    //  Abort threads that have passed the end of the input vector
    if (gloId >= vecSize) return; // on SI this doesn't mess-up barriers

    //  Make a copy of the entire input array into fast local memory
    iType val = inputBuffer[ gloId ];
    lds[ locId ] = val;
    //t_idx.barrier.wait();

    //  Sorts a workgroup using a naive insertion sort
    //  The sort uses one thread within a workgroup to sort the entire workgroup
    if( locId == 0 )
    {
        //  The last workgroup may have an irregular size, so we calculate a per-block endIndex
        //  endIndex is essentially emulating a mod operator with subtraction and multiply
        unsigned int endIndex = vecSize - ( groId * wgSize );
        endIndex = min( endIndex, wgSize );

        // printf( "Debug: endIndex[%i]=%i\n", groId, endIndex );

        //  Indices are signed because the while loop will generate a -1 index inside of the max function
        for( unsigned int currIndex = 1; currIndex < endIndex; ++currIndex )
        {
            val = lds[ currIndex ];
            int scanIndex = currIndex;
            iType ldsVal = lds[scanIndex - 1];
            while( scanIndex > 0 && comp( val, ldsVal ) )
            {
                lds[ scanIndex ] = ldsVal;
                scanIndex = scanIndex - 1;
                ldsVal = lds[ max(0, scanIndex - 1) ];  // scanIndex-1 may be -1
            }
            lds[ scanIndex ] = val;
        }
    }
    //t_idx.barrier.wait();

    val = lds[ locId ];
    inputBuffer[ gloId ] = val;
    
  } );
    }
     catch(std::exception &e)
      {
        std::cout << "Exception while calling bolt::amp::stablesort parallel_for_each " ;
        std::cout<< e.what() << std::endl;
        throw std::exception();
      }	


    //  An odd number of elements requires an extra merge pass to sort
    size_t numMerges = 0;

    //  Calculate the log2 of vecSize, taking into account our block size from kernel 1 is 64
    //  this is how many merge passes we want
    size_t log2BlockSize = vecSize >> 6;
    for( ; log2BlockSize > 1; log2BlockSize >>= 1 )
    {
        ++numMerges;
    }

    //  Check to see if the input vector size is a power of 2, if not we will need last merge pass
    size_t vecPow2 = (vecSize & (vecSize-1));
    numMerges += vecPow2? 1: 0;

    //  Allocate a flipflop buffer because the merge passes are out of place

    std::vector<iType> stdtmpBuffer(vecSize);
    device_vector<iType, concurrency::array_view > tmpBufferVec(stdtmpBuffer.begin(), stdtmpBuffer.end(), true, ctrl );
    auto&  tmpBuffer   =  tmpBufferVec.begin().getContainer().getBuffer(tmpBufferVec.begin()); 


    /**********************************************************************************
     *  Kernel 1
     *********************************************************************************/
    
    concurrency::extent< 1 > globalSizeK1( globalRange );
    concurrency::tiled_extent< localRange > tileK1 = globalSizeK1.tile< localRange >();

    for( size_t pass = 1; pass <= numMerges; ++pass )
    {

         //  For each pass, the merge window doubles
        unsigned srcLogicalBlockSize = static_cast< unsigned >( localRange << (pass-1) );


        if( pass & 0x1 )

        {

            try
            {
    concurrency::parallel_for_each( av, tileK1,
        [
            inputBuffer,
            tmpBuffer,
            vecSize,
            comp,
            localRange,
            srcLogicalBlockSize
        ] ( concurrency::tiled_index< localRange > t_idx ) restrict(amp)
  {

    unsigned int gloID = t_idx.global[ 0 ];
    unsigned int groID = t_idx.tile[ 0 ];
    unsigned int locID = t_idx.local[ 0 ];
    unsigned int wgSize = localRange;

    tile_static iType lds[BUFFER_SIZE]; 
     //  Abort threads that are passed the end of the input vector
    if( gloID >= vecSize )
        return; // on SI this doesn't mess-up barriers

    //  For an element in sequence A, find the lowerbound index for it in sequence B
    unsigned int srcBlockNum = gloID / srcLogicalBlockSize;
    unsigned int srcBlockIndex = gloID % srcLogicalBlockSize;
    
    //printf( "mergeTemplate: srcBlockNum[%i]=%i\n", srcBlockNum, srcBlockIndex );

    //  Pairs of even-odd blocks will be merged together 
    //  An even block should search for an insertion point in the next odd block, 
    //  and the odd block should look for an insertion point in the corresponding previous even block
    unsigned int dstLogicalBlockSize = srcLogicalBlockSize<<1;
    unsigned int leftBlockIndex = gloID & ~(dstLogicalBlockSize - 1 );
    //printf("mergeTemplate: leftBlockIndex=%d\n", leftBlockIndex );
    leftBlockIndex += (srcBlockNum & 0x1) ? 0 : srcLogicalBlockSize;
    leftBlockIndex = min( leftBlockIndex, vecSize );
    unsigned int rightBlockIndex = min( leftBlockIndex + srcLogicalBlockSize, vecSize );
    
    //  For a particular element in the input array, find the lowerbound index for it in the search sequence given by leftBlockIndex & rightBlockIndex
    // uint insertionIndex = lowerBoundLinear( source_ptr, leftBlockIndex, rightBlockIndex, source_ptr[ globalID ], lessOp ) - leftBlockIndex;
    unsigned int insertionIndex = 0;
    if( (srcBlockNum & 0x1) == 0 )
    {
        insertionIndex = lowerBoundBinary( inputBuffer, leftBlockIndex, rightBlockIndex, inputBuffer[ gloID ], comp ) - leftBlockIndex;
    }
    else
    {
        insertionIndex = upperBoundBinary( inputBuffer, leftBlockIndex, rightBlockIndex, inputBuffer[ gloID ], comp ) - leftBlockIndex;
    }
    
    //  The index of an element in the result sequence is the summation of it's indixes in the two input 
    //  sequences
    unsigned int dstBlockIndex = srcBlockIndex + insertionIndex;
    unsigned int dstBlockNum = srcBlockNum/2;
    
    tmpBuffer[ (dstBlockNum*dstLogicalBlockSize)+dstBlockIndex ] = inputBuffer[ gloID ];
    
  } );
    }
     catch(std::exception &e)
      {
        std::cout << "Exception while calling bolt::amp::stablesort parallel_for_each " ;
        std::cout<< e.what() << std::endl;
        throw std::exception();
      }	 
        }


        else
        {
			
            try
            {
    concurrency::parallel_for_each( av, tileK1,
        [
            inputBuffer,
            tmpBuffer,
            vecSize,
            comp,
            localRange,
            srcLogicalBlockSize
        ] ( concurrency::tiled_index< localRange > t_idx ) restrict(amp)
  {

    unsigned int gloID = t_idx.global[ 0 ];
    unsigned int groID = t_idx.tile[ 0 ];
    unsigned int locID = t_idx.local[ 0 ];
    unsigned int wgSize = localRange;

    tile_static iType lds[BUFFER_SIZE]; 
     //  Abort threads that are passed the end of the input vector
    if( gloID >= vecSize )
        return; // on SI this doesn't mess-up barriers

    //  For an element in sequence A, find the lowerbound index for it in sequence B
    unsigned int srcBlockNum = gloID / srcLogicalBlockSize;
    unsigned int srcBlockIndex = gloID % srcLogicalBlockSize;
    
    //printf( "mergeTemplate: srcBlockNum[%i]=%i\n", srcBlockNum, srcBlockIndex );

    //  Pairs of even-odd blocks will be merged together 
    //  An even block should search for an insertion point in the next odd block, 
    //  and the odd block should look for an insertion point in the corresponding previous even block
    unsigned int dstLogicalBlockSize = srcLogicalBlockSize<<1;
    unsigned int leftBlockIndex = gloID & ~(dstLogicalBlockSize - 1 );
    //printf("mergeTemplate: leftBlockIndex=%d\n", leftBlockIndex );
    leftBlockIndex += (srcBlockNum & 0x1) ? 0 : srcLogicalBlockSize;
    leftBlockIndex = min( leftBlockIndex, vecSize );
    unsigned int rightBlockIndex = min( leftBlockIndex + srcLogicalBlockSize, vecSize );
    
    //  For a particular element in the input array, find the lowerbound index for it in the search sequence given by leftBlockIndex & rightBlockIndex
    // uint insertionIndex = lowerBoundLinear( source_ptr, leftBlockIndex, rightBlockIndex, source_ptr[ globalID ], lessOp ) - leftBlockIndex;
    unsigned int insertionIndex = 0;
    if( (srcBlockNum & 0x1) == 0 )
    {
        insertionIndex = lowerBoundBinary( tmpBuffer, leftBlockIndex, rightBlockIndex, tmpBuffer[ gloID ], comp ) - leftBlockIndex;
    }
    else
    {
        insertionIndex = upperBoundBinary( tmpBuffer, leftBlockIndex, rightBlockIndex, tmpBuffer[ gloID ], comp ) - leftBlockIndex;
    }
    
    //  The index of an element in the result sequence is the summation of it's indixes in the two input 
    //  sequences
    unsigned int dstBlockIndex = srcBlockIndex + insertionIndex;
    unsigned int dstBlockNum = srcBlockNum/2;
    
    inputBuffer[ (dstBlockNum*dstLogicalBlockSize)+dstBlockIndex ] = tmpBuffer[ gloID ];
    
  } );
    }
     catch(std::exception &e)
      {
        std::cout << "Exception while calling bolt::amp::stablesort parallel_for_each " ;
        std::cout<< e.what() << std::endl;
        throw std::exception();
      }	 
        } 

    }

     //  If there are an odd number of merges, then the output data is sitting in the temp buffer.  We need to copy
    //  the results back into the input array
    if( numMerges & 1 )
    {
       for(unsigned int i=0; i<vecSize; i++)
           inputBuffer[i] = tmpBuffer[i];  
    }

    return;
}// END of stablesort_enqueue


//Non Device Vector specialization.
//This implementation creates a cl::Buffer and passes the cl buffer to the sort specialization whichtakes the
//cl buffer as a parameter.
//In the future, Each input buffer should be mapped to the device_vector and the specialization specific to
//device_vector should be called.
template< typename RandomAccessIterator, typename StrictWeakOrdering >
void stablesort_pick_iterator( control &ctl, const RandomAccessIterator& first, const RandomAccessIterator& last,
                            const StrictWeakOrdering& comp, 
                            std::random_access_iterator_tag )
{

    typedef typename std::iterator_traits< RandomAccessIterator >::value_type Type;

    size_t vecSize = std::distance( first, last );
    if( vecSize < 2 )
        return;

    bolt::amp::control::e_RunMode runMode = ctl.getForceRunMode( );

    if( runMode == bolt::amp::control::Automatic )
    {
        runMode = ctl.getDefaultPathToRun();
    }
   
    if( (runMode == bolt::amp::control::SerialCpu) || (vecSize < BOLT_AMP_STABLESORT_CPU_THRESHOLD) )
    {
        
        std::stable_sort( first, last, comp );
        return;
    }
    else if( runMode == bolt::amp::control::MultiCoreCpu )
    {
        #ifdef ENABLE_TBB
            bolt::btbb::stable_sort( first, last, comp );
        #else
            throw std::runtime_error("MultiCoreCPU Version of stable_sort not Enabled! \n");
        #endif

        return;
    }
    else
    {
        device_vector< Type, concurrency::array_view > dvInputOutput(  first, last, true, ctl );

        //Now call the actual AMP algorithm
        stablesort_enqueue(ctl,dvInputOutput.begin(),dvInputOutput.end(),comp);

        //Map the buffer back to the host
        dvInputOutput.data( );
        return;
    }
}

//Device Vector specialization
template< typename DVRandomAccessIterator, typename StrictWeakOrdering >
void stablesort_pick_iterator( control &ctl,
                         const DVRandomAccessIterator& first, const DVRandomAccessIterator& last,
                         const StrictWeakOrdering& comp, 
                         bolt::amp::device_vector_tag )
{

    typedef typename std::iterator_traits< DVRandomAccessIterator >::value_type Type;

    size_t vecSize = std::distance( first, last );
    if( vecSize < 2 )
        return;

    bolt::amp::control::e_RunMode runMode = ctl.getForceRunMode( );

    if( runMode == bolt::amp::control::Automatic )
    {
        runMode = ctl.getDefaultPathToRun();
    }
   
    if( runMode == bolt::amp::control::SerialCpu || (vecSize < BOLT_AMP_STABLESORT_CPU_THRESHOLD) )
    {
        typename bolt::amp::device_vector< Type >::pointer firstPtr =  first.getContainer( ).data( );
        std::stable_sort( &firstPtr[ first.m_Index ], &firstPtr[ last.m_Index ], comp );
        return;
    }
    else if( runMode == bolt::amp::control::MultiCoreCpu )
    {
        #ifdef ENABLE_TBB
            typename bolt::amp::device_vector< Type >::pointer firstPtr =  first.getContainer( ).data( );
            bolt::btbb::stable_sort( &firstPtr[ first.m_Index ], &firstPtr[ last.m_Index ], comp );
        #else
            throw std::runtime_error("MultiCoreCPU Version of stable_sort not Enabled! \n");
        #endif
        return;
    }
    else
    {
        stablesort_enqueue(ctl,first,last,comp);
    }

    return;
}

//Device Vector specialization
template<typename DVRandomAccessIterator, typename StrictWeakOrdering>
void stablesort_pick_iterator( control &ctl,
                         const DVRandomAccessIterator& first, const DVRandomAccessIterator& last,
                         const StrictWeakOrdering& comp,
                         bolt::amp::fancy_iterator_tag )
{
    static_assert(std::is_same<DVRandomAccessIterator, bolt::amp::fancy_iterator_tag  >::value , "It is not possible to sort fancy iterators. They are not mutable" );
}




template<typename RandomAccessIterator, typename StrictWeakOrdering>
void stablesort_detect_random_access( control &ctl,
                                const RandomAccessIterator& first, const RandomAccessIterator& last,
                                const StrictWeakOrdering& comp, 
                                std::random_access_iterator_tag )
{
    return stablesort_pick_iterator(ctl, first, last,
                              comp,
                              typename std::iterator_traits< RandomAccessIterator >::iterator_category( ) );
};

// Wrapper that uses default control class, iterator interface
template<typename RandomAccessIterator, typename StrictWeakOrdering>
void stablesort_detect_random_access( control &ctl,
                                const RandomAccessIterator& first, const RandomAccessIterator& last,
                                const StrictWeakOrdering& comp,
                                std::input_iterator_tag )
{
    //  \TODO:  It should be possible to support non-random_access_iterator_tag iterators, if we copied the data
    //  to a temporary buffer.  Should we?
    static_assert( std::is_same< RandomAccessIterator, std::input_iterator_tag >::value , "Bolt only supports random access iterator types" );
};



}//namespace bolt::cl::detail


    template<typename RandomAccessIterator>
    void stable_sort(RandomAccessIterator first,
              RandomAccessIterator last)
    {
        typedef typename std::iterator_traits< RandomAccessIterator >::value_type T;

        detail::stablesort_detect_random_access( control::getDefault( ),
                                           first, last,
                                           less< T >( ),
                                           typename std::iterator_traits< RandomAccessIterator >::iterator_category( ) );
        return;
    }

    template<typename RandomAccessIterator, typename StrictWeakOrdering>
    void stable_sort(RandomAccessIterator first,
              RandomAccessIterator last,
              StrictWeakOrdering comp)
    {
        detail::stablesort_detect_random_access( control::getDefault( ),
                                           first, last,
                                           comp, 
                                           typename std::iterator_traits< RandomAccessIterator >::iterator_category( ) );
        return;
    }

    template<typename RandomAccessIterator>
    void stable_sort(control &ctl,
              RandomAccessIterator first,
              RandomAccessIterator last)
    {
        typedef typename std::iterator_traits< RandomAccessIterator >::value_type T;

        detail::stablesort_detect_random_access(ctl,
                                          first, last,
                                          less< T >( ), 
                                          typename std::iterator_traits< RandomAccessIterator >::iterator_category( ) );
        return;
    }

    template<typename RandomAccessIterator, typename StrictWeakOrdering>
    void stable_sort(control &ctl,
              RandomAccessIterator first,
              RandomAccessIterator last,
              StrictWeakOrdering comp)
    {
        detail::stablesort_detect_random_access(ctl,
                                          first, last,
                                          comp, 
                                          typename std::iterator_traits< RandomAccessIterator >::iterator_category( ) );
        return;
    }

}//namespace bolt::amp
}//namespace bolt

#endif
