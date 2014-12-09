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

#if !defined( BOLT_AMP_COPY_INL )
#define BOLT_AMP_COPY_INL
#pragma once

#define COPY_WAVEFRONT_SIZE 256 

#include <algorithm>
#include <type_traits>
#include <bolt/amp/bolt.h>
#include <bolt/amp/device_vector.h>
#include <bolt/amp/transform.h>
#include <bolt/amp/scan.h>
#include <bolt/amp/scatter.h>
#include <bolt/amp/iterator/iterator_traits.h>
#include <bolt/amp/iterator/addressof.h>
#include <amp.h>

#ifdef ENABLE_TBB
//TBB Includes
#include "bolt/btbb/copy.h"
#endif


namespace bolt {
namespace amp {


namespace detail {

template< typename DVInputIterator, typename Size, typename DVOutputIterator >
 void  copy_enqueue(bolt::amp::control &ctrl, const DVInputIterator& first, const Size& n,
    const DVOutputIterator& result)
{

	  concurrency::accelerator_view av = ctrl.getAccelerator().default_view;

      typedef typename std::iterator_traits<DVInputIterator>::value_type iType;
      typedef typename std::iterator_traits<DVOutputIterator>::value_type oType;
    
      const int szElements = static_cast< int >( n );
      const unsigned int leng =  szElements + COPY_WAVEFRONT_SIZE - (szElements % COPY_WAVEFRONT_SIZE);

	 concurrency::extent< 1 > inputExtent(leng);

      try
      {

         concurrency::parallel_for_each(av,  inputExtent, [=](concurrency::index<1> idx) restrict(amp)
         {
             int globalId = idx[ 0 ];

            if( globalId >= szElements)
                return;

             result[globalId] = first[globalId];
         });
      }

   
      catch(std::exception &e)
      {
        std::cout << "Exception while calling bolt::amp::copy parallel_for_each " ;
        std::cout<< e.what() << std::endl;
        throw std::exception();
      }	

}


/*! \brief This template function overload is used to seperate device_vector iterators from all other iterators
                \detail This template is called by the non-detail versions of copy, it already assumes
             *  random access iterators.  This overload is called strictly for non-device_vector iterators
            */
template<typename InputIterator, typename Size, typename OutputIterator>
void copy_pick_iterator( bolt::amp::control &ctrl,  const InputIterator& first, const Size& n,
                         const OutputIterator& result, std::random_access_iterator_tag, std::random_access_iterator_tag )
{

    typedef typename std::iterator_traits<InputIterator>::value_type iType;
    typedef typename std::iterator_traits<OutputIterator>::value_type oType;


     bolt::amp::control::e_RunMode runMode = ctrl.getForceRunMode( );
     if (runMode == bolt::amp::control::Automatic)
     {
         runMode = ctrl.getDefaultPathToRun();
     }


     if( runMode == bolt::amp::control::SerialCpu )
     {
         
         #if defined( _WIN32 )
           std::copy_n( first, n, stdext::checked_array_iterator<oType*>(&(*result), n ) );
         #else
           std::copy_n( first, n, result );
         #endif
     }
     else if( runMode == bolt::amp::control::MultiCoreCpu )
     {
        #ifdef ENABLE_TBB   
           bolt::btbb::copy_n(first, n, &(*result));
        #else
            throw std::runtime_error( "The MultiCoreCpu version of Copy is not enabled to be built." );
        #endif
     }
     else
     {
        // A host 2 host copy operation, just fallback on the optimized std:: implementation
        #if defined( _WIN32 )
          std::copy_n( first, n, stdext::checked_array_iterator<oType*>(&(*result), n ) );
        #else
          std::copy_n( first, n, result );
        #endif
     }
}



// This template is called by the non-detail versions of copy, it already assumes random access iterators
// This is called strictly for iterators that are derived from device_vector< T >::iterator
template<typename DVInputIterator, typename Size, typename DVOutputIterator>
void copy_pick_iterator( bolt::amp::control &ctrl,  const DVInputIterator& first, const Size& n,
                         const DVOutputIterator& result, bolt::amp::device_vector_tag, bolt::amp::device_vector_tag )
{
    typedef typename std::iterator_traits<DVInputIterator>::value_type iType;
    typedef typename std::iterator_traits<DVOutputIterator>::value_type oType;
     bolt::amp::control::e_RunMode runMode = ctrl.getForceRunMode( );
     if (runMode == bolt::amp::control::Automatic)
     {
         runMode = ctrl.getDefaultPathToRun();
     }
     
     if( runMode == bolt::amp::control::SerialCpu )
     {
          
            typename bolt::amp::device_vector< iType >::pointer copySrc =  first.getContainer( ).data( );
            typename bolt::amp::device_vector< oType >::pointer copyDest =  result.getContainer( ).data( );
#if defined( _WIN32 )
            std::copy_n( &copySrc[first.m_Index], n, stdext::make_checked_array_iterator( &copyDest[result.m_Index], n) );
#else
            std::copy_n( &copySrc[first.m_Index], n, &copyDest[result.m_Index] );
#endif
            

            return;
     }
     else if( runMode == bolt::amp::control::MultiCoreCpu )
     {

         #ifdef ENABLE_TBB
             typename bolt::amp::device_vector< iType >::pointer copySrc =  first.getContainer( ).data( );
             typename bolt::amp::device_vector< oType >::pointer copyDest =  result.getContainer( ).data( );
             bolt::btbb::copy_n( &copySrc[first.m_Index], n, &copyDest[result.m_Index] );    
            return;
         #else
                throw std::runtime_error( "The MultiCoreCpu version of Copy is not enabled to be built." );
         #endif
     }
     else
     {	
         copy_enqueue( ctrl, first, n, result);
     }
}

template<typename DVInputIterator, typename Size, typename DVOutputIterator>
void copy_pick_iterator( bolt::amp::control &ctrl,  const DVInputIterator& first, const Size& n,
                         const DVOutputIterator& result, bolt::amp::fancy_iterator_tag, bolt::amp::device_vector_tag )
{
    typedef typename std::iterator_traits<DVInputIterator>::value_type iType;
    typedef typename std::iterator_traits<DVOutputIterator>::value_type oType;
     bolt::amp::control::e_RunMode runMode = ctrl.getForceRunMode( );
     if (runMode == bolt::amp::control::Automatic)
     {
         runMode = ctrl.getDefaultPathToRun();
     }
     
     if( runMode == bolt::amp::control::SerialCpu )
     {
          
            typename bolt::amp::device_vector< oType >::pointer copyDest =  result.getContainer( ).data( );
#if defined( _WIN32 )
            std::copy_n( first, n, stdext::make_checked_array_iterator( &copyDest[result.m_Index], n) );
#else
            std::copy_n( first, n, &copyDest[result.m_Index] );
#endif
            

            return;
     }
     else if( runMode == bolt::amp::control::MultiCoreCpu )
     {

         #ifdef ENABLE_TBB
             typename bolt::amp::device_vector< oType >::pointer copyDest =  result.getContainer( ).data( );
             bolt::btbb::copy_n( first, n, &copyDest[result.m_Index] );    
            return;
         #else
                throw std::runtime_error( "The MultiCoreCpu version of Copy is not enabled to be built." );
         #endif
     }
     else
     {	
         copy_enqueue( ctrl, first, n, result);
     }
}

// This template is called by the non-detail versions of copy, it already assumes random access iterators
// This is called strictly for iterators that are derived from device_vector< T >::iterator
template<typename DVInputIterator, typename Size, typename DVOutputIterator>
void copy_pick_iterator( bolt::amp::control &ctrl,  const DVInputIterator& first, const Size& n,
                         const DVOutputIterator& result, std::random_access_iterator_tag, bolt::amp::device_vector_tag)
{
    typedef typename std::iterator_traits<DVInputIterator>::value_type iType;
    typedef typename std::iterator_traits<DVOutputIterator>::value_type oType;
     bolt::amp::control::e_RunMode runMode = ctrl.getForceRunMode( );

     if (runMode == bolt::amp::control::Automatic)
     {
         runMode = ctrl.getDefaultPathToRun();
     }

   
     if( runMode == bolt::amp::control::SerialCpu )
     {
           
            typename bolt::amp::device_vector< oType >::pointer copyDest =  result.getContainer( ).data( );
#if defined( _WIN32 )
            std::copy_n( first, n, stdext::make_checked_array_iterator( &copyDest[result.m_Index], n) );
#else
            std::copy_n( first, n, &copyDest[result.m_Index] );
#endif
           
            return;
     }
     else if( runMode == bolt::amp::control::MultiCoreCpu )
     {

         #ifdef ENABLE_TBB
              typename bolt::amp::device_vector< oType >::pointer copyDest =  result.getContainer( ).data( );
              bolt::btbb::copy_n( first, n, &copyDest[result.m_Index] );

            return;
         #else
                throw std::runtime_error( "The MultiCoreCpu version of Copy is not enabled to be built." );
         #endif
     }
     else
     {
       
        device_vector< iType, concurrency::array_view> dvInput( first, n, false, ctrl );
        //Now call the actual algorithm
        copy_enqueue( ctrl, dvInput.begin(), n, result );
        //Map the buffer back to the host
        dvInput.data( );
     }
}

// This template is called by the non-detail versions of copy, it already assumes random access iterators
// This is called strictly for iterators that are derived from device_vector< T >::iterator
template<typename DVInputIterator, typename Size, typename DVOutputIterator>
void copy_pick_iterator(bolt::amp::control &ctrl,  const DVInputIterator& first, const Size& n,
                        const DVOutputIterator& result, bolt::amp::device_vector_tag, std::random_access_iterator_tag)
{

    typedef typename std::iterator_traits<DVInputIterator>::value_type iType;
    typedef typename std::iterator_traits<DVOutputIterator>::value_type oType;

    bolt::amp::control::e_RunMode runMode = ctrl.getForceRunMode( );
    if (runMode == bolt::amp::control::Automatic)
    {
        runMode = ctrl.getDefaultPathToRun();
    }
     
     if( runMode == bolt::amp::control::SerialCpu )
     {
         #if defined( _WIN32 )
           std::copy_n( first, n, stdext::checked_array_iterator<oType*>(&(*result), n ) );
         #else
           std::copy_n( first, n, result );
         #endif
           return;
     }
     else if( runMode == bolt::amp::control::MultiCoreCpu )
     {

           #ifdef ENABLE_TBB
               typename bolt::amp::device_vector< iType >::pointer copySrc =  first.getContainer( ).data( );
               bolt::btbb::copy_n( &copySrc[first.m_Index], n, result );
           #else
                throw std::runtime_error( "The MultiCoreCpu version of Copy is not enabled to be built." );
           #endif
     }
     else
     {

        // Use host pointers memory since these arrays are only read once - no benefit to copying.
        // Map the output iterator to a device_vector
        device_vector< oType, concurrency::array_view> dvOutput( result, n, true, ctrl );
        copy_enqueue( ctrl, first, n, dvOutput.begin( ));
        dvOutput.data();
     }
}


template<typename DVInputIterator, typename Size, typename DVOutputIterator>
void copy_pick_iterator(bolt::amp::control &ctrl,  const DVInputIterator& first, const Size& n,
                        const DVOutputIterator& result, bolt::amp::fancy_iterator_tag, std::random_access_iterator_tag)
{

    typedef typename std::iterator_traits<DVInputIterator>::value_type iType;
    typedef typename std::iterator_traits<DVOutputIterator>::value_type oType;

    bolt::amp::control::e_RunMode runMode = ctrl.getForceRunMode( );
    if (runMode == bolt::amp::control::Automatic)
    {
        runMode = ctrl.getDefaultPathToRun();
    }
     
     if( runMode == bolt::amp::control::SerialCpu )
     {
         #if defined( _WIN32 )
           std::copy_n( first, n, stdext::checked_array_iterator<oType*>(&(*result), n ) );
         #else
           std::copy_n( first, n, result );
         #endif
           return;
     }
     else if( runMode == bolt::amp::control::MultiCoreCpu )
     {

           #ifdef ENABLE_TBB
               bolt::btbb::copy_n( first, n, result );
           #else
                throw std::runtime_error( "The MultiCoreCpu version of Copy is not enabled to be built." );
           #endif
     }
     else
     {

        // Use host pointers memory since these arrays are only read once - no benefit to copying.
        // Map the output iterator to a device_vector
        device_vector< oType, concurrency::array_view> dvOutput( result, n, true, ctrl );
        copy_enqueue( ctrl, first, n, dvOutput.begin( ));
        dvOutput.data();
     }
}


template<typename InputIterator, typename Size, typename OutputIterator >
OutputIterator copy_detect_random_access( bolt::amp::control& ctrl, const InputIterator& first, const Size& n,
                const OutputIterator& result, std::random_access_iterator_tag )
{
    if (n < 0)
    {
      std::cout<<"\n Number of elements to copy cannot be negative! "<< std::endl;
    }
    if (n > 0)
    {
      copy_pick_iterator( ctrl, first, n, result, typename std::iterator_traits< InputIterator >::iterator_category( ),
                          std::iterator_traits< OutputIterator >::iterator_category( ));
    }
    return (result+n);
};

template<typename InputIterator, typename Size, typename OutputIterator >
OutputIterator copy_detect_random_access( bolt::amp::control& ctrl, const InputIterator& first, const Size& n,
                const OutputIterator& result, bolt::amp::device_vector_tag )
{
    if (n < 0)
    {
      std::cout<<"\n Number of elements to copy cannot be negative! "<< std::endl;
    }
    if (n > 0)
    {

      copy_pick_iterator( ctrl, first, n, result, typename std::iterator_traits< InputIterator >::iterator_category( ),
                          std::iterator_traits< OutputIterator >::iterator_category( )); 
    }
    return (result+n);
};

template<typename InputIterator, typename Size, typename OutputIterator >
OutputIterator copy_detect_random_access( bolt::amp::control& ctrl, const InputIterator& first, const Size& n,
                const OutputIterator& result, bolt::amp::fancy_iterator_tag )
{
    if (n < 0)
    {
      std::cout<<"\n Number of elements to copy cannot be negative! "<< std::endl;
    }
    if (n > 0)
    {

      copy_pick_iterator( ctrl, first, n, result, typename std::iterator_traits< InputIterator >::iterator_category( ),
                          std::iterator_traits< OutputIterator >::iterator_category( )); 
    }
    return (result+n);
};


// Wrapper that uses default control class, iterator interface
template<typename InputIterator, typename Size, typename OutputIterator>
OutputIterator copy_detect_random_access( bolt::amp::control& ctrl, const InputIterator& first, const Size& n,
                const OutputIterator& result, std::input_iterator_tag )
{
    static_assert( std::is_same< InputIterator, bolt::amp::input_iterator_tag  >::value, "Bolt only supports random access iterator types" );
    return NULL;
};

}//End of detail namespace



// user control
template<typename InputIterator, typename OutputIterator>
OutputIterator copy(bolt::amp::control &ctrl,  InputIterator first, InputIterator last, OutputIterator result)
{
    int n = static_cast<int>( std::distance( first, last ) );
    return detail::copy_detect_random_access( ctrl, first, n, result,
         typename std::iterator_traits< InputIterator >::iterator_category( ) );
}

// default control
template<typename InputIterator, typename OutputIterator>
OutputIterator copy( InputIterator first, InputIterator last, OutputIterator result)
{

    int n = static_cast<int>( std::distance( first, last ) );
            return detail::copy_detect_random_access( control::getDefault(), first, n, result,
                typename std::iterator_traits< InputIterator >::iterator_category( ) );
}

// default control
template<typename InputIterator, typename Size, typename OutputIterator>
OutputIterator copy_n(InputIterator first, Size n, OutputIterator result)
{
            return detail::copy_detect_random_access( control::getDefault(), first, n, result,
                typename std::iterator_traits< InputIterator >::iterator_category( ) );
}

// user control
template<typename InputIterator, typename Size, typename OutputIterator>
OutputIterator copy_n(bolt::amp::control &ctrl, InputIterator first, Size n, OutputIterator result)
{
    return detail::copy_detect_random_access( ctrl, first, n, result,
                    typename std::iterator_traits< InputIterator >::iterator_category( ) );
}


/* Copy_if implementation*/
namespace detail
{
            
namespace serial{

    template<typename InputIterator1, typename InputIterator2, typename OutputIterator, typename Predicate>
    OutputIterator  copy_if( bolt::amp::control &ctl, InputIterator1 first, InputIterator1 last,
             InputIterator2 stencil, OutputIterator result, Predicate predicate)
    {

        int n = static_cast<int>(std::distance(first,last));
        auto mapped_first_itr = create_mapped_iterator(typename std::iterator_traits<InputIterator1>::iterator_category(), 
                                                           ctl, first);
        auto mapped_stencil_itr = create_mapped_iterator(typename std::iterator_traits<InputIterator2>::iterator_category(), 
                                                           ctl, stencil);

	
        auto mapped_result_itr = create_mapped_iterator(typename std::iterator_traits<OutputIterator>::iterator_category(), 
                                                           ctl, result);

		auto mapped_last_itr = create_mapped_iterator(typename std::iterator_traits<InputIterator1>::iterator_category(), 
                                                           ctl, last);

		auto mapped_result_itr_temp = mapped_result_itr;

		while (mapped_first_itr != mapped_last_itr ) 
        {
            if (predicate(*mapped_stencil_itr)) 
            {
                *mapped_result_itr = *mapped_first_itr;
                ++mapped_result_itr;
            }
            ++mapped_first_itr;
            ++mapped_stencil_itr;
        }

        return result + (mapped_result_itr - mapped_result_itr_temp);     
    }

}//end of serial namespace

#ifdef ENABLE_TBB
namespace btbb{

	template<typename InputIterator1, typename InputIterator2, typename OutputIterator, typename Predicate>
    OutputIterator copy_if( ::bolt::amp::control &ctl, InputIterator1 first, InputIterator1 last,
                      InputIterator2 stencil, OutputIterator result, Predicate pred)
    {

        int n = static_cast<int>(std::distance(first,last));
        auto mapped_first_itr = create_mapped_iterator(typename std::iterator_traits<InputIterator1>::iterator_category(), 
                                                           ctl, first);
		auto mapped_last_itr = create_mapped_iterator(typename std::iterator_traits<InputIterator1>::iterator_category(), 
                                                           ctl, last);
        auto mapped_stencil_itr = create_mapped_iterator(typename std::iterator_traits<InputIterator2>::iterator_category(), 
                                                           ctl, stencil);
        auto mapped_result_itr = create_mapped_iterator(typename std::iterator_traits<OutputIterator>::iterator_category(), 
                                                           ctl, result);
		auto return_itr = bolt::btbb::copy_if(mapped_first_itr, mapped_last_itr, mapped_stencil_itr, mapped_result_itr, pred);

		return result + (return_itr - mapped_result_itr);
    }

}//end of btbb namespace
#endif
namespace amp{

    template<typename Predicate>
    struct bool_to_int
    {
      Predicate pred;
 
      bool_to_int(Predicate& _pred) : pred(_pred) {} 
  
      template <typename T> 
      int operator()(T& x) const restrict(amp, cpu) 
      { 
        return pred(x) ? 1 : 0; 
      } 
    }; 

    template<typename IndexType>
    struct is_true
    { 
      bool operator()(IndexType& x) const restrict(amp, cpu) 
      { 
        return (x) ? true : false; 
      }
    };

    template<typename InputIterator1, typename InputIterator2, typename OutputIterator, typename Predicate>
    OutputIterator copy_if( ::bolt::amp::control &ctl, InputIterator1& first, InputIterator1& last,
                      InputIterator2& stencil, OutputIterator& result, Predicate& pred)
    {
        int sz = static_cast<int>(last - first);

        typedef typename std::iterator_traits<OutputIterator>::value_type  oType;

        // Map the input iterator to a device_vector iterator
		auto dvInput_itr   = bolt::amp::create_mapped_iterator(
                                    typename bolt::amp::iterator_traits< InputIterator1 >::iterator_category( ), 
                                    first, sz, false, ctl );
        auto dvStencil_itr = bolt::amp::create_mapped_iterator(
                                    typename bolt::amp::iterator_traits< InputIterator2 >::iterator_category( ), 
                                    stencil, sz, false, ctl);
        auto dvResult_itr = bolt::amp::create_mapped_iterator(
                                    typename bolt::amp::iterator_traits< OutputIterator >::iterator_category( ), 
                                    result, sz, false, ctl);

        // Map the output iterator to a device_vector
        device_vector< oType, concurrency::array_view > dvOutput( result, sz, true, ctl );


		typedef int IndexType;

        //Note IndexType is int. and bool_to_int converts to int.
        bool_to_int<Predicate> internal_pred(pred);

        device_vector< IndexType, concurrency::array > predicates( sz, 0, true, ctl );
        device_vector< IndexType, concurrency::array > scatter_indices( sz, 0, true, ctl );
        //Transform the input routine to fill with 0 for false and 1 for true. 
        bolt::amp::transform( ctl, dvStencil_itr , dvStencil_itr  + sz, predicates.begin(), internal_pred);

        bolt::amp::exclusive_scan( ctl, predicates.begin(), predicates.begin() + sz, scatter_indices.begin(), 0);

        bolt::amp::scatter_if( ctl, dvInput_itr, dvInput_itr+sz, scatter_indices.begin(), predicates.begin(), dvResult_itr, is_true<IndexType>() );

        // This should immediately map/unmap the buffer
        dvOutput.data( );
		return result + ((dvResult_itr + scatter_indices[sz - 1]) - dvOutput.begin() );
    }


}//end of amp namespace


	 /*! \brief This template function overload is used strictly for device vectors and std random access vectors. 
        \detail Here we branch out into the SerialCpu, MultiCore TBB or The AMP code paths. 
    */
    template<typename InputIterator1, typename InputIterator2, typename OutputIterator, typename Predicate>
    typename std::enable_if< 
             !(std::is_same< typename std::iterator_traits< OutputIterator>::iterator_category, 
                             std::input_iterator_tag 
                           >::value ||
               std::is_same< typename std::iterator_traits< OutputIterator>::iterator_category, 
                             bolt::amp::fancy_iterator_tag >::value) 
                           , OutputIterator
                           >::type
    copy_if(::bolt::amp::control& ctl, InputIterator1& first,
         InputIterator1& last, InputIterator2& stencil, OutputIterator& result,  Predicate& pred)
    {
        const int sz =  static_cast< int >( std::distance( first, last ) );
        if (sz == 0)
            return result;

        bolt::amp::control::e_RunMode runMode = ctl.getForceRunMode();  // could be dynamic choice some day.
        if(runMode == bolt::amp::control::Automatic)
        {
           runMode = ctl.getDefaultPathToRun();
        }
		
        if( runMode == bolt::amp::control::SerialCpu )
        {
            return serial::copy_if(ctl, first, last, stencil, result, pred );
        }
        else if( runMode == bolt::amp::control::MultiCoreCpu )
        {
#if defined( ENABLE_TBB )
            return btbb::copy_if(ctl, first, last, stencil, result, pred);
#else
            throw std::runtime_error( "The MultiCoreCpu version of transform is not enabled to be built! \n" );
#endif

        }
        else
        {
            return amp::copy_if( ctl, first, last, stencil, result, pred );
        }       
        return result;
    }
    

          
    /*! \brief This template function overload is used to seperate input_iterator and fancy_iterator as destination iterators from all other iterators
        \detail This template function overload is used to seperate input_iterator and fancy_iterator as destination iterators from all other iterators. 
                We enable this overload and should result in a compilation failure.
    */
    // TODO - test the below code path
    template<typename InputIterator1, typename InputIterator2, typename OutputIterator, typename Predicate>
    typename std::enable_if< 
               (std::is_same< typename std::iterator_traits< OutputIterator>::iterator_category, 
                             std::input_iterator_tag 
                           >::value ||
               std::is_same< typename std::iterator_traits< OutputIterator>::iterator_category, 
                             bolt::amp::fancy_iterator_tag >::value)
                           , OutputIterator
                           >::type
    copy_if(::bolt::amp::control& ctl, InputIterator1& first,
         InputIterator1& last, InputIterator2& stencil, OutputIterator& result,  Predicate& pred)
    {
        //TODO - Shouldn't we support transform for input_iterator_tag also. 
        static_assert( std::is_same< typename std::iterator_traits< OutputIterator>::iterator_category, 
                                     std::input_iterator_tag >::value , 
                       "Output vector should be a mutable vector. It cannot be of the type input_iterator_tag" );
        static_assert( std::is_same< typename std::iterator_traits< OutputIterator>::iterator_category, 
                                     bolt::amp::fancy_iterator_tag >::value , 
                       "Output vector should be a mutable vector. It cannot be of type fancy_iterator_tag" );
    }


}//end of detail namespace


    template<typename InputIterator, typename OutputIterator, typename Predicate>
    OutputIterator copy_if(bolt::amp::control &ctrl,  InputIterator first, InputIterator last, OutputIterator result, 
                           Predicate pred)
    {
        //using bolt::amp::detail::copy_if;
        return bolt::amp::detail::copy_if( ctrl, first, last, first, result, pred);
    }

    // default control
    template<typename InputIterator, typename OutputIterator, typename Predicate>
    OutputIterator copy_if( InputIterator first, InputIterator last, OutputIterator result, 
                            Predicate pred)
    {
        //using bolt::amp::detail::copy_if;
        return bolt::amp::detail::copy_if( control::getDefault(), first, last, first, result, pred);
    }

    template<typename InputIterator1, typename InputIterator2, typename OutputIterator, typename Predicate>
    OutputIterator copy_if(bolt::amp::control &ctrl,  InputIterator1 first, InputIterator1 last, 
                           InputIterator2 stencil, OutputIterator result, 
                           Predicate pred)
    {
        //using bolt::amp::detail::copy_if;
        return bolt::amp::detail::copy_if( ctrl, first, last, stencil, result, pred);
    }

    template<typename InputIterator1, typename InputIterator2, typename OutputIterator, typename Predicate>
    OutputIterator copy_if(InputIterator1 first, InputIterator1 last, InputIterator2 stencil, OutputIterator result, 
                           Predicate pred)
    {
        //using bolt::amp::detail::copy_if;
        return bolt::amp::detail::copy_if( control::getDefault(), first, last, stencil, result, pred);
    }


}//end of amp namespace
};//end of bolt namespace

#endif
