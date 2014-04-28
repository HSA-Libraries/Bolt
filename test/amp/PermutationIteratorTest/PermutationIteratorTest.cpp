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

#include "common/stdafx.h"

#include "bolt/amp/copy.h"
#include "bolt/amp/count.h"
#include "bolt/amp/inner_product.h"
#include "bolt/amp/transform.h"
#include "bolt/amp/reduce.h"
#include "bolt/amp/reduce_by_key.h"
#include "bolt/amp/scan.h"
#include "bolt/amp/scan_by_key.h"
#include "bolt/amp/transform_reduce.h"
#include "bolt/amp/transform_scan.h"

#include "bolt/unicode.h"
#include "bolt/miniDump.h"
#include <gtest/gtest.h>
#include <array>
#include "bolt/amp/functional.h"
#include "common/test_common.h"
#include "bolt/amp/iterator/constant_iterator.h"
#include "bolt/amp/iterator/counting_iterator.h"
#include "bolt/amp/iterator/permutation_iterator.h"
#include <boost/range/algorithm/transform.hpp>
#include <boost/iterator/permutation_iterator.hpp>


namespace gold
{
        template<
        typename InputIterator1,
        typename InputIterator2,
        typename OutputIterator,
        typename BinaryFunction>
    OutputIterator
    scan_by_key(
        InputIterator1 firstKey,
        InputIterator1 lastKey,
        InputIterator2 values,
        OutputIterator result,
        BinaryFunction binary_op)
    {
        if(std::distance(firstKey,lastKey) < 1)
             return result;
        typedef typename std::iterator_traits< InputIterator1 >::value_type kType;
        typedef typename std::iterator_traits< InputIterator2 >::value_type vType;
        typedef typename std::iterator_traits< OutputIterator >::value_type oType;

        static_assert( std::is_convertible< vType, oType >::value,
            "InputIterator2 and OutputIterator's value types are not convertible." );

        if(std::distance(firstKey,lastKey) < 1)
             return result;
        // do zeroeth element
        *result = *values; // assign value

        // scan oneth element and beyond
        for ( InputIterator1 key = (firstKey+1); key != lastKey; key++)
        {
            // move on to next element
            values++;
            result++;

            // load keys
            kType currentKey  = *(key);
            kType previousKey = *(key-1);

            // load value
            oType currentValue = *values; // convertible
            oType previousValue = *(result-1);

            // within segment
            if (currentKey == previousKey)
            {
                //std::cout << "continuing segment" << std::endl;
                oType r = binary_op( previousValue, currentValue);
                *result = r;
            }
            else // new segment
            {
                //std::cout << "new segment" << std::endl;
                *result = currentValue;
            }
        }

        return result;
    }

    template<
        typename InputIterator1,
        typename InputIterator2,
        typename OutputIterator1,
        typename OutputIterator2,
        typename BinaryPredicate,
        typename BinaryFunction>
    //std::pair<OutputIterator1, OutputIterator2>
    unsigned int
    reduce_by_key( InputIterator1 keys_first,
                   InputIterator1 keys_last,
                   InputIterator2 values_first,
                   OutputIterator1 keys_output,
                   OutputIterator2 values_output,
                   const BinaryPredicate binary_pred,
                   const BinaryFunction binary_op )
    {
        typedef typename std::iterator_traits< InputIterator1 >::value_type kType;
        typedef typename std::iterator_traits< InputIterator2 >::value_type vType;
        typedef typename std::iterator_traits< OutputIterator1 >::value_type koType;
        typedef typename std::iterator_traits< OutputIterator2 >::value_type voType;
        static_assert( std::is_convertible< vType, voType >::value,
                       "InputIterator2 and OutputIterator's value types are not convertible." );

       int numElements = static_cast< int >( std::distance( keys_first, keys_last ) );

        // do zeroeth element
        *values_output = *values_first;
        *keys_output = *keys_first;
        unsigned int count = 1;
        // rbk oneth element and beyond

        values_first++;
        for ( InputIterator1 key = (keys_first+1); key != keys_last; key++)
        {
            // load keys
            kType currentKey  = *(key);
            kType previousKey = *(key-1);

            // load value
            voType currentValue = *values_first;
            voType previousValue = *values_output;

            previousValue = *values_output;
            // within segment
            if (binary_pred(currentKey, previousKey))
            {
                voType r = binary_op( previousValue, currentValue);
                *values_output = r;
                *keys_output = currentKey;

            }
            else // new segment
            {
                values_output++;
                keys_output++;
                *values_output = currentValue;
                *keys_output = currentKey;
                count++; //To count the number of elements in the output array
            }
            values_first++;
        }

        //return std::pair(keys_output+1, values_output+1);
        return count;
    }

};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Transform tests
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
TEST(simple1,counting)
{
    bolt::amp::counting_iterator<int> iter(0);
    bolt::amp::counting_iterator<int> iter2=iter+1024;
    std::vector<int> input1(1024);
    std::vector<int> input2(1024);
    std::vector<int> stdOutput(1024);
     std::vector<int> boltOutput(1024);
     for(int i=0 ; i< 1024;i++)
     {
         input1[i] = i;
     }
    input2 = input1;
    std::transform( input1.begin(), input1.end(), input2.begin(), stdOutput.begin(), bolt::amp::plus<int>());
    bolt::amp::transform(iter,iter2,input1.begin(),boltOutput.begin(),bolt::amp::plus<int>());
    cmpArrays( stdOutput, boltOutput, 1024 );
}

TEST(simple1,Serial_counting)
{
    bolt::amp::counting_iterator<int> iter(0);
    bolt::amp::counting_iterator<int> iter2=iter+1024;
    std::vector<int> input1(1024);
    std::vector<int> input2(1024);
    std::vector<int> stdOutput(1024);
     std::vector<int> boltOutput(1024);
     for(int i=0 ; i< 1024;i++)
     {
         input1[i] = i;
     }
    input2 = input1;

    bolt::amp::control ctl = bolt::amp::control::getDefault( );
    ctl.setForceRunMode(bolt::amp::control::SerialCpu);

    std::transform( input1.begin(), input1.end(), input2.begin(), stdOutput.begin(), bolt::amp::plus<int>());
    bolt::amp::transform(ctl, iter,iter2,input1.begin(),boltOutput.begin(),bolt::amp::plus<int>());
    cmpArrays( stdOutput, boltOutput, 1024 );
}


TEST(AMPIterators, AVPermutation)
{
    int __index = 1;
    const unsigned int size = 256;

    typedef int etype;
    etype elements[size];
    etype empty[size];

    size_t view_size = size;


    std::fill(elements, elements+size, 100);
    std::fill(empty, empty+size, 0);


    bolt::amp::device_vector<int, concurrency::array_view> resultV(elements, elements + size);
    bolt::amp::device_vector<int, concurrency::array_view> dumpV(empty, empty + size);
    auto dvbegin = resultV.begin();
    auto dvend = resultV.end();
    auto dumpbegin = dumpV.begin().getContainer().getBuffer();

    bolt::amp::control ctl;
    concurrency::accelerator_view av = ctl.getAccelerator().default_view;

    concurrency::extent< 1 > inputExtent( size );
    concurrency::parallel_for_each(av, inputExtent, [=](concurrency::index<1> idx)restrict(amp)
    {
      dumpbegin[idx[0]] = dvbegin[idx[0]];
       
    });
    dumpbegin.synchronize();
    cmpArrays(resultV, dumpV);

}


TEST(AMPIterators, PermutationPFE)
{

    const unsigned int size = 256;

    typedef int etype;
    etype elements[size];
    etype key[size];
    etype empty[size];

    size_t view_size = size;

    std::iota(elements, elements+size, 1000);
    std::iota(key, key+size, 0);
    std::fill(empty, empty+size, 0);

    std::random_shuffle ( key, key+size );

    bolt::amp::device_vector<int, concurrency::array_view> dve(elements, elements + size);
    bolt::amp::device_vector<int, concurrency::array_view> dvk(key, key + size);
    bolt::amp::device_vector<int, concurrency::array_view> dumpV(empty, empty + size);

    auto dvebegin = dve.begin();
    auto dveend = dve.end();

    auto dvkbegin = dvk.begin();
    auto dvkend = dvk.end();

    auto __ebegin =  dvebegin;
    auto __eend =  dveend;

    auto __kbegin =  dvkbegin;
    auto __kend =    dvkend;

    typedef bolt::amp::permutation_iterator< bolt::amp::device_vector<int>::iterator,
                                             bolt::amp::device_vector<int>::iterator> intvpi;

    intvpi first = bolt::amp::make_permutation_iterator(__kbegin, __ebegin);
    //i = first;
    intvpi last = bolt::amp::make_permutation_iterator(__kend, __eend);

    bolt::amp::control ctl;
    concurrency::accelerator_view av = ctl.getAccelerator().default_view;

    //amp_stl_algorithms::array_view_iterator<int> x = first[0];
    int x = first[0];
    //int x = __kbegin[0];
    //int x = first.key_iterator[0];
    //int x = __ebegin[__kbegin[0]];
    std::cout<<x;


    auto dumpAV = dumpV.begin().getContainer().getBuffer();

    concurrency::extent< 1 > inputExtent( size );
    concurrency::parallel_for_each(av, inputExtent, [=](concurrency::index<1> idx)restrict(amp)
    {
      int gidx = idx[0];

      dumpAV[gidx] = first[gidx];
       
    });
    dumpAV.synchronize();

    //cmpArrays(dve,dumpV);
    
    //std::cout<<"index"<<"     element"<<"       output"<<std::endl;
    //for ( int i = 0 ; i < size ; i++ )
    //{
    //    std::cout<<dvk[i]<<"     "<<dve[i]<<"       "<<dumpAV[i]<<std::endl;
    //}

}

// Warning 4996
TEST(AMPIterators, PermutationGatherTest)
{
    int n_input[10] =  {0,1,2,3,4,5,6,7,8,9};
    int n_map[10] =  {9,8,7,6,5,4,3,2,1,0};
    std::vector<int> exp_result(10,0);    
    std::vector<int> result ( 10, 0 );
    std::vector<int> input ( n_input, n_input + 10 );
    std::vector<int> map ( n_map, n_map + 10 );


    bolt::amp::device_vector<int> dresult(result.begin(), result.end());
    bolt::amp::device_vector<int> dinput(input.begin(), input.end());
    bolt::amp::device_vector<int> dmap(map.begin(), map.end());
   
    // warning 4996
//    boost::transform( exp_result, boost::make_permutation_iterator( input.begin(), map.begin() ),bolt::amp::identity<int>( ) );

    //bolt::amp::transform( dinput.begin(), dinput.end(), bolt::amp::make_permutation_iterator( dresult.end(), dmap.end() ), bolt::amp::identity<int>( ) );
    bolt::amp::transform( bolt::amp::make_permutation_iterator( dinput.begin(), dmap.begin() ),
                          bolt::amp::make_permutation_iterator( dinput.end(), dmap.end() ),
                          dresult.begin(),
                          bolt::amp::identity<int>( ) );
  //  cmpArrays(exp_result, result);
}




#if 0
TEST(AMPIterators, PermutationGatherTestStd)
{
    int n_input[10] =  {0,1,2,3,4,5,6,7,8,9};
    int n_map[10] =  {9,8,7,6,5,4,3,2,1,0};
    std::vector<int> exp_result(10,0);    
    std::vector<int> result ( 10, 0 );
    std::vector<int> input ( n_input, n_input + 10 );
    std::vector<int> map ( n_map, n_map + 10 );


    // warning 4996
    boost::transform( exp_result, boost::make_permutation_iterator( input.begin(), map.begin() ),bolt::amp::identity<int>( ) );

    //bolt::amp::transform( dinput.begin(), dinput.end(), bolt::amp::make_permutation_iterator( dresult.end(), dmap.end() ), bolt::amp::identity<int>( ) );
    bolt::amp::transform( bolt::amp::make_permutation_iterator( input.begin(), map.begin() ),
                          bolt::amp::make_permutation_iterator( input.end(), map.end() ),
                          result.begin(),
                          bolt::amp::identity<int>( ) );
    cmpArrays(exp_result, result);
}


TEST(AMPIterators, PermutationScatterTest)
{
    int n_input[10] =  {0,1,2,3,4,5,6,7,8,9};
    int n_map[10] =  {9,8,7,6,5,4,3,2,1,0};
    std::vector<int> exp_result(10,0);    
    std::vector<int> result ( 10, 0 );
    std::vector<int> input ( n_input, n_input + 10 );
    std::vector<int> map ( n_map, n_map + 10 );


    bolt::amp::device_vector<int> dresult(result.begin(), result.end());
    bolt::amp::device_vector<int> dinput(input.begin(), input.end());
    bolt::amp::device_vector<int> dmap(map.begin(), map.end());
   
    // warning 4996
    boost::transform( input, boost::make_permutation_iterator( exp_result.begin(), map.begin() ),bolt::amp::identity<int>( ) );

    // Set CPU control
    //bolt::amp::control ctl;
    //ctl.setForceRunMode(bolt::amp::control::SerialCpu);

    bolt::amp::transform( dinput.begin(),
                          dinput.end(),
                          bolt::amp::make_permutation_iterator( dresult.end(), dmap.end() ),
                          bolt::amp::identity<int>( ) );
    cmpArrays(exp_result, result);

    std::cout<<"element"<<"     index"<<"       output"<<std::endl;
    for ( int i = 0 ; i < 10 ; i++ )
    {
        std::cout<<input[i]<<"     "<<map[i]<<"       "<<result[i]<<std::endl;
    }


    // Test for permutation iterator as a mutable iterator
    //auto indxx = boost::make_permutation_iterator( result.end()-1, map.end()-1 );
    //indxx[0] = 89;
    //std::cout<<result[9]<<"     "<<input[9]<<std::endl;
}

#endif

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Reduce tests
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
TEST(AMPIterators, PermutationReduceTest)
{
    int n_input[10] =  {0,1,2,3,4,5,6,7,8,9};
    int n_map[10] =  {9,8,7,6,5,4,3,2,1,0};
    std::vector<int> input ( n_input, n_input + 10 );
    std::vector<int> map ( n_map, n_map + 10 );


    bolt::amp::device_vector<int> dinput(input.begin(), input.end());
    bolt::amp::device_vector<int> dmap(map.begin(), map.end());
   
    int exp_out = std::accumulate( boost::make_permutation_iterator( input.begin(), map.begin() ),
      boost::make_permutation_iterator( input.end(), map.end() ), int(0), std::plus<int>( ) );

    //bolt::amp::transform( dinput.begin(), dinput.end(), bolt::amp::make_permutation_iterator( dresult.end(), dmap.end() ), bolt::amp::identity<int>( ) );
    int out =  bolt::amp::reduce( bolt::amp::make_permutation_iterator( dinput.begin(), dmap.begin() ),
                          bolt::amp::make_permutation_iterator( dinput.end(), dmap.end() ),
                          int(0),
                          bolt::amp::plus<int>( ) );
    EXPECT_EQ(exp_out, out);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Transform Reduce tests
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

TEST(AMPIterators, PermutationTransformReduceTest)
{
    int n_input[10] =  {0,1,2,3,4,5,6,7,8,9};
    int n_map[10] =  {9,8,7,6,5,4,3,2,1,0};
    std::vector<int> result ( 10, 0 );
    std::vector<int> input ( n_input, n_input + 10 );
    std::vector<int> map ( n_map, n_map + 10 );


    bolt::amp::device_vector<int> dresult(result.begin(), result.end());
    bolt::amp::device_vector<int> dinput(input.begin(), input.end());
    bolt::amp::device_vector<int> dmap(map.begin(), map.end());
   
    std::transform( boost::make_permutation_iterator( input.begin( ), map.begin( ) ),
                    boost::make_permutation_iterator( input.end( ), map.end( ) ),
                    result.begin( ), std::negate<int>( ) );
    int exp_out = std::accumulate( result.begin( ),
                                   result.end( ),
                                   int( 0 ), std::plus<int>( ) );

    int out = bolt::amp::transform_reduce( bolt::amp::make_permutation_iterator( dinput.begin( ), dmap.begin( ) ),
                                           bolt::amp::make_permutation_iterator( dinput.end( ), dmap.end( ) ),
                                           bolt::amp::negate<int>( ),
                                           int( 0 ),
                                           bolt::amp::plus<int>( ) );
    EXPECT_EQ(exp_out, out);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Transform Copy tests
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

TEST(AMPIterators, PermutationCopyTest)
{
    int n_input[10] =  {0,1,2,3,4,5,6,7,8,9};
    int n_map[10] =  {9,8,7,6,5,4,3,2,1,0};
    std::vector<int> result ( 10, 0 );
    std::vector<int> exp_result ( 10, 0 );
    std::vector<int> input ( n_input, n_input + 10 );
    std::vector<int> map ( n_map, n_map + 10 );


    bolt::amp::device_vector<int> dresult( result.begin( ), result.end( ) );
    bolt::amp::device_vector<int> dinput( input.begin( ), input.end( ) );
    bolt::amp::device_vector<int> dmap( map.begin( ), map.end( ) );
   
    std::copy( boost::make_permutation_iterator( input.begin( ), map.begin( ) ),
               boost::make_permutation_iterator( input.end( ), map.end( ) ),
               exp_result.begin( ) );

    bolt::amp::copy( bolt::amp::make_permutation_iterator( dinput.begin( ), dmap.begin( ) ),
                     bolt::amp::make_permutation_iterator( dinput.end( ), dmap.end( ) ),
                     dresult.begin( ) );


    cmpArrays(exp_result, dresult);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Inner Product tests
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

//TEST(AMPIterators, PermutationInnerProductTest)
//{
//    int n_input[10] =  {0,1,2,3,4,5,6,7,8,9};
//    int n_input2[10] =  {5,10,15,20,25,30,35,40,45,50};
//    int n_map[10] =  {9,8,7,6,5,4,3,2,1,0};
//    std::vector<int> result ( 10, 0 );
//    std::vector<int> input ( n_input, n_input + 10 );
//    std::vector<int> input2 ( n_input2, n_input2 + 10 );
//    std::vector<int> map ( n_map, n_map + 10 );
//
//
//    bolt::amp::device_vector<int> dinput(input.begin(), input.end());
//    bolt::amp::device_vector<int> dinput2(input2.begin(), input2.end());
//    bolt::amp::device_vector<int> dmap(map.begin(), map.end());
//
//    std::copy( boost::make_permutation_iterator( input2.begin( ), map.begin( ) ),
//               boost::make_permutation_iterator( input2.end( ), map.end( ) ),
//               result.begin( ) );
//
//    int exp_out = std::inner_product( boost::make_permutation_iterator( input.begin( ), map.begin( ) ),
//                                      boost::make_permutation_iterator( input.end( ), map.end( ) ),
//                                      result.begin( ),
//                                      int( 5 ),
//                                      std::plus<int>( ),
//                                      std::plus<int>( ) );
//
//    bolt::amp::control c;
//    c.setForceRunMode(bolt::amp::control::SerialCpu);
//    int out = bolt::amp::inner_product( c, bolt::amp::make_permutation_iterator( dinput.begin( ), dmap.begin( ) ),
//                                        bolt::amp::make_permutation_iterator( dinput.end( ), dmap.end( ) ),
//                                        bolt::amp::make_permutation_iterator( dinput2.begin( ), dmap.begin( ) ),
//                                        int( 5 ),
//                                        bolt::amp::plus<int>( ),
//                                        bolt::amp::plus<int>( ) );
//    EXPECT_EQ(exp_out, out);
//}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Max_element tests
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//TEST(AMPIterators, PermutationMinElementTest)
//{
//    int n_input[10] =  {0,1,2,3,4,5,6,7,8,9};
//    int n_map[10] =  {9,8,7,6,5,4,3,2,1,0};
//    std::vector<int> input ( n_input, n_input + 10 );
//    std::vector<int> map ( n_map, n_map + 10 );
//    //typedef typename bolt::amp::device_vector<int>::iterator iter
//
//    bolt::amp::device_vector<int> dinput(input.begin(), input.end());
//    bolt::amp::device_vector<int> dmap(map.begin(), map.end());
//   
//    std::vector<int>::iterator stlElement =
//        std::min_element( boost::make_permutation_iterator( input.begin( ), map.begin( ) ), 
//                          boost::make_permutation_iterator( input.begin( ), map.begin( ) ),
//                          std::less< int >( ) );
//    //bolt::amp::permutation_iterator<iter,iter> boltReduce = bolt::amp::min_element(first, last, bolt::amp::less< int >( ));
//
//    //EXPECT_EQ(*stlReduce, *boltReduce);
//
//}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Scan tests
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

TEST(AMPIterators, PermutationScanTest)
{
    int n_input[10] =  {0,1,2,3,4,5,6,7,8,9};
    int n_map[10] =  {9,8,7,6,5,4,3,2,1,0};
    std::vector<int> result ( 10, 0 );
    std::vector<int> exp_result ( 10, 0 );
    std::vector<int> input ( n_input, n_input + 10 );
    std::vector<int> map ( n_map, n_map + 10 );


    bolt::amp::device_vector<int> dresult( result.begin( ), result.end( ) );
    bolt::amp::device_vector<int> dinput( input.begin( ), input.end( ) );
    bolt::amp::device_vector<int> dmap( map.begin( ), map.end( ) );
   
	bolt::amp::inclusive_scan( bolt::amp::make_permutation_iterator( dinput.begin( ), dmap.begin( ) ), 
                               bolt::amp::make_permutation_iterator( dinput.begin( ), dmap.begin( ) ),
                               dresult.begin( ), bolt::amp::plus<int>( ) );

    ::std::partial_sum( boost::make_permutation_iterator( input.begin( ), map.begin( ) ), 
                        boost::make_permutation_iterator( input.begin( ), map.begin( ) ),
                        result.begin( ), std::plus<int>( ) );
    // compare results
    cmpArrays(result, dresult);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Scan_By_Key tests
///////////////////////////////////////////////////////////////////////////////////////////////////////////////


TEST(AMPIterators, PermutationScanByKeyTest)
{
    int n_input[10] =  {0,1,2,3,4,5,6,7,8,9};
    int n_keys[10] = { 7, 0, 0, 3, 3, 3, -5, -5, -5, -5 };
    int n_map[10] =  {9,8,7,6,5,4,3,2,1,0};
    std::vector<int> result ( 10, 0 );
    std::vector<int> exp_result ( 10, 0 );
    std::vector<int> input ( n_input, n_input + 10 );
    std::vector<int> keys ( n_keys, n_keys + 10 );
    std::vector<int> map ( n_map, n_map + 10 );


    bolt::amp::device_vector<int> dresult( result.begin( ), result.end( ) );
    bolt::amp::device_vector<int> dinput( input.begin( ), input.end( ) );
    bolt::amp::device_vector<int> dkeys( keys.begin( ), keys.end( ) );
    bolt::amp::device_vector<int> dmap( map.begin( ), map.end( ) );
       
    bolt::amp::equal_to<int> eq; 
    bolt::amp::multiplies<int> mult; 
   
    bolt::amp::inclusive_scan_by_key( bolt::amp::make_permutation_iterator(dkeys.begin( ),dmap.begin()),
        bolt::amp::make_permutation_iterator(dkeys.end( ),dmap.end()),
        dinput.begin( ), dresult.begin( ), eq, mult );

    gold::scan_by_key( boost::make_permutation_iterator(keys.begin( ),map.begin()),
        boost::make_permutation_iterator(keys.end( ),map.end()), input.begin( ), result.begin( ), mult );
    // compare results
    cmpArrays(result, dresult);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Reduce_By_Key tests
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Add pair support
//TEST(AMPIterators, PermutationReduceByKeyTest)
//{
//    int n_input[10] =  {0,1,2,3,4,5,6,7,8,9};
//    int n_keys[10] = { 7, 0, 0, 3, 3, 3, -5, -5, -5, -5 };
//    int n_map[10] =  {9,8,7,6,5,4,3,2,1,0};
//    std::vector<int> result ( 10, 0 );
//    std::vector<int> exp_result ( 10, 0 );
//    std::vector<int> input ( n_input, n_input + 10 );
//    std::vector<int> keys ( n_keys, n_keys + 10 );
//    std::vector<int> map ( n_map, n_map + 10 );
//
//
//    bolt::amp::device_vector<int> dresult( result.begin( ), result.end( ) );
//    bolt::amp::device_vector<int> dinput( input.begin( ), input.end( ) );
//    bolt::amp::device_vector<int> dkeys( keys.begin( ), keys.end( ) );
//    bolt::amp::device_vector<int> dmap( map.begin( ), map.end( ) );
//
//	std::vector<int>  std_koutput( 10 );
//	std::vector<int>  std_voutput( 10);
//
//    bolt::amp::device_vector<int>  koutput( std_koutput.begin(), std_koutput.end() );
//    bolt::amp::device_vector<int>  voutput( std_voutput.begin(), std_voutput.end() );
//       
//    bolt::amp::equal_to<int> binary_predictor;
//    bolt::amp::plus<int> binary_operator;
//
//   
//    bolt::amp::reduce_by_key( bolt::amp::make_permutation_iterator(dkeys.begin( ),dmap.begin()),
//        bolt::amp::make_permutation_iterator(dkeys.end( ),dmap.end()),
//        dinput.begin( ), koutput.begin( ), voutput.begin( ), binary_predictor, binary_operator );
//
//    gold::reduce_by_key( boost::make_permutation_iterator(keys.begin( ),map.begin()),
//        boost::make_permutation_iterator(keys.end( ),map.end()), input.begin( ),
//        std_koutput.begin( ), std_voutput.begin( ), binary_predictor,  binary_operator );
//    // compare results
//    cmpArrays(result, dresult);
//}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Transform_Scan tests
///////////////////////////////////////////////////////////////////////////////////////////////////////////////


TEST(AMPIterators, PermutationTransformScanTest)
{
    int n_input[10] =  {0,1,2,3,4,5,6,7,8,9};
    int n_map[10] =  {9,8,7,6,5,4,3,2,1,0};
    std::vector<int> result ( 10, 0 );
    std::vector<int> exp_result ( 10, 0 );
    std::vector<int> input ( n_input, n_input + 10 );
    std::vector<int> map ( n_map, n_map + 10 );


    bolt::amp::device_vector<int> dresult( result.begin( ), result.end( ) );
    bolt::amp::device_vector<int> dinput( input.begin( ), input.end( ) );
    bolt::amp::device_vector<int> dmap( map.begin( ), map.end( ) );

    bolt::amp::plus<int> aI2;
    bolt::amp::negate<int> nI2;

    bolt::amp::transform_inclusive_scan(
                            bolt::amp::make_permutation_iterator(dinput.begin(),dmap.begin()),
                            bolt::amp::make_permutation_iterator(dinput.end(),dmap.end()),
                            dresult.begin(), nI2, aI2 );

    ::std::transform( boost::make_permutation_iterator(input.begin(),map.begin()),
                      boost::make_permutation_iterator(input.end(),map.end()),  exp_result.begin(), nI2);
    ::std::partial_sum( exp_result.begin(), exp_result.end(),  exp_result.begin(), aI2);

    cmpArrays(exp_result, dresult);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Count tests
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

template<typename T>
// Functor for range checking.
struct InRange {
    InRange (T low, T high) restrict (amp,cpu) {
        _low=low;
        _high=high;
    };

    bool operator()(const T& value) const restrict (amp,cpu) {
        //printf("Val=%4.1f, Range:%4.1f ... %4.1f\n", value, _low, _high);
        return (value >= _low) && (value <= _high) ;
    };

    T _low;
    T _high;
};


//TEST(AMPIterators, PermutationCountTest)
//{
//    std::vector<int> input(10);
//    std::vector<int> map(10);
//
//    for ( int i=0, j=9 ; i < 10; i++, j--) {
//        input[ i ] = ( i + 1 );
//        map[ i ]  = ( j );
//    };
//
//
//    bolt::amp::device_vector<int> dinput( input.begin( ), input.end( ) );
//    bolt::amp::device_vector<int> dmap( map.begin( ), map.end( ) );
//    //  4996 expected
//    //int stdCount = std::count_if (
//    //                              boost::make_permutation_iterator( input.begin( ), map.begin( ) ),
//    //                              boost::make_permutation_iterator( input.end( ), map.end( ) ),
//    //                              InRange<float>( 6.0f, 10.0f )
//    //                              );
//    //int boltCount = bolt::amp::count_if (
//    //                                     bolt::amp::make_permutation_iterator( dinput.begin( ), dmap.begin( ) ),
//    //                                     bolt::amp::make_permutation_iterator( dinput.end( ), dmap.end( ) ),
//    //                                     InRange<float>( 6, 10 )
//    //                                     );
//
//    //EXPECT_EQ (stdCount, boltCount);
//
//    //auto stdCount = std::count_if (
//    //                              input.begin( ),
//    //                              input.end( )  ,
//    //                              InRange<int>( 6, 10 )
//    //                              );
//    //auto boltCount = bolt::amp::count_if (
//    //                                     bolt::amp::make_permutation_iterator( dinput.begin( ), dmap.begin( ) ),
//    //                                     bolt::amp::make_permutation_iterator( dinput.end( ), dmap.end( ) ),
//    //                                     InRange<int>( 6, 10 )
//    //                                     );
//
//    typedef bolt::amp::permutation_iterator<bolt::amp::device_vector<int>::iterator,
//                                    bolt::amp::device_vector<int>::iterator> pit;
//    pit __pi = bolt::amp::make_permutation_iterator( dinput.begin( ), dmap.begin( ) );
//   for (int i=0; i<10; i++)
//   {
//       //std::cout<<dinput[i]<<"     "<<dmap[i]<<"         "<<*__pi<<std::endl;
//       //__pi++;
//       std::cout<<dinput[i]<<"     "<<dmap[i]<<"         "<<__pi[i]<<std::endl;
//   }
//
//
//    //EXPECT_EQ (stdCount, boltCount);
//
//}

int main(int argc, char* argv[])
{
    ::testing::InitGoogleTest( &argc, &argv[ 0 ] );

    //    Set the standard OpenCL wait behavior to help debugging
    bolt::amp::control& myControl = bolt::amp::control::getDefault( );
    myControl.setWaitMode( bolt::amp::control::NiceWait );
    myControl.setForceRunMode( bolt::amp::control::Automatic );  // choose tbb


    int retVal = RUN_ALL_TESTS( );

#ifdef BUILD_TBB

    bolt::amp::control& myControl = bolt::amp::control::getDefault( );
    myControl.setWaitMode( bolt::amp::control::NiceWait );
    myControl.setForceRunMode( bolt::amp::control::MultiCoreCpu );  // choose tbb


    int retVal = RUN_ALL_TESTS( );

#endif
    
    //  Reflection code to inspect how many tests failed in gTest
    ::testing::UnitTest& unitTest = *::testing::UnitTest::GetInstance( );

    unsigned int failedTests = 0;
    for( int i = 0; i < unitTest.total_test_case_count( ); ++i )
    {
        const ::testing::TestCase& testCase = *unitTest.GetTestCase( i );
        for( int j = 0; j < testCase.total_test_count( ); ++j )
        {
            const ::testing::TestInfo& testInfo = *testCase.GetTestInfo( j );
            if( testInfo.result( )->Failed( ) )
                ++failedTests;
        }
    }

    //  Print helpful message at termination if we detect errors, to help users figure out what to do next
    if( failedTests )
    {
        bolt::tout << _T( "\nFailed tests detected in test pass; please run test again with:" ) << std::endl;
        bolt::tout << _T( "\t--gtest_filter=<XXX> to select a specific failing test of interest" ) << std::endl;
        bolt::tout << _T( "\t--gtest_catch_exceptions=0 to generate minidump of failing test, or" ) << std::endl;      
        bolt::tout << _T( "\t--gtest_break_on_failure to debug interactively with debugger" ) << std::endl;
        bolt::tout << _T( "\t    (only on googletest assertion failures, not SEH exceptions)" ) << std::endl;
    }

    return retVal;


}
