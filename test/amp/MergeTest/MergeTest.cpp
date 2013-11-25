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
#include "bolt/amp/merge.h"
#include "bolt/unicode.h"
#include <gtest/gtest.h>
#include <array>

//  A very generic template that takes two container, and compares their values assuming a vector interface
template< typename S, typename B >
::testing::AssertionResult cmpArrays(const S& ref, const B& calc)
{
	for (size_t i = 0; i < ref.size(); ++i)
	{
		EXPECT_EQ(ref[i], calc[i]) << _T("Where i = ") << i;
	}

	return ::testing::AssertionSuccess();
}

struct UDD
{
	int a;
	int b;

	bool operator() (const UDD& lhs, const UDD& rhs) const {
		return ((lhs.a + lhs.b) > (rhs.a + rhs.b));
	}
	bool operator < (const UDD& other) const {
		return ((a + b) < (other.a + other.b));
	}
	bool operator > (const UDD& other) const {
		return ((a + b) > (other.a + other.b));
	}
	bool operator == (const UDD& other) const restrict (cpu,amp)  {
		return ((a + b) == (other.a + other.b));
	}

	UDD operator + (const UDD &rhs) const
	{
		UDD _result;
		_result.a = a + rhs.a;
		_result.b = b + rhs.b;
		return _result;
	}

	UDD()
		: a(0), b(0) { }
	UDD(int _in)
		: a(_in), b(_in + 1)  { }
};


struct UDDless
{
	bool operator() (const UDD &lhs, const UDD &rhs) const restrict(cpu, amp)
	{

		if ((lhs.a + lhs.b) < (rhs.a + rhs.b))
			return true;
		else
			return false;
	}

};


TEST(MergeUDD, UDDPlusOperatorInts)
{
	int length = 1 << 8;

	std::vector<UDD> std1_source(length);
	std::vector<UDD> std2_source(length);
	std::vector<UDD> std_res(length * 2);
	std::vector<UDD> bolt_res(length * 2);

	// populate source vector with random ints
	for (int j = 0; j < length; j++)
	{
		std1_source[j].a = rand();
		std1_source[j].b = rand();
		std2_source[j].a = rand();
		std2_source[j].b = rand();
	}

	// perform sort
	std::sort(std1_source.begin(), std1_source.end());
	std::sort(std2_source.begin(), std2_source.end());

	UDDless lessop;

	std::merge(std1_source.begin(), std1_source.end(),
		std2_source.begin(), std2_source.end(),
		std_res.begin(), lessop);


	bolt::amp::merge(std1_source.begin(), std1_source.end(),
		std2_source.begin(), std2_source.end(),
		bolt_res.begin(), lessop);

	// GoogleTest Comparison
	cmpArrays(std_res, bolt_res);

}


TEST(MergeTest, MergeAuto)
{
	int stdVectSize1 = 10;
	int stdVectSize2 = 20;

	std::vector<int> A(stdVectSize1);
	std::vector<int> B(stdVectSize1);
	std::vector<int> stdmerge(stdVectSize2);
	std::vector<int> boltmerge(stdVectSize2);

	for (int i = 0; i < stdVectSize1; i++){
		A[i] = 10;
		B[i] = 20;
	}

	std::merge(A.begin(), A.end(), B.begin(), B.end(), stdmerge.begin());
	bolt::amp::control ctl;
	ctl.setForceRunMode(bolt::amp::control::Automatic);
	bolt::amp::merge(ctl, A.begin(), A.end(), B.begin(), B.end(), boltmerge.begin());

	for (int i = 0; i < stdVectSize2; i++) {
		EXPECT_EQ(boltmerge[i], stdmerge[i]);
	}
}




int main(int argc, char* argv[])
{

    
    ::testing::InitGoogleTest( &argc, &argv[ 0 ] );


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

