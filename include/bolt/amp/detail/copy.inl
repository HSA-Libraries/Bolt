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

#if !defined( BOLT_AMP_COPY_INL )
#define BOLT_AMP_COPY_INL
#pragma once

#define WAVEFRONT_SIZE 256 

#include <algorithm>
#include <type_traits>
#include "bolt/amp/bolt.h"
#include "bolt/amp/device_vector.h"
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

      typedef typename std::iterator_traits<DVInputIterator>::value_type iType;
      typedef typename std::iterator_traits<DVOutputIterator>::value_type oType;
    
	  const unsigned int szElements = static_cast< unsigned int >( n );
    
      concurrency::array_view< iType, 1 > inputV (first.getContainer().getBuffer(first));
	  concurrency::array_view< oType, 1 > outputV (result.getContainer().getBuffer(result));

	  unsigned int wavefrontMultiple = szElements;
      const unsigned int lowerBits = ( szElements & ( WAVEFRONT_SIZE -1 ) );

	  concurrency::extent< 1 > inputExtent( wavefrontMultiple );

      int boundsCheck = 0;

      if( lowerBits )
      {
                   wavefrontMultiple &= ~lowerBits;
                   wavefrontMultiple += WAVEFRONT_SIZE;
	  }
			   else
				    boundsCheck = 1;

	  try
      {

		       concurrency::parallel_for_each(ctrl.getAccelerator().default_view, inputExtent, [=](concurrency::index<1> idx) restrict(amp)
               {
                   unsigned int globalId = idx[0];

				   if(boundsCheck == 0)
				   {
                     //if( globalId >= wavefrontMultiple )
					 if( globalId >= szElements)
                       return;
				   }
				   
                   outputV[globalId] = inputV[globalId];
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
                \detail This template is called by the non-detail versions of inclusive_scan, it already assumes
             *  random access iterators.  This overload is called strictly for non-device_vector iterators
            */
template<typename InputIterator, typename Size, typename OutputIterator>
typename std::enable_if<
                   !(std::is_base_of<typename device_vector<typename std::iterator_traits<InputIterator>::value_type>::iterator,InputIterator>::value) &&
				   !(std::is_base_of<typename device_vector<typename std::iterator_traits<OutputIterator>::value_type>::iterator,OutputIterator>::value)
				   ,void >::type
copy_pick_iterator(bolt::amp::control &ctrl,  const InputIterator& first, const Size& n,
        const OutputIterator& result)
{

    typedef typename  std::iterator_traits<InputIterator>::value_type iType;
    typedef typename std::iterator_traits<OutputIterator>::value_type oType;


     bolt::amp::control::e_RunMode runMode = ctrl.getForceRunMode( );

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



// This template is called by the non-detail versions of inclusive_scan, it already assumes random access iterators
// This is called strictly for iterators that are derived from device_vector< T >::iterator
template<typename DVInputIterator, typename Size, typename DVOutputIterator>
typename std::enable_if<
                   (std::is_base_of<typename device_vector<typename std::iterator_traits<DVInputIterator>::value_type>::iterator,DVInputIterator>::value) &&
				   (std::is_base_of<typename device_vector<typename std::iterator_traits<DVOutputIterator>::value_type>::iterator,DVOutputIterator>::value)
				   ,void >::type
copy_pick_iterator(bolt::amp::control &ctrl,  const DVInputIterator& first, const Size& n,
    const DVOutputIterator& result )
{
    typedef typename std::iterator_traits<DVInputIterator>::value_type iType;
    typedef typename std::iterator_traits<DVOutputIterator>::value_type oType;
     bolt::amp::control::e_RunMode runMode = ctrl.getForceRunMode( );

	 
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

// This template is called by the non-detail versions of inclusive_scan, it already assumes random access iterators
// This is called strictly for iterators that are derived from device_vector< T >::iterator
template<typename DVInputIterator, typename Size, typename DVOutputIterator>
typename std::enable_if<
                   !(std::is_base_of<typename device_vector<typename std::iterator_traits<DVInputIterator>::value_type>::iterator,DVInputIterator>::value) &&
				   (std::is_base_of<typename device_vector<typename std::iterator_traits<DVOutputIterator>::value_type>::iterator,DVOutputIterator>::value)
				   ,void >::type
copy_pick_iterator( bolt::amp::control &ctrl,  const DVInputIterator& first, const Size& n,
    const DVOutputIterator& result)
{
    typedef typename std::iterator_traits<DVInputIterator>::value_type iType;
    typedef typename std::iterator_traits<DVOutputIterator>::value_type oType;
     bolt::amp::control::e_RunMode runMode = ctrl.getForceRunMode( );

   
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

// This template is called by the non-detail versions of inclusive_scan, it already assumes random access iterators
// This is called strictly for iterators that are derived from device_vector< T >::iterator
template<typename DVInputIterator, typename Size, typename DVOutputIterator>
typename std::enable_if<
                   (std::is_base_of<typename device_vector<typename std::iterator_traits<DVInputIterator>::value_type>::iterator,DVInputIterator>::value) &&
				   !(std::is_base_of<typename device_vector<typename std::iterator_traits<DVOutputIterator>::value_type>::iterator,DVOutputIterator>::value)
				   ,void >::type
copy_pick_iterator(bolt::amp::control &ctrl,  const DVInputIterator& first, const Size& n,
    const DVOutputIterator& result)
{

    typedef typename std::iterator_traits<DVInputIterator>::value_type iType;
    typedef typename std::iterator_traits<DVOutputIterator>::value_type oType;

    bolt::amp::control::e_RunMode runMode = ctrl.getForceRunMode( );
	 
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
        copy_pick_iterator( ctrl, first, n, result);
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

        copy_pick_iterator( ctrl, first, n, result); 
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

}//End OF detail namespace



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

}//end of amp namespace
};//end of bolt namespace

#endif
