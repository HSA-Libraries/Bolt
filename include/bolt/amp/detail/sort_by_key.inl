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

/***************************************************************************
* The Radix sort algorithm implementation in BOLT library is a derived work from 
* the radix sort sample which is provided in the Book. "Heterogeneous Computing with OpenCL"
* Link: http://www.heterogeneouscompute.org/?page_id=7
* The original Authors are: Takahiro Harada and Lee Howes. A detailed explanation of 
* the algorithm is given in the publication linked here. 
* http://www.heterogeneouscompute.org/wordpress/wp-content/uploads/2011/06/RadixSort.pdf
* 
* The derived work adds support for descending sort and signed integers. 
* Performance optimizations were provided for the AMD GCN architecture. 
* 
*  Besides this following publications were referred: 
*  1. "Parallel Scan For Stream Architectures"  
*     Technical Report CS2009-14Department of Computer Science, University of Virginia. 
*     Duane Merrill and Andrew Grimshaw
*    https://sites.google.com/site/duanemerrill/ScanTR2.pdf
*  2. "Revisiting Sorting for GPGPU Stream Architectures" 
*     Duane Merrill and Andrew Grimshaw
*    https://sites.google.com/site/duanemerrill/RadixSortTR.pdf
*  3. The SHOC Benchmark Suite 
*     https://github.com/vetter/shoc
*
***************************************************************************/

#pragma once
#if !defined( BOLT_AMP_SORT_BY_KEY_INL )
#define BOLT_AMP_SORT_BY_KEY_INL

#include <algorithm>
#include <type_traits>
#include <amp.h>
#include "bolt/amp/pair.h"
#include "bolt/amp/device_vector.h"
#include "bolt/amp/iterator/iterator_traits.h"
#include "bolt/amp/detail/stablesort_by_key.inl"

#ifdef ENABLE_TBB
//TBB Includes
#include "bolt/btbb/sort_by_key.h"
#endif

#define BITONIC_SORT_WGSIZE 64
#define DEBUG 1
namespace bolt {
namespace amp {

namespace detail {


    //Serial CPU code path implementation.
    //Class to hold the key value pair. This will be used to zip th ekey and value together in a vector.
    template <typename keyType, typename valueType>
    class std_sort
    {
    public:
        keyType   key;
        valueType value;
    };
    
    //This is the functor which will sort the std_sort vector.
    template <typename keyType, typename valueType, typename StrictWeakOrdering>
    class std_sort_comp
    {
    public:
        typedef std_sort<keyType, valueType> KeyValueType;
        std_sort_comp(const StrictWeakOrdering &_swo):swo(_swo)
        {}
        StrictWeakOrdering swo;
        bool operator() (const KeyValueType &lhs, const KeyValueType &rhs) const
        {
            return swo(lhs.key, rhs.key);
        }
    };

    //The serial CPU implementation of sort_by_key routine. This routines zips the key value pair and then sorts
    //using the std::sort routine.
    template< typename RandomAccessIterator1, typename RandomAccessIterator2, typename StrictWeakOrdering >
    void serialCPU_sort_by_key( const RandomAccessIterator1 keys_first, const RandomAccessIterator1 keys_last,
                                const RandomAccessIterator2 values_first,
                                const StrictWeakOrdering& comp)
    {
        typedef typename std::iterator_traits< RandomAccessIterator1 >::value_type keyType;
        typedef typename std::iterator_traits< RandomAccessIterator2 >::value_type valType;
        typedef std_sort<keyType, valType> KeyValuePair;
        typedef std_sort_comp<keyType, valType, StrictWeakOrdering> KeyValuePairFunctor;

        size_t vecSize = std::distance( keys_first, keys_last );
        std::vector<KeyValuePair> KeyValuePairVector(vecSize);
        KeyValuePairFunctor functor(comp);
        //Zip the key and values iterators into a std_sort vector.
        for (size_t i=0; i< vecSize; i++)
        {
            KeyValuePairVector[i].key   = *(keys_first + i);
            KeyValuePairVector[i].value = *(values_first + i);
        }
        //Sort the std_sort vector using std::sort
        std::sort(KeyValuePairVector.begin(), KeyValuePairVector.end(), functor);
        //Extract the keys and values from the KeyValuePair and fill the respective iterators.
        for (size_t i=0; i< vecSize; i++)
        {
            *(keys_first + i)   = KeyValuePairVector[i].key;
            *(values_first + i) = KeyValuePairVector[i].value;
        }
    }


    template< typename DVKeys, typename DVValues, typename StrictWeakOrdering>
    typename std::enable_if<
        !( std::is_same< typename std::iterator_traits<DVKeys >::value_type, unsigned int >::value ||
           std::is_same< typename std::iterator_traits<DVKeys >::value_type, int >::value 
         )
                           >::type
    sort_by_key_enqueue(control &ctl, const DVKeys& keys_first,
                        const DVKeys& keys_last, const DVValues& values_first,
                        const StrictWeakOrdering& comp)
    {
		printf("Calling Merge Sort................\n");
        stablesort_by_key_enqueue(ctl, keys_first, keys_last, values_first, comp);
        return;
    }// END of sort_by_key_enqueue

    template<typename DVKeys, typename DVValues, typename StrictWeakOrdering>
    typename std::enable_if< std::is_same< typename std::iterator_traits<DVKeys >::value_type,
                                           unsigned int
                                         >::value
                           >::type  /*If enabled then this typename will be evaluated to void*/
    sort_by_key_enqueue( control &ctl,
                         DVKeys keys_first, DVKeys keys_last,
                         DVValues values_first,
                         StrictWeakOrdering comp)
{
	


    
    return;
}


    template<typename DVKeys, typename DVValues, typename StrictWeakOrdering>
    typename std::enable_if< std::is_same< typename std::iterator_traits<DVKeys >::value_type,
                                           int
                                         >::value
                           >::type  /*If enabled then this typename will be evaluated to void*/
    sort_by_key_enqueue( control &ctl,
                         DVKeys keys_first, DVKeys keys_last,
                         DVValues values_first,
                         StrictWeakOrdering comp)
{
    
	printf("****************************\n");
  
    return;
}

    //Fancy iterator specialization
    template<typename DVRandomAccessIterator1, typename DVRandomAccessIterator2, typename StrictWeakOrdering>
    void sort_by_key_pick_iterator(control &ctl, DVRandomAccessIterator1 keys_first,
                                   DVRandomAccessIterator1 keys_last, DVRandomAccessIterator2 values_first,
                                   StrictWeakOrdering comp,  bolt::amp::fancy_iterator_tag )
    {
        static_assert( std::is_same<DVRandomAccessIterator1, bolt::amp::fancy_iterator_tag  >::value, "It is not possible to output to fancy iterators; they are not mutable! " );
    }

    //Device Vector specialization
    template<typename DVRandomAccessIterator1, typename DVRandomAccessIterator2, typename StrictWeakOrdering>
    void sort_by_key_pick_iterator(control &ctl, DVRandomAccessIterator1 keys_first,
                                   DVRandomAccessIterator1 keys_last, DVRandomAccessIterator2 values_first,
                                   StrictWeakOrdering comp,
                                   bolt::amp::device_vector_tag, bolt::amp::device_vector_tag )
    {
        typedef typename std::iterator_traits< DVRandomAccessIterator1 >::value_type keyType;
        typedef typename std::iterator_traits< DVRandomAccessIterator2 >::value_type valueType;
        // User defined Data types are not supported with device_vector. Hence we have a static assert here.
        // The code here should be in compliant with the routine following this routine.
        size_t szElements = (size_t)(keys_last - keys_first);
        if (szElements == 0 )
                return;
        bolt::amp::control::e_RunMode runMode = ctl.getForceRunMode( );
        if( runMode == bolt::amp::control::Automatic )
        {
            runMode = ctl.getDefaultPathToRun( );
        }
		
        if (runMode == bolt::amp::control::SerialCpu) 
		{
		   			
            typename bolt::amp::device_vector< keyType >::pointer   keysPtr   =  keys_first.getContainer( ).data( );
            typename bolt::amp::device_vector< valueType >::pointer valuesPtr =  values_first.getContainer( ).data( );
            serialCPU_sort_by_key(&keysPtr[keys_first.m_Index], &keysPtr[keys_last.m_Index],
                                            &valuesPtr[values_first.m_Index], comp);
            return;
        } 
		else if (runMode == bolt::amp::control::MultiCoreCpu) 
		{

            #ifdef ENABLE_TBB
                typename bolt::amp::device_vector< keyType >::pointer   keysPtr   =  keys_first.getContainer( ).data( );
                typename bolt::amp::device_vector< valueType >::pointer valuesPtr =  values_first.getContainer( ).data( );
                bolt::btbb::sort_by_key(&keysPtr[keys_first.m_Index], &keysPtr[keys_last.m_Index],
                                                &valuesPtr[values_first.m_Index], comp);
                return;
            #else
               throw std::runtime_error( "The MultiCoreCpu version of Sort_by_key is not enabled to be built with TBB!\n");
            #endif
        }

        else 
		{   
            sort_by_key_enqueue(ctl, keys_first, keys_last, values_first, comp);
        }
        return;
    }

    //Non Device Vector specialization.
    //This implementation creates a Buffer and passes the AMP buffer to the
    //sort specialization whichtakes the AMP buffer as a parameter.
    //In the future, Each input buffer should be mapped to the device_vector and the
    //specialization specific to device_vector should be called.
    template<typename RandomAccessIterator1, typename RandomAccessIterator2, typename StrictWeakOrdering>
    void sort_by_key_pick_iterator(control &ctl, RandomAccessIterator1 keys_first,
                                   RandomAccessIterator1 keys_last, RandomAccessIterator2 values_first,
                                   StrictWeakOrdering comp, 
                                   std::random_access_iterator_tag, std::random_access_iterator_tag )
    {

        typedef typename std::iterator_traits<RandomAccessIterator1>::value_type T_keys;
        typedef typename std::iterator_traits<RandomAccessIterator2>::value_type T_values;
        size_t szElements = (size_t)(keys_last - keys_first);
        if (szElements == 0)
            return;

        bolt::amp::control::e_RunMode runMode = ctl.getForceRunMode( );
        if( runMode == bolt::amp::control::Automatic )
        {
            runMode = ctl.getDefaultPathToRun( );
        }
	    
        if ((runMode == bolt::amp::control::SerialCpu) /*|| (szElements < WGSIZE) */)
		{   
            serialCPU_sort_by_key(keys_first, keys_last, values_first, comp);
            return;
        } 
		else if (runMode == bolt::amp::control::MultiCoreCpu) 
		{
            #ifdef ENABLE_TBB
                serialCPU_sort_by_key(keys_first, keys_last, values_first, comp);
                return;
            #else
                throw std::runtime_error("The MultiCoreCpu Version of Sort_by_key is not enabled to be built with TBB!\n");
            #endif
        } 
		else 
		{
            

			device_vector< T_keys, concurrency::array_view > dvInputKeys(   keys_first, keys_last, true, ctl );
			device_vector<  T_values, concurrency::array_view > dvInputValues(  values_first, szElements, true, ctl );

            //Now call the actual AMP algorithm
            sort_by_key_enqueue(ctl,dvInputKeys.begin(),dvInputKeys.end(), dvInputValues.begin(), comp);
            //Map the buffer back to the host
            dvInputValues.data( );
            dvInputKeys.data( );
            return;
        }
    }


    template< typename RandomAccessIterator1, typename RandomAccessIterator2, typename StrictWeakOrdering >
    void sort_by_key_detect_random_access( control &ctl,
                                    const RandomAccessIterator1 keys_first, const RandomAccessIterator1 keys_last,
                                    const RandomAccessIterator2 values_first,
                                    const StrictWeakOrdering& comp, 
                                    std::random_access_iterator_tag, std::random_access_iterator_tag )
    {
        return sort_by_key_pick_iterator( ctl, keys_first, keys_last, values_first,
                                    comp, 
                                    typename std::iterator_traits< RandomAccessIterator1 >::iterator_category( ),
                                    typename std::iterator_traits< RandomAccessIterator2 >::iterator_category( ) );
    };

    template< typename RandomAccessIterator1, typename RandomAccessIterator2, typename StrictWeakOrdering >
    void sort_by_key_detect_random_access( control &ctl,
                                    const RandomAccessIterator1 keys_first, const RandomAccessIterator1 keys_last,
                                    const RandomAccessIterator2 values_first,
                                    const StrictWeakOrdering& comp, 
                                    std::input_iterator_tag, std::input_iterator_tag )
    {
        //  \TODO:  It should be possible to support non-random_access_iterator_tag iterators, if we copied the data
        //  to a temporary buffer.  Should we?
        static_assert(std::is_same< RandomAccessIterator1, std::input_iterator_tag >::value , "Bolt only supports random access iterator types" );
        static_assert(std::is_same< RandomAccessIterator2, std::input_iterator_tag >::value , "Bolt only supports random access iterator types" );
    };

    template< typename RandomAccessIterator1, typename RandomAccessIterator2, typename StrictWeakOrdering >
    void sort_by_key_detect_random_access( control &ctl,
                                    const RandomAccessIterator1 keys_first, const RandomAccessIterator1 keys_last,
                                    const RandomAccessIterator2 values_first,
                                    const StrictWeakOrdering& comp,
                                    bolt::amp::fancy_iterator_tag, std::input_iterator_tag )
    {
        static_assert( std::is_same< RandomAccessIterator1, bolt::amp::fancy_iterator_tag >::value , "It is not possible to sort fancy iterators. They are not mutable" );
        static_assert( std::is_same< RandomAccessIterator2, std::input_iterator_tag >::value , "It is not possible to sort fancy iterators. They are not mutable" );
    }

    template< typename RandomAccessIterator1, typename RandomAccessIterator2, typename StrictWeakOrdering >
    void sort_by_key_detect_random_access( control &ctl,
                                    const RandomAccessIterator1 keys_first, const RandomAccessIterator1 keys_last,
                                    const RandomAccessIterator2 values_first,
                                    const StrictWeakOrdering& comp, 
                                    std::input_iterator_tag, bolt::amp::fancy_iterator_tag )
    {


        static_assert( std::is_same< RandomAccessIterator2, bolt::amp::fancy_iterator_tag >::value , "It is not possible to sort fancy iterators. They are not mutable" );
        static_assert( std::is_same< RandomAccessIterator1, std::input_iterator_tag >::value , "It is not possible to sort fancy iterators. They are not mutable" );

    }


}//namespace bolt::amp::detail


        template<typename RandomAccessIterator1 , typename RandomAccessIterator2>
        void sort_by_key(RandomAccessIterator1 keys_first,
                         RandomAccessIterator1 keys_last,
                         RandomAccessIterator2 values_first)
        {
            typedef typename std::iterator_traits< RandomAccessIterator1 >::value_type keys_T;

            detail::sort_by_key_detect_random_access( control::getDefault( ),
                                       keys_first, keys_last,
                                       values_first,
                                       less< keys_T >( ),
                                       typename std::iterator_traits< RandomAccessIterator1 >::iterator_category( ),
                                       typename std::iterator_traits< RandomAccessIterator2 >::iterator_category( ) );
            return;
        }

        template<typename RandomAccessIterator1 , typename RandomAccessIterator2, typename StrictWeakOrdering>
        void sort_by_key(RandomAccessIterator1 keys_first,
                         RandomAccessIterator1 keys_last,
                         RandomAccessIterator2 values_first,
                         StrictWeakOrdering comp)
        {
            typedef typename std::iterator_traits< RandomAccessIterator1 >::value_type keys_T;

            detail::sort_by_key_detect_random_access( control::getDefault( ),
                                       keys_first, keys_last,
                                       values_first,
                                       comp,
                                       typename std::iterator_traits< RandomAccessIterator1 >::iterator_category( ),
                                       typename std::iterator_traits< RandomAccessIterator2 >::iterator_category( ) );
            return;
        }

        template<typename RandomAccessIterator1 , typename RandomAccessIterator2>
        void sort_by_key(control &ctl,
                         RandomAccessIterator1 keys_first,
                         RandomAccessIterator1 keys_last,
                         RandomAccessIterator2 values_first)
        {
            typedef typename std::iterator_traits< RandomAccessIterator1 >::value_type keys_T;

            detail::sort_by_key_detect_random_access( ctl,
                                       keys_first, keys_last,
                                       values_first,
                                       less< keys_T >( ),
                                       typename std::iterator_traits< RandomAccessIterator1 >::iterator_category( ),
                                       typename std::iterator_traits< RandomAccessIterator2 >::iterator_category( ) );
            return;
        }

        template<typename RandomAccessIterator1 , typename RandomAccessIterator2, typename StrictWeakOrdering>
        void sort_by_key(control &ctl,
                         RandomAccessIterator1 keys_first,
                         RandomAccessIterator1 keys_last,
                         RandomAccessIterator2 values_first,
                         StrictWeakOrdering comp)
        {
            typedef typename std::iterator_traits< RandomAccessIterator1 >::value_type keys_T;

            detail::sort_by_key_detect_random_access( ctl,
                                       keys_first, keys_last,
                                       values_first,
                                       comp,
                                       typename std::iterator_traits< RandomAccessIterator1 >::iterator_category( ),
                                       typename std::iterator_traits< RandomAccessIterator2 >::iterator_category( ) );
            return;
        }

    }
};

#endif
