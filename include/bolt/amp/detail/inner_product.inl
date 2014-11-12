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

/*
TODO:
1. Optimize the code. In this version transform and reduce are called directly which performs better.
2. Found a caveat in Multi-GPU scenario (Evergreen+Tahiti). Which basically applies to most of the routines.
*/

#if !defined( BOLT_AMP_INNERPRODUCT_INL )
#define BOLT_AMP_INNERPRODUCT_INL


#pragma once


#include <type_traits>
#include <bolt/amp/detail/reduce.inl>
#include <bolt/amp/detail/transform.inl>
#include "bolt/amp/device_vector.h"
#include "bolt/amp/bolt.h"

//TBB Includes
#ifdef ENABLE_TBB
#include "bolt/btbb/inner_product.h"
#endif

namespace bolt {
    namespace amp {

namespace detail {

namespace serial
{

	template< typename InputIterator, typename OutputType, typename BinaryFunction1,typename BinaryFunction2>
    OutputType inner_product(bolt::amp::control &ctl, InputIterator& first1,
        InputIterator& last1, InputIterator& first2, OutputType& init,
        BinaryFunction1& f1, BinaryFunction2& f2, 
		bolt::amp::device_vector_tag)
    {
		 typedef typename std::iterator_traits<InputIterator>::value_type iType1;
         typename bolt::amp::device_vector< iType1 >::pointer firstPtr =  first1.getContainer( ).data( );
         typename bolt::amp::device_vector< iType1 >::pointer first2Ptr =  first2.getContainer( ).data( );

		 int sz = static_cast< int >(last1 - first1);
                if (sz == 0)
                    return init;

         #if defined( _WIN32 )
            return std::inner_product(  &firstPtr[ first1.m_Index ],
                                     &firstPtr[ last1.m_Index ],
                                     stdext::make_checked_array_iterator( &first2Ptr[ first2.m_Index ], sz),
                                     init, f1, f2);
         #else
            return std::inner_product(  &firstPtr[ first1.m_Index ],
                                     &firstPtr[ last1.m_Index ],
                                     &first2Ptr[ first2.m_Index ], init, f1, f2);
         #endif

    }

	template<typename InputIterator, typename OutputType, typename BinaryFunction1,typename BinaryFunction2>
    OutputType inner_product(bolt::amp::control &ctl,  InputIterator& first1,
                InputIterator& last1, InputIterator& first2, OutputType& init,
                BinaryFunction1& f1, BinaryFunction2& f2, const std::string& user_code,
                bolt::amp::fancy_iterator_tag )
    {
		return inner_product_enqueue(ctl, first1, last1, first2, init, f1, f2);
	}


	template<typename InputIterator, typename OutputType, typename BinaryFunction1,typename BinaryFunction2>
    OutputType inner_product(bolt::amp::control &ctl,  InputIterator& first1,
                InputIterator& last1, InputIterator& first2, OutputType& init,
                BinaryFunction1& f1, BinaryFunction2& f2,
                std::random_access_iterator_tag )
    {	
		
		return inner_product_enqueue(ctl, first1, last1, first2, init, f1, f2);
	}

	template<typename InputIterator, typename OutputType, typename BinaryFunction1,typename BinaryFunction2>
    OutputType inner_product_enqueue(bolt::amp::control &ctl,  InputIterator& first1,
                InputIterator& last1, InputIterator& first2, OutputType& init,
                BinaryFunction1& f1, BinaryFunction2& f2)
	{

		size_t sz = (last1 - first1);
        if (sz == 0)
            return init;
        OutputType accumulator = init;
        for(int index=0; index < (int)(sz); index++)
        {
            accumulator = f1( accumulator, f2(*(first1+index), *(first2+index)) );
        }
        return accumulator;
	}

}//end of namespace serial


#ifdef ENABLE_TBB
namespace btbb
{

	template< typename InputIterator, typename OutputType, typename BinaryFunction1,typename BinaryFunction2>
    OutputType inner_product(bolt::amp::control &ctl, InputIterator& first1,
        InputIterator& last1, InputIterator& first2, OutputType& init,
        BinaryFunction1& f1, BinaryFunction2& f2, 
		bolt::amp::device_vector_tag)
    {
		 typedef typename std::iterator_traits<InputIterator>::value_type iType1;
         typename bolt::amp::device_vector< iType1 >::pointer firstPtr =  first1.getContainer( ).data( );
         typename bolt::amp::device_vector< iType1 >::pointer first2Ptr =  first2.getContainer( ).data( );

		 int sz = static_cast< int >(last1 - first1);
                if (sz == 0)
                    return init;

         return bolt::btbb::inner_product(  &firstPtr[ first1.m_Index ],  &firstPtr[ last1.m_Index ],
                                                &first2Ptr[ first2.m_Index ], init, f1, f2);

    }

	template<typename InputIterator, typename OutputType, typename BinaryFunction1,typename BinaryFunction2>
    OutputType inner_product(bolt::amp::control &ctl,  InputIterator& first1,
                InputIterator& last1, InputIterator& first2, OutputType& init,
                BinaryFunction1& f1, BinaryFunction2& f2, const std::string& user_code,
                bolt::amp::fancy_iterator_tag )
    {
		 return bolt::btbb::inner_product(first1, last1, first2, init, f1, f2);
	}


	template<typename InputIterator, typename OutputType, typename BinaryFunction1,typename BinaryFunction2>
    OutputType inner_product(bolt::amp::control &ctl,  InputIterator& first1,
                InputIterator& last1, InputIterator& first2, OutputType& init,
                BinaryFunction1& f1, BinaryFunction2& f2,
                std::random_access_iterator_tag )
    {	
		
		return bolt::btbb::inner_product(first1, last1, first2, init, f1, f2);
	}

	

}
//end of namespace btbb
#endif


namespace amp{

    template< typename DVInputIterator, typename OutputType, typename BinaryFunction1,typename BinaryFunction2>
    OutputType inner_product(bolt::amp::control &ctl,  DVInputIterator& first1,
        DVInputIterator& last1,  DVInputIterator& first2, OutputType& init,
        BinaryFunction1& f1,  BinaryFunction2& f2, bolt::amp::device_vector_tag)
    {

        typedef typename std::iterator_traits<DVInputIterator>::value_type iType;

        const int distVec = static_cast< int >( std::distance( first1, last1 ) );

        if( distVec == 0 )
            return init;

        device_vector< iType> tempDV( distVec, iType(), false, ctl);

        detail::amp::binary_transform( ctl, first1, last1, first2, tempDV.begin() ,f2);
        return detail::reduce( ctl, tempDV.begin(), tempDV.end(), init, f1);

    };

	template<typename InputIterator, typename OutputType, typename BinaryFunction1,typename BinaryFunction2>
    OutputType inner_product(bolt::amp::control &ctl,  InputIterator& first1,
                InputIterator& last1, InputIterator& first2, OutputType& init,
                BinaryFunction1& f1, BinaryFunction2& f2,
                std::random_access_iterator_tag )
    {
		
		int sz = static_cast<int>(last1 - first1);

        typedef typename std::iterator_traits<InputIterator>::value_type  iType;
              
        // Use host pointers memory since these arrays are only read once - no benefit to copying.
        // Map the input iterator to a device_vector

        device_vector< iType, concurrency::array_view> dvInput( first1, last1, false, ctl);
        device_vector< iType, concurrency::array_view> dvInput2( first2, sz, false, ctl);

        return inner_product( ctl, dvInput.begin( ), dvInput.end( ), dvInput2.begin( ),
                                                   init, f1, f2, bolt::amp::device_vector_tag() );

	}


	template<typename InputIterator, typename OutputType, typename BinaryFunction1,typename BinaryFunction2>
    OutputType inner_product(bolt::amp::control &ctl,  InputIterator& first1,
                InputIterator& last1, InputIterator& first2, OutputType& init,
                BinaryFunction1& f1, BinaryFunction2& f2, 
                bolt::amp::fancy_iterator_tag )
    {
		return inner_product( ctl, first1, last1, first2, init, f1, f2, bolt::amp::device_vector_tag() );
    }

}// end of namespace amp

	template<typename InputIterator, typename OutputType, typename BinaryFunction1, typename BinaryFunction2>
    typename std::enable_if< 
           !(std::is_same< typename std::iterator_traits< InputIterator>::iterator_category, 
                         std::input_iterator_tag 
                       >::value), OutputType
                       >::type
    inner_product( bolt::amp::control& ctl, InputIterator& first1,
                InputIterator& last1, InputIterator& first2, OutputType& init,
                BinaryFunction1 f1, BinaryFunction2 f2 )
    {
        typedef typename std::iterator_traits<InputIterator>::value_type iType;
        int sz = static_cast<int>( std::distance( first1, last1 ) );

        if( sz == 0 )
            return init;

        bolt::amp::control::e_RunMode runMode = ctl.getForceRunMode();  // could be dynamic choice some day.
        if(runMode == bolt::amp::control::Automatic)
        {
             runMode = ctl.getDefaultPathToRun();
        }
        
        if( runMode == bolt::amp::control::SerialCpu)
        {
            return serial::inner_product( ctl, first1, last1, first2, init,
				f1, f2, typename std::iterator_traits<InputIterator>::iterator_category()  );

        }
        else if(runMode == bolt::amp::control::MultiCoreCpu)
        {
            #ifdef ENABLE_TBB
                   return btbb::inner_product(ctl, first1, last1, first2,
					   init, f1, f2, typename std::iterator_traits<InputIterator>::iterator_category());
            #else
                   throw std::runtime_error("MultiCoreCPU Version of inner_product is not Enabled! \n");
            #endif
        }
        else
        {
            return amp::inner_product( ctl, first1, last1, first2, init,
				f1, f2, typename std::iterator_traits<InputIterator>::iterator_category() );
        } 
    }
    

    template<typename InputIterator, typename OutputType, typename BinaryFunction1, typename BinaryFunction2>
    typename std::enable_if< 
           (std::is_same< typename std::iterator_traits< InputIterator>::iterator_category, 
                         std::input_iterator_tag 
                       >::value), OutputType
                       >::type
    inner_product( bolt::amp::control& ctl, InputIterator& first1,
                InputIterator& last1, InputIterator& first2, OutputType& init,
                BinaryFunction1 f1, BinaryFunction2 f2 )
    {
         //TODO - Shouldn't we support inner_product for input_iterator_tag also. 
         static_assert( std::is_same< typename std::iterator_traits< InputIterator>::iterator_category, 
                                           std::input_iterator_tag >::value , 
                             "Input vector cannot be of the type input_iterator_tag" );
    }



}//End of detail namespace


        // default control, two-input transform, std:: iterator
        template<typename InputIterator, typename OutputType, typename BinaryFunction1, typename BinaryFunction2>
         OutputType inner_product(bolt::amp::control& ctl, InputIterator first1, InputIterator last1,
         InputIterator first2, OutputType init, BinaryFunction1 f1, BinaryFunction2 f2)
        {
           return detail::inner_product( ctl, first1, last1, first2, init, f1, f2);
        }

        // default control, two-input transform, std:: iterator
        template<typename InputIterator, typename OutputType, typename BinaryFunction1, typename BinaryFunction2>
        OutputType inner_product( InputIterator first1, InputIterator last1, InputIterator first2, OutputType init,
            BinaryFunction1 f1, BinaryFunction2 f2)
        {
            return inner_product( control::getDefault(), first1, last1, first2, init, f1, f2 );
        }

        template<typename InputIterator, typename OutputType>
        OutputType inner_product(bolt::amp::control& ctl,InputIterator first1,InputIterator last1,InputIterator first2,
            OutputType init )
        {
            typedef typename std::iterator_traits<InputIterator>::value_type iType;
            return detail::inner_product(ctl, first1,last1,first2,init,bolt::amp::plus< iType >( ),
                bolt::amp::multiplies< iType >( ));
        }

        // default control, two-input transform, std:: iterator
        template<typename InputIterator, typename OutputType>
        OutputType inner_product( InputIterator first1, InputIterator last1, InputIterator first2, OutputType init)
        {
            typedef typename std::iterator_traits<InputIterator>::value_type iType;
            return inner_product( control::getDefault(), first1, last1, first2, init,
                bolt::amp::plus< iType >( ), bolt::amp::multiplies< iType >( ));
        }


    }//end of amp namespace
};//end of bolt namespace



#endif
