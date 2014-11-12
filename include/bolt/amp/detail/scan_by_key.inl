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
#if !defined( BOLT_AMP_SCAN_BY_KEY_INL )
#define BOLT_AMP_SCAN_BY_KEY_INL

#define SCANBYKEY_KERNELWAVES 4
#define SCANBYKEY_WAVESIZE 64
#define SCANBYKEY_TILE_MAX 65535

#include "bolt/amp/iterator/iterator_traits.h"
#ifdef ENABLE_TBB
//TBB Includes
#include "bolt/btbb/scan_by_key.h"
#endif
#ifdef BOLT_ENABLE_PROFILING
#include "bolt/AsyncProfiler.h"
//AsyncProfiler aProfiler("transform_scan");
#endif

#include "bolt/amp/iterator/addressof.h"


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



namespace bolt
{
namespace amp
{

namespace detail
{
/*!
*   \internal
*   \addtogroup detail
*   \ingroup scan
*   \{
*/
namespace serial{

    template<
			typename InputIterator1,
			typename InputIterator2,
			typename OutputIterator,
			typename T,
			typename BinaryPredicate,
			typename BinaryFunction >
			typename std::enable_if< (std::is_same< typename std::iterator_traits< OutputIterator >::iterator_category ,
                                       bolt::amp::device_vector_tag
                                     >::value), void
			>::type

			scan_by_key(
			::bolt::amp::control &ctl, 
			const InputIterator1& first1,
			const InputIterator1& last1, 
			const InputIterator2& first2,
			const OutputIterator& result,
			const T& init,
			const BinaryPredicate& binary_pred,
			const BinaryFunction& binary_op,
			const bool& inclusive,
			const std::string& user_code)
			{
	
				size_t sz = (last1 - first1);
				
				typedef typename std::iterator_traits<OutputIterator>::value_type oType;
				
				auto mapped_fst1_itr = create_mapped_iterator(typename std::iterator_traits<InputIterator1>::iterator_category(), 
																ctl, first1);
				auto mapped_fst2_itr = create_mapped_iterator(typename std::iterator_traits<InputIterator2>::iterator_category(), 
																ctl, first2);
				auto mapped_res_itr = create_mapped_iterator(typename std::iterator_traits<OutputIterator>::iterator_category(), 
																ctl, result);

				if(inclusive)
				{					
					// do zeroeth element
					*mapped_res_itr = (*mapped_fst2_itr); // assign value
					// scan oneth element and beyond
					for ( unsigned int i=1; i< sz;  i++)
					{
						// load value
						oType currentValue = *(mapped_fst2_itr +i ); 
						oType previousValue = *(mapped_res_itr + i -1 );

						// within segment
						if (binary_pred(mapped_fst1_itr [i], mapped_fst1_itr [i -1]))
						{
							oType r = binary_op( previousValue, currentValue);
							*(mapped_res_itr + i) = r;
						}
						else // new segment
						{
							*(mapped_res_itr + i) = currentValue;
						}
					}
				}
				else
				{
					oType temp = *mapped_fst2_itr;
					*mapped_res_itr = static_cast<oType>( init );
					// scan oneth element and beyond
					for ( unsigned int i= 1; i<sz; i++)
					{
						// load value
						oType currentValue = temp; // convertible
						oType previousValue = static_cast<oType>( *(mapped_res_itr + i -1 ));

						// within segment
						if (binary_pred(mapped_fst1_itr[i], mapped_fst1_itr[i -1]))
						{
							temp = *(mapped_fst2_itr + i);
							oType r = binary_op( previousValue, currentValue);
							*(mapped_res_itr + i) = r;
						}
						else // new segment
						{
							 temp = *(mapped_fst2_itr + i);
							*(mapped_res_itr + i) =  static_cast<oType>(init);
						}
				
				    }		
			    }
				 
				return ;			
	
	         }

			template<
			typename InputIterator1,
			typename InputIterator2,
			typename OutputIterator,
			typename T,
			typename BinaryPredicate,
			typename BinaryFunction >
			typename std::enable_if< (std::is_same< typename std::iterator_traits< OutputIterator >::iterator_category ,
                                       std::random_access_iterator_tag
                                     >::value), void
            >::type

			scan_by_key(
			::bolt::amp::control &ctl, 
			const InputIterator1& first1,
			const InputIterator1& last1, 
			const InputIterator2& first2,
			const OutputIterator& result,
			const T& init,
			const BinaryPredicate& binary_pred,
			const BinaryFunction& binary_op,
			const bool& inclusive,
			const std::string& user_code)
			{			
	
				size_t sz = (last1 - first1);

				typedef typename std::iterator_traits<InputIterator1>::value_type kType;
				typedef typename std::iterator_traits<InputIterator2>::value_type iType;
				typedef typename std::iterator_traits<OutputIterator>::value_type oType;

				if(inclusive)
				{
					// do zeroeth element
					*result = *first2; // assign value
					// scan oneth element and beyond
					for ( unsigned int i=1; i< sz;  i++)
					{
						// load value
						oType currentValue = *(first2 + i); // convertible
						oType previousValue = *(result + i - 1);

						// within segment
						if (binary_pred(*(first1 + i), *(first1 + i - 1)))
						{
							oType r = binary_op( previousValue, currentValue);
							*(result+i) = r;
						}
						else // new segment
						{
							*(result + i) = currentValue;
						}
					}
				
				}
				else
				{
					// do zeroeth element
					//*result = *values; // assign value
					oType temp = *first2;
					*result = static_cast<oType>(init);
					// scan oneth element and beyond
					for ( unsigned int i= 1; i<sz; i++)
					{
						// load value
						oType currentValue = temp; // convertible
						oType previousValue = static_cast<oType>(*(result + i -1 ));

						// within segment
						if (binary_pred(*(first1 + i), *(first1+ i -1 )))
						{
							temp = *(first2 + i);
							oType r = binary_op( previousValue, currentValue);
							*(result + i) = r;
						}
						else // new segment
						{
							 temp = *(first2 + i);
							 *(result + i) = static_cast<oType>(init);
						}
					}
				
				}
				return;
			}
}//end of namespace serial

#ifdef ENABLE_TBB
namespace btbb{

template<
    typename InputIterator1,
    typename InputIterator2,
    typename OutputIterator,
    typename T,
    typename BinaryPredicate,
    typename BinaryFunction >
typename std::enable_if< (std::is_same< typename std::iterator_traits< OutputIterator >::iterator_category ,
                                       std::random_access_iterator_tag
                                     >::value), void
           >::type
scan_by_key(
    bolt::amp::control& ctl,
    InputIterator1 firstKey,
	InputIterator1 lastKey,
    InputIterator2 values,
    OutputIterator result,
	const T &init,
    const BinaryPredicate binary_pred,
    const BinaryFunction binary_funct,
    const bool& inclusive,
    const std::string& user_code)
{

	if(inclusive)
		bolt::btbb::inclusive_scan_by_key( firstKey, lastKey, values, result, binary_pred, binary_funct);
	else
		bolt::btbb::exclusive_scan_by_key(firstKey, lastKey, values, result, init, binary_pred, binary_funct);
	return;
}

template<
    typename InputIterator1,
    typename InputIterator2,
    typename OutputIterator,
    typename T,
    typename BinaryPredicate,
    typename BinaryFunction >
typename std::enable_if< (std::is_same< typename std::iterator_traits< OutputIterator >::iterator_category ,
                                        bolt::amp::device_vector_tag
                                     >::value), void
           >::type
scan_by_key(
    bolt::amp::control& ctl,
    InputIterator1 first1,
	InputIterator1 last1,
    InputIterator2 first2,
    OutputIterator result,
	const T &init,
    const BinaryPredicate binary_pred,
    const BinaryFunction binary_op,
    const bool& inclusive,
    const std::string& user_code)
{

	    size_t sz = (last1 - first1);
		
		auto mapped_fst1_itr = create_mapped_iterator(typename std::iterator_traits<InputIterator1>::iterator_category(), 
														ctl, first1);
		auto mapped_fst2_itr = create_mapped_iterator(typename std::iterator_traits<InputIterator2>::iterator_category(), 
														ctl, first2);
		auto mapped_res_itr = create_mapped_iterator(typename std::iterator_traits<OutputIterator>::iterator_category(),
														ctl, result);
		if(inclusive)
			bolt::btbb::inclusive_scan_by_key(mapped_fst1_itr, mapped_fst1_itr + (int)sz, mapped_fst2_itr, mapped_res_itr, binary_pred, binary_op );
		else
			bolt::btbb::exclusive_scan_by_key(mapped_fst1_itr, mapped_fst1_itr + (int)sz, mapped_fst2_itr, mapped_res_itr, init, binary_pred, binary_op );
		
		return ;	
}


}//end of namespace btbb
#endif

namespace amp{

//  All calls to scan_by_key end up here, unless an exception was thrown
//  This is the function that sets up the kernels to compile (once only) and execute
template<
    typename DVInputIterator1,
    typename DVInputIterator2,
    typename DVOutputIterator,
    typename T,
    typename BinaryPredicate,
    typename BinaryFunction >
typename std::enable_if< std::is_same< typename std::iterator_traits< DVOutputIterator >::iterator_category ,
									bolt::amp::device_vector_tag
									>::value
					>::type
scan_by_key(
    bolt::amp::control& ctl,
    const DVInputIterator1& firstKey,
    const DVInputIterator1& lastKey,
    const DVInputIterator2& firstValue,
    const DVOutputIterator& result,
    const T& init,
    const BinaryPredicate& binary_pred,
    const BinaryFunction& binary_funct,
	const bool& inclusive,
    const std::string& user_code)
{
    concurrency::accelerator_view av = ctl.getAccelerator().default_view;

    /**********************************************************************************
     * Type Names - used in KernelTemplateSpecializer
     *********************************************************************************/
    typedef std::iterator_traits< DVInputIterator1 >::value_type kType;
    typedef std::iterator_traits< DVInputIterator2 >::value_type vType;
  	typedef std::iterator_traits< DVOutputIterator >::value_type oType;

    int exclusive = inclusive ? 0 : 1;

    int numElements = static_cast< int >( std::distance( firstKey, lastKey ) );
    const unsigned int kernel0_WgSize = SCANBYKEY_WAVESIZE*SCANBYKEY_KERNELWAVES;
    const unsigned int kernel1_WgSize = SCANBYKEY_WAVESIZE*SCANBYKEY_KERNELWAVES;
    const unsigned int kernel2_WgSize = SCANBYKEY_WAVESIZE*SCANBYKEY_KERNELWAVES;

    //  Ceiling function to bump the size of input to the next whole wavefront size
    unsigned int sizeInputBuff = numElements;
    unsigned int modWgSize = (sizeInputBuff & ((kernel0_WgSize*2)-1));
    if( modWgSize )
    {
        sizeInputBuff &= ~modWgSize;
        sizeInputBuff += (kernel0_WgSize*2);
    }
    int numWorkGroupsK0 = static_cast< int >( sizeInputBuff / (kernel0_WgSize*2) );
    //  Ceiling function to bump the size of the sum array to the next whole wavefront size
    unsigned int sizeScanBuff = numWorkGroupsK0;
    modWgSize = (sizeScanBuff & ((kernel0_WgSize*2)-1));
    if( modWgSize )
    {
        sizeScanBuff &= ~modWgSize;
        sizeScanBuff += (kernel0_WgSize*2);
    }

  	concurrency::array< kType >  keySumArray( sizeScanBuff, av );
    concurrency::array< vType >  preSumArray( sizeScanBuff, av );
    concurrency::array< vType >  preSumArray1( sizeScanBuff, av );


    /**********************************************************************************
     *  Kernel 0
     *********************************************************************************/
  //	Loop to calculate the inclusive scan of each individual tile, and output the block sums of every tile
  //	This loop is inherently parallel; every tile is independant with potentially many wavefronts

	const unsigned int tile_limit = SCANBYKEY_TILE_MAX;
	const unsigned int max_ext = (tile_limit*kernel0_WgSize);
	unsigned int	   tempBuffsize = (sizeInputBuff/2); 
	unsigned int	   iteration = (tempBuffsize-1)/max_ext; 

    for(unsigned int i=0; i<=iteration; i++)
	{
	    unsigned int extent_sz =  (tempBuffsize > max_ext) ? max_ext : tempBuffsize; 
		concurrency::extent< 1 > inputExtent( extent_sz );
		concurrency::tiled_extent< kernel0_WgSize > tileK0 = inputExtent.tile< kernel0_WgSize >();
		unsigned int index = i*(tile_limit*kernel0_WgSize);
		unsigned int tile_index = i*tile_limit;

			concurrency::parallel_for_each( av, tileK0,
			[
				firstKey,
				firstValue,
				init,
				numElements,
				&keySumArray,
				&preSumArray,
				&preSumArray1,
		      	binary_pred,
				exclusive,
				index,
				tile_index,
		      	binary_funct,
				kernel0_WgSize
			] ( concurrency::tiled_index< kernel0_WgSize > t_idx ) restrict(amp)
			  {

				int gloId = t_idx.global[ 0 ] + index;
				unsigned int groId = t_idx.tile[ 0 ] + tile_index;
				unsigned int locId = t_idx.local[ 0 ];
				int wgSize = kernel0_WgSize;

				tile_static vType ldsVals[ SCANBYKEY_WAVESIZE*SCANBYKEY_KERNELWAVES*2 ];
				tile_static kType ldsKeys[ SCANBYKEY_WAVESIZE*SCANBYKEY_KERNELWAVES*2 ];
   				wgSize *=2;

  				unsigned int  offset = 1;
				// load input into shared memory
				int input_offset = (groId*wgSize)+locId;

				if (exclusive)
				{
				   if (gloId > 0 && input_offset < numElements)
				   {
					  kType key1 = firstKey[ input_offset ];
					  kType key2 = firstKey[ input_offset-1 ];
					  if( binary_pred( key1, key2 )  )
					  {
						 ldsVals[locId] = firstValue[ input_offset ];
					  }
					  else 
					  {
						 vType start_val = firstValue[ input_offset ]; 
						 ldsVals[locId] = binary_funct(init, start_val);
					  }
					  ldsKeys[locId] = firstKey[ input_offset ];
				   }
				   else{ 
					  vType start_val = firstValue[0];
					  ldsVals[locId] = binary_funct(init, start_val);
					  ldsKeys[locId] = firstKey[0];
				   }
				   if(input_offset + (wgSize/2) < numElements)
				   {
					  kType key1 = firstKey[ input_offset + (wgSize/2)];
					  kType key2 = firstKey[ input_offset + (wgSize/2) -1];
					  if( binary_pred( key1, key2 )  )
					  {
						 ldsVals[locId+(wgSize/2)] = firstValue[ input_offset + (wgSize/2)];
					  }
					  else 
					  {
						 vType start_val = firstValue[ input_offset + (wgSize/2)]; 
						 ldsVals[locId+(wgSize/2)] = binary_funct(init, start_val);
					  } 
					  ldsKeys[locId+(wgSize/2)] = firstKey[ input_offset + (wgSize/2)];
				   }
				}
				else
				{
				   if(input_offset < numElements)
				   {
					   ldsVals[locId] = firstValue[input_offset];
					   ldsKeys[locId] = firstKey[input_offset];
				   }
				   if(input_offset + (wgSize/2) < numElements)
				   {
					   ldsVals[locId+(wgSize/2)] = firstValue[ input_offset + (wgSize/2)];
					   ldsKeys[locId+(wgSize/2)] = firstKey[ input_offset + (wgSize/2)];
				   }
				}
				for (unsigned int start = wgSize>>1; start > 0; start >>= 1) 
				{
				   t_idx.barrier.wait();
				   if (locId < start)
				   {
					  unsigned int temp1 = offset*(2*locId+1)-1;
					  unsigned int temp2 = offset*(2*locId+2)-1;
       
					  kType key = ldsKeys[temp2]; 
					  kType key1 = ldsKeys[temp1];
					  if(binary_pred( key, key1 )) 
					  {
						 oType y = ldsVals[temp2];
						 oType y1 =ldsVals[temp1];
						 ldsVals[temp2] = binary_funct(y, y1);
					  }

				   }
				   offset *= 2;
				}
				t_idx.barrier.wait();
				if (locId == 0)
				{
					keySumArray[ groId ] = ldsKeys[ wgSize-1 ];
					preSumArray[ groId ] = ldsVals[wgSize -1];
					preSumArray1[ groId ] = ldsVals[wgSize/2 -1];
				}

		 } );
	     tempBuffsize = tempBuffsize - max_ext;
	}

	PEEK_AT( output )

   /**********************************************************************************
     *  Kernel 1
     *********************************************************************************/
    int workPerThread = static_cast< int >( sizeScanBuff / kernel1_WgSize );
    workPerThread = workPerThread ? workPerThread : 1;

	concurrency::extent< 1 > globalSizeK1( kernel1_WgSize );
    concurrency::tiled_extent< kernel1_WgSize > tileK1 = globalSizeK1.tile< kernel1_WgSize >();

    //std::cout << "Kernel 1 Launching w/" << sizeScanBuff << " threads for " << numWorkGroupsK0 << " elements. " << std::endl;
    concurrency::parallel_for_each( av, tileK1,
        [
	      	&keySumArray, 
            &preSumArray,
            numWorkGroupsK0,
            workPerThread,
	        binary_pred,
		    numElements,
            binary_funct,
		    kernel1_WgSize
        ] ( concurrency::tiled_index< kernel1_WgSize > t_idx ) restrict(amp)
  {

	unsigned int gloId = t_idx.global[ 0 ];
    unsigned int groId = t_idx.tile[ 0 ];
    int locId = t_idx.local[ 0 ];
    int wgSize = kernel1_WgSize;
    int mapId  = gloId * workPerThread;

    tile_static kType ldsKeys[ SCANBYKEY_WAVESIZE*SCANBYKEY_KERNELWAVES];
	tile_static vType ldsVals[ SCANBYKEY_WAVESIZE*SCANBYKEY_KERNELWAVES ];

	// do offset of zero manually
    int offset;
    kType key;
    vType workSum;
    if (mapId < numWorkGroupsK0)
    {
        kType prevKey;

        // accumulate zeroth value manually
        offset = 0;
        key = keySumArray[ mapId+offset ];
        workSum = preSumArray[ mapId+offset ];
        //  Serial accumulation
        for( offset = offset+1; offset < workPerThread; offset += 1 )
        {
            prevKey = key;
            key = keySumArray[ mapId+offset ];
            if (mapId+offset<numWorkGroupsK0 )
            {
                vType y = preSumArray[ mapId+offset ];
                if ( binary_pred(key, prevKey ) )
                {
                    workSum = binary_funct( workSum, y );
                }
                else
                {
                    workSum = y;
                }
                preSumArray[ mapId+offset ] = workSum;
            }
        }
    }
    t_idx.barrier.wait();
    vType scanSum = workSum;
    offset = 1;
    // load LDS with register sums
    ldsVals[ locId ] = workSum;
    ldsKeys[ locId ] = key;
    // scan in lds

    for( offset = offset*1; offset < wgSize; offset *= 2 )
    {
        t_idx.barrier.wait();
        if (mapId < numWorkGroupsK0)
        {
            if (locId >= offset  )
            {
                vType y = ldsVals[ locId - offset ];
                kType key1 = ldsKeys[ locId ];
                kType key2 = ldsKeys[ locId-offset ];
                if ( binary_pred( key1, key2 ) )
                {
                    scanSum = binary_funct( scanSum, y );
                }
                else
                    scanSum = ldsVals[ locId ];
            }
      
        }
        t_idx.barrier.wait();
        ldsVals[ locId ] = scanSum;
    } // for offset
    t_idx.barrier.wait();

    // write final scan from pre-scan and lds scan
    for( offset = 0; offset < workPerThread; offset += 1 )
    {
        t_idx.barrier.wait_with_global_memory_fence();

        if (mapId+offset < numWorkGroupsK0 && locId > 0)
        {
            vType y = preSumArray[ mapId+offset ];
            kType key1 = keySumArray[ mapId+offset ]; // change me
            kType key2 = ldsKeys[ locId-1 ];
            if ( binary_pred( key1, key2 ) )
            {
                vType y2 = ldsVals[locId-1];
                y = binary_funct( y, y2 );
            }
            preSumArray[ mapId+offset ] = y;
        } // thread in bounds
    } // for 

  });

   PEEK_AT( postSumArray )

 /**********************************************************************************
     *  Kernel 2
 *********************************************************************************/
 	tempBuffsize = (sizeInputBuff); 
	iteration = (tempBuffsize-1)/max_ext; 

    for(unsigned int a=0; a<=iteration ; a++)
	{
	    unsigned int extent_sz =  (tempBuffsize > max_ext) ? max_ext : tempBuffsize; 
		concurrency::extent< 1 > inputExtent( extent_sz );
		concurrency::tiled_extent< kernel2_WgSize > tileK2 = inputExtent.tile< kernel2_WgSize >();
		unsigned int index = a*(tile_limit*kernel2_WgSize);
		unsigned int tile_index = a*tile_limit;

			concurrency::parallel_for_each( av, tileK2,
				[
	      			firstKey,
					firstValue,
					result,
					&preSumArray,
					&preSumArray1,
					binary_pred,
					exclusive,
					index,
					tile_index,
					numElements,
					init,
					binary_funct,
					kernel2_WgSize
				] ( concurrency::tiled_index< kernel2_WgSize > t_idx ) restrict(amp)
		  {

			int gloId = t_idx.global[ 0 ] + index;
			unsigned int groId = t_idx.tile[ 0 ] + tile_index;
			unsigned int locId = t_idx.local[ 0 ];
			unsigned int wgSize = kernel2_WgSize;

			tile_static kType ldsKeys[ SCANBYKEY_WAVESIZE*SCANBYKEY_KERNELWAVES ];
			tile_static vType ldsVals[ SCANBYKEY_WAVESIZE*SCANBYKEY_KERNELWAVES ];
	
				 // if exclusive, load gloId=0 w/ init, and all others shifted-1
			kType key;
			oType val;
			if (gloId < numElements){
			   if (exclusive)
			   {
				  if (gloId > 0)
				  { // thread>0
					  key = firstKey[ gloId];
					  kType key1 = firstKey[ gloId];
					  kType key2 = firstKey[ gloId-1];
					  if( binary_pred( key1, key2 )  )
						  val = firstValue[ gloId-1 ];
					  else 
						  val = init;
					  ldsKeys[ locId ] = key;
					  ldsVals[ locId ] = val;
				  }
				  else
				  { // thread=0
					  val = init;
					  ldsVals[ locId ] = val;
					  ldsKeys[ locId ] = firstKey[gloId];
					// key stays null, this thread should never try to compare it's key
					// nor should any thread compare it's key to ldsKey[ 0 ]
					// I could put another key into lds just for whatevs
					// for now ignore this
				  }
			   }
			   else
			   {
				  key = firstKey[ gloId ];
				  val = firstValue[ gloId ];
				  ldsKeys[ locId ] = key;
				  ldsVals[ locId ] = val;
			   }
			 }
	
			 // Each work item writes out its calculated scan result, relative to the beginning
			// of each work group
			vType scanResult = ldsVals[ locId ];
			vType postBlockSum, newResult;
			vType y, y1, sum;
			kType key1, key2, key3, key4;
	
			if(locId == 0 && gloId < numElements)
			{
				if(groId > 0)
				{
				   key1 = firstKey[gloId];
				   key2 = firstKey[groId*wgSize -1 ];

				   if(groId % 2 == 0)
				   {
					  postBlockSum = preSumArray[ groId/2 -1 ];
				   }
				   else if(groId == 1)
				   {
					  postBlockSum = preSumArray1[0];
				   }
				   else
				   {
					  key3 = firstKey[groId*wgSize -1 ];
					  key4 = firstKey[(groId-1)*wgSize -1];
					  if(binary_pred(key3 ,key4))
					  {
						 y = preSumArray[ groId/2 -1 ];
						 y1 = preSumArray1[groId/2];
						 postBlockSum = binary_funct(y, y1);
					  }
					  else
					  {
						 postBlockSum = preSumArray1[groId/2];
					  }
				   }
				   if (!exclusive)
				   {
					  if(binary_pred( key1, key2))
					  {
						 newResult = binary_funct( scanResult, postBlockSum );
					  }
					  else
					  {
						 newResult = scanResult;
					  }
				   }
				   else
				   {
					  if(binary_pred( key1, key2)) 
						 newResult = postBlockSum;
					  else
						 newResult = init;  
				   }
				}
				else
				{
					 newResult = scanResult;
				}
				ldsVals[ locId ] = newResult;
			}
			
			// Computes a scan within a workgroup
			// updates vals in lds but not keys
			sum = ldsVals[ locId ];
			for( unsigned int offset = 1; offset < wgSize; offset *= 2 )
			{
				t_idx.barrier.wait();
				if (locId >= offset )
				{
					kType key2 = ldsKeys[ locId - offset];
					if( binary_pred( key, key2 )  )
					{
						oType y = ldsVals[ locId - offset];
						sum = binary_funct( sum, y );
					}
					else
						sum = ldsVals[ locId];
					
				}
				t_idx.barrier.wait();
				ldsVals[ locId ] = sum;
			}
			 t_idx.barrier.wait(); // needed for large data types
			//  Abort threads that are passed the end of the input vector
			if (gloId >= numElements) return; 
			result[ gloId ] = sum;
	
	
		  } );
		  tempBuffsize = tempBuffsize - max_ext;
	}
    PEEK_AT( output )
}   //end of scan_by_key_enqueue( )

    template<
		typename InputIterator1,
		typename InputIterator2,
		typename OutputIterator,
		typename T,
		typename BinaryPredicate,
		typename BinaryFunction >
		typename std::enable_if< std::is_same< typename std::iterator_traits< OutputIterator >::iterator_category ,
                                std::random_access_iterator_tag
                                >::value
                >::type
		scan_by_key(
		bolt::amp::control& ctl,
		const InputIterator1& first1,
		const InputIterator1& last1,
		const InputIterator2& first2,
		const OutputIterator& result,
		const T& init,
		const BinaryPredicate& binary_pred,
		const BinaryFunction& binary_funct,
		const bool& inclusive, 
		const std::string& user_code)
		{
				 typedef typename std::iterator_traits< OutputIterator >::value_type oType;	    
	    
				 int sz = static_cast< int >( std::distance( first1, last1 ) );
				
				 // Map the output iterator to a device_vector
                 device_vector< oType, concurrency::array_view > dvOutput( result, sz, true, ctl );
	             
		         // Use host pointers memory since these arrays are only read once - no benefit to copying.
                 // Map the input iterator to a device_vector
				 auto dvKeys_itr  = bolt::amp::create_mapped_iterator(typename bolt::amp::iterator_traits< InputIterator1 >::iterator_category( ), first1, sz, false, ctl );
				 auto dvValues_itr  = bolt::amp::create_mapped_iterator(typename bolt::amp::iterator_traits< InputIterator2 >::iterator_category( ), first2, sz, false, ctl );
                 amp::scan_by_key( ctl, dvKeys_itr , dvKeys_itr +sz, dvValues_itr, dvOutput.begin(), init, binary_pred, binary_funct, inclusive, user_code);
                 // This should immediately map/unmap the buffer

                 dvOutput.data( );
	    
				 return ; 
		}
		
} //end of namespace amp


template<
		typename InputIterator1,
		typename InputIterator2,
		typename OutputIterator,
		typename T,
		typename BinaryPredicate,
		typename BinaryFunction >
		
		typename std::enable_if< 
				   !(std::is_same< typename std::iterator_traits< OutputIterator>::iterator_category, 
								 std::input_iterator_tag 
							   >::value ||
				   std::is_same< typename std::iterator_traits< OutputIterator>::iterator_category, 
								 bolt::amp::fancy_iterator_tag >::value ),
				   OutputIterator
							   >::type
		scan_by_key(
		control& ctl,
		const InputIterator1& first1,
		const InputIterator1& last1,
		const InputIterator2& first2,
		const OutputIterator& result,
		const T& init,
		const BinaryPredicate& binary_pred,
		const BinaryFunction& binary_funct,
		const bool& inclusive, 
		const std::string& user_code)
		{
			
			typedef typename std::iterator_traits< InputIterator1 >::value_type kType;			
			typedef typename std::iterator_traits< InputIterator2 >::value_type iType;
			typedef typename std::iterator_traits< OutputIterator >::value_type oType;

			unsigned int numElements = static_cast< unsigned int >( std::distance( first1, last1 ) );
			if( numElements == 0 )
				return result;

			bolt::amp::control::e_RunMode runMode = ctl.getForceRunMode( );

			if( runMode == bolt::amp::control::Automatic )
			{
				runMode = ctl.getDefaultPathToRun();
			}
	    
			if( runMode == bolt::amp::control::SerialCpu )
			{
	    		serial::scan_by_key(ctl, first1, last1, first2, result, init, binary_pred, binary_funct, inclusive, user_code );
				return result + numElements;
			}
			else if( runMode == bolt::amp::control::MultiCoreCpu )
			{
				#ifdef ENABLE_TBB
	    			  btbb::scan_by_key(ctl, first1, last1, first2, result, init, binary_pred, binary_funct, inclusive, user_code );
				#else
					  throw std::runtime_error("The MultiCoreCpu version of scan_by_key is not enabled to be built! \n");
				#endif
	    
				return result + numElements;
	    
			}
			else
			{
	    		amp::scan_by_key(ctl, first1, last1, first2, result, init, binary_pred, binary_funct, inclusive, user_code );
			}
		    return result + numElements;
	
    }


    template<
	typename InputIterator1,
	typename InputIterator2,
	typename OutputIterator,
	typename T,
	typename BinaryPredicate,
	typename BinaryFunction >
	
	typename std::enable_if< 
              (std::is_same< typename std::iterator_traits< OutputIterator>::iterator_category, 
                            std::input_iterator_tag 
                          >::value ||
              std::is_same< typename std::iterator_traits< OutputIterator>::iterator_category, 
                            bolt::amp::fancy_iterator_tag >::value ),
              OutputIterator
                          >::type
	scan_by_key(
	control& ctl,
	const InputIterator1& first1,
	const InputIterator1& last1,
	const InputIterator2& first2,
	const OutputIterator& result,
	const T& init,
	const BinaryPredicate& binary_pred,
	const BinaryFunction& binary_funct,
	const bool& inclusive, 
	const std::string& user_code)
	{	
		//  TODO:  It should be possible to support non-random_access_iterator_tag iterators, if we copied the data
		//  to a temporary buffer.  Should we?
		static_assert( std::is_same< typename std::iterator_traits< OutputIterator>::iterator_category, 
										std::input_iterator_tag >::value , 
						"Output vector should be a mutable vector. It cannot be of the type input_iterator_tag" );
		static_assert( std::is_same< typename std::iterator_traits< OutputIterator>::iterator_category, 
										bolt::amp::fancy_iterator_tag >::value , 
						"Output vector should be a mutable vector. It cannot be of type fancy_iterator_tag" );
	
	}


    /*!   \}  */
} //namespace detail

/*********************************************************************************************************************
 * Inclusive Segmented Scan
 ********************************************************************************************************************/

template<
    typename InputIterator1,
    typename InputIterator2,
    typename OutputIterator,
    typename BinaryPredicate,
    typename BinaryFunction>
OutputIterator
inclusive_scan_by_key(
    InputIterator1  first1,
    InputIterator1  last1,
    InputIterator2  first2,
    OutputIterator  result,
    BinaryPredicate binary_pred,
    BinaryFunction  binary_funct,
    const std::string& user_code )
{
    using bolt::amp::inclusive_scan_by_key;
	return inclusive_scan_by_key(
           control::getDefault( ), 
		   first1, 
		   last1,  
		   first2,
		   result,
           binary_pred,
	       binary_funct,
		   user_code );
}

template<
    typename InputIterator1,
    typename InputIterator2,
    typename OutputIterator,
    typename BinaryPredicate>
OutputIterator
inclusive_scan_by_key(
    InputIterator1  first1,
    InputIterator1  last1,
    InputIterator2  first2,
    OutputIterator  result,
    BinaryPredicate binary_pred,
    const std::string& user_code )
{
    using bolt::amp::inclusive_scan_by_key;
	return inclusive_scan_by_key(
           control::getDefault( ), 
		   first1, 
		   last1,  
		   first2,
		   result,
           binary_pred,
		   user_code );
}

template<
    typename InputIterator1,
    typename InputIterator2,
    typename OutputIterator>
OutputIterator
inclusive_scan_by_key(
    InputIterator1  first1,
    InputIterator1  last1,
    InputIterator2  first2,
    OutputIterator  result,
    const std::string& user_code )
{
    using bolt::amp::inclusive_scan_by_key;
	return inclusive_scan_by_key(
           control::getDefault( ), 
		   first1, 
		   last1,  
		   first2,
		   result,
		   user_code );
}

///////////////////////////// CTRL ////////////////////////////////////////////

template<
    typename InputIterator1,
    typename InputIterator2,
    typename OutputIterator,
    typename BinaryPredicate,
    typename BinaryFunction>
OutputIterator
inclusive_scan_by_key(
    bolt::amp::control &ctl,
    InputIterator1  first1,
    InputIterator1  last1,
    InputIterator2  first2,
    OutputIterator  result,
    BinaryPredicate binary_pred,
    BinaryFunction  binary_funct,
    const std::string& user_code )
{
    typedef typename std::iterator_traits<OutputIterator>::value_type oType;
    oType init; memset(&init, 0, sizeof(oType) );
	using bolt::amp::detail::scan_by_key;
    return detail::scan_by_key(
        ctl,
        first1,
        last1,
        first2,
        result,
        init,
        binary_pred,
        binary_funct,
        true, // inclusive
        user_code); 
}


template<
    typename InputIterator1,
    typename InputIterator2,
    typename OutputIterator,
    typename BinaryPredicate>
OutputIterator
inclusive_scan_by_key(
    bolt::amp::control &ctl,
    InputIterator1  first1,
    InputIterator1  last1,
    InputIterator2  first2,
    OutputIterator  result,
    BinaryPredicate binary_pred,
    const std::string& user_code )
{
    typedef typename std::iterator_traits<OutputIterator>::value_type oType;
    oType init; memset(&init, 0, sizeof(oType) );
    plus<oType> binary_funct;
	using bolt::amp::detail::scan_by_key;
    return detail::scan_by_key(
        ctl,
        first1,
        last1,
        first2,
        result,
        init,
        binary_pred,
        binary_funct,
        true, // inclusive
        user_code );
}

template<
    typename InputIterator1,
    typename InputIterator2,
    typename OutputIterator>
OutputIterator
inclusive_scan_by_key(
    bolt::amp::control &ctl,
    InputIterator1  first1,
    InputIterator1  last1,
    InputIterator2  first2,
    OutputIterator  result,
    const std::string& user_code )
{
    typedef typename std::iterator_traits<InputIterator1>::value_type kType;
    typedef typename std::iterator_traits<OutputIterator>::value_type oType;
    oType init; memset(&init, 0, sizeof(oType) );
    equal_to<kType> binary_pred;
    plus<oType> binary_funct;
	using bolt::amp::detail::scan_by_key;
    return detail::scan_by_key(
        ctl,
        first1,
        last1,
        first2,
        result,
        init,
        binary_pred,
        binary_funct,
        true, // inclusive
        user_code);
}




/*********************************************************************************************************************
 * Exclusive Segmented Scan
 ********************************************************************************************************************/

template<
    typename InputIterator1,
    typename InputIterator2,
    typename OutputIterator,
    typename T,
    typename BinaryPredicate,
    typename BinaryFunction>
OutputIterator
exclusive_scan_by_key(
    InputIterator1  first1,
    InputIterator1  last1,
    InputIterator2  first2,
    OutputIterator  result,
    T               init,
    BinaryPredicate binary_pred,
    BinaryFunction  binary_funct,
    const std::string& user_code )
{
    using bolt::amp::exclusive_scan_by_key;
	return exclusive_scan_by_key(
           control::getDefault( ), 
		   first1, 
		   last1,  
		   first2,
		   result,
		   init,
		   binary_pred,
		   binary_funct,
		   user_code );
}

template<
    typename InputIterator1,
    typename InputIterator2,
    typename OutputIterator,
    typename T,
    typename BinaryPredicate>
OutputIterator
exclusive_scan_by_key(
    InputIterator1  first1,
    InputIterator1  last1,
    InputIterator2  first2,
    OutputIterator  result,
    T               init,
    BinaryPredicate binary_pred,
    const std::string& user_code )
{
    using bolt::amp::exclusive_scan_by_key;
	return exclusive_scan_by_key(
           control::getDefault( ), 
		   first1, 
		   last1,  
		   first2,
		   result,
		   init,
		   binary_pred,
		   user_code );
}

template<
    typename InputIterator1,
    typename InputIterator2,
    typename OutputIterator,
    typename T>
OutputIterator
exclusive_scan_by_key(
    InputIterator1  first1,
    InputIterator1  last1,
    InputIterator2  first2,
    OutputIterator  result,
    T               init,
    const std::string& user_code )
{
    using bolt::amp::exclusive_scan_by_key;
	return exclusive_scan_by_key(
           control::getDefault( ), 
		   first1, 
		   last1,  
		   first2,
		   result,
		   init,
		   user_code );
}

template<
    typename InputIterator1,
    typename InputIterator2,
    typename OutputIterator>
OutputIterator
exclusive_scan_by_key(
    InputIterator1  first1,
    InputIterator1  last1,
    InputIterator2  first2,
    OutputIterator  result,
    const std::string& user_code )
{
    using bolt::amp::exclusive_scan_by_key;
	return exclusive_scan_by_key(
           control::getDefault( ), 
		   first1, 
		   last1,  
		   first2,
		   result,
		   user_code );
}

///////////////////////////// CTRL ////////////////////////////////////////////

template<
    typename InputIterator1,
    typename InputIterator2,
    typename OutputIterator,
    typename T,
    typename BinaryPredicate,
    typename BinaryFunction>
OutputIterator
exclusive_scan_by_key(
    bolt::amp::control &ctl,
    InputIterator1  first1,
    InputIterator1  last1,
    InputIterator2  first2,
    OutputIterator  result,
    T               init,
    BinaryPredicate binary_pred,
    BinaryFunction  binary_funct,
    const std::string& user_code )
{
    using bolt::amp::detail::scan_by_key;
    return detail::scan_by_key(
        ctl,
        first1,
        last1,
        first2,
        result,
        init,
        binary_pred,
        binary_funct,
        false, // exclusive
        user_code ); 
}

template<
    typename InputIterator1,
    typename InputIterator2,
    typename OutputIterator,
    typename T,
    typename BinaryPredicate>
OutputIterator
exclusive_scan_by_key(
    bolt::amp::control &ctl,
    InputIterator1  first1,
    InputIterator1  last1,
    InputIterator2  first2,
    OutputIterator  result,
    T               init,
    BinaryPredicate binary_pred,
    const std::string& user_code )
{
    typedef typename std::iterator_traits<OutputIterator>::value_type oType;
    plus<oType> binary_funct;
	using bolt::amp::detail::scan_by_key;
    return detail::scan_by_key(
        ctl,
        first1,
        last1,
        first2,
        result,
        init,
        binary_pred,
        binary_funct,
        false, // exclusive
        user_code ); 
}

template<
    typename InputIterator1,
    typename InputIterator2,
    typename OutputIterator,
    typename T>
OutputIterator
exclusive_scan_by_key(
    bolt::amp::control &ctl,
    InputIterator1  first1,
    InputIterator1  last1,
    InputIterator2  first2,
    OutputIterator  result,
    T               init,
    const std::string& user_code )
{
    typedef typename std::iterator_traits<InputIterator1>::value_type kType;
    typedef typename std::iterator_traits<OutputIterator>::value_type oType;
    equal_to<kType> binary_pred;
    plus<oType> binary_funct;
	using bolt::amp::detail::scan_by_key;
    return detail::scan_by_key(
        ctl,
        first1,
        last1,
        first2,
        result,
        init,
        binary_pred,
        binary_funct,
        false, // exclusive
        user_code); 
}

template<
    typename InputIterator1,
    typename InputIterator2,
    typename OutputIterator>
OutputIterator
exclusive_scan_by_key(
    bolt::amp::control &ctl,
    InputIterator1  first1,
    InputIterator1  last1,
    InputIterator2  first2,
    OutputIterator  result,
    const std::string& user_code )
{
    typedef typename std::iterator_traits<InputIterator1>::value_type kType;
    typedef typename std::iterator_traits<OutputIterator>::value_type oType;
	oType init; memset(&init, 0, sizeof(oType) );
    equal_to<kType> binary_pred;
    plus<oType> binary_funct;
	using bolt::amp::detail::scan_by_key;
    return detail::scan_by_key(
        ctl,
        first1,
        last1,
        first2,
        result,
        init,
        binary_pred,
        binary_funct,
        false, // exclusive
        user_code); 
}

   } //namespace amp
} //namespace bolt

#endif