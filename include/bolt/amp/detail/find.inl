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
// AMP REDUCE
//////////////////////////////////////////////////////////////////////////////

#if !defined( BOLT_AMP_FIND_INL )
#define BOLT_AMP_FIND_INL
#define TRANSFORM_WAVEFRNT_SIZE 256

#ifdef BOLT_ENABLE_PROFILING
#include "bolt/AsyncProfiler.h"
//AsyncProfiler aProfiler("transform");
#endif


#ifdef ENABLE_TBB
//TBB Includes
#include <bolt/btbb/find.h>

#endif

#include <algorithm>
#include <type_traits>
#include <bolt/amp/bolt.h>
#include <bolt/amp/device_vector.h>
#include <bolt/amp/iterator/iterator_traits.h>
#include "bolt/amp/iterator/addressof.h"
#include <bolt/amp/min_element.h>
#include <bolt/amp/functional.h>


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace bolt{
namespace amp{
namespace detail{

namespace serial{

	template<typename InputIterator, typename Predicate>
    InputIterator find_if(bolt::amp::control &ctl,
						  InputIterator& first,
						  InputIterator& last,
						  Predicate& pred
						  )
    {
		  const int szElements = static_cast< int >( std::distance( first, last ) );
		  auto mapped_first_itr = create_mapped_iterator(typename std::iterator_traits<InputIterator>::iterator_category(), 
                                                        ctl, first);

		  auto itr  =  std::find_if(mapped_first_itr, mapped_first_itr+szElements, pred);
		  int index =  static_cast< int >( std::distance( mapped_first_itr, itr ) ); 
		  return first + index;
    }

} // end of namespace serial

#ifdef ENABLE_TBB
namespace btbb{

    template<typename InputIterator, typename Predicate>
    InputIterator find_if(bolt::amp::control &ctl,
						  InputIterator& first,
						  InputIterator& last,
						  Predicate& pred
						  )
    {
		const int szElements = static_cast< int >( std::distance( first, last ) );
		auto mapped_first_itr = create_mapped_iterator(typename std::iterator_traits<InputIterator>::iterator_category(), 
                                                        ctl, first);
        auto itr = bolt::btbb::find_if(  mapped_first_itr, mapped_first_itr+szElements, pred);
		int index = static_cast< int >( std::distance( mapped_first_itr, itr ) ); 
		return first + index;
	  
    }
} // end of namespace btbb
#endif

namespace amp{

	template<typename DVInputIterator, typename Predicate>
    DVInputIterator find_if(bolt::amp::control &ctl,
						   DVInputIterator& first,
						   DVInputIterator& last,
						   Predicate& pred
						   )
    {
		    typedef typename std::iterator_traits< DVInputIterator >::value_type iType;
			const int szElements = static_cast< int >( std::distance( first, last ) );
			concurrency::accelerator_view av = ctl.getAccelerator().default_view;

			const unsigned int leng =  szElements + TRANSFORM_WAVEFRNT_SIZE - (szElements % TRANSFORM_WAVEFRNT_SIZE);
			concurrency::extent< 1 > inputExtent(leng);
			device_vector< int, concurrency::array > index( szElements, 0, true, ctl );

			auto dvInput1_itr  = bolt::amp::create_mapped_iterator(
									typename bolt::amp::iterator_traits< DVInputIterator >::iterator_category( ),
									first, szElements, false, ctl );


			try
			{
				concurrency::parallel_for_each(av,  inputExtent, [=](concurrency::index<1> idx) restrict(amp)
				{
					int globalId = idx[ 0 ];
					if( globalId >= szElements)
						return;
					if(pred( dvInput1_itr[globalId]))
						index[globalId] = globalId;
					else
						index[globalId] = szElements;
				});
			}
			catch(std::exception &e)
			{
				  std::cout << "Exception while calling bolt::amp::find parallel_for_each"<<e.what()<<std::endl;
				  return first;
			}
			device_vector< int, concurrency::array>::iterator itr = bolt::amp::min_element( ctl, index.begin(), index.end());
			return first + itr[0];
    }
   
} //end of namespace amp


    /*! \brief This template function overload is used strictly for device vectors and std random access vectors. 
        \detail Here we branch out into the SerialCpu, MultiCore TBB or The AMP code paths. 
    */
    template<typename InputIterator, typename Predicate>
    InputIterator find_if(bolt::amp::control &ctl,
						 InputIterator& first,
						 InputIterator& last,
						 Predicate& pred
						 )
    {
        int sz = static_cast<int>( std::distance(first, last ) );
        if (sz == 0)
            return first;

        bolt::amp::control::e_RunMode runMode = ctl.getForceRunMode();  // could be dynamic choice some day.
        if(runMode == bolt::amp::control::Automatic)
        {
           runMode = ctl.getDefaultPathToRun();
        }
    
        if( runMode == bolt::amp::control::SerialCpu )
        {
            return serial::find_if(ctl, first, last, pred);
        }
        else if( runMode == bolt::amp::control::MultiCoreCpu )
        {
#if defined( ENABLE_TBB )
            return btbb::find_if(ctl, first, last, pred);
#else
            throw std::runtime_error( "The MultiCoreCpu version of find is not enabled to be built! \n" );
#endif
        }
        else
        {
            return amp::find_if(ctl, first, last, pred );
        }
        return first;
    }


};//end of namespace detail


		 template <typename InputIterator, typename Predicate>
		 InputIterator find_if(bolt::amp::control &ctl,
							   InputIterator first,
							   InputIterator last,
							   Predicate pred)
		 {
			    return detail::find_if(ctl, first, last, pred);
		 }

         template <typename InputIterator, typename Predicate>
		 InputIterator find_if(InputIterator first,
							   InputIterator last,
							   Predicate pred)
		 {
				return find_if(bolt::amp::control::getDefault(), first, last, pred);
		 }

		 

		 template <typename InputIterator, typename T>
		 InputIterator find(bolt::amp::control &ctl,
							InputIterator first,
				            InputIterator last,
						    const T& value)
		 {
			 return find_if(ctl, first, last, bolt::amp::equal_to_value<T>(value));
		 }
		 template <typename InputIterator, typename T>
		 InputIterator find(InputIterator first,
				            InputIterator last,
						    const T& value)
		 {
			 return find(bolt::amp::control::getDefault(), first, last, value);
		 }
		 

		 template <typename InputIterator, typename Predicate>
		 InputIterator find_if_not(bolt::amp::control &ctl,
								   InputIterator first,
								   InputIterator last,
								   Predicate pred)
		 {
			 typedef typename std::iterator_traits< InputIterator >::value_type iType;
			 return find_if(ctl, first, last, bolt::amp::not_pred<Predicate, iType>(pred));

		 }
		 template <typename InputIterator, typename Predicate>
		 InputIterator find_if_not(InputIterator first,
								   InputIterator last,
								   Predicate pred)
		 {
			 return find_if_not(bolt::amp::control::getDefault(), first, last, pred);
		 }
		 



    }; //end of namespace amp
}; //end of namespace bolt

#endif // AMP_FIND_INL