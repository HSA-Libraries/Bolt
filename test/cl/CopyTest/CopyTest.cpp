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

#define TEST_DOUBLE 1
#define TEST_DEVICE_VECTOR 1
#define TEST_CPU_DEVICE 1

#include "common/stdafx.h"
#include "common/myocl.h"

#include <bolt/cl/copy.h>
#include <bolt/cl/functional.h>
#include <bolt/miniDump.h>

#include <gtest/gtest.h>
#include <boost/shared_array.hpp>
#include <array>


BOLT_FUNCTOR(UserStruct,
struct UserStruct
{
    bool a;
    char b;
    int c;
    float d;
    double e;

    UserStruct() :
        a(true),
        b('b'),
        c(3),
        d(4.f),
        e(5.0)
    { }

    bool operator==(const UserStruct& rhs) const
    {
        return
            (a == rhs.a) &&
            (b == rhs.b) &&
            (c == rhs.c) &&
            (d == rhs.d) &&
            (e == rhs.e)
            ;
    }

};
);  // end BOLT_FUNCTOR

/////////////////////////////////////////////////////////////////////////////////
//  Below are helper routines to compare the results of two arrays for googletest
//  They return an assertion object that googletest knows how to track
//  This is a compare routine for naked pointers.
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

//  A very generic template that takes two container, and compares their values assuming a vector interface
template< typename S, typename B >
::testing::AssertionResult cmpArrays( const S& ref, const B& calc )
{
    for( size_t i = 0; i < ref.size( ); ++i )
    {
        EXPECT_EQ( ref[ i ], calc[ i ] ) << _T( "Where i = " ) << i;
    }

    return ::testing::AssertionSuccess( );
}


/******************************************************************************
 * Tests
 *****************************************************************************/
// device_vector vs std_vector
// copy vs copy_n
// primitive vs struct
// mult64 vs not mult
// zero vs positive

static const int numLengths = 8;
static const int lengths[8] = {0, 1, 63, 64, 65, 1023, 1024, 1025};
//static const int numLengths = 1;
//static const int lengths[1] = {13};


TEST(Copy, DevPrim)
{
    for (int i = 0; i < numLengths; i++)
    {
        // test length
        int length = lengths[i];
        // populate source vector with random ints
        bolt::cl::device_vector<int> source(length);
        for (int j = 0; j < length; j++)
        {
            source[j] = rand();
        }
        // destination vector
        bolt::cl::device_vector<int> destination(length);
        // perform copy
        bolt::cl::copy(source.begin(), source.end(), destination.begin());
        // GoogleTest Comparison
        cmpArrays(source, destination, length);
    }
}


TEST(CopyN, DevPrim)
{
    for (int i = 0; i < numLengths; i++)
    {
        // test length
        int length = lengths[i];
        // populate source vector with random ints
        bolt::cl::device_vector<int> source(length);
        for (int j = 0; j < length; j++)
        {
            source[j] = rand();
        }
        // destination vector
        bolt::cl::device_vector<int> destination(length);
        // perform copy
        bolt::cl::copy_n(source.begin(), length, destination.begin());
        // GoogleTest Comparison
        cmpArrays(source, destination, length);
    }
}

TEST(Copy, StdPrim)
{
    for (int i = 0; i < numLengths; i++)
    {
        // test length
        int length = lengths[i];
        // populate source vector with random ints
        std::vector<int> source(length);
        for (int j = 0; j < length; j++)
        {
            source[j] = rand();
        }
        // destination vector
        std::vector<int> destination(length);
        // perform copy
        bolt::cl::copy(source.begin(), source.end(), destination.begin());
        // GoogleTest Comparison
        cmpArrays(source, destination, length);
    }
}

TEST(CopyN, StdPrim)
{
    for (int i = 0; i < numLengths; i++)
    {
        // test length
        int length = lengths[i];
        // populate source vector with random ints
        std::vector<int> source(length);
        for (int j = 0; j < length; j++)
        {
            source[j] = rand();
        }
        // destination vector
        std::vector<int> destination(length);
        // perform copy
        bolt::cl::copy_n(source.begin(), length, destination.begin());
        // GoogleTest Comparison
        cmpArrays(source, destination, length);
    }
}

TEST(Copy, DevStruct)
{
    for (int i = 0; i < numLengths; i++)
    {
        // test length
        int length = lengths[i];
        // populate source vector with random ints
        bolt::cl::device_vector<UserStruct> source;
        for (int j = 0; j < length; j++)
        {
            UserStruct us;
            us.a = (bool) (rand()%2 ? true : false);
            us.b = (char) (rand()%128);
            us.c = (int)  (rand());
            us.d = (float) (1.f*rand());
            us.e = (double) (1.0*rand()/rand());
            source.push_back(us);
        }
        // destination vector
        bolt::cl::device_vector<UserStruct> destination(length, UserStruct(), CL_MEM_READ_WRITE, false);
        // perform copy
        bolt::cl::copy(source.begin(), source.end(), destination.begin());
        // GoogleTest Comparison
        cmpArrays(source, destination, length);
    }
}

TEST(CopyN, DevStruct)
{
    for (int i = 0; i < numLengths; i++)
    {
        // test length
        int length = lengths[i];
        // populate source vector with random ints
        bolt::cl::device_vector<UserStruct> source;
        for (int j = 0; j < length; j++)
        {
            UserStruct us;
            us.a = (bool) (rand()%2 ? true : false);
            us.b = (char) (rand()%128);
            us.c = (int)  (rand());
            us.d = (float) (1.f*rand());
            us.e = (double) (1.0*rand()/rand());
            source.push_back(us);
        }
        // destination vector
        bolt::cl::device_vector<UserStruct> destination(length, UserStruct(), CL_MEM_READ_WRITE, false);
        // perform copy
        bolt::cl::copy_n(source.begin(), length, destination.begin());
        // GoogleTest Comparison
        cmpArrays(source, destination, length);
    }
}

TEST(Copy, StdStruct)
{
    for (int i = 0; i < numLengths; i++)
    {
        // test length
        int length = lengths[i];
        // populate source vector with random ints
        std::vector<UserStruct> source;
        for (int j = 0; j < length; j++)
        {
            UserStruct us;
            us.a = (bool) (rand()%2 ? true : false);
            us.b = (char) (rand()%128);
            us.c = (int)  (rand());
            us.d = (float) (1.f*rand());
            us.e = (double) (1.0*rand()/rand());
            source.push_back(us);
        }
        // destination vector
        std::vector<UserStruct> destination(length);
        // perform copy
        bolt::cl::copy(source.begin(), source.end(), destination.begin());
        // GoogleTest Comparison
        cmpArrays(source, destination, length);
    }
}

TEST(CopyN, StdStruct)
{
    for (int i = 0; i < numLengths; i++)
    {
        // test length
        int length = lengths[i];
        // populate source vector with random ints
        std::vector<UserStruct> source;
        for (int j = 0; j < length; j++)
        {
            UserStruct us;
            us.a = (bool) (rand()%2 ? true : false);
            us.b = (char) (rand()%128);
            us.c = (int)  (rand());
            us.d = (float) (1.f*rand());
            us.e = (double) (1.0*rand()/rand());
            source.push_back(us);
        }
        // destination vector
        std::vector<UserStruct> destination(length);
        // perform copy
        bolt::cl::copy_n(source.begin(), length, destination.begin());
        // GoogleTest Comparison
        cmpArrays(source, destination, length);
    }
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

