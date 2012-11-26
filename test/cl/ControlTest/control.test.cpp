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

#include "stdafx.h"

#include <vector>
#include <array>

#include <bolt/cl/control.h>
#include <bolt/cl/functional.h>
#include <bolt/cl/device_vector.h>
#include <bolt/unicode.h>
#include <bolt/miniDump.h>

#include <gtest/gtest.h>

/////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Below are helper routines to compare the results of two arrays for googletest
//  They return an assertion object that googletest knows how to track

template< typename T, size_t N >
::testing::AssertionResult cmpArrays( const T (&ref)[N], const T (&calc)[N] )
{
    for( size_t i = 0; i < N; ++i )
    {
        EXPECT_EQ( ref[ i ], calc[ i ] ) << _T( "Where i = " ) << i;
    }

    return ::testing::AssertionSuccess( );
}

//  Primary class template for std::array types
//  The struct wrapper is necessary to partially specialize the member function
template< typename T, size_t N >
struct cmpStdArray
{
    static ::testing::AssertionResult cmpArrays( const std::array< T, N >& ref, const std::array< T, N >& calc )
    {
        for( size_t i = 0; i < N; ++i )
        {
            EXPECT_EQ( ref[ i ], calc[ i ] ) << _T( "Where i = " ) << i;
        }

        return ::testing::AssertionSuccess( );
    }
};

//  Partial template specialization for float types
//  Partial template specializations only works for objects, not functions
template< size_t N >
struct cmpStdArray< float, N >
{
    static ::testing::AssertionResult cmpArrays( const std::array< float, N >& ref, const std::array< float, N >& calc )
    {
        for( size_t i = 0; i < N; ++i )
        {
            EXPECT_FLOAT_EQ( ref[ i ], calc[ i ] ) << _T( "Where i = " ) << i;
        }

        return ::testing::AssertionSuccess( );
    }
};

//  Partial template specialization for float types
//  Partial template specializations only works for objects, not functions
template< size_t N >
struct cmpStdArray< double, N >
{
    static ::testing::AssertionResult cmpArrays( const std::array< double, N >& ref, const std::array< double, N >& calc )
    {
        for( size_t i = 0; i < N; ++i )
        {
            EXPECT_DOUBLE_EQ( ref[ i ], calc[ i ] ) << _T( "Where i = " ) << i;
        }

        return ::testing::AssertionSuccess( );
    }
};

//  The following cmpArrays verify the correctness of std::vectors's
template< typename T >
::testing::AssertionResult cmpArrays( const std::vector< T >& ref, const std::vector< T >& calc )
{
    for( size_t i = 0; i < ref.size( ); ++i )
    {
        EXPECT_EQ( ref[ i ], calc[ i ] ) << _T( "Where i = " ) << i;
    }

    return ::testing::AssertionSuccess( );
}

::testing::AssertionResult cmpArrays( const std::vector< float >& ref, const std::vector< float >& calc )
{
    for( size_t i = 0; i < ref.size( ); ++i )
    {
        EXPECT_FLOAT_EQ( ref[ i ], calc[ i ] ) << _T( "Where i = " ) << i;
    }

    return ::testing::AssertionSuccess( );
}

::testing::AssertionResult cmpArrays( const std::vector< double >& ref, const std::vector< double >& calc )
{
    for( size_t i = 0; i < ref.size( ); ++i )
    {
        EXPECT_DOUBLE_EQ( ref[ i ], calc[ i ] ) << _T( "Where i = " ) << i;
    }

    return ::testing::AssertionSuccess( );
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Fixture classes are now defined to enable googletest to process value parameterized tests

//  ::testing::TestWithParam< int > means that GetParam( ) returns int values, which i use for array size
class ControlTest: public testing::Test 
{
public:
    //  Create an std and a bolt vector of requested size, and initialize all the elements to 1
    ControlTest( ): myControl( bolt::cl::control::getDefault( ) )
    {}

    virtual void SetUp( )
    {
    };

    virtual void TearDown( )
    {
        myControl.freePrivateMemory( );
    };

protected:
    bolt::cl::control& myControl;
};

TEST_F( ControlTest, init )
{
    size_t internalBuffSize = myControl.privateMemorySize( );

    EXPECT_EQ( 0, internalBuffSize );
}

TEST_F( ControlTest, zeroMemory )
{
    myControl.acquireBuffer( 100 * sizeof( int ) );

    size_t internalBuffSize = myControl.privateMemorySize( );
    EXPECT_EQ( 400, internalBuffSize );

    myControl.freePrivateMemory( );
    internalBuffSize = myControl.privateMemorySize( );
    EXPECT_EQ( 0, internalBuffSize );
}

TEST_F( ControlTest, acquire1Buffer )
{
    bolt::cl::control::buffPointer myBuff = myControl.acquireBuffer( 100 * sizeof( int ) );
    cl_uint myRefCount = myBuff->getInfo< CL_MEM_REFERENCE_COUNT >( );
    EXPECT_EQ( 1, myRefCount );

    size_t internalBuffSize = myControl.privateMemorySize( );
    EXPECT_EQ( 400, internalBuffSize );
}

TEST_F( ControlTest, acquire1BufferReleaseAcquireSame )
{
    bolt::cl::control::buffPointer myBuff = myControl.acquireBuffer( 100 * sizeof( int ) );
    cl_uint myRefCount = myBuff->getInfo< CL_MEM_REFERENCE_COUNT >( );
    EXPECT_EQ( 1, myRefCount );
    myBuff.reset( );

    myBuff = myControl.acquireBuffer( 100 * sizeof( int ) );
    myRefCount = myBuff->getInfo< CL_MEM_REFERENCE_COUNT >( );
    EXPECT_EQ( 1, myRefCount );

    size_t internalBuffSize = myControl.privateMemorySize( );
    EXPECT_EQ( 400, internalBuffSize );
}

TEST_F( ControlTest, acquire1BufferReleaseAcquireSmaller )
{
    bolt::cl::control::buffPointer myBuff = myControl.acquireBuffer( 100 * sizeof( int ) );
    cl_uint myRefCount = myBuff->getInfo< CL_MEM_REFERENCE_COUNT >( );
    EXPECT_EQ( 1, myRefCount );
    myBuff.reset( );

    myBuff = myControl.acquireBuffer( 99 * sizeof( int ) );
    myRefCount = myBuff->getInfo< CL_MEM_REFERENCE_COUNT >( );
    EXPECT_EQ( 1, myRefCount );

    size_t internalBuffSize = myControl.privateMemorySize( );
    EXPECT_EQ( 400, internalBuffSize );
}

TEST_F( ControlTest, acquire1BufferReleaseAcquireBigger )
{
    bolt::cl::control::buffPointer myBuff = myControl.acquireBuffer( 100 * sizeof( int ) );
    cl_uint myRefCount = myBuff->getInfo< CL_MEM_REFERENCE_COUNT >( );
    EXPECT_EQ( 1, myRefCount );
    myBuff.reset( );

    myBuff = myControl.acquireBuffer( 101 * sizeof( int ) );
    myRefCount = myBuff->getInfo< CL_MEM_REFERENCE_COUNT >( );
    EXPECT_EQ( 1, myRefCount );

    size_t internalBuffSize = myControl.privateMemorySize( );
    EXPECT_EQ( 404, internalBuffSize );
}

TEST_F( ControlTest, acquire2BufferEqual )
{
    bolt::cl::control::buffPointer myBuff1 = myControl.acquireBuffer( 100 * sizeof( int ) );
    cl_uint myRefCount1 = myBuff1->getInfo< CL_MEM_REFERENCE_COUNT >( );
    EXPECT_EQ( 1, myRefCount1 );

    bolt::cl::control::buffPointer myBuff2 = myControl.acquireBuffer( 100 * sizeof( int ) );
    cl_uint myRefCount2 = myBuff2->getInfo< CL_MEM_REFERENCE_COUNT >( );
    EXPECT_EQ( 1, myRefCount2 );

    size_t internalBuffSize = myControl.privateMemorySize( );
    EXPECT_EQ( 800, internalBuffSize );
}

TEST_F( ControlTest, acquire2BufferBigger )
{
    bolt::cl::control::buffPointer myBuff1 = myControl.acquireBuffer( 100 * sizeof( int ) );
    cl_uint myRefCount1 = myBuff1->getInfo< CL_MEM_REFERENCE_COUNT >( );
    EXPECT_EQ( 1, myRefCount1 );

    bolt::cl::control::buffPointer myBuff2 = myControl.acquireBuffer( 101 * sizeof( int ) );
    cl_uint myRefCount2 = myBuff2->getInfo< CL_MEM_REFERENCE_COUNT >( );
    EXPECT_EQ( 1, myRefCount2 );

    size_t internalBuffSize = myControl.privateMemorySize( );
    EXPECT_EQ( 804, internalBuffSize );
}

TEST_F( ControlTest, acquire2BufferSmaller )
{
    bolt::cl::control::buffPointer myBuff1 = myControl.acquireBuffer( 100 * sizeof( int ) );
    cl_uint myRefCount1 = myBuff1->getInfo< CL_MEM_REFERENCE_COUNT >( );
    EXPECT_EQ( 1, myRefCount1 );

    bolt::cl::control::buffPointer myBuff2 = myControl.acquireBuffer( 99 * sizeof( int ) );
    cl_uint myRefCount2 = myBuff2->getInfo< CL_MEM_REFERENCE_COUNT >( );
    EXPECT_EQ( 1, myRefCount2 );

    size_t internalBuffSize = myControl.privateMemorySize( );
    EXPECT_EQ( 796, internalBuffSize );
}

int _tmain(int argc, _TCHAR* argv[])
{
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