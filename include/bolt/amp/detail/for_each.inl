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
// AMP ForEach
//////////////////////////////////////////////////////////////////////////////

#pragma once
#if !defined( BOLT_AMP_FOR_EACH_INL )
#define BOLT_AMP_FOR_EACH_INL
#define TRANSFORM_WAVEFRNT_SIZE 256

#include <algorithm>
#include <type_traits>
#include "bolt/amp/bolt.h"
#include "bolt/amp/device_vector.h"
#include "bolt/amp/iterator/iterator_traits.h"
#include "bolt/amp/iterator/addressof.h"

#ifdef ENABLE_TBB
    #include "bolt/btbb/for_each.h"
#endif


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace bolt
{
namespace amp
{
namespace detail
{
            
namespace serial{


    template<typename InputIterator, typename Size, typename UnaryFunction>
    void for_each_n(  control& ctl,  InputIterator& first,  Size& n,  UnaryFunction& f, std::random_access_iterator_tag )
    {
        std::for_each(first, first+n, f);
    }

    template<typename InputIterator, typename Size, typename UnaryFunction>
    void for_each_n(  control& ctl,  InputIterator& first,  Size& n,  UnaryFunction& f,  bolt::amp::device_vector_tag )
    {

		auto mapped_first_itr = create_mapped_iterator(typename std::iterator_traits<InputIterator>::iterator_category(), ctl, first);
		std::for_each(mapped_first_itr, mapped_first_itr+n, f);  
    }

	template<typename InputIterator, typename Size, typename UnaryFunction>
    void for_each_n(  control& ctl,  InputIterator& first,  Size& n,  UnaryFunction& f,  bolt::amp::fancy_iterator_tag )
	{
		std::for_each(first, first+n, f);  
    }

}//end of namespace serial

#ifdef ENABLE_TBB
namespace btbb{
    
    template<typename InputIterator, typename Size, typename UnaryFunction>
	void for_each_n( control& ctl, InputIterator& first,  Size& n,  UnaryFunction& f,  bolt::amp::device_vector_tag ) 
    {

		auto mapped_first_itr = create_mapped_iterator(typename std::iterator_traits<InputIterator>::iterator_category(), 
                                                        ctl, first);
        bolt::btbb::for_each_n(mapped_first_itr,  n,  f);
    }

	template<typename InputIterator, typename Size, typename UnaryFunction>
    void for_each_n( control& ctl, InputIterator& first,  Size& n,  UnaryFunction& f, std::random_access_iterator_tag )
    {
        bolt::btbb::for_each_n(first, n, f);
    }

	template<typename InputIterator, typename Size, typename UnaryFunction>
    void for_each_n( control& ctl,  InputIterator& first,  Size& n,  UnaryFunction& f,  bolt::amp::fancy_iterator_tag )
    {	
		bolt::btbb::for_each_n(first, n, f);
    }

}//end of namespace btbb
#endif

namespace amp{

	template<typename DVInputIterator, typename Size, typename UnaryFunction>
    void for_each_n( control& ctl, DVInputIterator first,  Size& n,  UnaryFunction& f, bolt::amp::device_vector_tag)
    {

        concurrency::accelerator_view av = ctl.getAccelerator().default_view;

        const unsigned int leng =  n + TRANSFORM_WAVEFRNT_SIZE - (n % TRANSFORM_WAVEFRNT_SIZE);

        concurrency::extent< 1 > inputExtent(leng);

        try
        {

            concurrency::parallel_for_each(av,  inputExtent, [=](concurrency::index<1> idx) restrict(amp)
            {
                int globalId = idx[ 0 ];

				if(globalId >= n)
					return;
                f(first[globalId]);
            });

        }
		
		catch(std::exception &e)
        {

               std::cout << "Exception while calling bolt::amp::transform parallel_for_each"<<e.what()<<std::endl;
			   std::cout<< e.what() << std::endl;
			   throw std::exception();
        }

		return; 
    }




	/*! \brief This template function overload is used strictly for std random access vectors and AMP implementations. 
        \detail 
    */
    template<typename InputIterator, typename Size, typename UnaryFunction>
    void for_each_n(control& ctl,  InputIterator& first,  Size& n,  UnaryFunction& f, std::random_access_iterator_tag )
    {
		
		typedef typename std::iterator_traits<InputIterator>::value_type  iType;
		device_vector< iType, concurrency::array_view > dvInput( first, n, false, ctl );
		for_each_n(ctl, dvInput.begin(), n, f, bolt::amp::device_vector_tag());
		dvInput.data();


    }

	template<typename InputIterator, typename Size, typename UnaryFunction>
    void for_each_n(control& ctl,  InputIterator& first,  Size& n,  UnaryFunction& f, bolt::amp::fancy_iterator_tag )
    {
		
		for_each_n(ctl, first, n, f, bolt::amp::device_vector_tag());

    }




}//namespace amp


	/*! \brief This template function overload is used strictly for device vectors and std random access vectors. 
        \detail Here we branch out into the SerialCpu, MultiCore TBB or The AMP code paths. 
    */
    template<typename InputIterator, typename Size, typename UnaryFunction >  
    typename std::enable_if< 
             !(std::is_same< typename std::iterator_traits< InputIterator>::iterator_category, 
                             std::input_iterator_tag 
                           >::value), InputIterator
                           >::type
    for_each_n  ( control& ctl,  InputIterator  &first,   Size &n,  UnaryFunction &f)
    {
        if (n == 0)
            return first;

        bolt::amp::control::e_RunMode runMode = ctl.getForceRunMode();  // could be dynamic choice some day.
        if(runMode == bolt::amp::control::Automatic)
        {
           runMode = ctl.getDefaultPathToRun();
        }
		
        if( runMode == bolt::amp::control::SerialCpu )
        {
            serial::for_each_n(ctl, first, n, f, typename std::iterator_traits<InputIterator>::iterator_category() );
			return first;
        }
        else if( runMode == bolt::amp::control::MultiCoreCpu )
        {
#if defined( ENABLE_TBB )
          
            btbb::for_each_n(ctl, first, n, f, typename std::iterator_traits<InputIterator>::iterator_category());
			return first;
#else
            throw std::runtime_error( "The MultiCoreCpu version of for_each_n is not enabled to be built! \n" );
#endif
            return first;
        }
        else
        {
            amp::for_each_n( ctl, first, n,  f, typename bolt::amp::iterator_traits<InputIterator>::iterator_category());
			return first;
        }  

		return first;
    }
    

}//end of namespace detail


        //////////////////////////////////////////
        //  ForEach overloads
        //////////////////////////////////////////
        // default control
        template<typename InputIterator , typename UnaryFunction >   
        InputIterator  for_each (control &ctl, InputIterator first, InputIterator last, UnaryFunction f)
        {
			  const int n =  static_cast< int >( std::distance( first, last ) );
              return for_each_n( ctl, first, n, f );

        }


        template<typename InputIterator , typename UnaryFunction >   
        InputIterator for_each (InputIterator first, InputIterator last, UnaryFunction f)
        {
			  const int n =  static_cast< int >( std::distance( first, last ) );
              return for_each_n( control::getDefault(), first, n, f);
        }

        // default control
        template<typename InputIterator , typename Size , typename UnaryFunction >  
        InputIterator for_each_n  ( control &ctl, InputIterator  first,  Size  n,  UnaryFunction  f)
        {
              return detail::for_each_n( ctl, first, n, f);
        }

    
        template<typename InputIterator , typename Size , typename UnaryFunction >  
        InputIterator for_each_n  (InputIterator  first,  Size  n,  UnaryFunction  f)
        {
              return for_each_n( control::getDefault(), first, n,  f );
        }


    } //end of namespace amp
} //end of namespace bolt

#endif // AMP_FOR_EACH_INL