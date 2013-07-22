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
#include "common/myocl.h"
#include "common/test_common.h"

#include "bolt/cl/functional.h"
#include "bolt/cl/iterator/constant_iterator.h"
#include "bolt/cl/iterator/counting_iterator.h"
#include "bolt/miniDump.h"
#include "bolt/cl/gather.h"

#include <gtest/gtest.h>
#include <boost/shared_array.hpp>
#include <boost/range/algorithm/transform.hpp>
#include <boost/iterator/permutation_iterator.hpp>
#include <array>


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// GatherIf tests
/////////////////////////////////////////////////////////////////////////////////////////////////////////////

BOLT_FUNCTOR( is_even,
struct is_even{
    bool operator () (int x)
    {
        return ( x % 2 == 0 );
    }
};
);

TEST( TrivialTest, GatherIfHostMemoryPredicate )
{
    // VS2012 doesn't support initializer list

    //std::vector<int> input {0,1,2,3,4,5,6,7,8,9};
    //std::vector<int> map {9,8,7,6,5,4,3,2,1,0};
    //std::vector<int> stencil {0,1,0,1,0,1,0,1,0,1};
    //std::vector<int> result ( 10, 0 );

    int n_map[10]     =  {0,1,2,3,4,5,6,7,8,9};
    int n_input[10]   =  {9,8,7,6,5,4,3,2,1,0};
    int n_stencil[10] =  {0,1,0,1,0,1,0,1,0,1};

    std::vector<int> exp_result;
    {
        exp_result.push_back(-1);exp_result.push_back(8);
        exp_result.push_back(-1);exp_result.push_back(6);
        exp_result.push_back(-1);exp_result.push_back(4);
        exp_result.push_back(-1);exp_result.push_back(2);
        exp_result.push_back(-1);exp_result.push_back(0);
    }
    std::vector<int> result ( 10, -1 );
    std::vector<int> input ( n_input, n_input + 10 );
    std::vector<int> map ( n_map, n_map + 10 );
    std::vector<int> stencil ( n_stencil, n_stencil + 10 );

    
    is_even iepred;
    bolt::cl::gather_if( map.begin(), map.end(), stencil.begin(), input.begin(), result.begin(), iepred );
    //for(int i=0; i<10 ; i++){ std::cout<<result[ i ]<<std::endl; }

    EXPECT_EQ(exp_result, result);
}

TEST( TrivialTest, GatherIfHostMemory )
{
    int n_map[10] =  {0,1,2,3,4,5,6,7,8,9};
    int n_input[10] =  {9,8,7,6,5,4,3,2,1,0};
    int n_stencil[10] =  {0,1,0,1,0,1,0,1,0,1};

    std::vector<int> exp_result;
    {
        exp_result.push_back(9);exp_result.push_back(0);
        exp_result.push_back(7);exp_result.push_back(0);
        exp_result.push_back(5);exp_result.push_back(0);
        exp_result.push_back(3);exp_result.push_back(0);
        exp_result.push_back(1);exp_result.push_back(0);
    }
    std::vector<int> result ( 10, 0 );
    std::vector<int> input ( n_input, n_input + 10 );
    std::vector<int> map ( n_map, n_map + 10 );
    std::vector<int> stencil ( n_stencil, n_stencil + 10 );

    
    bolt::cl::gather_if( map.begin(), map.end(), stencil.begin(), input.begin(), result.begin() );

    //for(int i=0; i<10 ; i++){ std::cout<<result[ i ]<<std::endl; }
    EXPECT_EQ(exp_result, result);


}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Gather tests
/////////////////////////////////////////////////////////////////////////////////////////////////////////////

TEST( TrivialTest, GatherHostMemory )
{
    int n_map[10] =  {0,1,2,3,4,5,6,7,8,9};
    int n_input[10] =  {9,8,7,6,5,4,3,2,1,0};

    std::vector<int> exp_result;
    {
        exp_result.push_back(9);exp_result.push_back(8);
        exp_result.push_back(7);exp_result.push_back(6);
        exp_result.push_back(5);exp_result.push_back(4);
        exp_result.push_back(3);exp_result.push_back(2);
        exp_result.push_back(1);exp_result.push_back(0);
    }
    std::vector<int> result ( 10, 0 );
    std::vector<int> input ( n_input, n_input + 10 );
    std::vector<int> map ( n_map, n_map + 10 );

    bolt::cl::gather( map.begin(), map.end(), input.begin(), result.begin() );

    //for(int i=0; i<10 ; i++){ std::cout<<result[ i ]<<std::endl; }
    EXPECT_EQ(exp_result, result);


}

TEST( TrivialTest, GatherDeviceMemory )
{
    int n_map[10] =  {0,1,2,3,4,5,6,7,8,9};
    int n_input[10] =  {9,8,7,6,5,4,3,2,1,0};

    bolt::cl::device_vector<int> exp_result;
    {
        exp_result.push_back(9);exp_result.push_back(8);
        exp_result.push_back(7);exp_result.push_back(6);
        exp_result.push_back(5);exp_result.push_back(4);
        exp_result.push_back(3);exp_result.push_back(2);
        exp_result.push_back(1);exp_result.push_back(0);
    }
    bolt::cl::device_vector<int> result ( 10, 0 );
    bolt::cl::device_vector<int> input ( n_input, n_input + 10 );
    bolt::cl::device_vector<int> map ( n_map, n_map + 10 );

    bolt::cl::gather( map.begin(), map.end(), input.begin(), result.begin() );

    //for(int i=0; i<10 ; i++){ std::cout<<result[ i ]<<std::endl; }
    cmpArrays( exp_result, result );

}


TEST(FloatTest, Gather )
{
    size_t sz = 65;
    std::vector<float> exp_result( sz , 0 );
    std::vector<float> std_map ( sz, (float)rand() );
    std::vector<int> std_input ( sz );

    for( int i=0; i < sz ; i++ )
    {
        std_input[i] = i;
    }
    std::random_shuffle( std_input.begin(), std_input.end() );

    bolt::cl::device_vector<float> result ( sz , 0 );
    bolt::cl::device_vector<int> input ( std_input.begin(), std_input.end() );
    bolt::cl::device_vector<float> map ( std_map.begin(), std_map.end() );

    bolt::cl::gather( map.begin(), map.end(), input.begin(), result.begin() );
    boost::transform( std_map,
                      boost::make_permutation_iterator( exp_result.begin(), std_input.begin() ),
                      std::identity<int>( ) );
    cmpArrays( exp_result, result );
}



int main(int argc, char* argv[])
{
    //  Register our minidump generating logic
    bolt::miniDumpSingleton::enableMiniDumps( );

    // Define MEMORYREPORT on windows platfroms to enable debug memory heap checking
#if defined( MEMORYREPORT ) && defined( _WIN32 )
    TCHAR logPath[ MAX_PATH ];
    ::GetCurrentDirectory( MAX_PATH, logPath );
    ::_tcscat_s( logPath, _T( "\\MemoryReport.txt") );

    // We leak the handle to this file, on purpose, so that the ::_CrtSetReportFile() can output it's memory 
    // statistics on app shutdown
    HANDLE hLogFile;
    hLogFile = ::CreateFile( logPath, GENERIC_WRITE, 
        FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );

    ::_CrtSetReportMode( _CRT_ASSERT, _CRTDBG_MODE_FILE | _CRTDBG_MODE_WNDW | _CRTDBG_MODE_DEBUG );
    ::_CrtSetReportMode( _CRT_ERROR, _CRTDBG_MODE_FILE | _CRTDBG_MODE_WNDW | _CRTDBG_MODE_DEBUG );
    ::_CrtSetReportMode( _CRT_WARN, _CRTDBG_MODE_FILE | _CRTDBG_MODE_DEBUG );

    ::_CrtSetReportFile( _CRT_ASSERT, hLogFile );
    ::_CrtSetReportFile( _CRT_ERROR, hLogFile );
    ::_CrtSetReportFile( _CRT_WARN, hLogFile );

    int tmp = ::_CrtSetDbgFlag( _CRTDBG_REPORT_FLAG );
    tmp |= _CRTDBG_LEAK_CHECK_DF | _CRTDBG_ALLOC_MEM_DF | _CRTDBG_CHECK_ALWAYS_DF;
    ::_CrtSetDbgFlag( tmp );

    // By looking at the memory leak report that is generated by this debug heap, there is a number with 
    // {} brackets that indicates the incremental allocation number of that block.  If you wish to set
    // a breakpoint on that allocation number, put it in the _CrtSetBreakAlloc() call below, and the heap
    // will issue a bp on the request, allowing you to look at the call stack
    // ::_CrtSetBreakAlloc( 1833 );

#endif /* MEMORYREPORT */

    ::testing::InitGoogleTest( &argc, &argv[ 0 ] );

    // Set the standard OpenCL wait behavior to help debugging

    //bolt::cl::control& myControl = bolt::cl::control::getDefault( );
    //myControl.waitMode( bolt::cl::control::NiceWait );
    //myControl.forceRunMode( bolt::cl::control::MultiCoreCpu );  // choose tbb

    int retVal = RUN_ALL_TESTS( );

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
