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

/******************************************************************************
 * AMP Scan
 *****************************************************************************/

#if !defined( BOLT_AMP_SCAN_INL )
#define BOLT_AMP_SCAN_INL
#pragma once


#define KERNEL02WAVES 4
#define KERNEL1WAVES 4
#define WAVESIZE 64

#if 0
#define NUM_PEEK 128
#define PEEK_AT( ... ) \
    { \
        unsigned int numPeek = NUM_PEEK; \
        numPeek = (__VA_ARGS__.get_extent().size() < numPeek) ? __VA_ARGS__.get_extent().size() : numPeek; \
        std::vector< oType > hostMem( numPeek ); \
        concurrency::array_view< oType > peekOutput( static_cast< int >( numPeek ), (oType *)&hostMem.begin()[ 0 ] ); \
        __VA_ARGS__.section( Concurrency::extent< 1 >( numPeek ) ).copy_to( peekOutput ); \
        for ( unsigned int i = 0; i < numPeek; i++) \
        { \
            std::cout << #__VA_ARGS__ << "[ " << i << " ] = " << peekOutput[ i ] << std::endl; \
        } \
    }
#else
#define PEEK_AT( ... )
#endif

#ifdef BOLT_ENABLE_PROFILING
#include "bolt/AsyncProfiler.h"
//AsyncProfiler aProfiler("transform_scan");
#endif

#include <algorithm>
#include <type_traits>
#include "bolt/amp/bolt.h"
#include "bolt/amp/device_vector.h"
#include <amp.h>

#ifdef ENABLE_TBB
//TBB Includes
#include "bolt/btbb/scan.h"

#endif

namespace bolt
{
namespace amp
{
namespace detail
{


//  All calls to inclusive_scan end up here, unless an exception was thrown
//  This is the function that sets up the kernels to compile (once only) and execute
template< typename DVInputIterator, typename DVOutputIterator, typename T, typename BinaryFunction >
void scan_enqueue(
    control &ctl,
    const DVInputIterator& first,
    const DVInputIterator& last,
    const DVOutputIterator& result,
    const T& init,
    const BinaryFunction& binary_op,
    const bool& inclusive = true )
{
#ifdef BOLT_ENABLE_PROFILING
aProfiler.setName("scan");
aProfiler.startTrial();
aProfiler.setStepName("Setup");
aProfiler.set(AsyncProfiler::device, control::SerialCpu);

size_t k0_stepNum, k1_stepNum, k2_stepNum;
#endif
  
    concurrency::accelerator_view av = ctl.getAccelerator().default_view;

    /**********************************************************************************
     * Type Names - used in KernelTemplateSpecializer
     *********************************************************************************/
    typedef std::iterator_traits< DVInputIterator >::value_type iType;
    typedef std::iterator_traits< DVOutputIterator >::value_type oType;

    int exclusive = inclusive ? 0 : 1;

    unsigned int numElements = static_cast< unsigned int >( std::distance( first, last ) );
    const unsigned int kernel0_WgSize = WAVESIZE*KERNEL02WAVES;
    const unsigned int kernel1_WgSize = WAVESIZE*KERNEL1WAVES ;
    const unsigned int kernel2_WgSize = WAVESIZE*KERNEL02WAVES;

    //  Ceiling function to bump the size of input to the next whole wavefront size
    unsigned int sizeInputBuff = numElements;
    size_t modWgSize = (sizeInputBuff & ((kernel0_WgSize*2)-1));
    if( modWgSize )
    {
        sizeInputBuff &= ~modWgSize;
        sizeInputBuff += (kernel0_WgSize*2);
    }
    unsigned int numWorkGroupsK0 = static_cast< unsigned int >( sizeInputBuff / (kernel0_WgSize*2) );
    //  Ceiling function to bump the size of the sum array to the next whole wavefront size
    unsigned int sizeScanBuff = numWorkGroupsK0;
    modWgSize = (sizeScanBuff & ((kernel0_WgSize*2)-1));
    if( modWgSize )
    {
        sizeScanBuff &= ~modWgSize;
        sizeScanBuff += (kernel0_WgSize*2);
    }

    concurrency::array< iType >  preSumArray( sizeScanBuff, av );
    concurrency::array< iType >  preSumArray1( sizeScanBuff, av );
    concurrency::array< iType > postSumArray( sizeScanBuff, av );

    /**********************************************************************************
     *  Kernel 0
     *********************************************************************************/
    //	Wrap our output data in an array_view, and discard input data so it is not transferred to device
    //  Use of the auto keyword here is OK, because AMP is restricted by definition to vs11 or above
    //  The auto keyword is useful here in a polymorphic sense, because it does not care if the container
    //  is wrapping an array or an array_view
    auto&  input = first.getContainer().getBuffer(first); 
    auto& output = result.getContainer().getBuffer(result); 
     
  //	Loop to calculate the inclusive scan of each individual tile, and output the block sums of every tile
  //	This loop is inherently parallel; every tile is independant with potentially many wavefronts
#ifdef BOLT_ENABLE_PROFILING
aProfiler.nextStep();
k0_stepNum = aProfiler.getStepNum();
aProfiler.setStepName("Kernel 0");
aProfiler.set(AsyncProfiler::device, ctl.forceRunMode());
aProfiler.set(AsyncProfiler::flops, 2*numElements);
aProfiler.set(AsyncProfiler::memory, 2*numElements*sizeof(iType) + 1*sizeScanBuff*sizeof(oType));
#endif

    concurrency::extent< 1 > globalSizeK0( sizeInputBuff/2 );
    concurrency::tiled_extent< kernel0_WgSize > tileK0 = globalSizeK0.tile< kernel0_WgSize >();
    //std::cout << "Kernel 0 Launching w/ " << sizeInputBuff << " threads for " << numElements << " elements. " << std::endl;
  concurrency::parallel_for_each( av, tileK0, 
        [
            output,
            input,
            init,
            numElements,
            &preSumArray,
            &preSumArray1,
            binary_op,
            exclusive,
            kernel0_WgSize
        ] ( concurrency::tiled_index< kernel0_WgSize > t_idx ) restrict(amp)
  {
        unsigned int gloId = t_idx.global[ 0 ];
        unsigned int groId = t_idx.tile[ 0 ];
        unsigned int locId = t_idx.local[ 0 ];
        unsigned int wgSize = kernel0_WgSize;

        tile_static iType lds[ WAVESIZE*KERNEL02WAVES*2 ];

        wgSize *=2;
        // if exclusive, load gloId=0 w/ identity, and all others shifted-1
		    if(groId*wgSize+locId < numElements)
            lds[locId] = input[groId*wgSize+locId];
        if(groId*wgSize +locId+ (wgSize/2) < numElements)
            lds[locId+(wgSize/2)] = input[ groId*wgSize +locId+ (wgSize/2)];

	    	// Exclusive case
        if(exclusive && gloId == 0)
	      {
	        iType start_val = input[0];
		      lds[locId] = binary_op(init, start_val);
        }
        unsigned int  offset = 1;
        //  Computes a scan within a workgroup with two data per element

         for (unsigned int start = wgSize>>1; start > 0; start >>= 1) 
         {
           t_idx.barrier.wait();
           if (locId < start)
           {
              unsigned int temp1 = offset*(2*locId+1)-1;
              unsigned int temp2 = offset*(2*locId+2)-1;
              iType y = lds[temp2];
              iType y1 =lds[temp1];

              lds[temp2] = binary_op(y, y1);
           }
           offset *= 2;
         }
         t_idx.barrier.wait();
         if (locId == 0)
         {
            preSumArray[ groId ] = lds[wgSize -1];
            preSumArray1[ groId ] = lds[wgSize/2 -1];
         }
  } );
    //std::cout << "Kernel 0 Done" << std::endl;
    PEEK_AT( output )

    /**********************************************************************************
     *  Kernel 1
     *********************************************************************************/
#ifdef BOLT_ENABLE_PROFILING
//aProfiler.nextStep();
//aProfiler.setStepName("Setup Kernel 1");
//aProfiler.set(AsyncProfiler::device, control::SerialCpu);
#endif

    unsigned int workPerThread = static_cast< unsigned int >( sizeScanBuff / kernel1_WgSize );
    workPerThread = workPerThread ? workPerThread : 1;


#ifdef BOLT_ENABLE_PROFILING
aProfiler.nextStep();
k1_stepNum = aProfiler.getStepNum();
aProfiler.setStepName("Kernel 1");
aProfiler.set(AsyncProfiler::device, ctl.forceRunMode());
aProfiler.set(AsyncProfiler::flops, 2*sizeScanBuff);
aProfiler.set(AsyncProfiler::memory, 4*sizeScanBuff*sizeof(oType));
#endif

    concurrency::extent< 1 > globalSizeK1( sizeScanBuff );
    concurrency::tiled_extent< kernel1_WgSize > tileK1 = globalSizeK1.tile< kernel1_WgSize >();
    //std::cout << "Kernel 1 Launching w/" << sizeScanBuff << " threads for " << numWorkGroupsK0 << " elements. " << std::endl;
  concurrency::parallel_for_each( av, tileK1,
        [
            &postSumArray,
            &preSumArray,
            numWorkGroupsK0,
            workPerThread,
            binary_op,
            kernel1_WgSize
        ] ( concurrency::tiled_index< kernel1_WgSize > t_idx ) restrict(amp)
  {
        unsigned int gloId = t_idx.global[ 0 ];
        unsigned int groId = t_idx.tile[ 0 ];
        unsigned int locId = t_idx.local[ 0 ];
        unsigned int wgSize = kernel1_WgSize;
        unsigned int mapId  = gloId * workPerThread;

        tile_static iType lds[ WAVESIZE*KERNEL1WAVES ];

        // do offset of zero manually
        unsigned int offset;
        iType workSum;
        if (mapId < numWorkGroupsK0)
        {
            // accumulate zeroth value manually
            offset = 0;
            workSum = preSumArray[mapId+offset];
            postSumArray[ mapId + offset ] = workSum;
            //  Serial accumulation
            for( offset = offset+1; offset < workPerThread; offset += 1 )
            {
                if (mapId+offset<numWorkGroupsK0)
                {
                    iType y = preSumArray[mapId+offset];
                    workSum = binary_op( workSum, y );
                    postSumArray[ mapId + offset ] = workSum;
                }
            }
        }
        t_idx.barrier.wait();
        iType scanSum = workSum;
        offset = 1;
        // load LDS with register sums
        lds[ locId ] = workSum;

        // scan in lds
        for( offset = offset*1; offset < wgSize; offset *= 2 )
        {
            t_idx.barrier.wait();
            if (mapId < numWorkGroupsK0)
            {
                if (locId >= offset)
                {
                    iType y = lds[ locId - offset ];
                    scanSum = binary_op( scanSum, y );
                }

            }
            t_idx.barrier.wait();
            lds[ locId ] = scanSum;
        } // for offset
        t_idx.barrier.wait();

        // write final scan from pre-scan and lds scan
        workSum = preSumArray[mapId];
        if (mapId < numWorkGroupsK0 && locId > 0){
           iType y = lds[locId-1];
           workSum = binary_op(workSum, y);
           postSumArray[ mapId] = workSum;
        }
        else if(mapId < numWorkGroupsK0 ){
           postSumArray[ mapId] = workSum;
        }
        // write final scan from pre-scan and lds scan
        for( offset = 1; offset < workPerThread; offset += 1 )
        {
             t_idx.barrier.wait_with_global_memory_fence();
             if (mapId+offset < numWorkGroupsK0 && locId > 0)
             {
                iType y  = preSumArray[ mapId + offset ] ;
                iType y1 = binary_op(y, workSum);
                postSumArray[ mapId + offset ] = y1;
                workSum = y1;

             } // thread in bounds
             else if(mapId+offset < numWorkGroupsK0 ){
               iType y  = preSumArray[ mapId + offset ] ;
               postSumArray[ mapId + offset ] = binary_op(y, workSum);
               workSum = postSumArray[ mapId + offset ];
            }

        } // for

    } );
    //std::cout << "Kernel 1 Done" << std::endl;
    PEEK_AT( postSumArray )

    /**********************************************************************************
     *  Kernel 2
     *********************************************************************************/
#ifdef BOLT_ENABLE_PROFILING
//aProfiler.nextStep();
//aProfiler.setStepName("Setup Kernel 2");
//aProfiler.set(AsyncProfiler::device, control::SerialCpu);
aProfiler.nextStep();
k2_stepNum = aProfiler.getStepNum();
aProfiler.setStepName("Kernel 2");
aProfiler.set(AsyncProfiler::device, ctl.forceRunMode());
aProfiler.set(AsyncProfiler::flops, numElements);
aProfiler.set(AsyncProfiler::memory, 2*numElements*sizeof(oType) + 1*sizeScanBuff*sizeof(oType));
#endif
    concurrency::extent< 1 > globalSizeK2( sizeInputBuff );
    concurrency::tiled_extent< kernel2_WgSize > tileK2 = globalSizeK2.tile< kernel2_WgSize >();
    //std::cout << "Kernel 2 Launching w/ " << sizeInputBuff << " threads for " << numElements << " elements. " << std::endl;
    concurrency::parallel_for_each( av, tileK2,
        [
            input,
            output,
            &postSumArray,
            &preSumArray1,
            numElements,
            binary_op,
            init,
            exclusive,
            kernel2_WgSize
        ] ( concurrency::tiled_index< kernel2_WgSize > t_idx ) restrict(amp)
  {
        unsigned int gloId = t_idx.global[ 0 ];
        unsigned int groId = t_idx.tile[ 0 ];
        unsigned int locId = t_idx.local[ 0 ];
        unsigned int wgSize = kernel2_WgSize;

         tile_static iType lds[ WAVESIZE*KERNEL1WAVES ];
        // if exclusive, load gloId=0 w/ identity, and all others shifted-1
        iType val;
  
        if (gloId < numElements){
           if (exclusive)
           {
              if (gloId > 0)
              { // thread>0
                  val = input[gloId-1];
                  lds[ locId ] = val;
              }
              else
              { // thread=0
                  val = init;
                  lds[ locId ] = val;
              }
           }
           else
           {
              val = input[gloId];
              lds[ locId ] = val;
           }
        }
  
	    	iType scanResult = lds[locId];
        iType postBlockSum, newResult;
        // accumulate prefix
        iType y, y1, sum;
        if(locId == 0 && gloId < numElements)
        {
            if(groId > 0) {
                if(groId % 2 == 0)
                   postBlockSum = postSumArray[ groId/2 -1 ];
                else if(groId == 1)
                   postBlockSum = preSumArray1[0];
                else {
                   y = postSumArray[ groId/2 -1 ];
                   y1 = preSumArray1[groId/2];
                   postBlockSum = binary_op(y, y1);
                }
			    if (!exclusive)
                   newResult = binary_op( scanResult, postBlockSum );
			    else 
				   newResult =  postBlockSum;
            }
            else {
               newResult = scanResult;
            }
		    lds[ locId ] = newResult;
        } 
        //  Computes a scan within a workgroup
        sum = lds[ locId ];
        unsigned int  offset = 1;
        for( unsigned int offset = 1; offset < wgSize; offset *= 2 )
        {
            t_idx.barrier.wait();
            if (locId >= offset)
            {
                iType y = lds[ locId - offset ];
                sum = binary_op( sum, y );
            }
            t_idx.barrier.wait();
            lds[ locId ] = sum;
        }
         t_idx.barrier.wait();
    //  Abort threads that are passed the end of the input vector
        if (gloId >= numElements) return; 

        output[ gloId ] = sum;

    } );
    //std::cout << "Kernel 2 Done" << std::endl;
    PEEK_AT( output )

#ifdef BOLT_ENABLE_PROFILING
aProfiler.nextStep();
aProfiler.setStepName("Copy Results Back");
aProfiler.set(AsyncProfiler::device, control::SerialCpu);
aProfiler.setDataSize(numElements*sizeof(oType));
#endif

}   //end of inclusive_scan_enqueue( )

						
template<
    typename vType,
    typename oType,
    typename BinaryFunction,
    typename T>
oType*
Serial_scan(
    vType *values,
    oType *result,
    unsigned int  num,
    const BinaryFunction binary_op,
    const bool Incl,
    const T &init)
{
    vType  sum, temp;
    if(Incl){
      *result = *values; // assign value
      sum = *values;
    }
    else {
        temp = *values;
       *result = (oType)init;
       sum = binary_op( *result, temp);
    }
    for ( unsigned int i= 1; i<num; i++)
    {
        vType currentValue = *(values + i); // convertible
        if (Incl)
        {
            vType r = binary_op( sum, currentValue);
            *(result + i) = r;
            sum = r;
        }
        else // new segment
        {
            *(result + i) = sum;
            sum = binary_op( sum, currentValue);

        }
    }
    return result;
}



/*!
* \brief This overload is called strictly for non-device_vector iterators
* \details This template function overload is used to seperate device_vector iterators from all other iterators
*/
template< typename InputIterator, typename OutputIterator, typename T, typename BinaryFunction >
typename std::enable_if<
    !(std::is_base_of<typename device_vector<typename
           std::iterator_traits<InputIterator>::value_type>::iterator,InputIterator>::value &&
      std::is_base_of<typename device_vector<typename
           std::iterator_traits<OutputIterator>::value_type>::iterator,OutputIterator>::value),
OutputIterator >::type
scan_pick_iterator(
    control &ctl,
    const InputIterator& first,
    const InputIterator& last,
    const OutputIterator& result,
    const T& init,
    const bool& inclusive,
    const BinaryFunction& binary_op )
{

    typedef typename std::iterator_traits< InputIterator >::value_type iType;
    typedef typename std::iterator_traits< OutputIterator >::value_type oType;
    static_assert( std::is_convertible< iType, oType >::value, "Input and Output iterators are incompatible" );

    unsigned int numElements = static_cast< unsigned int >( std::distance( first, last ) );
    if( numElements < 1 )
        return result;

    bolt::amp::control::e_RunMode runMode = ctl.getForceRunMode( );  // could be dynamic choice some day.
	if (runMode == bolt::amp::control::Automatic)
	{
		runMode = ctl.getDefaultPathToRun();
	}
    if( runMode == bolt::amp::control::SerialCpu )
    {
#ifdef BOLT_ENABLE_PROFILING
aProfiler.setName("scan");
aProfiler.startTrial();
aProfiler.setStepName("serial");
aProfiler.set(AsyncProfiler::device, control::SerialCpu);

size_t k0_stepNum, k1_stepNum, k2_stepNum;
#endif
         Serial_scan<iType, oType, BinaryFunction, T>(&(*first), &(*result), numElements, binary_op, inclusive, init);
#ifdef BOLT_ENABLE_PROFILING
aProfiler.setDataSize(numElements*sizeof(iType));
aProfiler.stopTrial();
#endif
         return result + numElements;
    }
    else if( runMode == bolt::amp::control::MultiCoreCpu )
    {
#ifdef ENABLE_TBB

           if(inclusive)
               {
                 return bolt::btbb::inclusive_scan(first, last, result, binary_op);
               }
               else
               {
                return bolt::btbb::exclusive_scan( first, last, result, init, binary_op);
               }
#else

        throw std::exception( "The MultiCoreCpu version of Scan is not enabled to be built." );
        return result;
#endif
    }
    else
    {

        // Map the input iterator to a device_vector
        device_vector< iType, concurrency::array_view > dvInput( first, last, false, ctl );
        device_vector< oType, concurrency::array_view > dvOutput( result, numElements, true, ctl );

        //Now call the actual AMP algorithm
        scan_enqueue( ctl, dvInput.begin( ), dvInput.end( ), dvOutput.begin( ), init, binary_op, inclusive );
       
        PEEK_AT( dvOutput.begin().getContainer().getBuffer())

        // This should immediately map/unmap the buffer
        dvOutput.data( );
    }

    return result + numElements;
}

/*!
* \brief This overload is called strictly for non-device_vector iterators
* \details This template function overload is used to seperate device_vector iterators from all other iterators
*/
template< typename DVInputIterator, typename DVOutputIterator, typename T, typename BinaryFunction >
typename std::enable_if<
    (std::is_base_of<typename device_vector<typename std::iterator_traits<DVInputIterator>::value_type>::iterator,DVInputIterator>::value &&
     std::is_base_of<typename device_vector<typename std::iterator_traits<DVOutputIterator>::value_type>::iterator,DVOutputIterator>::value),
DVOutputIterator >::type
scan_pick_iterator(
    control &ctl,
    const DVInputIterator& first,
    const DVInputIterator& last,
    const DVOutputIterator& result,
    const T& init,
    const bool& inclusive,
    const BinaryFunction& binary_op )
{

    typedef typename std::iterator_traits< DVInputIterator >::value_type iType;
    typedef typename std::iterator_traits< DVOutputIterator >::value_type oType;
    static_assert( std::is_convertible< iType, oType >::value, "Input and Output iterators are incompatible" );

    typedef typename std::vector<iType>::iterator InputIterator;
    typedef typename std::vector<oType>::iterator OutputIterator;

    unsigned int numElements = static_cast< unsigned int >( std::distance( first, last ) );
    if( numElements < 1 )
        return result;

    bolt::amp::control::e_RunMode runMode = ctl.getForceRunMode( );  // could be dynamic choice some day.
	if (runMode == bolt::amp::control::Automatic)
	{
		runMode = ctl.getDefaultPathToRun();
	}
    if( runMode == bolt::amp::control::SerialCpu )
    {
        bolt::amp::device_vector< iType >::pointer scanInputBuffer =  first.getContainer( ).data( );
        bolt::amp::device_vector< oType >::pointer scanResultBuffer =  result.getContainer( ).data( );
        Serial_scan<iType, oType, BinaryFunction, T>(&scanInputBuffer[first.m_Index], &scanResultBuffer[result.m_Index],
                                                              numElements, binary_op, inclusive, init);
        return result + numElements;
    }
    else if( runMode == bolt::amp::control::MultiCoreCpu )
    {

#ifdef ENABLE_TBB
        bolt::amp::device_vector< iType >::pointer scanInputBuffer =  first.getContainer( ).data( );
        bolt::amp::device_vector< oType >::pointer scanResultBuffer =  result.getContainer( ).data( );
        if(inclusive)
            bolt::btbb::inclusive_scan(&scanInputBuffer[first.m_Index], &scanInputBuffer[first.m_Index + numElements], &scanResultBuffer[result.m_Index], binary_op);
        else
            bolt::btbb::exclusive_scan( &scanInputBuffer[first.m_Index], &scanInputBuffer[first.m_Index + numElements], &scanResultBuffer[result.m_Index], init, binary_op);

        return result + numElements;
#else
        throw std::exception( "The MultiCoreCpu version of Scan with device vector is not enabled to be built." );
        return result;
#endif
    }
    else{

        //Now call the actual AMP algorithm
        scan_enqueue( ctl, first, last, result, init, binary_op, inclusive );
 
        PEEK_AT( result.getContainer().getBuffer())
    }

    return result + numElements;
}

template< typename InputIterator, typename OutputIterator, typename T, typename BinaryFunction >
OutputIterator scan_detect_random_access(
    control &ctl,
    const InputIterator& first,
    const InputIterator& last,
    const OutputIterator& result,
    const T& init,
    const bool& inclusive,
    BinaryFunction binary_op,
    std::input_iterator_tag )
{
    //  TODO:  It should be possible to support non-random_access_iterator_tag iterators, if we copied the data
    //  to a temporary buffer.  Should we?
    static_assert( false, "Bolt only supports random access iterator types" );
};

template< typename InputIterator, typename OutputIterator, typename T, typename BinaryFunction >
OutputIterator scan_detect_random_access(
    control &ctl,
    const InputIterator& first,
    const InputIterator& last,
    const OutputIterator& result,
    const T& init,
    const bool& inclusive,
    const BinaryFunction& binary_op,
    std::random_access_iterator_tag )
{
    return detail::scan_pick_iterator( ctl, first, last, result, init, inclusive, binary_op );
};

}   //namespace detail

//////////////////////////////////////////
//  Inclusive scan overloads
//////////////////////////////////////////
template< typename InputIterator, typename OutputIterator >
OutputIterator inclusive_scan(
    InputIterator first,
    InputIterator last,
    OutputIterator result)
{
    typedef std::iterator_traits<InputIterator>::value_type iType;
    iType init; memset(&init, 0, sizeof(iType) );
    return detail::scan_detect_random_access(
        control::getDefault( ), first, last, result, init, true, plus< iType >( ),
        std::iterator_traits< InputIterator >::iterator_category( ) );
};

template< typename InputIterator, typename OutputIterator, typename BinaryFunction >
OutputIterator inclusive_scan(
    InputIterator first,
    InputIterator last,
    OutputIterator result,
    BinaryFunction binary_op )
{
    typedef std::iterator_traits<InputIterator>::value_type iType;
    iType init; memset(&init, 0, sizeof(iType) );
    return detail::scan_detect_random_access(
        control::getDefault( ), first, last, result, init, true, binary_op,
        std::iterator_traits< InputIterator >::iterator_category( ) );
};

template< typename InputIterator, typename OutputIterator >
OutputIterator inclusive_scan(
    control &ctl,
    InputIterator first,
    InputIterator last,
    OutputIterator result)
{
    typedef std::iterator_traits<InputIterator>::value_type iType;
    iType init; memset(&init, 0, sizeof(iType) );
    return detail::scan_detect_random_access(
        ctl, first, last, result, init, true, plus< iType >( ),
        std::iterator_traits< InputIterator >::iterator_category( ) );
};

template< typename InputIterator, typename OutputIterator, typename BinaryFunction >
OutputIterator inclusive_scan(
    control &ctl,
    InputIterator first,
    InputIterator last,
    OutputIterator result,
    BinaryFunction binary_op)
{
    typedef std::iterator_traits<InputIterator>::value_type iType;
    iType init; memset(&init, 0, sizeof(iType) );
    return detail::scan_detect_random_access(
        ctl, first, last, result, init, true, binary_op,
        std::iterator_traits< InputIterator >::iterator_category( ) );
};

//////////////////////////////////////////
//  Exclusive scan overloads
//////////////////////////////////////////
template< typename InputIterator, typename OutputIterator >
OutputIterator exclusive_scan(
    InputIterator first,
    InputIterator last,
    OutputIterator result)
{
    typedef std::iterator_traits<InputIterator>::value_type iType;
    iType init; memset(&init, 0, sizeof(iType) );
    return detail::scan_detect_random_access(
        control::getDefault( ), first, last, result, init, false, plus< iType >( ),
        std::iterator_traits< InputIterator >::iterator_category( ) );
};

template< typename InputIterator, typename OutputIterator, typename T >
OutputIterator exclusive_scan(
    InputIterator first,
    InputIterator last,
    OutputIterator result,
    T init)
{
    typedef std::iterator_traits<InputIterator>::value_type iType;
    return detail::scan_detect_random_access(
        control::getDefault( ), first, last, result, init, false, plus< iType >( ),
        std::iterator_traits< InputIterator >::iterator_category( ) );
};

template< typename InputIterator, typename OutputIterator, typename T, typename BinaryFunction >
OutputIterator exclusive_scan(
    InputIterator first,
    InputIterator last,
    OutputIterator result,
    T init,
    BinaryFunction binary_op)
{
    return detail::scan_detect_random_access(
        control::getDefault( ), first, last, result, init, false, binary_op,
        std::iterator_traits< InputIterator >::iterator_category( ) );
};

template< typename InputIterator, typename OutputIterator >
OutputIterator exclusive_scan(
    control &ctl,
    InputIterator first,
    InputIterator last,
    OutputIterator result ) // assumes addition of numbers
{
    typedef std::iterator_traits<InputIterator>::value_type iType;
    iType init; memset(&init, 0, sizeof(iType) );
    return detail::scan_detect_random_access(
        ctl, first, last, result, init, false, plus< iType >( ),
        std::iterator_traits< InputIterator >::iterator_category( ) );
};

template< typename InputIterator, typename OutputIterator, typename T >
OutputIterator exclusive_scan(
    control &ctl,
    InputIterator first,
    InputIterator last,
    OutputIterator result,
    T init)
{
    typedef std::iterator_traits<InputIterator>::value_type iType;
    return detail::scan_detect_random_access(
        ctl, first, last, result, init, false, plus< iType >( ),
        std::iterator_traits< InputIterator >::iterator_category( ) );
};

template< typename InputIterator, typename OutputIterator, typename T, typename BinaryFunction >
OutputIterator exclusive_scan(
    control &ctl,
    InputIterator first,
    InputIterator last,
    OutputIterator result,
    T init,
    BinaryFunction binary_op)
{
    return detail::scan_detect_random_access(
        ctl, first, last, result, init, false, binary_op,
        std::iterator_traits< InputIterator >::iterator_category( ) );
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

}   //namespace amp
}//namespace bolt

#endif // AMP_SCAN_INL
