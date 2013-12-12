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

#define TEST_DOUBLE 0
#define TEST_DEVICE_VECTOR 1
#define TEST_CPU_DEVICE 0
#define TEST_MULTICORE_TBB_SEARCH 1
#define TEST_LARGE_BUFFERS 0
#define GOOGLE_TEST 1
#define BKND cl 
#define SEARCH_FUNC binary_search

#include <gtest/gtest.h>
#include "common/stdafx.h"
#include "common/test_common.h"
#include <bolt/amp/BinarySearch.h>

#include <bolt/amp/sort.h>
#include <bolt/miniDump.h>
//#include <bolt/unicode.h>
#include <boost/shared_array.hpp>
#include <array>
#include <algorithm>

/////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Below are helper routines to compare the results of two arrays for googletest
//  They return an assertion object that googletest knows how to track
//This is a compare routine for naked pointers.


TEST(BSearch, Normal)  
{
        // test length
        int length = (128);

        std::vector<int> bolt_source(length);
        std::vector<int> std_source(length);

        // populate source vector with random ints
        for (int j = 0; j < length; j++)
        {
            bolt_source[j] = (int)rand();
            std_source[j] = bolt_source[j];
        }
    
        int search_index = rand()%length;
        int val = bolt_source[search_index];
        int std_val = std_source[search_index];
        bool stdresult, boltresult;

        //Sorting the Input
        std::sort(std_source.begin(), std_source.end());
        bolt::amp::sort(bolt_source.begin(), bolt_source.end());

        // perform search
        stdresult = std::binary_search(std_source.begin(), std_source.end(), std_val);
        std::cout<<stdresult<<"\n";
        boltresult = bolt::amp::binary_search(bolt_source.begin(), bolt_source.end(), val);		
        std::cout<<boltresult<<"\n";

        // GoogleTest Comparison
        EXPECT_EQ( stdresult ,  boltresult);

} 


int main(int argc, char* argv[])
{
    ::testing::InitGoogleTest( &argc, &argv[ 0 ] );

    //  Register our minidump generating logic
    //bolt::miniDumpSingleton::enableMiniDumps( );

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
