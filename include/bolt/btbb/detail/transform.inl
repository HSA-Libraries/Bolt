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

/*! \file bolt/amp/transform.h
	\brief  Applies a specific function object to each element pair in the specified input ranges.
*/
#include "tbb/task_scheduler_init.h"

#pragma once
#if !defined( BOLT_BTBB_TRANSFORM_INL )
#define BOLT_BTBB_TRANSFORM_INL

namespace bolt
{
	namespace btbb
	{

	template< typename tbbInputIterator1, typename tbbInputIterator2, typename tbbOutputIterator, typename tbbFunctor >
	struct transformBinaryRange
	{
		tbbInputIterator1 first1, last1;
		tbbInputIterator2 first2;
		tbbOutputIterator result;
		tbbFunctor func;
		static const size_t divSize = 1024;
		typedef typename std::iterator_traits< tbbInputIterator1 >::value_type T_input1;
		typedef typename std::iterator_traits< tbbInputIterator2 >::value_type T_input2;
		typedef typename std::iterator_traits< tbbOutputIterator >::value_type T_output;
		bool empty( ) const
		{
			return (std::distance( first1, last1 ) == 0);
		}

		bool is_divisible( ) const
		{
			return (std::distance( first1, last1 ) > divSize);
		}

		transformBinaryRange( tbbInputIterator1 begin1, tbbInputIterator1 end1, tbbInputIterator2 begin2,
			tbbOutputIterator out, tbbFunctor func1 ):
			first1( begin1 ), last1( end1 ),
			first2( begin2 ), result( out ), func( func1 )
		{}

		transformBinaryRange( transformBinaryRange& r, tbb::split ): first1( r.first1 ), last1( r.last1 ), first2( r.first2 ),
			result( r.result ), func( r.func )
		{
			int halfSize = static_cast<int>(std::distance( r.first1, r.last1 ) >> 1);
			r.last1 = r.first1 + halfSize;

			first1 = r.last1;
			first2 = r.first2 + halfSize;
			result = r.result + halfSize;
		}
	};

	template< typename tbbInputIterator1, typename tbbOutputIterator, typename tbbFunctor >
	struct transformUnaryRange
	{
		tbbInputIterator1 first1, last1;
		tbbOutputIterator result;
		tbbFunctor func;
		static const size_t divSize = 1024;

		bool empty( ) const
		{
			return (std::distance( first1, last1 ) == 0);
		}

		bool is_divisible( ) const
		{
			return (std::distance( first1, last1 ) > divSize);
		}

		transformUnaryRange( tbbInputIterator1 begin1, tbbInputIterator1 end1, tbbOutputIterator out, tbbFunctor func1 ):
			first1( begin1 ), last1( end1 ), result( out ), func( func1 )
		{}

		transformUnaryRange( transformUnaryRange& r, tbb::split ): first1( r.first1 ), last1( r.last1 ),
			 result( r.result ), func( r.func )
		{
			int halfSize = static_cast<int>(std::distance( r.first1, r.last1 ) >> 1);
			r.last1 = r.first1 + halfSize;

			first1 = r.last1;
			result = r.result + halfSize;
		}
	};

	template< typename tbbInputIterator1, typename tbbInputIterator2, typename tbbOutputIterator, typename tbbFunctor >
	struct transformBinaryRangeBody
	{
		void operator( )( transformBinaryRange< tbbInputIterator1, tbbInputIterator2, tbbOutputIterator, tbbFunctor >& r ) const
		{
			//size_t sz = std::distance( r.first1, r.last1 );
            size_t sz = (r.last1 - r.first1);
            if (sz == 0)
                return;
            //std::transform( first, last, result, f );
            for(int index=0; index < (int)(sz); index++)
            {
                *(r.result + index) = r.func( *(r.first1+index), *(r.first2+index) );
            }
		}
	};

	template< typename tbbInputIterator1, typename tbbOutputIterator, typename tbbFunctor >
	struct transformUnaryRangeBody
	{
		void operator( )( transformUnaryRange< tbbInputIterator1, tbbOutputIterator, tbbFunctor >& r ) const
		{
			size_t sz = std::distance( r.first1, r.last1 );
            for(int index=0; index < (int)(sz); index++)
            {
                *(r.result + index) = r.func( *(r.first1+index) );
            }

		}
	};


	template<typename InputIterator1, typename InputIterator2, typename Stencil, typename OutputIterator, typename BinaryFunction, typename Predicate>
     struct Transform_If {

               Transform_If  () {}

				void operator()( InputIterator1 first, InputIterator1 last, InputIterator2 first2, Stencil s, OutputIterator result, BinaryFunction op, Predicate p)
                {
					typename std::iterator_traits<InputIterator1>::difference_type n = (last - first);

                    tbb::parallel_for(  tbb::blocked_range<int>(0, (int) n) ,
                        [&] (const tbb::blocked_range<int> &r) -> void
                        {
                              
                              for(int i = r.begin(); i!=r.end(); i++)
                              {
                                 
								  if(p(s[i]))
                                   *(result+i) = op(*(first+i), *(first2+i));
								  else
									*(result+i) = *(first+i);
                              }   
                          
                        });

                }


     };


		template<typename InputIterator, typename OutputIterator, typename UnaryFunction>
		void transform(InputIterator first,
					   InputIterator last,
					   OutputIterator result,
					   UnaryFunction op)
		{

			tbb::parallel_for(
				transformUnaryRange< InputIterator, OutputIterator, UnaryFunction >( first, last, result, op ),
				transformUnaryRangeBody< InputIterator, OutputIterator, UnaryFunction >( ),
				tbb::simple_partitioner( ) );

		}

		template<typename InputIterator1, typename InputIterator2, typename OutputIterator, typename BinaryFunction>
		void transform(InputIterator1 first1,
					   InputIterator1 last1,
					   InputIterator2 first2,
					   OutputIterator result,
					   BinaryFunction op)
		{
				tbb::parallel_for(
					transformBinaryRange< InputIterator1, InputIterator2, OutputIterator, BinaryFunction >(
						first1, last1, first2, result, op ),
					transformBinaryRangeBody< InputIterator1, InputIterator2, OutputIterator, BinaryFunction >( ),
					tbb::simple_partitioner( ) );

		}


		template<typename InputIterator1, typename InputIterator2, typename Stencil, typename OutputIterator, typename BinaryFunction, typename Predicate>
		void transform_if(InputIterator1 first1, InputIterator1 last1,
                        InputIterator2 first2,  Stencil& s, OutputIterator result, BinaryFunction f, Predicate p)
		{

			   //Gets the number of concurrent threads supported by the underlying platform
               //unsigned int concurentThreadsSupported = std::thread::hardware_concurrency();

               //This allows TBB to choose the number of threads to spawn.
               tbb::task_scheduler_init initialize(tbb::task_scheduler_init::automatic);

               //Explicitly setting the number of threads to spawn
               //tbb::task_scheduler_init((int) concurentThreadsSupported);

               Transform_If <InputIterator1, InputIterator2, Stencil, OutputIterator, BinaryFunction, Predicate> transform_if;
               transform_if(first1, last1, first2, s, result, f, p);
		}



	}
}
#endif // TBB_TRANSFORM_INL


