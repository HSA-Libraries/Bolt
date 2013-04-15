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


#include <array>

#include "common/stdafx.h"
#include "common/myocl.h"
#include "bolt/cl/generate.h"
#include "bolt/cl/scan.h"

//#include <bolt/miniDump.h>

#include <gtest/gtest.h>
//#include <boost/shared_array.hpp>

template< typename T >
::testing::AssertionResult cmpArrays( const T ref, const T calc, size_t N )
{
    for( size_t i = 0; i < N; ++i )
    {
        EXPECT_EQ( ref[ i ], calc[ i ] ) << _T( "Where i = " ) << i;
    }

    return ::testing::AssertionSuccess( );
}
template< typename T, size_t N >
::testing::AssertionResult cmpArrays( const T (&ref)[N], const T (&calc)[N] )
{
    for( size_t i = 0; i < N; ++i )
    {
        EXPECT_EQ( ref[ i ], calc[ i ] ) << _T( "Where i = " ) << i;
    }

    return ::testing::AssertionSuccess( );
}
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
template< typename S, typename B >
::testing::AssertionResult cmpArrays( const S& ref, const B& calc )
{
    for( size_t i = 0; i < ref.size( ); ++i )
    {
        EXPECT_EQ( ref[ i ], calc[ i ] ) << _T( "Where i = " ) << i;
    }

    return ::testing::AssertionSuccess( );
}

BOLT_FUNCTOR(GenDbl,
struct GenDbl
{
    const float _a;
    GenDbl( float a ) : _a(a) {};

	float operator() ()
	{
		return _a;
	};

};
);  // end BOLT_FUNCTOR

/******************************************************************************
 * Generator Gen2: return incrementing int, begining at base value
 *****************************************************************************/
BOLT_FUNCTOR(GenInt,
struct GenInt
{
	const int _a;
	GenInt( int a ) : _a(a) {};

	int operator() ()
	{
        return _a;
	};
};
);  // end BOLT_FUNCTOR

/******************************************************************************
 * Generator GenConst: return constant
 *****************************************************************************/
BOLT_TEMPLATE_FUNCTOR2(GenConst, int, double,
template< typename T >
struct GenConst
{
    // return value
	T _a;

    // constructor
	GenConst( T a ) : _a(a) {};

    // functor
	T operator() () { return _a; };
};
);  // end BOLT_FUNCTOR

class HostIntVector: public ::testing::TestWithParam< int >
{
public:
    //  Create an std and a bolt vector of requested size, and initialize all the elements to -1
    HostIntVector( ): stdInput( GetParam( ), -1 ), boltInput( GetParam( ), -1 )
    {}

protected:
    std::vector< int > stdInput, boltInput;
};

class DevIntVector: public ::testing::TestWithParam< int >
{
public:
    //  Create an std and a bolt vector of requested size, and initialize all the elements to -1
    DevIntVector( ): stdInput( GetParam( ), -1 ), boltInput( GetParam( ), -1 )
    {}

protected:
    std::vector< int > stdInput;
    bolt::cl::device_vector< int > boltInput;
};

class HostDblVector: public ::testing::TestWithParam< int >
{
public:
    //  Create an std and a bolt vector of requested size, and initialize all the elements to -1
    HostDblVector( ): stdInput( GetParam( ), -1.0 ), boltInput( GetParam( ), -1.0 )
    {}

protected:
    std::vector< double > stdInput, boltInput;
};

class DevDblVector: public ::testing::TestWithParam< int >
{
public:
    //  Create an std and a bolt vector of requested size, and initialize all the elements to -1
    DevDblVector( ): stdInput( GetParam( ), -1.0 ), boltInput( GetParam( ), -1.0 )
    {}

protected:
    std::vector< double > stdInput;
    bolt::cl::device_vector< double > boltInput;
};

TEST_P( HostIntVector, Generate )
{
    // create generator
    GenConst<int> gen(1234);

    //  Calling the actual functions under test
         std::generate(  stdInput.begin( ),  stdInput.end( ), gen );
    bolt::cl::generate( boltInput.begin( ), boltInput.end( ), gen );

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdInput, boltInput );
}

TEST_P( HostDblVector, Generate )
{
    // create generator
    GenConst<double> gen(1.234);
    //  Calling the actual functions under test

         std::generate(  stdInput.begin( ),  stdInput.end( ), gen );
    bolt::cl::generate( boltInput.begin( ), boltInput.end( ), gen );

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdInput, boltInput );
}

TEST_P( DevIntVector, Generate )
{
    // create generator
    GenConst<int> gen(2345);

    //  Calling the actual functions under test
         std::generate(  stdInput.begin( ),  stdInput.end( ), gen );
    bolt::cl::generate( boltInput.begin( ), boltInput.end( ), gen );

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdInput, boltInput );
}

TEST_P( DevDblVector, Generate )
{
    // create generator
    GenConst<double> gen(2.345);

    //  Calling the actual functions under test
         std::generate(  stdInput.begin( ),  stdInput.end( ), gen );
    bolt::cl::generate( boltInput.begin( ), boltInput.end( ), gen );
    //  Loop through the array and compare all the values with each other
    cmpArrays( stdInput, boltInput );
}

///////////////////////////////////////////////////////////////////////////////

#if( MSVC_VER > 1600 )
TEST_P( HostIntVector, GenerateN )
{
    int size = GetParam();
    // create generator
    GenConst<int> gen(3456);

    //  Calling the actual functions under test
    std::vector< int >::iterator  stdEnd  =       std::generate_n(  stdInput.begin( ), size, gen );
    std::vector< int >::iterator  stdEnd  =       std::generate_n(  stdInput.begin( ), size, gen );
    std::vector< int >::iterator boltEnd  =  bolt::cl::generate_n( boltInput.begin( ), size, gen );

    //  The returned iterator should be at the end
    EXPECT_EQ( stdInput.end( ), stdEnd );
    EXPECT_EQ( boltInput.end( ), boltEnd );
    
    //  Both collections should have the same number of elements
    std::vector< int >::iterator::difference_type  stdNumElements = std::distance(  stdInput.begin( ),  stdEnd );
    std::vector< int >::iterator::difference_type boltNumElements = std::distance( boltInput.begin( ), boltEnd );
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdInput, boltInput );
}

TEST_P( HostDblVector, GenerateN )
{
    int size = GetParam();
    // create generator
    GenConst<double> gen(3.456);

    //  Calling the actual functions under test
    std::vector< double >::iterator  stdEnd  =      std::generate_n(  stdInput.begin( ), size, gen );
    std::vector< double >::iterator boltEnd  = bolt::cl::generate_n( boltInput.begin( ), size, gen );
    
    //  The returned iterator should be at the end
    EXPECT_EQ( stdInput.end( ), stdEnd );
    EXPECT_EQ( boltInput.end( ), boltEnd );
    
    //  Both collections should have the same number of elements
    std::vector< double >::iterator::difference_type  stdNumElements = std::distance(  stdInput.begin( ),  stdEnd );
    std::vector< double >::iterator::difference_type boltNumElements = std::distance( boltInput.begin( ), boltEnd );
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdInput, boltInput );
}

TEST_P( DevIntVector, GenerateN )
{
    int size = GetParam();
    // create generator
    GenConst<int> gen(4567);

    //  Calling the actual functions under test
    std::vector< int >::iterator              stdEnd =      std::generate_n(  stdInput.begin( ), size, gen );
    bolt::cl::device_vector< int >::iterator boltEnd = bolt::cl::generate_n( boltInput.begin( ), size, gen );

    //  The returned iterator should be at the end
    EXPECT_EQ( stdInput.end( ), stdEnd );
    EXPECT_EQ( boltInput.end( ), boltEnd );
    
    //  Both collections should have the same number of elements
    std::vector< int >::iterator::difference_type  stdNumElements = std::distance(  stdInput.begin( ),  stdEnd );
    std::vector< int >::iterator::difference_type boltNumElements = std::distance( boltInput.begin( ), boltEnd );
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdInput, boltInput );
}

TEST_P( DevDblVector, GenerateN )
{
    int size = GetParam();
    // create generator
    GenConst<double> gen(4.567);

    //  Calling the actual functions under test
    std::vector< double >::iterator                 stdEnd =      std::generate_n(  stdInput.begin( ), size, gen );
    bolt::cl::device_vector< double >::iterator boltEnd = bolt::cl::generate_n( boltInput.begin( ), size, gen );
    
    //  The returned iterator should be at the end
    EXPECT_EQ( stdInput.end( ), stdEnd );
    EXPECT_EQ( boltInput.end( ), boltEnd );
    
    //  Both collections should have the same number of elements
    std::vector< double >::iterator::difference_type  stdNumElements = std::distance(  stdInput.begin( ),  stdEnd );
    std::vector< double >::iterator::difference_type boltNumElements = std::distance( boltInput.begin( ), boltEnd );
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdInput, boltInput );
}
#endif

INSTANTIATE_TEST_CASE_P( GenSmall, HostIntVector, ::testing::Range( 1, 256, 3 ) );
INSTANTIATE_TEST_CASE_P( GenLarge, HostIntVector, ::testing::Range( 1023, 1050000, 350001 ) );
INSTANTIATE_TEST_CASE_P( GenSmall, DevIntVector,  ::testing::Range( 2, 256, 3 ) );
INSTANTIATE_TEST_CASE_P( GenLarge, DevIntVector,  ::testing::Range( 1024, 1050000, 350003 ) );
INSTANTIATE_TEST_CASE_P( GenSmall, HostDblVector, ::testing::Range( 3, 256, 3 ) );
INSTANTIATE_TEST_CASE_P( GenLarge, HostDblVector, ::testing::Range( 1025, 1050000, 350007 ) );
INSTANTIATE_TEST_CASE_P( GenSmall, DevDblVector,  ::testing::Range( 4, 256, 3 ) );
INSTANTIATE_TEST_CASE_P( GenLarge, DevDblVector,  ::testing::Range( 1026, 1050000, 350011 ) );

//////////////////////////////////////////////////////////////////////////////////////
// Tests for EPRid#375981
/////////////////////////////////////////////////////////////////////////////////////
BOLT_FUNCTOR(ConstFunctor,
struct ConstFunctor {
int val;

ConstFunctor(int a) : val(a) {
}

int operator() () {
return val;
}
};
);

TEST(ControlObjectTests, GenIntegersSerialCpu)
{

int size = 100;
ConstFunctor cf(1);
std::vector<int> vec(size);
bolt::cl::control ctl = bolt::cl::control::getDefault();
ctl.setForceRunMode(bolt::cl::control::SerialCpu);
bolt::cl::generate_n(ctl, vec.begin(), size, cf);

for (int i = 1 ; i < size; ++i){
EXPECT_EQ(1, vec[i]);
}

}

TEST(ControlObjectTests, GenIntegersAutomatic)
{

int size = 100;
ConstFunctor cf(1);
std::vector<int> vec(size);
bolt::cl::control ctl = bolt::cl::control::getDefault();
ctl.setForceRunMode(bolt::cl::control::Automatic);
bolt::cl::generate_n(ctl, vec.begin(), size, cf);

for (int i = 1 ; i < size; ++i){
EXPECT_EQ(1, vec[i]);
}

}

TEST(ControlObjectTests, GenIntegersMultiCoreCPU)
{

int size = 100;
ConstFunctor cf(1);
std::vector<int> vec(size);
bolt::cl::control ctl = bolt::cl::control::getDefault();
ctl.setForceRunMode(bolt::cl::control::MultiCoreCpu);
bolt::cl::generate_n(ctl, vec.begin(), size, cf);

for (int i = 1 ; i < size; ++i){
EXPECT_EQ(1, vec[i]);
}

}


TEST(ControlObjectTests, GenIntegersOpenCL)
{

int size = 100;
ConstFunctor cf(1);
std::vector<int> vec(size);
bolt::cl::control ctl = bolt::cl::control::getDefault();
ctl.setForceRunMode(bolt::cl::control::OpenCL);
bolt::cl::generate_n(ctl, vec.begin(), size, cf);

for (int i = 1 ; i < size; ++i){
EXPECT_EQ(1, vec[i]);
}

}


int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

