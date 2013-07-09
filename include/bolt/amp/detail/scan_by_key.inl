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

#if !defined( BOLT_AMP_SCAN_BY_KEY_INL )
#define BOLT_AMP_SCAN_BY_KEY_INL

#pragma once

#include <vector>
#include <array>
#include <stdexcept>
#include <numeric>

#include <amp.h>

#include <bolt/AMP/functional.h>
#include <bolt/countof.h>
// #include <bolt/AMP/sequentialTrait.h>

namespace bolt {

	const int scanMultiCpuThreshold	= 4; // FIXME, artificially low to force use of GPU
	const int scanGpuThreshold		= 8; // FIXME, artificially low to force use of GPU
	const int maxThreadsInTile		= 1024;
	const int maxTilesPerDim		= 65535;
	const int maxTilesPerPFE		= maxThreadsInTile*maxTilesPerDim;

	//	Work routine for inclusive_scan that contains a compile time constant size
	template< typename InputIterator, typename OutputIterator, typename BinaryFunction >
	OutputIterator
		inclusive_scan( const concurrency::accelerator_view& av, InputIterator first, InputIterator last,
		OutputIterator result, BinaryFunction binary_op )
	{
//		typedef seqTrait< InputIterator > Trait;
		//typedef seqTrait< std::vector< int > > Trait;
		//if( !Trait::seqPointer )
		//{
		//	throw std::domain_error( "Scan requires iterators that guarantee values in sequential memory layout" );
		//}

		unsigned int numElements = static_cast< unsigned int >( std::distance( first, last ) );

		if( numElements < scanMultiCpuThreshold )
		{
			//	Serial CPU implementation
			return std::partial_sum( first, last, result, binary_op);
		}
		else if( numElements < scanGpuThreshold )
		{
			//	Implement this in TBB as tbb::parallel_scan( range, body )
			//	Does not appear to have an implementation in PPL
			//	TODO: Bring in the dependency to TBB and replace this STD call
			return std::partial_sum( first, last, result, binary_op);
		}
		else
		{
			// FIXME - determine from HSA Runtime
			// - based on est of how many threads needed to hide memory latency.
			static const unsigned int waveSize  = 64; // FIXME, read from device attributes.
			static_assert( (waveSize & (waveSize-1)) == 0, "Scan depends on wavefronts being a power of 2" );

			//	AMP code can not read size_t as input, need to cast to int
			//	Note: It would be nice to have 'constexpr' here, then we could use tileSize as the extent dimension
			unsigned int tileSize = std::min(  numElements, waveSize );

			//int computeUnits		= 10; // FIXME - determine from HSA Runtime
			//int wgPerComputeUnit	=  6;
			unsigned int sizeDeviceBuff = numElements;
			size_t modWaveFront = (numElements & (waveSize-1));
			if( modWaveFront )
			{
				sizeDeviceBuff &= ~modWaveFront;
				sizeDeviceBuff += waveSize;
			}
			unsigned int numWorkGroups = sizeDeviceBuff / waveSize;
			unsigned int sizeScanBuff = numWorkGroups;
			modWaveFront = (sizeScanBuff & (waveSize-1));
			if( modWaveFront )
			{
				sizeScanBuff &= ~modWaveFront;
				sizeScanBuff += waveSize;
			}

			//	Wrap our input data in an array_view, and mark it const so data is not read back from device
			typedef std::iterator_traits< InputIterator >::value_type iType;
			typedef std::iterator_traits< OutputIterator >::value_type oType;
			concurrency::array_view< const iType > hostInput( static_cast< int >( numElements ), &first[ 0 ] );

			//	Wrap our output data in an array_view, and discard input data so it is not transferred to device
			concurrency::array< iType > deviceInput( sizeDeviceBuff, av );
			hostInput.copy_to( deviceInput.section( concurrency::extent< 1 >( numElements ) ) );

			concurrency::array< oType > deviceOutput( sizeDeviceBuff, av );
			concurrency::array< oType > scanBuffer( sizeScanBuff, av );

			//	Loop to calculate the inclusive scan of each individual tile, and output the block sums of every tile
			//	This loop is inherently parallel; every tile is independant with potentially many wavefronts
			concurrency::parallel_for_each( av, deviceOutput.extent.tile< waveSize >(), [&deviceOutput, &deviceInput, &scanBuffer, tileSize, binary_op]( concurrency::tiled_index< waveSize > idx ) restrict(amp)
			{
				tile_static iType LDS[ waveSize + ( waveSize / 2 ) ];

				int localID		= idx.local[ 0 ];
				int globalID	= idx.global[ 0 ];

				//	Initialize the padding to 0, for when the scan algorithm looks left.
				//	Then bump the LDS pointer past the extra padding.
				LDS[ localID ] = 0;
				iType* pLDS = LDS + ( waveSize / 2 );

				iType val = deviceInput[ globalID ];
				pLDS[ localID ] = val;

				//	This loop essentially computes a scan within a tile, read from global memory.  No communication with other tiles yet.
				iType sum = val;
				for( unsigned int offset = 1; offset < tileSize; offset *= 2 )
				{
					iType y = pLDS[ localID - offset ];
					sum = binary_op( sum, y );
					pLDS[ localID ] = sum;
				}

				//	Write out the values of the per-tile scan
				deviceOutput[ globalID ] = sum;

				//	Take the very last thread in a tile, and save its value into a buffer for further processing
				if( localID == (waveSize-1) )
				{
					scanBuffer[ idx.tile[ 0 ] ] = pLDS[ localID ];
				}
			} );

			std::vector< oType > scanData( sizeScanBuff );
			scanData = scanBuffer;
			concurrency::array< oType > exclusiveBuffer( sizeScanBuff, av );

			//	Loop to calculate the exclusive scan of the block sums buffer
			//	This loop is inherently serial; we need to calculate the exclusive scan of a single 'array'
			//	This loop serves as a 'reduction' in spirit, and is calculated in a single wavefront
			//	NOTE: TODO:  On an APU, it might be more efficient to calculate this on CPU
			tileSize = static_cast< unsigned int >( std::min( numWorkGroups, waveSize ) );
			unsigned int workPerThread = sizeScanBuff / waveSize;
			concurrency::parallel_for_each( av, concurrency::extent<1>( waveSize ).tile< waveSize >(), [&scanBuffer, &exclusiveBuffer, tileSize, workPerThread, binary_op]( concurrency::tiled_index< waveSize > idx ) restrict(amp)
			{
				tile_static oType LDS[ waveSize + ( waveSize / 2 ) ];

				int localID		= idx.local[ 0 ];
				int globalID	= idx.global[ 0 ];
				int mappedID	= globalID * workPerThread;

				//	Initialize the padding to 0, for when the scan algorithm looks left.
				//	Then bump the LDS pointer past the extra padding.
				LDS[ localID ] = 0;
				oType* pLDS = LDS + ( waveSize / 2 );

				//	Begin the loop reduction
				oType workSum = 0;
				for( unsigned int offset = 0; offset < workPerThread; offset += 1 )
				{
					oType y = scanBuffer[ mappedID + offset ];
					workSum = binary_op( workSum, y );
					exclusiveBuffer[ mappedID + offset ] = workSum;
				}
				pLDS[ localID ] = workSum;

				//	This loop essentially computes an exclusive scan within a tile, writing 0 out for first element.
				oType scanSum = workSum;
				for( unsigned int offset = 1; offset < tileSize; offset *= 2 )
				{
					oType y = pLDS[ localID - offset ];
					scanSum = binary_op( scanSum, y );
					pLDS[ localID ] = scanSum;
				}

				idx.barrier.wait( );

				//	Write out the values of the per-tile scan
				scanSum -= workSum;
//				scanBuffer[ mappedID ] = scanSum;
				for( unsigned int offset = 0; offset < workPerThread; offset += 1 )
				{
					oType y = exclusiveBuffer[ mappedID + offset ];
					y = binary_op( y, scanSum );
					y -= scanBuffer[ mappedID + offset ];
					exclusiveBuffer[ mappedID + offset ] = y;
				}
			} );
			scanData = exclusiveBuffer;

			//	Loop through the entire output array and add the exclusive scan back into the output array
			concurrency::parallel_for_each( av, deviceOutput.extent.tile< waveSize >(), [&deviceOutput, &exclusiveBuffer, binary_op]( concurrency::tiled_index< waveSize > idx ) restrict(amp)
			{
				int globalID	= idx.global[ 0 ];
				int tileID		= idx.tile[ 0 ];

				//	Even though each wavefront threads access the same bank, it's the same location so there should not be bank conflicts
				oType val = exclusiveBuffer[ tileID ];

				//	Write out the values of the per-tile scan
				oType y = deviceOutput[ globalID ];
				deviceOutput[ globalID ] = binary_op( y, val );
			} );

			concurrency::array_view< oType > hostOutput( static_cast< int >( numElements ), &result[ 0 ] );
			hostOutput.discard_data( );

			deviceOutput.section( Concurrency::extent< 1 >( numElements ) ).copy_to( hostOutput );

		};

		return result + numElements;
	};

	/*
	* This version of inclusive_scan defaults to disallow the use of iterators, unless a specialization exists below
	*/
	template< typename InputIterator, typename OutputIterator, typename BinaryFunction >
	OutputIterator inclusive_scan( InputIterator begin, InputIterator end, OutputIterator result,
		BinaryFunction binary_op, std::input_iterator_tag )
	{
		return inclusive_scan( concurrency::accelerator().default_view, begin, end, result, binary_op);
	};

	/*
	* This version of inclusive_scan uses default accelerator
	*/
	template< typename InputIterator, typename OutputIterator, typename BinaryFunction >
	OutputIterator inclusive_scan( InputIterator first, InputIterator last, OutputIterator result, BinaryFunction binary_op )
	{

		return inclusive_scan( first, last, result, binary_op, std::iterator_traits< InputIterator >::iterator_category( ) );
	};

	/*
	* This version of inclusive_scan uses a default plus<> as default argument.
	*/
	template< typename InputIterator, typename OutputIterator >
	OutputIterator inclusive_scan( InputIterator first, InputIterator last, OutputIterator result )
	{
		typedef std::iterator_traits<InputIterator>::value_type T;

		return inclusive_scan( first, last, result, bolt::plus< T >( ), std::iterator_traits< InputIterator >::iterator_category( ) );
	};


	// still need more versions that take accelerator as first argument.


}; // end namespace bolt


#endif
