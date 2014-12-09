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

///////////////////////////////////////////////////////////////////////////////
// AMP Transform
//////////////////////////////////////////////////////////////////////////////

#pragma once
#if !defined( BOLT_AMP_TRANSFORM_INL )
#define BOLT_AMP_TRANSFORM_INL
#define TRANSFORM_WAVEFRNT_SIZE 256

#ifdef BOLT_ENABLE_PROFILING
#include "bolt/AsyncProfiler.h"
//AsyncProfiler aProfiler("transform");
#endif

#include <algorithm>
#include <type_traits>
#include "bolt/amp/bolt.h"
#include "bolt/amp/device_vector.h"
#include "bolt/amp/iterator/iterator_traits.h"
#include "bolt/amp/iterator/addressof.h"

#ifdef ENABLE_TBB
    #include "bolt/btbb/transform.h"
#endif


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace bolt
{
namespace amp
{
namespace detail
{
            
namespace serial{



	template<typename InputIterator1, typename InputIterator2, typename Stencil, typename OutputIterator, typename BinaryFunction, typename Predicate>
    typename std::enable_if< 
               std::is_same< typename std::iterator_traits< OutputIterator >::iterator_category ,
                                       bolt::amp::device_vector_tag
                           >::value
                           >::type
    binary_transform_if( bolt::amp::control &ctl,  InputIterator1& first1,  InputIterator1& last1,
                         InputIterator2& first2,  Stencil& s,  OutputIterator& result,  BinaryFunction& f,  Predicate &p)
    {
            typename std::iterator_traits<InputIterator1>::difference_type sz = (last1 - first1);
            if (sz == 0)
                return;

		
            auto mapped_first1_itr = create_mapped_iterator(typename std::iterator_traits<InputIterator1>::iterator_category(), 
                                                           ctl, first1);
            auto mapped_first2_itr = create_mapped_iterator(typename std::iterator_traits<InputIterator2>::iterator_category(), 
                                                           ctl, first2);
			auto mapped_first3_itr = create_mapped_iterator(typename std::iterator_traits<Stencil>::iterator_category(), 
                                                           ctl, s);
            auto mapped_result_itr = create_mapped_iterator(typename std::iterator_traits<OutputIterator>::iterator_category(), 
                                                           ctl, result);

			
            for(int index=0; index < (int)(sz); index++)
            {
				if(p(mapped_first3_itr[index]))
                   *(mapped_result_itr + index) = f( *(mapped_first1_itr+index), *(mapped_first2_itr+index) );
				else
				   *(mapped_result_itr + index) = *(mapped_first1_itr+index);
            }
          
            return;
    }


	template<typename InputIterator1, typename InputIterator2, typename Stencil, typename OutputIterator, typename BinaryFunction, typename Predicate>
    typename std::enable_if< 
               std::is_same< typename std::iterator_traits< OutputIterator >::iterator_category ,
                                       std::random_access_iterator_tag
                           >::value
                           >::type
    binary_transform_if( bolt::amp::control &ctl,  InputIterator1& first1,  InputIterator1& last1,
                         InputIterator2& first2,   Stencil& s,  OutputIterator& result,  BinaryFunction& f,  Predicate &p)
    {
        size_t sz = (last1 - first1);
        if (sz == 0)
            return;
        for(int index=0; index < (int)(sz); index++)
        {
			if(p(s[index]))
             *(result + index) = f( *(first1+index), *(first2+index) );
			else
              *(result + index) = *(first1+index);
        }
    }



    template<typename InputIterator1, typename InputIterator2, typename OutputIterator, typename BinaryFunction>
    typename std::enable_if< 
               std::is_same< typename std::iterator_traits< OutputIterator >::iterator_category ,
                                       bolt::amp::device_vector_tag
                           >::value
                           >::type
    binary_transform( bolt::amp::control &ctl,  InputIterator1& first1,  InputIterator1& last1,
                         InputIterator2& first2,  OutputIterator& result,  BinaryFunction& f)
    {
            typename std::iterator_traits<InputIterator1>::difference_type sz = (last1 - first1);
            if (sz == 0)
                return;

		
            auto mapped_first1_itr = create_mapped_iterator(typename std::iterator_traits<InputIterator1>::iterator_category(), 
                                                           ctl, first1);
            auto mapped_first2_itr = create_mapped_iterator(typename std::iterator_traits<InputIterator2>::iterator_category(), 
                                                           ctl, first2);
            auto mapped_result_itr = create_mapped_iterator(typename std::iterator_traits<OutputIterator>::iterator_category(), 
                                                           ctl, result);

			
            for(int index=0; index < (int)(sz); index++)
            {
                *(mapped_result_itr + index) = f( *(mapped_first1_itr+index), *(mapped_first2_itr+index) );
            }
          
            return;
    }


	template<typename InputIterator1, typename InputIterator2, typename OutputIterator, typename BinaryFunction>
    typename std::enable_if< 
               std::is_same< typename std::iterator_traits< OutputIterator >::iterator_category ,
                                       std::random_access_iterator_tag
                           >::value
                           >::type
    binary_transform( bolt::amp::control &ctl,  InputIterator1& first1,  InputIterator1& last1,
                         InputIterator2& first2,  OutputIterator& result,  BinaryFunction& f)
    {
        size_t sz = (last1 - first1);
        if (sz == 0)
            return;
        for(int index=0; index < (int)(sz); index++)
        {
            *(result + index) = f( *(first1+index), *(first2+index) );
        }
    }



    template<typename Iterator, typename OutputIterator, typename UnaryFunction>
	typename std::enable_if< 
               std::is_same< typename std::iterator_traits< OutputIterator >::iterator_category ,
                                       std::random_access_iterator_tag
                           >::value
                           >::type
    unary_transform( bolt::amp::control &ctl, Iterator& first, Iterator& last,
                    OutputIterator& result, UnaryFunction& f )
    {
        size_t sz = (last - first);
        if (sz == 0)
            return;
        for(int index=0; index < (int)(sz); index++)
        {
            *(result + index) = f( *(first+index) );
        }

        return;
    }

	template<typename Iterator, typename OutputIterator, typename UnaryFunction>
	typename std::enable_if< 
               std::is_same< typename std::iterator_traits< OutputIterator >::iterator_category ,
                                       bolt::amp::device_vector_tag
                           >::value
                           >::type
    unary_transform( bolt::amp::control &ctl, Iterator& first, Iterator& last,
                    OutputIterator& result, UnaryFunction& f )
    {
        size_t sz = (last - first);
        if (sz == 0)
            return;
     
		auto mapped_first_itr = create_mapped_iterator(typename std::iterator_traits<Iterator>::iterator_category(), 
                                                        ctl, first);
        auto mapped_result_itr = create_mapped_iterator(typename std::iterator_traits<OutputIterator>::iterator_category() , 
                                                        ctl, result);


        for(int index=0; index < (int)(sz); index++)
        {
            *(mapped_result_itr + index) = f( *(mapped_first_itr+index) );
        }
       
        return ;
    }


}//end of namespace serial

#ifdef ENABLE_TBB
namespace btbb{


	
	template<typename InputIterator1, typename InputIterator2, typename Stencil, typename OutputIterator, typename BinaryFunction, typename Predicate>
    typename std::enable_if< 
               std::is_same< typename std::iterator_traits< OutputIterator >::iterator_category ,
                                       bolt::amp::device_vector_tag
                           >::value
                           >::type
    binary_transform_if( bolt::amp::control &ctl,  InputIterator1& first1,  InputIterator1& last1,
                         InputIterator2& first2,  Stencil& s,  OutputIterator& result,  BinaryFunction& f,  Predicate &p)
    {
            typename std::iterator_traits<InputIterator1>::difference_type sz = (last1 - first1);
            if (sz == 0)
                return;

		
            auto mapped_first1_itr = create_mapped_iterator(typename std::iterator_traits<InputIterator1>::iterator_category(), 
                                                           ctl, first1);
            auto mapped_first2_itr = create_mapped_iterator(typename std::iterator_traits<InputIterator2>::iterator_category(), 
                                                           ctl, first2);
			auto mapped_first3_itr = create_mapped_iterator(typename std::iterator_traits<Stencil>::iterator_category(), 
                                                           ctl, s);
            auto mapped_result_itr = create_mapped_iterator(typename std::iterator_traits<OutputIterator>::iterator_category(), 
                                                           ctl, result);

			
            bolt::btbb::transform_if(mapped_first1_itr, mapped_first1_itr+(int)sz, mapped_first2_itr,mapped_first3_itr, mapped_result_itr, f, p);
          
            return;
    }


	template<typename InputIterator1, typename InputIterator2, typename Stencil, typename OutputIterator, typename BinaryFunction, typename Predicate>
    typename std::enable_if< 
               std::is_same< typename std::iterator_traits< OutputIterator >::iterator_category ,
                                       std::random_access_iterator_tag
                           >::value
                           >::type
    binary_transform_if( bolt::amp::control &ctl,  InputIterator1& first1,  InputIterator1& last1,
                         InputIterator2& first2,   Stencil& s,  OutputIterator& result,  BinaryFunction& f,  Predicate &p)
    {
        bolt::btbb::transform_if(first1, last1, first2, s, result, f, p);
    }



    template<typename InputIterator1, typename InputIterator2, typename OutputIterator, typename BinaryFunction>
	typename std::enable_if< std::is_same< typename std::iterator_traits< OutputIterator >::iterator_category ,
                                       bolt::amp::device_vector_tag
                                     >::value
                       >::type
    binary_transform( bolt::amp::control &ctl,  InputIterator1& first1,  InputIterator1& last1,
                         InputIterator2& first2,  OutputIterator& result,  BinaryFunction& f)
    {
        typename std::iterator_traits<InputIterator1>::difference_type sz = (last1 - first1);
        if (sz == 0)
            return;

        auto mapped_first1_itr = create_mapped_iterator(typename std::iterator_traits<InputIterator1>::iterator_category(), 
                                                           ctl, first1);
        auto mapped_first2_itr = create_mapped_iterator(typename std::iterator_traits<InputIterator2>::iterator_category(), 
                                                           ctl, first2);
        auto mapped_result_itr = create_mapped_iterator(typename std::iterator_traits<OutputIterator>::iterator_category(), 
                                                           ctl, result);


        bolt::btbb::transform(mapped_first1_itr, mapped_first1_itr+(int)sz, mapped_first2_itr, mapped_result_itr, f);

        return;
    }

	template<typename InputIterator1, typename InputIterator2, typename OutputIterator, typename BinaryFunction>
	typename std::enable_if< std::is_same< typename std::iterator_traits< OutputIterator >::iterator_category ,
                                       std::random_access_iterator_tag
                                     >::value
                       >::type
    binary_transform( bolt::amp::control &ctl,  InputIterator1& first1,  InputIterator1& last1,
                         InputIterator2& first2,  OutputIterator& result,  BinaryFunction& f)
    {
        bolt::btbb::transform(first1, last1, first2, result, f);
    }


    template<typename Iterator, typename OutputIterator, typename UnaryFunction>
	typename std::enable_if< std::is_same< typename std::iterator_traits< OutputIterator >::iterator_category ,
                                        bolt::amp::device_vector_tag
                                     >::value
                       >::type
    unary_transform( bolt::amp::control &ctl, Iterator& first, Iterator& last,
                    OutputIterator& result, UnaryFunction& f )
    {
        typename std::iterator_traits<Iterator>::difference_type sz = (last - first);
        if (sz == 0)
            return;


		auto mapped_first_itr = create_mapped_iterator(typename std::iterator_traits<Iterator>::iterator_category(), 
                                                        ctl, first);
        auto mapped_result_itr = create_mapped_iterator(typename std::iterator_traits<OutputIterator>::iterator_category(), 
                                                        ctl, result);


        bolt::btbb::transform(mapped_first_itr, mapped_first_itr+(int)sz,  mapped_result_itr, f);

        return;
    }

	template<typename Iterator, typename OutputIterator, typename UnaryFunction>
	typename std::enable_if< std::is_same< typename std::iterator_traits< OutputIterator >::iterator_category ,
                                       std::random_access_iterator_tag
                                     >::value
                       >::type
    unary_transform( bolt::amp::control &ctl, Iterator& first, Iterator& last,
                    OutputIterator& result, UnaryFunction& f )
    {
        bolt::btbb::transform(first, last, result, f);
    }

}//end of namespace btbb
#endif

namespace amp{

    /*! \brief This template function overload is used strictly for device_vector and AMP implementations. 
        \detail 
    */
    template<typename DVInputIterator1, typename DVInputIterator2, typename DVOutputIterator, typename BinaryFunction>
    typename std::enable_if< 
               std::is_same< typename std::iterator_traits< DVOutputIterator >::iterator_category ,
                                       bolt::amp::device_vector_tag
                           >::value
                           >::type
    binary_transform( ::bolt::amp::control &ctl,  DVInputIterator1 first1,  DVInputIterator1 last1,
                       DVInputIterator2 first2,  DVOutputIterator& result,  BinaryFunction& f)
           
    {
        concurrency::accelerator_view av = ctl.getAccelerator().default_view;

        typedef std::iterator_traits< DVInputIterator1 >::value_type iType1;
        typedef std::iterator_traits< DVInputIterator2 >::value_type iType2;
        typedef std::iterator_traits< DVOutputIterator >::value_type oType;

        const int szElements =  static_cast< int >( std::distance( first1, last1 ) );

        const unsigned int leng =  szElements + TRANSFORM_WAVEFRNT_SIZE - (szElements % TRANSFORM_WAVEFRNT_SIZE);

        concurrency::extent< 1 > inputExtent(leng);



		auto dvInput1_itr  = bolt::amp::create_mapped_iterator(typename bolt::amp::iterator_traits< DVInputIterator1 >::iterator_category( ), first1, szElements, false, ctl );
        auto dvInput2_itr  = bolt::amp::create_mapped_iterator(typename bolt::amp::iterator_traits< DVInputIterator2 >::iterator_category( ), first2, szElements, false, ctl);

        try
        {

            concurrency::parallel_for_each(av,  inputExtent, [=](concurrency::index<1> idx) restrict(amp)
            {
                int globalId = idx[ 0 ];

                if( globalId >= szElements)
                return;

                //result[globalId] = f(first1[globalId],first2[globalId]);
				result[globalId] = f(dvInput1_itr[globalId], dvInput2_itr[globalId]);
            });
        }

		catch(std::exception &e)
        {

              std::cout << "Exception while calling bolt::amp::transform parallel_for_each"<<e.what()<<std::endl;

              return;
        }
    }




	/*! \brief This template function overload is used strictly std random access vectors and AMP implementations. 
        \detail 
    */
    template<typename InputIterator1, typename InputIterator2, typename OutputIterator, typename BinaryFunction>
    typename std::enable_if< std::is_same< typename std::iterator_traits< OutputIterator >::iterator_category ,
                                       std::random_access_iterator_tag
                                     >::value
                           >::type
    binary_transform( ::bolt::amp::control &ctl,  InputIterator1& first1,  InputIterator1& last1,
                       InputIterator2& first2,  OutputIterator& result,  BinaryFunction& f)
    {

        int sz = static_cast<int>(last1 - first1);

        typedef typename std::iterator_traits<OutputIterator>::value_type  oType;

		// Use host pointers memory since these arrays are only read once - no benefit to copying.
        // Map the input iterator to a device_vector

		auto dvInput1_itr  = bolt::amp::create_mapped_iterator(typename bolt::amp::iterator_traits< InputIterator1 >::iterator_category( ), first1, sz, false, ctl );
        auto dvInput2_itr  = bolt::amp::create_mapped_iterator(typename bolt::amp::iterator_traits< InputIterator2 >::iterator_category( ), first2, sz, false, ctl);
     

        // Map the output iterator to a device_vector
        device_vector< oType, concurrency::array_view > dvOutput( result, sz, true, ctl );

		amp::binary_transform( ctl, dvInput1_itr, dvInput1_itr+sz, dvInput2_itr, dvOutput.begin(), f );

        // This should immediately map/unmap the buffer
        dvOutput.data( );
		return;

    }


	template<typename DVInputIterator, typename DVOutputIterator, typename UnaryFunction>
    typename std::enable_if< std::is_same< typename std::iterator_traits< DVOutputIterator >::iterator_category ,
                                       bolt::amp::device_vector_tag
                                     >::value
                       >::type
    unary_transform( ::bolt::amp::control &ctl,  DVInputIterator first,  DVInputIterator last,
     DVOutputIterator& result,  UnaryFunction& f)
    {
          
        typedef std::iterator_traits< DVInputIterator >::value_type iType;
        typedef std::iterator_traits< DVOutputIterator >::value_type oType;


        const int szElements =  static_cast< int >( std::distance( first, last ) );
        concurrency::accelerator_view av = ctl.getAccelerator().default_view;

        const unsigned int leng =  szElements + TRANSFORM_WAVEFRNT_SIZE - (szElements % TRANSFORM_WAVEFRNT_SIZE);

        concurrency::extent< 1 > inputExtent(leng);


		auto dvInput1_itr  = bolt::amp::create_mapped_iterator(typename bolt::amp::iterator_traits< DVInputIterator >::iterator_category( ), first, szElements, false, ctl );

        try
        {

            concurrency::parallel_for_each(av,  inputExtent, [=](concurrency::index<1> idx) restrict(amp)
            {
                int globalId = idx[ 0 ];

                if( globalId >= szElements)
                return;

                //result[globalId] = f(first[globalId]);
				result[globalId] = f(dvInput1_itr[globalId]);
            });
        }

		catch(std::exception &e)
        {

               std::cout << "Exception while calling bolt::amp::transform parallel_for_each"<<e.what()<<std::endl;

               return;
        }
    }


	/*! \brief This template function overload is used strictly std random access vectors and AMP implementations. 
        \detail 
    */
    template<typename InputIterator, typename OutputIterator, typename UnaryFunction>
    typename std::enable_if< std::is_same< typename std::iterator_traits< OutputIterator >::iterator_category ,
                                       std::random_access_iterator_tag
                                     >::value
                           >::type
    unary_transform( ::bolt::amp::control &ctl,  InputIterator& first,  InputIterator& last,
     OutputIterator& result,  UnaryFunction& f)
    {
        int sz = static_cast<int>(last - first);
        if (sz == 0)
            return;
        typedef typename std::iterator_traits<InputIterator>::value_type  iType;
        typedef typename std::iterator_traits<OutputIterator>::value_type oType;   

		auto dvInput_itr  = bolt::amp::create_mapped_iterator(typename bolt::amp::iterator_traits< InputIterator >::iterator_category( ), first, sz, false, ctl );
        device_vector< oType, concurrency::array_view > dvOutput( result, sz, true, ctl );

		amp::unary_transform(ctl, dvInput_itr, dvInput_itr+sz, dvOutput.begin(), f);

		dvOutput.data( );
		return;

    }


	
    /*! \brief This template function overload is used strictly for device_vector and AMP implementations. 
        \detail 
    */
    template<typename DVInputIterator1, typename DVInputIterator2, typename DVInputIterator3, typename DVOutputIterator, typename BinaryFunction, typename Predicate>
    typename std::enable_if< 
               std::is_same< typename std::iterator_traits< DVOutputIterator >::iterator_category ,
                                       bolt::amp::device_vector_tag
                           >::value
                           >::type
    binary_transform_if( ::bolt::amp::control &ctl,  DVInputIterator1 first1,   DVInputIterator1 last1,
                        DVInputIterator2 first2,   DVInputIterator3 stencil, DVOutputIterator& result,  BinaryFunction& f,  Predicate  pred)
           
    {

		concurrency::accelerator_view av = ctl.getAccelerator().default_view;

        typedef std::iterator_traits< DVInputIterator1 >::value_type iType1;
        typedef std::iterator_traits< DVInputIterator2 >::value_type iType2;
		typedef std::iterator_traits< DVInputIterator3 >::value_type iType3;
        typedef std::iterator_traits< DVOutputIterator >::value_type oType;

        const int szElements =  static_cast< int >( std::distance( first1, last1 ) );

        const unsigned int leng =  szElements + TRANSFORM_WAVEFRNT_SIZE - (szElements % TRANSFORM_WAVEFRNT_SIZE);

        concurrency::extent< 1 > inputExtent(leng);

        try
        {

            concurrency::parallel_for_each(av,  inputExtent, [=](concurrency::index<1> idx) restrict(amp)
            {
                int globalId = idx[ 0 ];

                if( globalId >= szElements)
                   return;

				if(pred(stencil[globalId]))
                     result[globalId] = f(first1[globalId],first2[globalId]);
				else
					 result[globalId] = first1[globalId];
            });
        }

		catch(std::exception &e)
        {

              std::cout << "Exception while calling bolt::amp::transform parallel_for_each"<<e.what()<<std::endl;

              return;
        }




		return;

    }




	/*! \brief This template function overload is used strictly std random access vectors and AMP implementations. 
        \detail 
    */
    template<typename InputIterator1, typename InputIterator2, typename InputIterator3, typename OutputIterator, typename BinaryFunction, typename Predicate>
    typename std::enable_if< std::is_same< typename std::iterator_traits< OutputIterator >::iterator_category ,
                                       std::random_access_iterator_tag
                                     >::value
                           >::type
    binary_transform_if( ::bolt::amp::control &ctl,  InputIterator1& first1,  InputIterator1& last1,
                        InputIterator2& first2,  InputIterator3& stencil,  OutputIterator& result,  BinaryFunction& f,  Predicate  pred)
    {

        int sz = static_cast<int>(last1 - first1);

        typedef typename std::iterator_traits<OutputIterator>::value_type  oType;

		// Use host pointers memory since these arrays are only read once - no benefit to copying.
        // Map the input iterator to a device_vector

		auto dvInput1_itr  = bolt::amp::create_mapped_iterator(typename bolt::amp::iterator_traits< InputIterator1 >::iterator_category( ), first1, sz, false, ctl );
		auto dvInput2_itr  = bolt::amp::create_mapped_iterator(typename bolt::amp::iterator_traits< InputIterator2 >::iterator_category( ), first2, sz, false, ctl );
		auto dvInput3_itr  = bolt::amp::create_mapped_iterator(typename bolt::amp::iterator_traits< InputIterator3 >::iterator_category( ), stencil, sz, false, ctl );
        device_vector< oType, concurrency::array_view > dvOutput( result, sz, true, ctl );

        amp::binary_transform_if( ctl, dvInput1_itr, dvInput1_itr + sz, dvInput2_itr, dvInput3_itr, dvOutput.begin(), f, pred  );

        // This should immediately map/unmap the buffer
        dvOutput.data( );
		return;

    }


}//namespace amp


	/*! \brief This template function overload is used strictly for device vectors and std random access vectors. 
        \detail Here we branch out into the SerialCpu, MultiCore TBB or The AMP code paths. 
    */
    template<typename InputIterator1, typename InputIterator2, typename OutputIterator, typename BinaryFunction>
    typename std::enable_if< 
             !(std::is_same< typename std::iterator_traits< OutputIterator>::iterator_category, 
                             std::input_iterator_tag 
                           >::value ||
               std::is_same< typename std::iterator_traits< OutputIterator>::iterator_category, 
                             bolt::amp::fancy_iterator_tag >::value) 
                           >::type
    binary_transform(::bolt::amp::control& ctl,  InputIterator1& first1,  InputIterator1& last1, 
                      InputIterator2& first2,  OutputIterator& result,  BinaryFunction& f)
    {
        const int sz =  static_cast< int >( std::distance( first1, last1 )); 
        if (sz == 0)
            return;

        bolt::amp::control::e_RunMode runMode = ctl.getForceRunMode();  // could be dynamic choice some day.
        if(runMode == bolt::amp::control::Automatic)
        {
           runMode = ctl.getDefaultPathToRun();
        }
		
        if( runMode == bolt::amp::control::SerialCpu )
        {
            serial::binary_transform(ctl, first1, last1, first2, result, f );
            return;
        }
        else if( runMode == bolt::amp::control::MultiCoreCpu )
        {
#if defined( ENABLE_TBB )
            btbb::binary_transform(ctl, first1, last1, first2, result, f);
#else
            throw std::runtime_error( "The MultiCoreCpu version of transform is not enabled to be built! \n" );
#endif
            return;
        }
        else
        {
            amp::binary_transform( ctl, first1, last1, first2, result, f );
            return;
        }       
        return;
    }
    

     
    /*! \brief This template function overload is used to seperate input_iterator and fancy_iterator as 
        destination iterators from all other iterators
        \detail This template function overload is used to seperate input_iterator and fancy_iterator as 
        destination iterators from all other iterators. We enable this overload and should result 
        in a compilation failure.
    */
    // TODO - test the below code path
    template<typename InputIterator1, typename InputIterator2, typename OutputIterator, typename BinaryFunction>
    typename std::enable_if< 
               std::is_same< typename std::iterator_traits< OutputIterator>::iterator_category, 
                             std::input_iterator_tag 
                           >::value ||
               std::is_same< typename std::iterator_traits< OutputIterator>::iterator_category, 
                             bolt::amp::fancy_iterator_tag >::value 
                           >::type
    binary_transform(::bolt::amp::control& ctl,  InputIterator1& first1,  InputIterator1& last1, 
                      InputIterator2& first2,  OutputIterator& result,  BinaryFunction& f)
    {
        //TODO - Shouldn't we support transform for input_iterator_tag also. 
        static_assert( std::is_same< typename std::iterator_traits< OutputIterator>::iterator_category, 
                                     std::input_iterator_tag >::value , 
                       "Output vector should be a mutable vector. It cannot be of the type input_iterator_tag" );
        static_assert( std::is_same< typename std::iterator_traits< OutputIterator>::iterator_category, 
                                     bolt::amp::fancy_iterator_tag >::value , 
                       "Output vector should be a mutable vector. It cannot be of type fancy_iterator_tag" );
    }



	 /*! \brief This template function overload is used strictly for device vectors and std random access vectors. 
        \detail Here we branch out into the SerialCpu, MultiCore TBB or The AMP code paths. 
    */
    template<typename InputIterator, typename OutputIterator, typename UnaryFunction>
    typename std::enable_if< 
             !(std::is_same< typename std::iterator_traits< OutputIterator>::iterator_category, 
                             std::input_iterator_tag 
                           >::value ||
               std::is_same< typename std::iterator_traits< OutputIterator>::iterator_category, 
                             bolt::amp::fancy_iterator_tag >::value) 
                           >::type
    unary_transform(::bolt::amp::control& ctl, InputIterator& first,
         InputIterator& last,  OutputIterator& result,  UnaryFunction& f)
    {
        const int sz =  static_cast< int >( std::distance( first, last ) );
        if (sz == 0)
            return;

        bolt::amp::control::e_RunMode runMode = ctl.getForceRunMode();  // could be dynamic choice some day.
        if(runMode == bolt::amp::control::Automatic)
        {
           runMode = ctl.getDefaultPathToRun();
        }
		
        if( runMode == bolt::amp::control::SerialCpu )
        {
            serial::unary_transform(ctl, first, last, result, f );
            return;
        }
        else if( runMode == bolt::amp::control::MultiCoreCpu )
        {
#if defined( ENABLE_TBB )
          
            btbb::unary_transform(ctl, first, last, result, f);
#else
            throw std::runtime_error( "The MultiCoreCpu version of transform is not enabled to be built! \n" );
#endif
            return;
        }
        else
        {
            amp::unary_transform( ctl, first, last, result, f );
            return;
        }       
        return;
    }
    

          
    /*! \brief This template function overload is used to seperate input_iterator and fancy_iterator as destination iterators from all other iterators
        \detail This template function overload is used to seperate input_iterator and fancy_iterator as destination iterators from all other iterators. 
                We enable this overload and should result in a compilation failure.
    */
    // TODO - test the below code path
    template<typename InputIterator, typename OutputIterator, typename UnaryFunction>
    typename std::enable_if< 
               std::is_same< typename std::iterator_traits< OutputIterator>::iterator_category, 
                             std::input_iterator_tag 
                           >::value ||
               std::is_same< typename std::iterator_traits< OutputIterator>::iterator_category, 
                             bolt::amp::fancy_iterator_tag >::value 
                           >::type
    unary_transform(::bolt::amp::control& ctl,  InputIterator& first1,
         InputIterator& last1,  OutputIterator& result,  UnaryFunction& f)
    {
        //TODO - Shouldn't we support transform for input_iterator_tag also. 
        static_assert( std::is_same< typename std::iterator_traits< OutputIterator>::iterator_category, 
                                     std::input_iterator_tag >::value , 
                       "Output vector should be a mutable vector. It cannot be of the type input_iterator_tag" );
        static_assert( std::is_same< typename std::iterator_traits< OutputIterator>::iterator_category, 
                                     bolt::amp::fancy_iterator_tag >::value , 
                       "Output vector should be a mutable vector. It cannot be of type fancy_iterator_tag" );
    }




		/*! \brief This template function overload is used strictly for device vectors and std random access vectors. 
        \detail Here we branch out into the SerialCpu, MultiCore TBB or The AMP code paths. 
    */
    template<typename InputIterator1, typename InputIterator2, typename InputIterator3, typename OutputIterator, typename BinaryFunction, typename Predicate>
    typename std::enable_if< 
             !(std::is_same< typename std::iterator_traits< OutputIterator>::iterator_category, 
                             std::input_iterator_tag 
                           >::value ||
               std::is_same< typename std::iterator_traits< OutputIterator>::iterator_category, 
                             bolt::amp::fancy_iterator_tag >::value) 
                           >::type
    binary_transform_if(::bolt::amp::control& ctl,   InputIterator1& first1,  InputIterator1& last1, 
                       InputIterator2& first2,  InputIterator3& stencil,  OutputIterator& result,  BinaryFunction& f,  Predicate  pred)
    {
        const int sz =  static_cast< int >( std::distance( first1, last1 )); 
        if (sz == 0)
            return;

        bolt::amp::control::e_RunMode runMode = ctl.getForceRunMode();  // could be dynamic choice some day.
        if(runMode == bolt::amp::control::Automatic)
        {
           runMode = ctl.getDefaultPathToRun();
        }
		
        if( runMode == bolt::amp::control::SerialCpu )
        {
            serial::binary_transform_if(ctl, first1, last1, first2, stencil, result, f, pred );
            return;
        }
        else if( runMode == bolt::amp::control::MultiCoreCpu )
        {
#if defined( ENABLE_TBB )
            btbb::binary_transform_if(ctl, first1, last1, first2, stencil, result, f, pred);
#else
            throw std::runtime_error( "The MultiCoreCpu version of transform is not enabled to be built! \n" );
#endif
            return;
        }
        else
        {
            amp::binary_transform_if( ctl, first1, last1, first2, stencil, result, f, pred );
            return;
        }       
        return;
    }
    

     
    /*! \brief This template function overload is used to seperate input_iterator and fancy_iterator as 
        destination iterators from all other iterators
        \detail This template function overload is used to seperate input_iterator and fancy_iterator as 
        destination iterators from all other iterators. We enable this overload and should result 
        in a compilation failure.
    */
    template<typename InputIterator1, typename InputIterator2, typename InputIterator3, typename OutputIterator, typename BinaryFunction, typename Predicate>
    typename std::enable_if< 
               std::is_same< typename std::iterator_traits< OutputIterator>::iterator_category, 
                             std::input_iterator_tag 
                           >::value ||
               std::is_same< typename std::iterator_traits< OutputIterator>::iterator_category, 
                             bolt::amp::fancy_iterator_tag >::value 
                           >::type
    binary_transform_if(::bolt::amp::control& ctl,  InputIterator1& first1, InputIterator1& last1, 
                      InputIterator2& first2,  InputIterator3& stencil,OutputIterator& result,  BinaryFunction& f,  Predicate  pred)
    {
        static_assert( std::is_same< typename std::iterator_traits< OutputIterator>::iterator_category, 
                                     std::input_iterator_tag >::value , 
                       "Output vector should be a mutable vector. It cannot be of the type input_iterator_tag" );
        static_assert( std::is_same< typename std::iterator_traits< OutputIterator>::iterator_category, 
                                     bolt::amp::fancy_iterator_tag >::value , 
                       "Output vector should be a mutable vector. It cannot be of type fancy_iterator_tag" );
    }



}//end of namespace detail


        //////////////////////////////////////////
        //  Transform overloads
        //////////////////////////////////////////
        // default control, two-input transform, std:: iterator
        template<typename InputIterator1, typename InputIterator2, typename OutputIterator, typename BinaryFunction>
        void transform( bolt::amp::control& ctl,
                       InputIterator1 first1,
                       InputIterator1 last1,
                       InputIterator2 first2,
                       OutputIterator result,
                       BinaryFunction f )
        {
			  using bolt::amp::detail::binary_transform;
              binary_transform( ctl, first1, last1, first2, result, f );

        }


        // default control, two-input transform, std:: iterator
        template<typename InputIterator1, typename InputIterator2, typename OutputIterator, typename BinaryFunction>
        void transform( InputIterator1 first1,
                        InputIterator1 last1,
                        InputIterator2 first2,
                        OutputIterator result,
                        BinaryFunction f )
        {
              using bolt::amp::transform;
              transform( control::getDefault(), first1, last1, first2, result, f);
        }

        // default control, two-input transform, std:: iterator
        template<typename InputIterator, typename OutputIterator, typename UnaryFunction>
        void transform( bolt::amp::control& ctl,
                        InputIterator first1,
                        InputIterator last1,
                        OutputIterator result,
                        UnaryFunction f )
        {
              //using ::bolt::amp::detail::unary_transform;
              ::bolt::amp::detail::unary_transform( ctl, first1, last1, result, f);
        }

        // default control, two-input transform, std:: iterator
        template<typename InputIterator, typename OutputIterator, typename UnaryFunction>
        void transform( InputIterator first1,
                        InputIterator last1,
                        OutputIterator result,
                        UnaryFunction f )
        {
              using bolt::amp::transform;
              ::bolt::amp::transform( control::getDefault(), first1, last1, result, f );
        }


		//////////////////////////////////////////
        //  TransformIf overloads
        //////////////////////////////////////////


		template<typename UnaryFunction, typename Predicate, typename iType, typename oType, typename S>
        struct unary_transform_if_functor
        {
          __declspec(align(4)) UnaryFunction unary_op;
          __declspec(align(4)) Predicate pred;
	    
          unary_transform_if_functor(UnaryFunction unary_op, Predicate pred)
            : unary_op(unary_op), pred(pred)
          {}
	    
	    
          oType operator()(iType &temp, S &stencil) const restrict(amp,cpu)
          {
	    	oType res;
	    
	    	if(pred(stencil))
                res = unary_op(temp);
	    	else
	    		res = temp;
	    	return res;
	    
          }

        }; // end unary_transform_if_functor


		template<typename InputIterator1, typename InputIterator2, typename InputIterator3, typename OutputIterator, typename BinaryFunction, typename Predicate>
        OutputIterator transform_if( bolt::amp::control& ctl,
                       InputIterator1 first1,
                       InputIterator1 last1,
                       InputIterator2 first2,
					   InputIterator3 stencil,
                       OutputIterator result,
                       BinaryFunction f,
					   Predicate  	pred)
        {
              bolt::amp::detail::binary_transform_if( ctl, first1, last1, first2, stencil, result, f, pred );
			  return result;

        }


        // default control, two-input transform, std:: iterator
        template<typename InputIterator1, typename InputIterator2, typename InputIterator3, typename OutputIterator, typename BinaryFunction, typename Predicate>
        OutputIterator transform_if( InputIterator1 first1,
                        InputIterator1 last1,
                        InputIterator2 first2,
						InputIterator3 stencil, 
                        OutputIterator result,
                        BinaryFunction f,
						Predicate  	pred)
        {
              return transform_if( control::getDefault(), first1, last1, first2, stencil, result, f, pred);
        }

        // default control, two-input transform, std:: iterator
        template<typename InputIterator, typename OutputIterator, typename UnaryFunction, typename Predicate>
        OutputIterator transform_if( bolt::amp::control& ctl,
                        InputIterator first1,
                        InputIterator last1,
                        OutputIterator result,
                        UnaryFunction f,
						Predicate  	pred)
        {
			  typedef typename std::iterator_traits<OutputIterator>::value_type  oType;
			  typedef typename std::iterator_traits<InputIterator>::value_type  iType;
			  typedef unary_transform_if_functor<UnaryFunction,Predicate, iType, oType, iType> UnaryTransformIfFunctor;

			  bolt::amp::transform(ctl,
                     first1,
                     last1,
					 first1,
					 result,
                     UnaryTransformIfFunctor(f, pred));
			  return result;
        }

        // default control, two-input transform, std:: iterator
        template<typename InputIterator, typename OutputIterator, typename UnaryFunction, typename Predicate>
        OutputIterator transform_if( InputIterator first1,
                        InputIterator last1,
                        OutputIterator result,
                        UnaryFunction f,
						Predicate  	pred)
        {
              return transform_if( control::getDefault(), first1, last1, result, f, pred );
        }


		template<typename InputIterator1, typename InputIterator2, typename OutputIterator, typename UnaryFunction, typename Predicate>
		OutputIterator  transform_if (bolt::amp::control& ctl, InputIterator1 first, InputIterator1 last, InputIterator2 stencil, OutputIterator result, UnaryFunction op, Predicate pred)
		{
			 typedef typename std::iterator_traits<OutputIterator>::value_type  oType;
			 typedef typename std::iterator_traits<InputIterator1>::value_type  iType;
		     typedef typename std::iterator_traits<InputIterator2>::value_type  sType;
		     typedef unary_transform_if_functor<UnaryFunction,Predicate, iType, oType, sType> UnaryTransformIfFunctor;

			 bolt::amp::transform(ctl,
                     first,
                     last,
					 stencil,
					 result,
                     UnaryTransformIfFunctor(op, pred));
			 return result;
		}

		template<typename InputIterator1, typename InputIterator2, typename OutputIterator, typename UnaryFunction, typename Predicate>
		OutputIterator  transform_if (InputIterator1 first, InputIterator1 last, InputIterator2 stencil, OutputIterator result, UnaryFunction op, Predicate pred)
		{
			 return transform_if (control::getDefault(), first, last, stencil, result, op, pred);
		}




    } //end of namespace amp
} //end of namespace bolt

#endif // AMP_TRANSFORM_INL