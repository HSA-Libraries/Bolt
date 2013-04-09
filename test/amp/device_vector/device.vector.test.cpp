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

#include "stdafx.h"
#include <vector>
#include <array>
#include <bolt/unicode.h>
#include <bolt/miniDump.h>
#include <gtest/gtest.h>
///////////////////////////////////////////////////////////////////////////////////////
//CL and AMP device_vector tests are integrated.To use AMP tests change AMP_TESTS to 1
///////////////////////////////////////////////////////////////////////////////////////
//#define AMP_TESTS 0

#if AMP_TESTS
    #include <bolt/amp/functional.h>
    #include <bolt/amp/device_vector.h>
    #define BCKND amp

#else

    #include <bolt/cl/functional.h>
    #include <bolt/cl/device_vector.h>
    #include <bolt/cl/fill.h>
    #define BCKND cl

#endif

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
//  Fixture classes are now defined to enable googletest to process type parameterized tests

//  This class creates a C++ 'TYPE' out of a size_t value
template< size_t N >
class TypeValue
{
public:
    static const size_t value = N;
};

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

TYPED_TEST_CASE_P( ScanArrayTest );

TYPED_TEST_P( ScanArrayTest, InPlace )
{
    typedef std::array< ArrayType, ArraySize > ArrayCont;

}

TYPED_TEST_P( ScanArrayTest, InPlacePlusFunction )
{
    typedef std::array< ArrayType, ArraySize > ArrayCont;

}

TYPED_TEST_P( ScanArrayTest, InPlaceMaxFunction )
{
    typedef std::array< ArrayType, ArraySize > ArrayCont;

}

TYPED_TEST_P( ScanArrayTest, OutofPlace )
{
    typedef std::array< ArrayType, ArraySize > ArrayCont;

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
}

TEST_P( ScanFloatVector, InclusiveInplace )
{
}

TEST_P( ScanDoubleVector, InclusiveInplace )
{
}

////  Test lots of consecutive numbers, but small range, suitable for integers because they overflow easier
//INSTANTIATE_TEST_CASE_P( Inclusive, ScanIntegerVector, ::testing::Range( 0, 1024, 1 ) );
//
////  Test a huge range, suitable for floating point as they are less prone to overflow (but floating point loses granularity at large values)
//INSTANTIATE_TEST_CASE_P( Inclusive, ScanFloatVector, ::testing::Range( 0, 1048576, 4096 ) );
//INSTANTIATE_TEST_CASE_P( Inclusive, ScanDoubleVector, ::testing::Range( 0, 1048576, 4096 ) );

typedef ::testing::Types< 
    std::tuple< int, TypeValue< 1 > >,
    //std::tuple< int, TypeValue< bolt::scanMultiCpuThreshold - 1 > >,
    //std::tuple< int, TypeValue< bolt::scanGpuThreshold - 1 > >,
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
    //std::tuple< int, TypeValue< 131032 > >,       // uncomment these to generate failures; stack overflow
    //std::tuple< int, TypeValue< 262154 > >,
    std::tuple< int, TypeValue< 65536 > >
> IntegerTests;

typedef ::testing::Types< 
    std::tuple< float, TypeValue< 1 > >,
    //std::tuple< float, TypeValue< bolt::scanMultiCpuThreshold - 1 > >,
    //std::tuple< float, TypeValue< bolt::scanGpuThreshold - 1 > >,
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

//INSTANTIATE_TYPED_TEST_CASE_P( Integer, ScanArrayTest, IntegerTests );
//INSTANTIATE_TYPED_TEST_CASE_P( Float, ScanArrayTest, FloatTests );

TEST( Constructor, ContainerIteratorEmpty )
{
    bolt::BCKND::device_vector< int > dV;

    EXPECT_EQ( 0, dV.size( ) );

    bolt::BCKND::device_vector< int >::iterator itBegin = dV.begin( );
    bolt::BCKND::device_vector< int >::iterator itEnd = dV.end( );

    EXPECT_TRUE( itBegin == itEnd );
    EXPECT_TRUE( dV.empty() );
}

TEST( Constructor, ConstContainerConstIteratorEmpty )
{
    const bolt::BCKND::device_vector< int > dV;

    EXPECT_EQ( 0, dV.size( ) );

    bolt::BCKND::device_vector< int >::const_iterator itBegin = dV.begin( );
    bolt::BCKND::device_vector< int >::const_iterator itEnd = dV.end( );

    EXPECT_TRUE( itBegin == itEnd );
}

TEST( Constructor, ConstContainerConstIteratorCEmpty )
{
    const bolt::BCKND::device_vector< int > dV;

    EXPECT_EQ( 0, dV.size( ) );

    bolt::BCKND::device_vector< int >::const_iterator itBegin = dV.cbegin( );
    bolt::BCKND::device_vector< int >::const_iterator itEnd = dV.cend( );

    EXPECT_TRUE( itBegin == itEnd );
}

TEST( Constructor, ContainerConstIteratorCEmpty )
{
    bolt::BCKND::device_vector< int > dV;

    EXPECT_EQ( 0, dV.size( ) );

    bolt::BCKND::device_vector< int >::const_iterator itBegin = dV.cbegin( );
    bolt::BCKND::device_vector< int >::const_iterator itEnd = dV.cend( );

    EXPECT_TRUE( itBegin == itEnd );
}

TEST( Constructor, ContainerConstIteratorEmpty )
{
    bolt::BCKND::device_vector< int > dV;

    EXPECT_EQ( 0, dV.size( ) );

    bolt::BCKND::device_vector< int >::const_iterator itBegin = dV.begin( );
    bolt::BCKND::device_vector< int >::const_iterator itEnd = dV.end( );

    EXPECT_TRUE( itBegin == itEnd );
}

TEST( Constructor, Size5AndValue3OperatorValueType )
{
    bolt::BCKND::device_vector< int > dV( 5, 3 );
    EXPECT_EQ( 5, dV.size( ) );

    EXPECT_EQ( 3, dV[ 0 ] );
    EXPECT_EQ( 3, dV[ 1 ] );
    EXPECT_EQ( 3, dV[ 2 ] );
    EXPECT_EQ( 3, dV[ 3 ] );
    EXPECT_EQ( 3, dV[ 4 ] );
}

TEST( Iterator, Compatibility )
{
    bolt::BCKND::device_vector< int > dV;
    EXPECT_EQ( 0, dV.size( ) );

    bolt::BCKND::device_vector< int >::iterator Iter0( dV, 0 );
    bolt::BCKND::device_vector< int >::const_iterator cIter0( dV, 0 );
    EXPECT_TRUE( Iter0 == cIter0 );

    bolt::BCKND::device_vector< int >::iterator Iter1( dV, 0 );
    bolt::BCKND::device_vector< int >::const_iterator cIter1( dV, 1 );
    EXPECT_TRUE( Iter1 != cIter1 );
}

TEST( Iterator, OperatorEqual )
{
    bolt::BCKND::device_vector< int > dV;
    EXPECT_EQ( 0, dV.size( ) );

    bolt::BCKND::device_vector< int >::iterator Iter0( dV, 0 );
    bolt::BCKND::device_vector< int >::iterator cIter0( dV, 0 );
    EXPECT_TRUE( Iter0 == cIter0 );

    bolt::BCKND::device_vector< int >::const_iterator Iter1( dV, 0 );
    bolt::BCKND::device_vector< int >::const_iterator cIter1( dV, 1 );
    EXPECT_TRUE( Iter1 != cIter1 );

    bolt::BCKND::device_vector< int > dV2;

    bolt::BCKND::device_vector< int >::const_iterator Iter2( dV, 0 );
    bolt::BCKND::device_vector< int >::const_iterator cIter2( dV2, 0 );
    EXPECT_TRUE( Iter2 != cIter2 );
}

//TODO Add all test cases for Reverse and Const Reverse Iterator
// insert/erase using base(), self, and constructing a base iterator

TEST( Constructor, ContainerReverseIteratorEmpty )
{
    bolt::BCKND::device_vector< int > dV;

    EXPECT_EQ( 0, dV.size( ) );

    bolt::BCKND::device_vector< int >::reverse_iterator itBegin = dV.rbegin( );
    bolt::BCKND::device_vector< int >::reverse_iterator itEnd = dV.rend( );

    EXPECT_TRUE( itBegin == itEnd );    
}

TEST( Constructor, ConstContainerConstReverseIteratorEmpty )
{
    const bolt::BCKND::device_vector< int > dV;

    EXPECT_EQ( 0, dV.size( ) );

    bolt::BCKND::device_vector< int >::const_reverse_iterator itBegin = dV.rbegin( );
    bolt::BCKND::device_vector< int >::const_reverse_iterator itEnd = dV.rend( );

    EXPECT_TRUE( itBegin == itEnd );
    
}

TEST( Constructor, ConstContainerConstReverseIteratorCEmpty )
{
    const bolt::BCKND::device_vector< int > dV;

    EXPECT_EQ( 0, dV.size( ) );

    bolt::BCKND::device_vector< int >::const_reverse_iterator itBegin = dV.crbegin( );
    bolt::BCKND::device_vector< int >::const_reverse_iterator itEnd = dV.crend( );

    EXPECT_TRUE( itBegin == itEnd );
}

TEST( Constructor, ContainerConstReverseIteratorCEmpty )
{
    bolt::BCKND::device_vector< int > dV;

    EXPECT_EQ( 0, dV.size( ) );

    bolt::BCKND::device_vector< int >::const_reverse_iterator itBegin = dV.crbegin( );
    bolt::BCKND::device_vector< int >::const_reverse_iterator itEnd = dV.crend( );

    EXPECT_TRUE( itBegin == itEnd );
}

TEST( Constructor, ContainerConstReverseIteratorEmpty )
{
    bolt::BCKND::device_vector< int > dV;

    EXPECT_EQ( 0, dV.size( ) );

    bolt::BCKND::device_vector< int >::const_reverse_iterator itBegin = dV.rbegin( );
    bolt::BCKND::device_vector< int >::const_reverse_iterator itEnd = dV.rend( );

    EXPECT_TRUE( itBegin == itEnd );
}


TEST( ReverseIterator, CompatibilityReverse )
{
    bolt::BCKND::device_vector< int > dV;
    EXPECT_EQ( 0, dV.size( ) );

    bolt::BCKND::device_vector< int >::reverse_iterator Iter0( dV, 0 );
    bolt::BCKND::device_vector< int >::const_reverse_iterator cIter0( dV, 0 );
    EXPECT_TRUE( Iter0 == cIter0 );

    bolt::BCKND::device_vector< int >::reverse_iterator Iter1( dV, 0 );
    bolt::BCKND::device_vector< int >::const_reverse_iterator cIter1( dV, 1 );
    EXPECT_TRUE( Iter1 != cIter1 );
}

TEST( ReverseIterator, OperatorEqualReverse )
{
    bolt::BCKND::device_vector< int > dV;
    EXPECT_EQ( 0, dV.size( ) );

    bolt::BCKND::device_vector< int >::reverse_iterator Iter0( dV, 0 );
    bolt::BCKND::device_vector< int >::reverse_iterator cIter0( dV, 0 );
    EXPECT_TRUE( Iter0 == cIter0 );

    bolt::BCKND::device_vector< int >::const_reverse_iterator Iter1( dV, 0 );
    bolt::BCKND::device_vector< int >::const_reverse_iterator cIter1( dV, 1 );
    EXPECT_TRUE( Iter1 != cIter1 );

    bolt::BCKND::device_vector< int > dV2;

    bolt::BCKND::device_vector< int >::const_reverse_iterator Iter2( dV, 0 );
    bolt::BCKND::device_vector< int >::const_reverse_iterator cIter2( dV2, 0 );
    EXPECT_TRUE( Iter2 != cIter2 );
}

TEST( VectorReverseIterator, Size6AndValue7Dereference )
{
    bolt::BCKND::device_vector< int > dV( 6ul, 7 );
    EXPECT_EQ( 6, dV.size( ) );

    bolt::BCKND::device_vector< int >::reverse_iterator myIter = dV.rbegin( );
    EXPECT_EQ( 7, *(myIter + 0) );
    EXPECT_EQ( 7, *(myIter + 1) );
    EXPECT_EQ( 7, *(myIter + 2) );
    EXPECT_EQ( 7, *(myIter + 3) );
    EXPECT_EQ( 7, *(myIter + 4) );
    EXPECT_EQ( 7, *(myIter + 5) );
}

//
//TEST( VectorReverseIterator, Size6AndValue7OperatorValueType )
//{
//    bolt::BCKND::device_vector< int > dV( 6, 7 );
//    EXPECT_EQ( 6, dV.size( ) );
//
//    bolt::BCKND::device_vector< int >::reverse_iterator myIter = dV.rbegin( );
//
//    EXPECT_EQ(  myIter[ 0 ],7 );
//    EXPECT_EQ(  myIter[ 1 ],7 );
//    EXPECT_EQ(  myIter[ 2 ],7 );
//    EXPECT_EQ(  myIter[ 3 ],7 );
//    EXPECT_EQ(  myIter[ 4 ],7 );
//    EXPECT_EQ(  myIter[ 5 ],7 );
//}

TEST( VectorReverseIterator, ArithmeticAndEqual )
{
    bolt::BCKND::device_vector< int > dV( 5 );
    EXPECT_EQ( 5, dV.size( ) );

    bolt::BCKND::device_vector< int >::reverse_iterator myIter = dV.rbegin( );
    *myIter = 1;
    ++myIter;
    *myIter = 2;
    myIter++;
    *myIter = 3;
    myIter += 1;
    *(myIter + 0) = 4;
    *(myIter + 1) = 5;
    myIter += 1;

    EXPECT_EQ( 1, dV[ 4 ] );
    EXPECT_EQ( 2, dV[ 3 ] );
    EXPECT_EQ( 3, dV[ 2 ] );
    EXPECT_EQ( 4, dV[ 1 ] );
    EXPECT_EQ( 5, dV[ 0 ] );
}



//Reverse Iterator test cases end

TEST( VectorReference, OperatorEqual )
{
    bolt::BCKND::device_vector< int > dV( 5 );

    dV[ 0 ] = 1;
    dV[ 1 ] = 2;
    dV[ 2 ] = 3;
    dV[ 3 ] = 4;
    dV[ 4 ] = 5;

    EXPECT_EQ( 1, dV[ 0 ] );
    EXPECT_EQ( 2, dV[ 1 ] );
    EXPECT_EQ( 3, dV[ 2 ] );
    EXPECT_EQ( 4, dV[ 3 ] );
    EXPECT_EQ( 5, dV[ 4 ] );
}

TEST( VectorReference, OperatorValueType )
{
    bolt::BCKND::device_vector< int > dV( 5 );

    dV[ 0 ] = 1;
    dV[ 1 ] = 2;
    dV[ 2 ] = 3;
    dV[ 3 ] = 4;
    dV[ 4 ] = 5;

    std::vector< int > readBack( 5 );
    readBack[ 0 ] = dV[ 0 ];
    readBack[ 1 ] = dV[ 1 ];
    readBack[ 2 ] = dV[ 2 ];
    readBack[ 3 ] = dV[ 3 ];
    readBack[ 4 ] = dV[ 4 ];

    EXPECT_EQ( readBack[ 0 ], dV[ 0 ] );
    EXPECT_EQ( readBack[ 1 ], dV[ 1 ] );
    EXPECT_EQ( readBack[ 2 ], dV[ 2 ] );
    EXPECT_EQ( readBack[ 3 ], dV[ 3 ] );
    EXPECT_EQ( readBack[ 4 ], dV[ 4 ] );
}

//TEST( VectorIterator, Size6AndValue7OperatorValueType )
//{
//    bolt::BCKND::device_vector< int > dV( 6, 7 );
//    EXPECT_EQ( 6, dV.size( ) );
//
//    bolt::BCKND::device_vector< int >::iterator myIter = dV.begin( );
//
//    EXPECT_EQ(  myIter[ 0 ],7 );
//    EXPECT_EQ(  myIter[ 1 ],7 );
//    EXPECT_EQ(  myIter[ 2 ],7 );
//    EXPECT_EQ(  myIter[ 3 ],7 );
//    EXPECT_EQ(  myIter[ 4 ],7 );
//    EXPECT_EQ(  myIter[ 5 ],7 );
//}

TEST( VectorIterator, Size6AndValue7Dereference )
{
    bolt::BCKND::device_vector< int > dV( 6ul, 7 );
    EXPECT_EQ( 6, dV.size( ) );

    bolt::BCKND::device_vector< int >::iterator myIter = dV.begin( );

    EXPECT_EQ( 7, *(myIter + 0) );
    EXPECT_EQ( 7, *(myIter + 1) );
    EXPECT_EQ( 7, *(myIter + 2) );
    EXPECT_EQ( 7, *(myIter + 3) );
    EXPECT_EQ( 7, *(myIter + 4) );
    EXPECT_EQ( 7, *(myIter + 5) );
}


//// Compilation errors
//TEST( VectorIterator, BackFront )
//{
//    bolt::BCKND::device_vector< int > dV( 6ul, 7 );
//    EXPECT_EQ( 6, dV.size( ) );
//
//    bolt::BCKND::device_vector< int >::iterator myIter = dV.begin( );
//
//    EXPECT_EQ( 7, dV.front( ) );
//    EXPECT_EQ( 7, dV.back( ) );
//}

TEST( VectorIterator, ArithmeticAndEqual )
{
    bolt::BCKND::device_vector< int > dV( 5 );
    EXPECT_EQ( 5, dV.size( ) );

    bolt::BCKND::device_vector< int >::iterator myIter = dV.begin( );

    *myIter = 1;
    ++myIter;
    *myIter = 2;
    myIter++;
    *myIter = 3;
    myIter += 1;
    *(myIter + 0) = 4;
    *(myIter + 1) = 5;
    myIter += 1;

    EXPECT_EQ( 1, dV[ 0 ] );
    EXPECT_EQ( 2, dV[ 1 ] );
    EXPECT_EQ( 3, dV[ 2 ] );
    EXPECT_EQ( 4, dV[ 3 ] );
    EXPECT_EQ( 5, dV[ 4 ] );
}

TEST( Vector, Erase )
{
    bolt::BCKND::device_vector< int > dV( 5 );
    EXPECT_EQ( 5, dV.size( ) );

    dV[ 0 ] = 1;
    dV[ 1 ] = 2;
    dV[ 2 ] = 3;
    dV[ 3 ] = 4;
    dV[ 4 ] = 5;

    bolt::BCKND::device_vector< int >::iterator myIter = dV.begin( );
    myIter += 2;

    bolt::BCKND::device_vector< int >::iterator myResult = dV.erase( myIter );
    EXPECT_EQ( 4, dV.size( ) );
    EXPECT_EQ( 4, *myResult );

    EXPECT_EQ( 1, dV[ 0 ] );
    EXPECT_EQ( 2, dV[ 1 ] );
    EXPECT_EQ( 4, dV[ 2 ] );
    EXPECT_EQ( 5, dV[ 3 ] );
}

TEST( Vector, Clear )
{
    bolt::BCKND::device_vector< int > dV( 5ul, 3 );
    EXPECT_EQ( 5, dV.size( ) );

    dV.clear( );
    EXPECT_EQ( 0, dV.size( ) );
}

TEST( Vector, EraseEntireRange )
{
    bolt::BCKND::device_vector< int > dV( 5 );
    EXPECT_EQ( 5, dV.size( ) );

    dV[ 0 ] = 1;
    dV[ 1 ] = 2;
    dV[ 2 ] = 3;
    dV[ 3 ] = 4;
    dV[ 4 ] = 5;

    bolt::BCKND::device_vector< int >::iterator myBegin = dV.begin( );
    bolt::BCKND::device_vector< int >::iterator myEnd = dV.end( );

    bolt::BCKND::device_vector< int >::iterator myResult = dV.erase( myBegin, myEnd );
    EXPECT_EQ( 0, dV.size( ) );
}

TEST( Vector, EraseSubRange )
{
    bolt::BCKND::device_vector< int > dV( 5 );
    EXPECT_EQ( 5, dV.size( ) );

    dV[ 0 ] = 1;
    dV[ 1 ] = 2;
    dV[ 2 ] = 3;
    dV[ 3 ] = 4;
    dV[ 4 ] = 5;

    bolt::BCKND::device_vector< int >::iterator myBegin = dV.begin( );
    bolt::BCKND::device_vector< int >::iterator myEnd = dV.end( );
    myEnd -= 2;

    bolt::BCKND::device_vector< int >::iterator myResult = dV.erase( myBegin, myEnd );
    EXPECT_EQ( 2, dV.size( ) );
}

TEST( Vector, InsertBegin )
{
    bolt::BCKND::device_vector< int > dV( 5 );
    EXPECT_EQ( 5, dV.size( ) );
    dV[ 0 ] = 1;
    dV[ 1 ] = 2;
    dV[ 2 ] = 3;
    dV[ 3 ] = 4;
    dV[ 4 ] = 5;

    bolt::BCKND::device_vector< int >::iterator myResult = dV.insert( dV.cbegin( ), 7 );
    EXPECT_EQ( 7, *myResult );
    EXPECT_EQ( 6, dV.size( ) );
}

TEST( Vector, InsertEnd )
{
    bolt::BCKND::device_vector< int > dV( 5ul, 3 );
    EXPECT_EQ( 5, dV.size( ) );

    bolt::BCKND::device_vector< int >::iterator myResult = dV.insert( dV.cend( ), 1 );
    EXPECT_EQ( 1, *myResult );
    EXPECT_EQ( 6, dV.size( ) );
}

TEST( Vector, DataRead )
{
    bolt::BCKND::device_vector< int > dV( 5ul, 3 );
    EXPECT_EQ( 5, dV.size( ) );
    dV[ 0 ] = 1;
    dV[ 1 ] = 2;
    dV[ 2 ] = 3;
    dV[ 3 ] = 4;
    dV[ 4 ] = 5;

    bolt::BCKND::device_vector< int >::pointer mySP = dV.data( );

    EXPECT_EQ( 1, mySP[ 0 ] );
    EXPECT_EQ( 2, mySP[ 1 ] );
    EXPECT_EQ( 3, mySP[ 2 ] );
    EXPECT_EQ( 4, mySP[ 3 ] );
    EXPECT_EQ( 5, mySP[ 4 ] );
}

TEST( Vector, DataWrite )
{
    bolt::BCKND::device_vector< int > dV( 5 );
    EXPECT_EQ( 5, dV.size( ) );

    bolt::BCKND::device_vector< int >::pointer mySP = dV.data( );
    mySP[ 0 ] = 1;
    mySP[ 1 ] = 2;
    mySP[ 2 ] = 3;
    mySP[ 3 ] = 4;
    mySP[ 4 ] = 5;

    EXPECT_EQ( 1, mySP[ 0 ] );
    EXPECT_EQ( 2, mySP[ 1 ] );
    EXPECT_EQ( 3, mySP[ 2 ] );
    EXPECT_EQ( 4, mySP[ 3 ] );
    EXPECT_EQ( 5, mySP[ 4 ] );
}

TEST( Vector, wdSpecifyingSize )
{
    size_t mySize = 10;
    bolt::BCKND::device_vector<int> myIntDevVect;
    int myIntArray[10] = {2, 3, 5, 6, 76, 5, 8, -10, 30, 34};

    for (int i = 0; i < mySize; ++i){
        myIntDevVect.push_back(myIntArray[i]);
    }

    size_t DevSize = myIntDevVect.size();
    
    EXPECT_EQ (mySize, DevSize);
}

TEST( Vector, InsertFloatRangeEmpty )
{
    bolt::BCKND::device_vector< float > dV;
    EXPECT_EQ( 0, dV.size( ) );

    dV.insert( dV.cbegin( ), 5, 7.0f );
    EXPECT_EQ( 5, dV.size( ) );
    EXPECT_FLOAT_EQ( 7.0f, dV[ 0 ] );
    EXPECT_FLOAT_EQ( 7.0f, dV[ 1 ] );
    EXPECT_FLOAT_EQ( 7.0f, dV[ 2 ] );
    EXPECT_FLOAT_EQ( 7.0f, dV[ 3 ] );
    EXPECT_FLOAT_EQ( 7.0f, dV[ 4 ] );
}

//TEST( Vector, InsertIntegerRangeEmpty )
//{
//    bolt::BCKND::device_vector< int > dV;
//    EXPECT_EQ( 0, dV.size( ) );
//
//    dV.insert( dV.cbegin( ), 5, 7 );
//    EXPECT_EQ( 5, dV.size( ) );
//    EXPECT_EQ( 7, dV[ 0 ] );
//    EXPECT_EQ( 7, dV[ 1 ] );
//    EXPECT_EQ( 7, dV[ 2 ] );
//    EXPECT_EQ( 7, dV[ 3 ] );
//    EXPECT_EQ( 7, dV[ 4 ] );
//}

TEST( Vector, InsertFloatRangeIterator )
{
    bolt::BCKND::device_vector< float > dV;
    EXPECT_EQ( 0, dV.size( ) );

    std::vector< float > sV( 5 );
    sV[ 0 ] = 1.0f;
    sV[ 1 ] = 2.0f;
    sV[ 2 ] = 3.0f;
    sV[ 3 ] = 4.0f;
    sV[ 4 ] = 5.0f;

    dV.insert( dV.cbegin( ), sV.begin( ), sV.end( ) );
    EXPECT_EQ( 5, dV.size( ) );
    EXPECT_FLOAT_EQ( 1.0f, dV[ 0 ] );
    EXPECT_FLOAT_EQ( 2.0f, dV[ 1 ] );
    EXPECT_FLOAT_EQ( 3.0f, dV[ 2 ] );
    EXPECT_FLOAT_EQ( 4.0f, dV[ 3 ] );
    EXPECT_FLOAT_EQ( 5.0f, dV[ 4 ] );
}

TEST( Vector, Resize )
{
    bolt::BCKND::device_vector< float > dV;
    EXPECT_EQ( 0, dV.size( ) );

    std::vector< float > sV( 10 );
    for(int i=0; i<10; i++)
    {
        sV[i] = (float)i;
    }

    dV.insert( dV.cbegin( ), sV.begin( ), sV.end( ) );
    EXPECT_EQ( 10, dV.size( ) );
    dV.resize(7);
    EXPECT_EQ( 7, dV.size( ) );
    for(int i=0; i<7; i++)
    {
        EXPECT_FLOAT_EQ( (float)i, dV[ i ] );
    }
    dV.resize(15, 7);
    EXPECT_EQ( 15, dV.size( ) );
    for(int i=0; i<15; i++)
    {
        if(i<7)
            EXPECT_FLOAT_EQ( (float)i, dV[ i ] );
        else
            EXPECT_FLOAT_EQ( 7.0f, dV[ i ] );
    }
}

TEST( Vector, ShrinkToFit)
{
	bolt::BCKND::device_vector< int > dV(100);
	EXPECT_EQ(dV.size(),dV.capacity());
	dV.reserve(200);
	EXPECT_EQ(200,dV.capacity());
	dV.shrink_to_fit();
	EXPECT_EQ(dV.size(),dV.capacity());
#if 0
	//Just like that.
	for(int i=0; i<(2<<21);i+=(2<<3)){
		dV.reserve(i);
		dV.shrink_to_fit();
		EXPECT_EQ(dV.size(),dV.capacity());
	}

#endif
}

TEST( Vector, DataRoutine )
{
    std::vector<int>a( 100 );

    //  Initialize data (could use constructor?)
    std::fill( a.begin( ),a.end( ), 100 );

    //  deep copy, all data is duplicated
    bolt::BCKND::device_vector< int > da( a.begin( ),a.end( ) );

    //  Change value in device memory
    da[50] = 0;

    //  Get host readable pointer to device memory
    bolt::BCKND::device_vector< int >::pointer pDa = da.data();

    EXPECT_EQ( 0, pDa[50] );
}

//////////////////////////////////////////////////////////////////////////////////
// ARRAY_VIEW TEST CASES
//////////////////////////////////////////////////////////////////////////////////

//// This test case should throw compilation errors
//// You cannot call reserve() and shrink_to_fit()
//// using array_view as the container.
////
//TEST( Vector, ShrinkToFitView)
//{
//  std::vector< int > x( 100 );
//  bolt::BCKND::device_vector< int , concurrency::array_view > dV( x );
//	EXPECT_EQ(dV.size(),dV.capacity());
//	dV.reserve(200);
//	EXPECT_EQ(200,dV.capacity());
//	dV.shrink_to_fit();
//	EXPECT_EQ(dV.size(),dV.capacity());
//#if 0
//	//Just like that.
//	for(int i=0; i<(2<<21);i+=(2<<3)){
//		dV.reserve(i);
//		dV.shrink_to_fit();
//		EXPECT_EQ(dV.size(),dV.capacity());
//	}
//
//#endif
//}

//// This test case should throw compilation errors
//// You cannot call resize() using array_view as the container.
////
//TEST( Vector, ResizeView )
//{
//    std::vector< float > x( 100 );
//    bolt::BCKND::device_vector< float , concurrency::array_view> dV( x );
//    EXPECT_EQ( 0, dV.size( ) );
//
//    std::vector< float > sV( 10 );
//    for(int i=0; i<10; i++)
//    {
//        sV[i] = (float)i;
//    }
//
//    dV.insert( dV.cbegin( ), sV.begin( ), sV.end( ) );
//    EXPECT_EQ( 10, dV.size( ) );
//    dV.resize(7);
//    EXPECT_EQ( 7, dV.size( ) );
//    for(int i=0; i<7; i++)
//    {
//        EXPECT_FLOAT_EQ( (float)i, dV[ i ] );
//    }
//    dV.resize(15, 7);
//    EXPECT_EQ( 15, dV.size( ) );
//    for(int i=0; i<15; i++)
//    {
//        if(i<7)
//            EXPECT_FLOAT_EQ( (float)i, dV[ i ] );
//        else
//            EXPECT_FLOAT_EQ( 7.0f, dV[ i ] );
//    }
//}

//// This test case should throw compilation errors
//// You cannot call insert() using array_view as the container.
////
//TEST( Vector, InsertFloatRangeEmptyView )
//{
//  bolt::amp::device_vector< float , concurrency::array_view > dV;
//    EXPECT_EQ( 0, dV.size( ) );
//
//    dV.insert( dV.cbegin( ), 5, 7.0f );
//    EXPECT_EQ( 5, dV.size( ) );
//    EXPECT_FLOAT_EQ( 7.0f, dV[ 0 ] );
//    EXPECT_FLOAT_EQ( 7.0f, dV[ 1 ] );
//    EXPECT_FLOAT_EQ( 7.0f, dV[ 2 ] );
//    EXPECT_FLOAT_EQ( 7.0f, dV[ 3 ] );
//    EXPECT_FLOAT_EQ( 7.0f, dV[ 4 ] );
//}

TEST( Vector, DataReadView )
{
    std::vector< int > v( 5ul, 3 );
    bolt::amp::device_vector< int, concurrency::array_view > dV( v );
    EXPECT_EQ( 5, dV.size( ) );
    dV[ 0 ] = 1;
    dV[ 1 ] = 2;
    dV[ 2 ] = 3;
    dV[ 3 ] = 4;
    dV[ 4 ] = 5;

    // Any device_vector pointer can be used.
    bolt::amp::device_vector< int, concurrency::array_view >::pointer mySP = dV.data( );

    EXPECT_EQ( 1, mySP[ 0 ] );
    EXPECT_EQ( 2, mySP[ 1 ] );
    EXPECT_EQ( 3, mySP[ 2 ] );
    EXPECT_EQ( 4, mySP[ 3 ] );
    EXPECT_EQ( 5, mySP[ 4 ] );

}

TEST( Vector, DataWriteView )
{
    std::vector< int > v( 5 );
    bolt::amp::device_vector< int, concurrency::array_view > dV( v );
    EXPECT_EQ( 5, dV.size( ) );

    bolt::amp::device_vector< int, concurrency::array_view  >::pointer mySP = dV.data( );
    mySP[ 0 ] = 1;
    mySP[ 1 ] = 2;
    mySP[ 2 ] = 3;
    mySP[ 3 ] = 4;
    mySP[ 4 ] = 5;

    EXPECT_EQ( 1, mySP[ 0 ] );
    EXPECT_EQ( 2, mySP[ 1 ] );
    EXPECT_EQ( 3, mySP[ 2 ] );
    EXPECT_EQ( 4, mySP[ 3 ] );
    EXPECT_EQ( 5, mySP[ 4 ] );
}

TEST( Vector, ClearView )
{
    std::vector< int > v( 5ul, 3 );
    bolt::amp::device_vector< int, concurrency::array_view > dV( v );
    EXPECT_EQ( 5, dV.size( ) );
    dV.clear( );
    EXPECT_EQ( 0, dV.size( ) );
}

//TEST( Vector, EraseEntireRange )
//{
//    bolt::BCKND::device_vector< int > dV( 5 );
//    EXPECT_EQ( 5, dV.size( ) );
//
//    dV[ 0 ] = 1;
//    dV[ 1 ] = 2;
//    dV[ 2 ] = 3;
//    dV[ 3 ] = 4;
//    dV[ 4 ] = 5;
//
//    bolt::BCKND::device_vector< int >::iterator myBegin = dV.begin( );
//    bolt::BCKND::device_vector< int >::iterator myEnd = dV.end( );
//
//    bolt::BCKND::device_vector< int >::iterator myResult = dV.erase( myBegin, myEnd );
//    EXPECT_EQ( 0, dV.size( ) );
//}


TEST( Vector, DataRoutineView )
{
    std::vector<int> arr( 100, 99 );

    bolt::amp::device_vector< int, concurrency::array_view > av( arr );

    av[50] = 0;
    av.data( );

    EXPECT_EQ( arr[50], av[50] );
}

TEST( Vector, ArrayViewBegin )
{
    std::vector<int> arr( 100, 99 );

    bolt::amp::device_vector< int, concurrency::array_view > av( arr );

    bolt::amp::device_vector< int, concurrency::array_view >::iterator avIT = av.begin( );

    EXPECT_EQ( arr[0], *avIT );
}

// array_view test case ends.
///////////////////////////////////////////////////////////////////////////////////

TEST( DeviceVector, Swap )
{
    bolt::BCKND::device_vector< int > dV( 5ul, 3 ), dV2(5ul, 10);
    EXPECT_EQ( 5, dV.size( ) );
    dV.swap(dV2);
    EXPECT_EQ(3, dV2[0]);
    EXPECT_EQ(10, dV[0]);

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