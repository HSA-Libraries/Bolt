/***************************************************************************                                                                                     
*   Copyright 2012 Advanced Micro Devices, Inc.                                     
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
#include "bolt/amp/scan.h"
#include "bolt/unicode.h"
#include "bolt/miniDump.h"
#include <gtest/gtest.h>
#include <array>
#include "bolt/amp/functional.h"

#if 1

// Simple test case for bolt::inclusive_scan:
// Sum together specified numbers, compare against STL::partial_sum function.
// Demonstrates:
//    * use of bolt with STL::array iterators
//    * use of bolt with default plus 
//    * use of bolt with explicit plus argument
//template< size_t arraySize >
//void simpleScanArray( )
//{
	// Binary operator
	//bolt::inclusive_scan( boltA.begin( ), boltA.end(), boltA.begin( ), bolt::plus<int>( ) );

	// Invalid calls
	//bolt::inclusive_scan( boltA.rbegin( ), boltA.rend( ) );  // reverse iterators should not be supported

	//printf ("Sum: stl=%d,  bolt=%d %d %d\n", stlScan, boltScan, boltScan2, boltScan3 );
//};

/////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Below are helper routines to compare the results of two arrays for googletest
//  They return an assertion object that googletest knows how to track
#include "test_common.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Fixture classes are now defined to enable googletest to process type parameterized tests

//  This class creates a C++ 'TYPE' out of a size_t value
template< size_t N >
class TypeValue
{
public:
    static const size_t value = N;
};

//  Explicit initialization of the C++ static const
template< size_t N >
const size_t TypeValue< N >::value;

//  Test fixture class, used for the Type-parameterized tests
//  Namely, the tests that use std::array and TYPED_TEST_P macros
template< typename ArrayTuple >
class ScanArrayTest: public ::testing::Test
{
public:
    ScanArrayTest( ): m_Errors( 0 )
    {}

    virtual void SetUp( )
    {
        for( int i=0; i < ArraySize; i++ )
        {
            stdInput[ i ] = 1;
            boltInput[ i ] = 1;
        }
    };

    virtual void TearDown( )
    {};

    virtual ~ScanArrayTest( )
    {}

protected:
    typedef typename std::tuple_element< 0, ArrayTuple >::type ArrayType;
    // typedef typename std::tuple_element< 0, ArrayTuple >::type::value ArraySize;
    static const size_t ArraySize = typename std::tuple_element< 1, ArrayTuple >::type::value;

    typename std::array< ArrayType, ArraySize > stdInput, boltInput;
    int m_Errors;
};

//  Explicit initialization of the C++ static const
template< typename ArrayTuple >
const size_t ScanArrayTest< ArrayTuple >::ArraySize;

TYPED_TEST_CASE_P( ScanArrayTest );

TYPED_TEST_P( ScanArrayTest, InPlace )
{
    typedef std::array< ArrayType, ArraySize > ArrayCont;

    //  Calling the actual functions under test
    ArrayCont::iterator stdEnd  = std::partial_sum( stdInput.begin( ), stdInput.end( ), stdInput.begin( ) );
    ArrayCont::iterator boltEnd = bolt::amp::inclusive_scan( boltInput.begin( ), boltInput.end( ), boltInput.begin( ) );

    //  The returned iterator should be one past the 
    EXPECT_EQ( stdInput.end( ), stdEnd );
    EXPECT_EQ( boltInput.end( ), boltEnd );

    ArrayCont::difference_type stdNumElements = std::distance( stdInput.begin( ), stdEnd );
    ArrayCont::difference_type boltNumElements = std::distance( boltInput.begin( ), boltEnd );

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpStdArray< ArrayType, ArraySize >::cmpArrays( stdInput, boltInput );
}

TYPED_TEST_P( ScanArrayTest, InPlacePlusFunction )
{
    typedef std::array< ArrayType, ArraySize > ArrayCont;

    //  Calling the actual functions under test
    ArrayCont::iterator stdEnd  = std::partial_sum( stdInput.begin( ), stdInput.end( ), stdInput.begin( ), bolt::amp::plus< ArrayType >( ) );
    ArrayCont::iterator boltEnd = bolt::amp::inclusive_scan( boltInput.begin( ), boltInput.end( ), boltInput.begin( ), bolt::amp::plus< ArrayType >( ) );

    //  The returned iterator should be one past the 
    EXPECT_EQ( stdInput.end( ), stdEnd );
    EXPECT_EQ( boltInput.end( ), boltEnd );

    ArrayCont::difference_type stdNumElements = std::distance( stdInput.begin( ), stdEnd );
    ArrayCont::difference_type boltNumElements = std::distance( boltInput.begin( ), boltEnd );

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpStdArray< ArrayType, ArraySize >::cmpArrays( stdInput, boltInput );
}

TYPED_TEST_P( ScanArrayTest, InPlaceMaxFunction )
{
    typedef std::array< ArrayType, ArraySize > ArrayCont;

    //  Calling the actual functions under test
    ArrayCont::iterator stdEnd  = std::partial_sum( stdInput.begin( ), stdInput.end( ), stdInput.begin( ), bolt::amp::maximum< ArrayType >( ) );
    ArrayCont::iterator boltEnd = bolt::amp::inclusive_scan( boltInput.begin( ), boltInput.end( ), boltInput.begin( ), bolt::amp::maximum< ArrayType >( ) );

    //  The returned iterator should be one past the 
    EXPECT_EQ( stdInput.end( ), stdEnd );
    EXPECT_EQ( boltInput.end( ), boltEnd );

    ArrayCont::difference_type stdNumElements = std::distance( stdInput.begin( ), stdEnd );
    ArrayCont::difference_type boltNumElements = std::distance( boltInput.begin( ), boltEnd );

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpStdArray< ArrayType, ArraySize >::cmpArrays( stdInput, boltInput );
}

TYPED_TEST_P( ScanArrayTest, OutofPlace )
{
    typedef std::array< ArrayType, ArraySize > ArrayCont;

    //  Declare temporary arrays to store results for out of place computation
    ArrayCont stdResult, boltResult;

    //  Calling the actual functions under test, out of place semantics
    ArrayCont::iterator stdEnd  = std::partial_sum( stdInput.begin( ), stdInput.end( ), stdResult.begin( ) );
    ArrayCont::iterator boltEnd = bolt::amp::inclusive_scan( boltInput.begin( ), boltInput.end( ), boltResult.begin( ) );

    //  The returned iterator should be one past the end of the result array
    EXPECT_EQ( stdResult.end( ), stdEnd );
    EXPECT_EQ( boltResult.end( ), boltEnd );

    ArrayCont::difference_type stdNumElements = std::distance( stdResult.begin( ), stdEnd );
    ArrayCont::difference_type boltNumElements = std::distance( boltResult.begin( ), boltEnd );

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpStdArray< ArrayType, ArraySize >::cmpArrays( stdResult, boltResult );
}

REGISTER_TYPED_TEST_CASE_P( ScanArrayTest, InPlace, InPlacePlusFunction, InPlaceMaxFunction, OutofPlace );

/////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Fixture classes are now defined to enable googletest to process value parameterized tests

//  ::testing::TestWithParam< int > means that GetParam( ) returns int values, which i use for array size
class ScanIntegerVector: public ::testing::TestWithParam< int >
{
public:
    //  Create an std and a bolt vector of requested size, and initialize all the elements to 1
    ScanIntegerVector( ): stdInput( GetParam( ), 1 ), boltInput( GetParam( ), 1 )
    {}

protected:
    std::vector< int > stdInput, boltInput;
};

//  ::testing::TestWithParam< int > means that GetParam( ) returns int values, which i use for array size
class ScanFloatVector: public ::testing::TestWithParam< int >
{
public:
    //  Create an std and a bolt vector of requested size, and initialize all the elements to 1
    ScanFloatVector( ): stdInput( GetParam( ), 1.0f ), boltInput( GetParam( ), 1.0f )
    {}

protected:
    std::vector< float > stdInput, boltInput;
};

//  ::testing::TestWithParam< int > means that GetParam( ) returns int values, which i use for array size
class ScanDoubleVector: public ::testing::TestWithParam< int >
{
public:
    //  Create an std and a bolt vector of requested size, and initialize all the elements to 1
    ScanDoubleVector( ): stdInput( GetParam( ), 1.0 ), boltInput( GetParam( ), 1.0 )
    {}

protected:
    std::vector< double > stdInput, boltInput;
};

TEST_P( ScanIntegerVector, InclusiveInplace )
{
    //  Calling the actual functions under test
    std::vector< int >::iterator stdEnd  = std::partial_sum( stdInput.begin( ), stdInput.end( ), stdInput.begin( ) );
    std::vector< int >::iterator boltEnd = bolt::amp::inclusive_scan( boltInput.begin( ), boltInput.end( ), boltInput.begin( ) );

    //  The returned iterator should be one past the 
    EXPECT_EQ( stdInput.end( ), stdEnd );
    EXPECT_EQ( boltInput.end( ), boltEnd );

    std::vector< int >::iterator::difference_type stdNumElements = std::distance( stdInput.begin( ), stdEnd );
    std::vector< int >::iterator::difference_type boltNumElements = std::distance( boltInput.begin( ), boltEnd );

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdInput, boltInput );
}

TEST_P( ScanFloatVector, InclusiveInplace )
{
    //  Calling the actual functions under test
    std::vector< float >::iterator stdEnd  = std::partial_sum( stdInput.begin( ), stdInput.end( ), stdInput.begin( ) );
    std::vector< float >::iterator boltEnd = bolt::amp::inclusive_scan( boltInput.begin( ), boltInput.end( ), boltInput.begin( ) );

    //  The returned iterator should be one past the 
    EXPECT_EQ( stdInput.end( ), stdEnd );
    EXPECT_EQ( boltInput.end( ), boltEnd );

    std::vector< float >::iterator::difference_type stdNumElements = std::distance( stdInput.begin( ), stdEnd );
    std::vector< float >::iterator::difference_type boltNumElements = std::distance( boltInput.begin( ), boltEnd );

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdInput, boltInput );
}

TEST_P( ScanDoubleVector, InclusiveInplace )
{
    //  Calling the actual functions under test
    std::vector< double >::iterator stdEnd  = std::partial_sum( stdInput.begin( ), stdInput.end( ), stdInput.begin( ) );
    std::vector< double >::iterator boltEnd = bolt::amp::inclusive_scan( boltInput.begin( ), boltInput.end( ), boltInput.begin( ) );

    //  The returned iterator should be one past the 
    EXPECT_EQ( stdInput.end( ), stdEnd );
    EXPECT_EQ( boltInput.end( ), boltEnd );

    std::vector< double >::iterator::difference_type stdNumElements = std::distance( stdInput.begin( ), stdEnd );
    std::vector< double >::iterator::difference_type boltNumElements = std::distance( boltInput.begin( ), boltEnd );

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdInput, boltInput );
}

//  Test lots of consecutive numbers, but small range, suitable for integers because they overflow easier
INSTANTIATE_TEST_CASE_P( Inclusive, ScanIntegerVector, ::testing::Range( 0, 1024, 1 ) );

//  Test a huge range, suitable for floating point as they are less prone to overflow (but floating point loses granularity at large values)
INSTANTIATE_TEST_CASE_P( Inclusive, ScanFloatVector, ::testing::Range( 0, 1048576, 4096 ) );
INSTANTIATE_TEST_CASE_P( Inclusive, ScanDoubleVector, ::testing::Range( 0, 1048576, 4096 ) );

typedef ::testing::Types< 
    std::tuple< int, TypeValue< 1 > >,
    std::tuple< int, TypeValue< 31 > >,
    std::tuple< int, TypeValue< 32 > >,
    std::tuple< int, TypeValue< 63 > >,
    std::tuple< int, TypeValue< 64 > >,
    std::tuple< int, TypeValue< 127 > >,
    std::tuple< int, TypeValue< 128 > >,
    std::tuple< int, TypeValue< 129 > >,
    std::tuple< int, TypeValue< 1000 > >,
    std::tuple< int, TypeValue< 1053 > >,
    std::tuple< int, TypeValue< 4096 > >,
    std::tuple< int, TypeValue< 4097 > >,
    std::tuple< int, TypeValue< 65535 > >,
    std::tuple< int, TypeValue< 65536 > >
> IntegerTests;

typedef ::testing::Types< 
    std::tuple< float, TypeValue< 1 > >,
    std::tuple< float, TypeValue< 31 > >,
    std::tuple< float, TypeValue< 32 > >,
    std::tuple< float, TypeValue< 63 > >,
    std::tuple< float, TypeValue< 64 > >,
    std::tuple< float, TypeValue< 127 > >,
    std::tuple< float, TypeValue< 128 > >,
    std::tuple< float, TypeValue< 129 > >,
    std::tuple< float, TypeValue< 1000 > >,
    std::tuple< float, TypeValue< 1053 > >,
    std::tuple< float, TypeValue< 4096 > >,
    std::tuple< float, TypeValue< 4097 > >,
    std::tuple< float, TypeValue< 65535 > >,
    std::tuple< float, TypeValue< 65536 > >
> FloatTests;

INSTANTIATE_TYPED_TEST_CASE_P( Integer, ScanArrayTest, IntegerTests );
INSTANTIATE_TYPED_TEST_CASE_P( Float, ScanArrayTest, FloatTests );

#endif

int _tmain(int argc, _TCHAR* argv[])
{
    std::cout << "#######################################################################################" << std::endl;
    std::cout << "#######################################################################################" << std::endl;
    std::cout << "#######################################################################################" << std::endl;
    std::cout << "#######################################################################################" << std::endl;
    std::cout << "#######################################################################################" << std::endl;
    std::cout << "#######################################################################################" << std::endl;
    std::cout << "#######################################################################################" << std::endl;
    std::cout << "#######################################################################################" << std::endl;
    std::cout << "#######################################################################################" << std::endl;
    std::cout << "#######################################################################################" << std::endl;

    ::testing::InitGoogleTest( &argc, &argv[ 0 ] );

    //  Register our minidump generating logic
    bolt::miniDumpSingleton::enableMiniDumps( );

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
