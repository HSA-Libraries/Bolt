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
// AMP Remove
//////////////////////////////////////////////////////////////////////////////

#pragma once
#if !defined( BOLT_AMP_REMOVE_INL )
#define BOLT_AMP_REMOVE_INL

#include <bolt/amp/copy.h>

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace bolt
{
namespace amp
{

namespace detail{

namespace serial{

	template<typename ForwardIterator, typename Predicate>
    ForwardIterator remove_if(bolt::amp::control &ctl,
						  ForwardIterator first,
						  ForwardIterator last,
						  Predicate pred
						  )
    {
		    typedef typename std::iterator_traits<ForwardIterator>::value_type iType;
           
			// create temporary storage for an intermediate result
			std::vector<iType> temp(first, last);

			auto mapped_first_itr = create_mapped_iterator(typename std::iterator_traits<ForwardIterator>::iterator_category(), 
                                                        ctl, first);
			auto return_itr = bolt::amp::detail::serial::copy_if(ctl, temp.begin(), temp.end(), temp.begin(),  mapped_first_itr, bolt::amp::not_pred<Predicate, iType>(pred) );
			return first + (return_itr - mapped_first_itr);

    }


	template< typename ForwardIterator, typename InputIterator, typename Predicate>
    ForwardIterator remove_if( bolt::amp::control &ctl,
		                   ForwardIterator first,
                           ForwardIterator last,
                           InputIterator stencil,
                           Predicate pred)
	{
		    typedef typename std::iterator_traits<ForwardIterator>::value_type iType;
            
			// create temporary storage for an intermediate result
			std::vector<iType> temp(first, last);
            auto mapped_first_itr = create_mapped_iterator(typename std::iterator_traits<ForwardIterator>::iterator_category(), 
                                                        ctl, first);
			auto mapped_stencil_itr = create_mapped_iterator(typename std::iterator_traits<InputIterator>::iterator_category(), 
                                                        ctl, stencil);
			auto return_itr = bolt::amp::detail::serial::copy_if(ctl, temp.begin(), temp.end(), mapped_stencil_itr, mapped_first_itr, bolt::amp::not_pred<Predicate, iType>(pred) );
			return first + (return_itr - mapped_first_itr);

	}


} // end of namespace serial

#ifdef ENABLE_TBB
namespace btbb{

    template<typename ForwardIterator, typename Predicate>
    ForwardIterator remove_if(bolt::amp::control &ctl,
						  ForwardIterator first,
						  ForwardIterator last,
						  Predicate pred
						  )
    {
		    typedef typename std::iterator_traits<ForwardIterator>::value_type iType;

			// create temporary storage for an intermediate result
			std::vector<iType> temp(first, last);
			auto mapped_first_itr = create_mapped_iterator(typename std::iterator_traits<ForwardIterator>::iterator_category(), 
                                                        ctl, first);
			auto return_itr = bolt::amp::detail::btbb::copy_if(ctl, temp.begin(), temp.end(), temp.begin(), mapped_first_itr, bolt::amp::not_pred<Predicate, iType>(pred) );
			return first + (return_itr - mapped_first_itr);
	  
    }


	template< typename ForwardIterator, typename InputIterator, typename Predicate>
    ForwardIterator remove_if( bolt::amp::control &ctl,
		                   ForwardIterator first,
                           ForwardIterator last,
                           InputIterator stencil,
                           Predicate pred)
	{
		    typedef typename std::iterator_traits<ForwardIterator>::value_type iType;
            
			// create temporary storage for an intermediate result
			std::vector<iType> temp(first, last);
			auto mapped_first_itr = create_mapped_iterator(typename std::iterator_traits<ForwardIterator>::iterator_category(), 
                                                        ctl, first);
			auto mapped_stencil_itr = create_mapped_iterator(typename std::iterator_traits<InputIterator>::iterator_category(), 
                                                        ctl, stencil);
			auto return_itr = bolt::amp::detail::btbb::copy_if(ctl, temp.begin(), temp.end(), mapped_stencil_itr, mapped_first_itr, bolt::amp::not_pred<Predicate, iType>(pred) );
			return first + (return_itr - mapped_first_itr);

	}

} // end of namespace btbb
#endif

namespace amp{

	template<typename ForwardIterator, typename Predicate>
    ForwardIterator remove_if(bolt::amp::control &ctl,
						   ForwardIterator first,
						   ForwardIterator last,
						   Predicate& pred
						   )
    {
	

		typedef typename std::iterator_traits<ForwardIterator>::value_type iType;
		int sz = static_cast<int>(last - first);

		auto dvInput_itr   = bolt::amp::create_mapped_iterator(
                                    typename bolt::amp::iterator_traits< ForwardIterator >::iterator_category( ), 
                                    first, sz, false, ctl );

		// create temporary storage for an intermediate result
		bolt::amp::device_vector<iType> temp(dvInput_itr, dvInput_itr+sz, false, ctl);

        // remove into temp
        auto return_itr = remove_copy_if(ctl, temp.begin(), temp.end(), temp.begin(), dvInput_itr, pred); 
		return first + (return_itr - dvInput_itr);
    }
   
	template< typename ForwardIterator, typename InputIterator, typename Predicate>
    ForwardIterator remove_if( bolt::amp::control &ctl,
		                   ForwardIterator first,
                           ForwardIterator last,
                           InputIterator stencil,
                           Predicate pred)
	{
		typedef typename std::iterator_traits<ForwardIterator>::value_type iType;
		int sz = static_cast<int>(last - first);

		auto dvInput_itr   = bolt::amp::create_mapped_iterator(
                                    typename bolt::amp::iterator_traits< ForwardIterator >::iterator_category( ), 
                                    first, sz, false, ctl );
		auto dvStencil_itr   = bolt::amp::create_mapped_iterator(
                                    typename bolt::amp::iterator_traits< InputIterator >::iterator_category( ), 
                                    stencil, sz, false, ctl );


		// create temporary storage for an intermediate result
		bolt::amp::device_vector<iType> temp(dvInput_itr, dvInput_itr+sz, false, ctl);
        // remove into temp
        auto return_itr = remove_copy_if(ctl, temp.begin(), temp.end(), dvStencil_itr, dvInput_itr, pred);
		return first + (return_itr - dvInput_itr);

	}

} //end of namespace amp


 /*! \brief This template function overload is used strictly for device vectors and std random access vectors. 
        \detail Here we branch out into the SerialCpu, MultiCore TBB or The AMP code paths. 
    */
    template<typename ForwardIterator, typename Predicate>
    ForwardIterator remove_if(bolt::amp::control &ctl,
						 ForwardIterator first,
						 ForwardIterator last,
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
            return serial::remove_if(ctl, first, last, pred);
			
        }
        else if( runMode == bolt::amp::control::MultiCoreCpu )
        {
#if defined( ENABLE_TBB )
            return btbb::remove_if(ctl, first, last, pred);
#else
            throw std::runtime_error( "The MultiCoreCpu version of find is not enabled to be built! \n" );
#endif
        }
        else
        {
            return amp::remove_if(ctl, first, last, pred );
        }
        return first;
    }




	template< typename ForwardIterator, typename InputIterator, typename Predicate>
    ForwardIterator remove_if( bolt::amp::control &ctl,
		                   ForwardIterator first,
                           ForwardIterator last,
                           InputIterator stencil,
                           Predicate pred)
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
            return serial::remove_if(ctl, first, last, stencil, pred);
        }
        else if( runMode == bolt::amp::control::MultiCoreCpu )
        {
#if defined( ENABLE_TBB )
            return btbb::remove_if(ctl, first, last, stencil, pred);
#else
            throw std::runtime_error( "The MultiCoreCpu version of find is not enabled to be built! \n" );
#endif
        }
        else
        {
            return amp::remove_if(ctl, first, last, stencil, pred );
        }
        return first;
	}



};//end of namespace detail


        //////////////////////////////////////////
        //  Remove overloads
        ///////////////////////////////////////// 


	    template<typename ForwardIterator, typename Predicate>
        ForwardIterator remove_if(control &ctl,
		                ForwardIterator first,
                           ForwardIterator last,
                           Predicate pred)
		{
			return detail::remove_if(ctl, first, last, pred);
		}


	    template<typename ForwardIterator, typename Predicate>
        ForwardIterator remove_if(ForwardIterator first,
                            ForwardIterator last,
                            Predicate pred)
		{
			return remove_if(control::getDefault(), first, last, pred);
		}

		template< typename ForwardIterator, typename InputIterator, typename Predicate>
        ForwardIterator remove_if( control &ctl,
		                   ForwardIterator first,
                           ForwardIterator last,
                           InputIterator stencil,
                           Predicate pred)
		{
			return detail::remove_if(ctl, first, last, stencil, pred);

		}

		template< typename ForwardIterator, typename InputIterator, typename Predicate>
        ForwardIterator remove_if(ForwardIterator first,
                           ForwardIterator last,
                           InputIterator stencil,
                           Predicate pred)
		{
			return remove_if(control::getDefault(), first, last, stencil, pred);
		}


	    template< typename ForwardIterator, typename T>
        ForwardIterator remove(control &ctl,
			             ForwardIterator first,
                        ForwardIterator last,
                        const T &value)
		{
			 bolt::amp::equal_to_value<T> pred(value);
             return remove_if(ctl, first, last, pred);
		}

		template< typename ForwardIterator, typename T>
        ForwardIterator remove(ForwardIterator first,
                        ForwardIterator last,
                        const T &value)
		{
			 return remove(control::getDefault(), first, last, value);
		}


		template<typename InputIterator, typename OutputIterator, typename Predicate>
        OutputIterator remove_copy_if(control &ctl,
			                    InputIterator first,
                                InputIterator last,
                                OutputIterator result,
                                Predicate pred)
		{
			return remove_copy_if(ctl, first, last, first, result, pred);
		}


		template<typename InputIterator, typename OutputIterator, typename Predicate>
        OutputIterator remove_copy_if(InputIterator first,
                                InputIterator last,
                                OutputIterator result,
                                Predicate pred)
		{
			return remove_copy_if(control::getDefault(), first, last, result, pred);
		}


	    template<typename InputIterator1, typename InputIterator2, typename OutputIterator, typename Predicate>
        OutputIterator remove_copy_if(control &ctl,
			                    InputIterator1 first,
                                InputIterator1 last,
                                InputIterator2 stencil,
                                OutputIterator result,
                                Predicate pred)
		{
            typedef typename std::iterator_traits< InputIterator2 >::value_type iType;
			return bolt::amp::copy_if(ctl, first, last, stencil, result, bolt::amp::not_pred<Predicate, iType>(pred) );
		}

		template<typename InputIterator1, typename InputIterator2, typename OutputIterator, typename Predicate>
        OutputIterator remove_copy_if(InputIterator1 first,
                                InputIterator1 last,
                                InputIterator2 stencil,
                                OutputIterator result,
                                Predicate pred)
		{
			return remove_copy_if(control::getDefault(), first, last, stencil, result, pred);
		}


		template<typename InputIterator, typename OutputIterator, typename T>
        OutputIterator remove_copy(control &ctl,
			                 InputIterator first,
                             InputIterator last,
                             OutputIterator result,
                             const T &value)
		{
			bolt::amp::equal_to_value<T> pred(value);
            return remove_copy_if(ctl, first, last, result, pred);
		}


        template<typename InputIterator, typename OutputIterator, typename T>
        OutputIterator remove_copy(InputIterator first,
                             InputIterator last,
                             OutputIterator result,
                             const T &value)
		{
			return remove_copy(control::getDefault(), first, last, result, value);
		}




    } //end of namespace amp
} //end of namespace bolt

#endif // AMP_REMOVE_INL