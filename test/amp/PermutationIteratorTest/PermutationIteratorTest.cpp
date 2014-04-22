############################################################################

#   Copyright 2012 - 2013 Advanced Micro Devices, Inc.
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.

############################################################################

#include "common/stdafx.h"
#include "bolt/amp/transform.h"
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
//Teststotestthecountingiterator

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


//INSTANTIATE_TYPED_TEST_CASE_P( UDDTest, TransformArrayTest, UDDTests );
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

    // Everyday 'am shufflin'
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

    intvpi first, last, i;
    first = bolt::amp::make_permutation_iterator(__kbegin, __ebegin);
    //i = first;
    last = bolt::amp::make_permutation_iterator(__kend, __eend);

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
    boost::transform( exp_result, boost::make_permutation_iterator( input.begin(), map.begin() ),bolt::amp::identity<int>( ) );

    //bolt::amp::transform( dinput.begin(), dinput.end(), bolt::amp::make_permutation_iterator( dresult.end(), dmap.end() ), bolt::amp::identity<int>( ) );
    bolt::amp::transform( bolt::amp::make_permutation_iterator( dinput.begin(), dmap.begin() ),
                          bolt::amp::make_permutation_iterator( dinput.end(), dmap.end() ),
                          dresult.begin(),
                          bolt::amp::identity<int>( ) );
    cmpArrays(exp_result, result);
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
