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
#define BOLT_DEBUG 0

template<typename DVRandomAccessIterator, typename StrictWeakOrdering>
void sort_enqueue(bolt::amp::control &ctl, const DVRandomAccessIterator& first, const DVRandomAccessIterator& last,
const StrictWeakOrdering& comp)
{
	unsigned int numStages,stage,passOfStage;
    size_t  temp;
	typedef typename std::iterator_traits< DVRandomAccessIterator >::value_type T;
    concurrency::accelerator_view av = ctl.getAccelerator().default_view;
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
    
    concurrency::extent< 1 > globalSizeK0( szElements/2 );
    concurrency::tiled_extent< WGSIZE > tileK0 = globalSizeK0.tile< WGSIZE >();

	for(stage = 0; stage < numStages; ++stage)
	{
		for(passOfStage = 0; passOfStage < stage + 1; ++passOfStage) 
		{
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
