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
// AMP Unique
//////////////////////////////////////////////////////////////////////////////

#pragma once
#if !defined( BOLT_AMP_UNIQUE_INL )
#define BOLT_AMP_UNIQUE_INL

#include "bolt/amp/transform.h"
#include "bolt/amp/copy.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace bolt
{
namespace amp
{

namespace detail{

namespace serial{

	template<typename InputIterator, typename OutputIterator, typename BinaryPredicate>
    OutputIterator unique_copy(control &ctl,  
                           InputIterator first,
                           InputIterator last,
                           OutputIterator output,
                           BinaryPredicate binary_pred)
    {
		    typedef typename std::iterator_traits<InputIterator>::value_type iType;
            
			// create temporary storage for an intermediate result
			std::vector<iType> temp(first, last);

			int sz = static_cast<int>( std::distance(first, last ) );
			std::vector<int> stencil(sz);
			// mark first element in each group
            stencil[0] = 1; 

			auto dvInput_itr = create_mapped_iterator(typename std::iterator_traits<InputIterator>::iterator_category(), 
                                                        ctl, first);
			auto dvLast_itr = create_mapped_iterator(typename std::iterator_traits<InputIterator>::iterator_category(), 
                                                        ctl, last);
			auto dvOutput_itr = create_mapped_iterator(typename std::iterator_traits<OutputIterator>::iterator_category(), 
                                                        ctl, output);

			//bolt::amp::detail::serial::binary_transform(ctl, dvInput_itr , dvLast_itr - 1, dvInput_itr + 1, stencil.begin() + 1, bolt::amp::not2(binary_pred)); 
			bolt::amp::transform(ctl, dvInput_itr , dvLast_itr - 1, dvInput_itr + 1, stencil.begin() + 1, bolt::amp::not2(binary_pred)); 

            auto return_itr = bolt::amp::detail::serial::copy_if(ctl, dvInput_itr , dvLast_itr, stencil.begin(), dvOutput_itr, bolt::amp::identity<int>());

			return output + (return_itr - dvOutput_itr);
    }


	template<typename ForwardIterator, typename BinaryPredicate>
    ForwardIterator unique(control &ctl, 
                      ForwardIterator first,
                      ForwardIterator last,
                      BinaryPredicate binary_pred)
	{

		typedef typename std::iterator_traits<ForwardIterator>::value_type iType;

        std::vector<iType> input(first, last);
      
		auto mapped_first_itr = create_mapped_iterator(typename std::iterator_traits<ForwardIterator>::iterator_category(), 
                                                       ctl, first);
        auto return_itr = bolt::amp::detail::serial::unique_copy(ctl, input.begin(), input.end(), mapped_first_itr , binary_pred);
		return first + (return_itr - mapped_first_itr);
    }


} // end of namespace serial

#ifdef ENABLE_TBB
namespace btbb{

	template<typename InputIterator, typename OutputIterator, typename BinaryPredicate>
    OutputIterator unique_copy(control &ctl,  
                           InputIterator first,
                           InputIterator last,
                           OutputIterator output,
                           BinaryPredicate binary_pred)
    {
		    typedef typename std::iterator_traits<InputIterator>::value_type iType;
            
			// create temporary storage for an intermediate result
			std::vector<iType> temp(first, last);

			int sz = static_cast<int>( std::distance(first, last ) );
			std::vector<int> stencil(sz);
			// mark first element in each group
            stencil[0] = 1; 

			auto dvInput_itr = create_mapped_iterator(typename std::iterator_traits<InputIterator>::iterator_category(), 
                                                        ctl, first);
			auto dvLast_itr = create_mapped_iterator(typename std::iterator_traits<InputIterator>::iterator_category(), 
                                                        ctl, last);
			auto dvOutput_itr = create_mapped_iterator(typename std::iterator_traits<OutputIterator>::iterator_category(), 
                                                        ctl, output);

			//bolt::amp::detail::btbb::binary_transform(ctl, dvInput_itr , dvLast_itr - 1, dvInput_itr + 1, stencil.begin() + 1, bolt::amp::not2(binary_pred)); 
			bolt::amp::transform(ctl, dvInput_itr , dvLast_itr - 1, dvInput_itr + 1, stencil.begin() + 1, bolt::amp::not2(binary_pred)); 

            auto return_itr = bolt::amp::detail::btbb::copy_if(ctl, dvInput_itr , dvLast_itr, stencil.begin(), dvOutput_itr, bolt::amp::identity<int>());

			return output + (return_itr - dvOutput_itr);
    }

	template<typename ForwardIterator, typename BinaryPredicate>
    ForwardIterator unique(control &ctl, 
                      ForwardIterator first,
                      ForwardIterator last,
                      BinaryPredicate binary_pred)
	{

		typedef typename std::iterator_traits<ForwardIterator>::value_type iType;

        std::vector<iType> input(first, last);
      
		auto mapped_first_itr = create_mapped_iterator(typename std::iterator_traits<ForwardIterator>::iterator_category(), 
                                                       ctl, first);
        auto return_itr = bolt::amp::detail::serial::unique_copy(ctl, input.begin(), input.end(), mapped_first_itr, binary_pred);
		return first + (return_itr - mapped_first_itr);
    }


} // end of namespace btbb
#endif

namespace amp{


	    template<typename InputIterator, typename OutputIterator, typename BinaryPredicate>
        OutputIterator unique_copy(control &ctl,  
                           InputIterator first,
                           InputIterator last,
                           OutputIterator output,
                           BinaryPredicate binary_pred)
		{
			 // empty sequence
             if(first == last)
               return output;
             
			 typename std::iterator_traits<InputIterator>::difference_type sz = (last - first);

			 std::vector<int> stencil_t(sz);

			 bolt::amp::device_vector<int> stencil(sz, 0, false, ctl);
             
             // mark first element in each group
             stencil[0] = 1; 

			 auto dvInput_itr   = bolt::amp::create_mapped_iterator(
                                    typename bolt::amp::iterator_traits< InputIterator  >::iterator_category( ), 
                                    first, sz, false, ctl );
			 auto dvOutput_itr   = bolt::amp::create_mapped_iterator(
                                    typename bolt::amp::iterator_traits< OutputIterator >::iterator_category( ), 
                                    output, sz, false, ctl );

             bolt::amp::transform(ctl, dvInput_itr , dvInput_itr + sz - 1, dvInput_itr + 1, stencil.begin() + 1, bolt::amp::not2(binary_pred)); 

             auto return_itr = bolt::amp::copy_if(ctl, dvInput_itr , dvInput_itr + sz, stencil.begin(), dvOutput_itr, bolt::amp::identity<int>());
			 return output + (return_itr - dvOutput_itr);

		}

		template<typename ForwardIterator, typename BinaryPredicate>
        ForwardIterator unique(control &ctl, 
                      ForwardIterator first,
                      ForwardIterator last,
                      BinaryPredicate binary_pred)
		{
			typedef typename std::iterator_traits<ForwardIterator>::value_type iType;
			typename std::iterator_traits<ForwardIterator>::difference_type sz = (last - first);
  
			auto dvInput_itr   = bolt::amp::create_mapped_iterator(
                                    typename bolt::amp::iterator_traits< ForwardIterator  >::iterator_category( ), 
                                    first, sz, false, ctl );

			// create temporary storage for an intermediate result
			bolt::amp::device_vector<iType> input(dvInput_itr, dvInput_itr+sz, false, ctl);

            auto return_itr = bolt::amp::detail::amp::unique_copy(ctl, input.begin(), input.end(), dvInput_itr , binary_pred);
			return first + (return_itr - dvInput_itr);
		}

} //end of namespace amp

/*! \brief This template function overload is used strictly for device vectors and std random access vectors. 
        \detail Here we branch out into the SerialCpu, MultiCore TBB or The AMP code paths. 
    */
    template<typename InputIterator, typename OutputIterator, typename BinaryPredicate>
    OutputIterator unique_copy(control &ctl,  
                           InputIterator first,
                           InputIterator last,
                           OutputIterator output,
                           BinaryPredicate binary_pred)
    {
        int sz = static_cast<int>( std::distance(first, last ) );
        if (sz == 0)
            return output;

        bolt::amp::control::e_RunMode runMode = ctl.getForceRunMode();  // could be dynamic choice some day.
        if(runMode == bolt::amp::control::Automatic)
        {
           runMode = ctl.getDefaultPathToRun();
        }
    
        if( runMode == bolt::amp::control::SerialCpu )
        {
            return serial::unique_copy(ctl, first, last, output, binary_pred);
			
        }
        else if( runMode == bolt::amp::control::MultiCoreCpu )
        {
#if defined( ENABLE_TBB )
            return btbb::unique_copy(ctl, first, last, output, binary_pred);
#else
            throw std::runtime_error( "The MultiCoreCpu version of find is not enabled to be built! \n" );
#endif
        }
        else
        {
            return amp::unique_copy(ctl, first, last, output, binary_pred );
        }
        return output;
    }



	template<typename ForwardIterator, typename BinaryPredicate>
    ForwardIterator unique(control &ctl, 
                      ForwardIterator first,
                      ForwardIterator last,
                      BinaryPredicate binary_pred)

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
            return serial::unique(ctl, first, last, binary_pred);
			
        }
        else if( runMode == bolt::amp::control::MultiCoreCpu )
        {
#if defined( ENABLE_TBB )
            return btbb::unique(ctl, first, last, binary_pred);
#else
            throw std::runtime_error( "The MultiCoreCpu version of find is not enabled to be built! \n" );
#endif
        }
        else
        {
            return amp::unique(ctl, first, last, binary_pred );
        }
        return first;

	}

};//end of namespace detail

        //////////////////////////////////////////
        //  Unique overloads
        ///////////////////////////////////////// 

	    
        // binary_negate does not need to know first_argument_type or second_argument_type
        template <typename Predicate>
        struct binary_negate
        {
            typedef bool result_type;
        
            Predicate pred;
        
            binary_negate(const Predicate& pred) : pred(pred) {}
        
            template <typename T1, typename T2>
            bool operator()(const T1& x, const T2& y) const restrict(amp,cpu)
            {
                if(pred(x,y))
					return false;
				else
					return true;
            }
        };
        
       
        
        template<typename Predicate>
        bolt::amp::binary_negate<Predicate> not2(const Predicate &pred)
        {
            return bolt::amp::binary_negate<Predicate>(pred);
        }
	


		template<typename InputIterator, typename OutputIterator, typename BinaryPredicate>
        OutputIterator unique_copy(control &ctl,  
                           InputIterator first,
                           InputIterator last,
                           OutputIterator output,
                           BinaryPredicate binary_pred)
		{
			 return detail::unique_copy(ctl, first, last, output, binary_pred);
		}


		template<typename InputIterator, typename OutputIterator, typename BinaryPredicate>
        OutputIterator unique_copy(InputIterator first,
                           InputIterator last,
                           OutputIterator output,
                           BinaryPredicate binary_pred)
		{
			return unique_copy(control::getDefault(), first, last, output, binary_pred);
		}


	    template<typename InputIterator, typename OutputIterator>
        OutputIterator unique_copy(control &ctl, 
                           InputIterator first,
                           InputIterator last,
                           OutputIterator output)
		{
			typedef typename std::iterator_traits<InputIterator>::value_type iType;
            return unique_copy(ctl, first,last,output,bolt::amp::equal_to<iType>());
        }

		template<typename InputIterator, typename OutputIterator>
        OutputIterator unique_copy(InputIterator first,
                           InputIterator last,
                           OutputIterator output)
		{
			return unique_copy(control::getDefault(), first, last, output);
		}

		template<typename ForwardIterator, typename BinaryPredicate>
        ForwardIterator unique(control &ctl, 
                      ForwardIterator first,
                      ForwardIterator last,
                      BinaryPredicate binary_pred)
		{
			 return detail::unique(ctl, first, last, binary_pred);
		}
		
		template<typename ForwardIterator, typename BinaryPredicate>
        ForwardIterator unique(ForwardIterator first,
                       ForwardIterator last,
                       BinaryPredicate binary_pred)
		{
			return unique(control::getDefault(), first, last, binary_pred);
		}


		template<typename ForwardIterator>
        ForwardIterator unique(control &ctl, ForwardIterator first, ForwardIterator last)
		{
			typedef typename std::iterator_traits<ForwardIterator>::value_type iType;
            return unique(ctl, first, last, bolt::amp::equal_to<iType>());
		}

		template<typename ForwardIterator>
        ForwardIterator unique( ForwardIterator first, ForwardIterator last)
		{
			return unique(control::getDefault(), first, last);
		}

    } //end of namespace amp
} //end of namespace bolt

#endif // AMP_UNIQUE_INL