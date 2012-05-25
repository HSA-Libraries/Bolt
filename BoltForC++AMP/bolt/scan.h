#pragma once

#include <iostream>  // FIXME, remove as this is only here for debug output
#include <vector>
#include <array>
#include <amp.h>
#include <bolt/functional.h>

namespace bolt {

	const int reduceMultiCpuThreshold	= 2; // FIXME, artificially low to force use of GPU
	const int reduceGpuThreshold		= 4; // FIXME, artificially low to force use of GPU

	template<typename InputIterator, typename OutputIterator, typename BinaryFunction> 
	OutputIterator inclusive_scan( concurrency::accelerator_view av, InputIterator first, InputIterator last, 
		OutputIterator result, BinaryFunction binary_op )
	{
		auto sz = std::distance( first, last );

		if( sz < reduceMultiCpuThreshold )
		{
			//	Serial CPU implementation
			return std::partial_sum( first, last, result, binary_op);
		} 
		else if( sz < reduceGpuThreshold )
		{
			//	Multi-core CPU implementation:
			typedef std::iterator_traits< InputIterator >::value_type T;
			concurrency::combinable< T > results1;

			//	This is implemented in TBB as tbb::parallel_scan( range, body )
			//	Does not appear to have an implementation in PPL

			return result;
		}
		else
		{
			int computeUnits     = 10; // FIXME - determine from HSA Runtime

			// FIXME - determine from HSA Runtime 
			// - based on est of how many threads needed to hide memory latency.
			int wgPerComputeUnit =  6; 
			int resultCnt = computeUnits * wgPerComputeUnit;
			static const int waveSize  = 64; // FIXME, read from device attributes.


			typedef std::iterator_traits< InputIterator >::value_type T;

			return last;
		};
	};

	/*
	* This version of inclusive_scan defaults to disallow the use of iterators, unless a specialization exists below
	*/
	template< typename InputIterator, typename OutputIterator, typename BinaryFunction >
	OutputIterator inclusive_scan( InputIterator begin, InputIterator end, OutputIterator result,
		BinaryFunction binary_op, std::input_iterator_tag )
	{
		return std::iterator_traits< OutputIterator >::value_type( );
	};

	/*
	* Partial specialization of inclusive_scan which allows the use of naked pointer types
	*/
	template< typename T, typename BinaryFunction >
	T* inclusive_scan( T* begin, typename T* end, 
		BinaryFunction binary_op, std::random_access_iterator_tag )
	{
		return inclusive_scan( concurrency::accelerator().default_view, begin, end, binary_op);
	};

	/*
	* Partial specialization of inclusive_scan which allows the use of constant naked pointer types
	*/
	template< typename T, typename BinaryFunction >
	const T* inclusive_scan( const T* begin, const T* end, 
		BinaryFunction binary_op, std::random_access_iterator_tag )
	{
		return inclusive_scan( concurrency::accelerator().default_view, begin, end, binary_op);
	};

	/*
	* Partial specialization of inclusive_scan which allows the use of std::vector< T >::iterator types
	*/
	template< typename T, typename BinaryFunction >
	typename std::vector< T >::iterator
		inclusive_scan(typename std::vector< T >::iterator begin, typename std::vector< T >::iterator end, 
		BinaryFunction binary_op, std::random_access_iterator_tag )
	{
		return inclusive_scan( concurrency::accelerator().default_view, begin, end, binary_op );
	};

	/*
	* Partial specialization of inclusive_scan which allows the use of std::vector< T >::const_iterator types
	*/
	template< typename T, typename BinaryFunction >
	typename std::vector< T >::const_iterator
		inclusive_scan(typename std::vector< T >::const_iterator begin, typename std::vector< T >::const_iterator end, 
		BinaryFunction binary_op, std::random_access_iterator_tag )
	{
		return inclusive_scan( concurrency::accelerator().default_view, begin, end, binary_op );
	};

	/*
	* Partial specialization of inclusive_scan which disallows the use of std::vector< bool >::iterator types
	*/
	template< typename BinaryFunction >
	typename std::vector< bool >::iterator
		inclusive_scan(typename std::vector< bool >::iterator begin, typename std::vector< bool >::iterator end, 
		BinaryFunction binary_op, std::random_access_iterator_tag )
	{
		return std::iterator_traits<typename std::vector< bool >::iterator >::value_type( );
	};

	/*
	* Partial specialization of inclusive_scan which disallows the use of std::vector< bool >::const_iterator types
	*/
	template< typename BinaryFunction >
	typename std::vector< bool >::const_iterator
		inclusive_scan(typename std::vector< bool >::const_iterator begin, typename std::vector< bool >::const_iterator end, 
		BinaryFunction binary_op, std::random_access_iterator_tag )
	{
		return std::iterator_traits< typename std::vector< bool >::const_iterator >::value_type( );
	};

	/*
	* Partial specialization of inclusive_scan which allows the use of std::array< T, N >::iterator types
	*/
	template< typename I, typename O, size_t N, typename BinaryFunction >
	typename std::_Array_iterator< O, N >
		inclusive_scan(typename std::_Array_iterator< I, N > first, typename std::_Array_iterator< I, N > last, typename std::_Array_iterator< O, N > result,
		BinaryFunction binary_op, std::random_access_iterator_tag )
	{
		return inclusive_scan( concurrency::accelerator().default_view, first, last, result, binary_op );
	};

	/*
	* Partial specialization of inclusive_scan which allows the use of std::array< T, N >::const_iterator types
	*/
	template< typename I, typename O, size_t N, typename BinaryFunction >
	typename std::_Array_const_iterator< O, N >
		inclusive_scan(typename std::_Array_const_iterator< I, N > first, typename std::_Array_const_iterator< I, N > last, typename std::_Array_const_iterator< O, N > result,
		BinaryFunction binary_op, std::random_access_iterator_tag )
	{
		return inclusive_scan( concurrency::accelerator().default_view, first, last, result, binary_op );
	};

	/*
	* This version of inclusive_scan uses default accelerator
	*/
	template< typename InputIterator, typename OutputIterator, typename BinaryFunction > 
	OutputIterator inclusive_scan(InputIterator first, InputIterator last, OutputIterator result, BinaryFunction binary_op )
	{

		return inclusive_scan( first, last, result, binary_op, std::iterator_traits< InputIterator >::iterator_category( ) );
	};

	/*
	* This version of inclusive_scan uses a default init value of 0 and plus<> as default argument.
	*/
	template< typename InputIterator, typename OutputIterator >
	OutputIterator inclusive_scan( InputIterator first, InputIterator last, OutputIterator result )
	{
		typedef std::iterator_traits<InputIterator>::value_type T;

		return inclusive_scan( first, last, result, bolt::plus< T >( ), std::iterator_traits< InputIterator >::iterator_category( ) );
	};


	// still need more versions that take accelerator as first argument.


}; // end namespace bolt
