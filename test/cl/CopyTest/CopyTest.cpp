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























#if 0


///////////////////////////////////////////////////////////////////////////////
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
class CopyArrayTest: public ::testing::Test
{
public:
    CopyArrayTest( ): m_Errors( 0 )
    {}

    virtual void SetUp( )
    {
        std::generate(stdInput.begin(), stdInput.end(), rand);
        stdOutput = stdInput;
        boltInput = stdInput;
        boltOutput = stdInput;
    };

    virtual void TearDown( )
    {};

    virtual ~CopyArrayTest( )
    {}

protected:
    typedef typename std::tuple_element< 0, ArrayTuple >::type ArrayType;
    static const size_t ArraySize = typename std::tuple_element< 1, ArrayTuple >::type::value;
    typename std::array< ArrayType, ArraySize > stdInput, boltInput, stdOutput, boltOutput;
    int m_Errors;
};

//  Test fixture class, used for the Type-parameterized tests
//  Namely, the tests that use std::array and TYPED_TEST_P macros
template< typename ArrayTuple >
class UnaryCopyArrayTest: public ::testing::Test
{
public:
    UnaryCopyArrayTest( ): m_Errors( 0 )
    {}

    virtual void SetUp( )
    {
        std::generate(stdInput.begin(), stdInput.end(), rand);
        boltInput = stdInput;
        boltOutput = stdInput;
        stdOutput = stdInput;
    };

    virtual void TearDown( )
    {};

    virtual ~UnaryCopyArrayTest( )
    {}

protected:
    typedef typename std::tuple_element< 0, ArrayTuple >::type ArrayType;
    static const size_t ArraySize = typename std::tuple_element< 1, ArrayTuple >::type::value;
    typename std::array< ArrayType, ArraySize > stdInput, boltInput, stdOutput, boltOutput;
    int m_Errors;
};

TYPED_TEST_CASE_P( CopyArrayTest );

TYPED_TEST_P( CopyArrayTest, Normal )
{
    typedef std::array< ArrayType, ArraySize > ArrayCont;

    //  Calling the actual functions under test
    std::copy( stdInput.begin( ), stdInput.end( ), stdOutput.begin( ), stdOutput.begin( ), bolt::cl::plus<ArrayType>());
    bolt::cl::copy( boltInput.begin( ), boltInput.end( ), boltOutput.begin( ), boltOutput.begin( ), bolt::cl::plus<ArrayType>());

    ArrayCont::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end() );
    ArrayCont::difference_type boltNumElements = std::distance( boltInput.begin( ), boltInput.end() );

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpStdArray< ArrayType, ArraySize >::cmpArrays( stdOutput, boltOutput );
    
}

TYPED_TEST_P( CopyArrayTest, GPU_DeviceNormal )
{
    typedef std::array< ArrayType, ArraySize > ArrayCont;

    //  The first time our routines get called, we compile the library kernels with a certain context
    //  OpenCL does not allow the context to change without a recompile of the kernel
    // MyOclContext oclgpu = initOcl(CL_DEVICE_TYPE_GPU, 0);
    //bolt::cl::control c_gpu(oclgpu._queue);  // construct control structure from the queue.

    //  Create a new command queue for a different device, but use the same context as was provided
    //  by the default control device
    ::cl::Context myContext = bolt::cl::control::getDefault( ).context( );
    std::vector< cl::Device > devices = myContext.getInfo< CL_CONTEXT_DEVICES >();
    ::cl::CommandQueue myQueue( myContext, devices[ 0 ] );
    bolt::cl::control c_gpu( myQueue );  // construct control structure from the queue.

    //  Calling the actual functions under test
    std::copy( stdInput.begin( ), stdInput.end( ), stdOutput.begin( ), stdOutput.begin( ), bolt::cl::plus<ArrayType>());
    bolt::cl::copy( c_gpu, boltInput.begin( ), boltInput.end( ), boltOutput.begin( ), boltOutput.begin( ), bolt::cl::plus<ArrayType>());

    ArrayCont::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end() );
    ArrayCont::difference_type boltNumElements = std::distance( boltInput.begin( ), boltInput.end() );

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpStdArray< ArrayType, ArraySize >::cmpArrays( stdOutput, boltOutput );
    // FIXME - releaseOcl(ocl);
}

#if (TEST_CPU_DEVICE == 1)
TYPED_TEST_P( CopyArrayTest, CPU_DeviceNormal )
{
    typedef std::array< ArrayType, ArraySize > ArrayCont;
    MyOclContext oclcpu = initOcl(CL_DEVICE_TYPE_CPU, 0);
	bolt::cl::control c_cpu(oclcpu._queue);  // construct control structure from the queue.

    //  Calling the actual functions under test
    std::copy( stdInput.begin( ), stdInput.end( ), stdOutput.begin( ), stdOutput.begin( ), bolt::cl::plus<ArrayType>());
    bolt::cl::copy( c_cpu, boltInput.begin( ), boltInput.end( ), boltOutput.begin( ), boltOutput.begin( ), bolt::cl::plus<ArrayType>());

    ArrayCont::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end() );
    ArrayCont::difference_type boltNumElements = std::distance( boltInput.begin( ), boltInput.end() );

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpStdArray< ArrayType, ArraySize >::cmpArrays( stdOutput, boltOutput );
    // FIXME - releaseOcl(ocl);
}
#endif

TYPED_TEST_P( CopyArrayTest, MultipliesFunction )
{
    typedef std::array< ArrayType, ArraySize > ArrayCont;

    //  Calling the actual functions under test
    std::copy( stdInput.begin( ), stdInput.end( ), stdOutput.begin( ), stdOutput.begin( ), bolt::cl::multiplies<ArrayType>());
    bolt::cl::copy( boltInput.begin( ), boltInput.end( ), boltOutput.begin( ), boltOutput.begin( ), bolt::cl::multiplies<ArrayType>());

    ArrayCont::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end() );
    ArrayCont::difference_type boltNumElements = std::distance( boltInput.begin( ), boltInput.end() );

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpStdArray< ArrayType, ArraySize >::cmpArrays( stdOutput, boltOutput );
    
}

TYPED_TEST_P( CopyArrayTest, GPU_DeviceMultipliesFunction )
{
    typedef std::array< ArrayType, ArraySize > ArrayCont;

    //  The first time our routines get called, we compile the library kernels with a certain context
    //  OpenCL does not allow the context to change without a recompile of the kernel
    // MyOclContext oclgpu = initOcl(CL_DEVICE_TYPE_GPU, 0);
    //bolt::cl::control c_gpu(oclgpu._queue);  // construct control structure from the queue.

    //  Create a new command queue for a different device, but use the same context as was provided
    //  by the default control device
    ::cl::Context myContext = bolt::cl::control::getDefault( ).context( );
    std::vector< cl::Device > devices = myContext.getInfo< CL_CONTEXT_DEVICES >();
    ::cl::CommandQueue myQueue( myContext, devices[ 0 ] );
    bolt::cl::control c_gpu( myQueue );  // construct control structure from the queue.

    //  Calling the actual functions under test
    std::copy( stdInput.begin( ), stdInput.end( ), stdOutput.begin( ), stdOutput.begin( ), bolt::cl::multiplies<ArrayType>());
    bolt::cl::copy( c_gpu, boltInput.begin( ), boltInput.end( ), boltOutput.begin( ), boltOutput.begin( ), bolt::cl::multiplies<ArrayType>());

    ArrayCont::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end() );
    ArrayCont::difference_type boltNumElements = std::distance( boltInput.begin( ), boltInput.end() );

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpStdArray< ArrayType, ArraySize >::cmpArrays( stdOutput, boltOutput );
    // FIXME - releaseOcl(ocl);
}
#if (TEST_CPU_DEVICE == 1)
TYPED_TEST_P( CopyArrayTest, CPU_DeviceMultipliesFunction )
{
    typedef std::array< ArrayType, ArraySize > ArrayCont;
    MyOclContext oclcpu = initOcl(CL_DEVICE_TYPE_CPU, 0);
	bolt::cl::control c_cpu(oclcpu._queue);  // construct control structure from the queue.

    //  Calling the actual functions under test
    std::copy( stdInput.begin( ), stdInput.end( ), stdOutput.begin( ), stdOutput.begin( ), bolt::cl::multiplies<ArrayType>());
    bolt::cl::copy( c_cpu, boltInput.begin( ), boltInput.end( ), boltOutput.begin( ), boltOutput.begin( ), bolt::cl::multiplies<ArrayType>());

    ArrayCont::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end() );
    ArrayCont::difference_type boltNumElements = std::distance( boltInput.begin( ), boltInput.end() );

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpStdArray< ArrayType, ArraySize >::cmpArrays( stdOutput, boltOutput );
    // FIXME - releaseOcl(ocl);
}
#endif
TYPED_TEST_P( CopyArrayTest, MinusFunction )
{
    typedef std::array< ArrayType, ArraySize > ArrayCont;

    //  Calling the actual functions under test
    std::copy( stdInput.begin( ), stdInput.end( ), stdOutput.begin( ), stdOutput.begin( ), bolt::cl::minus<ArrayType>());
    bolt::cl::copy( boltInput.begin( ), boltInput.end( ), boltOutput.begin( ), boltOutput.begin( ), bolt::cl::minus<ArrayType>());

    ArrayCont::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end() );
    ArrayCont::difference_type boltNumElements = std::distance( boltInput.begin( ), boltInput.end() );

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpStdArray< ArrayType, ArraySize >::cmpArrays( stdOutput, boltOutput );

}

TYPED_TEST_P( CopyArrayTest, GPU_DeviceMinusFunction )
{
    typedef std::array< ArrayType, ArraySize > ArrayCont;

    //  The first time our routines get called, we compile the library kernels with a certain context
    //  OpenCL does not allow the context to change without a recompile of the kernel
    // MyOclContext oclgpu = initOcl(CL_DEVICE_TYPE_GPU, 0);
    //bolt::cl::control c_gpu(oclgpu._queue);  // construct control structure from the queue.

    //  Create a new command queue for a different device, but use the same context as was provided
    //  by the default control device
    ::cl::Context myContext = bolt::cl::control::getDefault( ).context( );
    std::vector< cl::Device > devices = myContext.getInfo< CL_CONTEXT_DEVICES >();
    ::cl::CommandQueue myQueue( myContext, devices[ 0 ] );
    bolt::cl::control c_gpu( myQueue );  // construct control structure from the queue.

    //  Calling the actual functions under test
    std::copy( stdInput.begin( ), stdInput.end( ), stdOutput.begin( ), stdOutput.begin( ), bolt::cl::minus<ArrayType>());
    bolt::cl::copy( c_gpu, boltInput.begin( ), boltInput.end( ), boltOutput.begin( ), boltOutput.begin( ), bolt::cl::minus<ArrayType>());

    ArrayCont::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end() );
    ArrayCont::difference_type boltNumElements = std::distance( boltInput.begin( ), boltInput.end() );

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpStdArray< ArrayType, ArraySize >::cmpArrays( stdOutput, boltOutput );
    // FIXME - releaseOcl(ocl);
}
#if (TEST_CPU_DEVICE == 1)
TYPED_TEST_P( CopyArrayTest, CPU_DeviceMinusFunction )
{
    typedef std::array< ArrayType, ArraySize > ArrayCont;
    MyOclContext oclcpu = initOcl(CL_DEVICE_TYPE_CPU, 0);
	bolt::cl::control c_cpu(oclcpu._queue);  // construct control structure from the queue.

    //  Calling the actual functions under test
    std::copy( stdInput.begin( ), stdInput.end( ), stdOutput.begin( ), stdOutput.begin( ), bolt::cl::minus<ArrayType>());
    bolt::cl::copy( c_cpu, boltInput.begin( ), boltInput.end( ), boltOutput.begin( ), boltOutput.begin( ), bolt::cl::minus<ArrayType>());

    ArrayCont::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end() );
    ArrayCont::difference_type boltNumElements = std::distance( boltInput.begin( ), boltInput.end() );

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpStdArray< ArrayType, ArraySize >::cmpArrays( stdOutput, boltOutput );
    // FIXME - releaseOcl(ocl);
}
#endif

TYPED_TEST_CASE_P( UnaryCopyArrayTest );

TYPED_TEST_P( UnaryCopyArrayTest, Normal )
{
    typedef std::array< ArrayType, ArraySize > ArrayCont;

    //  Calling the actual functions under test
    std::copy( stdInput.begin( ), stdInput.end( ), stdOutput.begin( ), bolt::cl::negate<ArrayType>());
    bolt::cl::copy( boltInput.begin( ), boltInput.end( ), boltOutput.begin( ), bolt::cl::negate<ArrayType>());

    ArrayCont::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end() );
    ArrayCont::difference_type boltNumElements = std::distance( boltInput.begin( ), boltInput.end() );

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpStdArray< ArrayType, ArraySize >::cmpArrays( stdOutput, boltOutput );
}

TYPED_TEST_P( UnaryCopyArrayTest, GPU_DeviceNormal )
{
    typedef std::array< ArrayType, ArraySize > ArrayCont;

    //  The first time our routines get called, we compile the library kernels with a certain context
    //  OpenCL does not allow the context to change without a recompile of the kernel
    // MyOclContext oclgpu = initOcl(CL_DEVICE_TYPE_GPU, 0);
    //bolt::cl::control c_gpu(oclgpu._queue);  // construct control structure from the queue.

    //  Create a new command queue for a different device, but use the same context as was provided
    //  by the default control device
    ::cl::Context myContext = bolt::cl::control::getDefault( ).context( );
    std::vector< cl::Device > devices = myContext.getInfo< CL_CONTEXT_DEVICES >();
    ::cl::CommandQueue myQueue( myContext, devices[ 0 ] );
    bolt::cl::control c_gpu( myQueue );  // construct control structure from the queue.

    //  Calling the actual functions under test
    std::copy( stdInput.begin( ), stdInput.end( ), stdOutput.begin( ), bolt::cl::negate<ArrayType>());
    bolt::cl::copy( c_gpu, boltInput.begin( ), boltInput.end( ),  boltOutput.begin( ), bolt::cl::negate<ArrayType>());

    ArrayCont::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end() );
    ArrayCont::difference_type boltNumElements = std::distance( boltInput.begin( ), boltInput.end() );

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpStdArray< ArrayType, ArraySize >::cmpArrays( stdOutput, boltOutput );
    // FIXME - releaseOcl(ocl);
}

#if (TEST_CPU_DEVICE == 1)
TYPED_TEST_P( UnaryCopyArrayTest, CPU_DeviceNormal )
{
    typedef std::array< ArrayType, ArraySize > ArrayCont;
    MyOclContext oclcpu = initOcl(CL_DEVICE_TYPE_CPU, 0);
	bolt::cl::control c_cpu(oclcpu._queue);  // construct control structure from the queue.

    //  Calling the actual functions under test
    std::copy( stdInput.begin( ), stdInput.end( ), stdOutput.begin( ), bolt::cl::negate<ArrayType>());
    bolt::cl::copy( c_cpu, boltInput.begin( ), boltInput.end( ), boltOutput.begin( ), bolt::cl::negate<ArrayType>());

    ArrayCont::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end() );
    ArrayCont::difference_type boltNumElements = std::distance( boltInput.begin( ), boltInput.end() );

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpStdArray< ArrayType, ArraySize >::cmpArrays( stdOutput, boltOutput );
    // FIXME - releaseOcl(ocl);
}
#endif

TYPED_TEST_P( UnaryCopyArrayTest, MultipliesFunction )
{
    typedef std::array< ArrayType, ArraySize > ArrayCont;

    //  Calling the actual functions under test
    std::copy( stdInput.begin( ), stdInput.end( ), stdOutput.begin( ), bolt::cl::square<ArrayType>());
    bolt::cl::copy( boltInput.begin( ), boltInput.end( ), boltOutput.begin( ), bolt::cl::square<ArrayType>());

    ArrayCont::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end() );
    ArrayCont::difference_type boltNumElements = std::distance( boltInput.begin( ), boltInput.end() );

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpStdArray< ArrayType, ArraySize >::cmpArrays( stdOutput, boltOutput );
}

TYPED_TEST_P( UnaryCopyArrayTest, GPU_DeviceMultipliesFunction )
{
    typedef std::array< ArrayType, ArraySize > ArrayCont;

    //  The first time our routines get called, we compile the library kernels with a certain context
    //  OpenCL does not allow the context to change without a recompile of the kernel
    // MyOclContext oclgpu = initOcl(CL_DEVICE_TYPE_GPU, 0);
    //bolt::cl::control c_gpu(oclgpu._queue);  // construct control structure from the queue.

    //  Create a new command queue for a different device, but use the same context as was provided
    //  by the default control device
    ::cl::Context myContext = bolt::cl::control::getDefault( ).context( );
    std::vector< cl::Device > devices = myContext.getInfo< CL_CONTEXT_DEVICES >();
    ::cl::CommandQueue myQueue( myContext, devices[ 0 ] );
    bolt::cl::control c_gpu( myQueue );  // construct control structure from the queue.

    //  Calling the actual functions under test
    std::copy( stdInput.begin( ), stdInput.end( ), stdOutput.begin( ), bolt::cl::square<ArrayType>());
    bolt::cl::copy( c_gpu, boltInput.begin( ), boltInput.end( ), boltOutput.begin( ), bolt::cl::square<ArrayType>());

    ArrayCont::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end() );
    ArrayCont::difference_type boltNumElements = std::distance( boltInput.begin( ), boltInput.end() );

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpStdArray< ArrayType, ArraySize >::cmpArrays( stdOutput, boltOutput );
    // FIXME - releaseOcl(ocl);
}
#if (TEST_CPU_DEVICE == 1)
TYPED_TEST_P( UnaryCopyArrayTest, CPU_DeviceMultipliesFunction )
{
    typedef std::array< ArrayType, ArraySize > ArrayCont;
    MyOclContext oclcpu = initOcl(CL_DEVICE_TYPE_CPU, 0);
	bolt::cl::control c_cpu(oclcpu._queue);  // construct control structure from the queue.

    //  Calling the actual functions under test
    std::copy( stdInput.begin( ), stdInput.end( ), stdOutput.begin( ), bolt::cl::square<ArrayType>());
    bolt::cl::copy( c_cpu, boltInput.begin( ), boltInput.end( ), boltOutput.begin( ), bolt::cl::square<ArrayType>());

    ArrayCont::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end() );
    ArrayCont::difference_type boltNumElements = std::distance( boltInput.begin( ), boltInput.end() );

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpStdArray< ArrayType, ArraySize >::cmpArrays( stdOutput, boltOutput );
    // FIXME - releaseOcl(ocl);
}
#endif
TYPED_TEST_P( UnaryCopyArrayTest, MinusFunction )
{
    typedef std::array< ArrayType, ArraySize > ArrayCont;

    //  Calling the actual functions under test
    std::copy( stdInput.begin( ), stdInput.end( ), stdOutput.begin( ), bolt::cl::square<ArrayType>());
    bolt::cl::copy( boltInput.begin( ), boltInput.end( ), boltOutput.begin( ), bolt::cl::square<ArrayType>());

    ArrayCont::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end() );
    ArrayCont::difference_type boltNumElements = std::distance( boltInput.begin( ), boltInput.end() );

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpStdArray< ArrayType, ArraySize >::cmpArrays( stdOutput, boltOutput );

}

TYPED_TEST_P( UnaryCopyArrayTest, GPU_DeviceMinusFunction )
{
    typedef std::array< ArrayType, ArraySize > ArrayCont;

    //  The first time our routines get called, we compile the library kernels with a certain context
    //  OpenCL does not allow the context to change without a recompile of the kernel
    // MyOclContext oclgpu = initOcl(CL_DEVICE_TYPE_GPU, 0);
    //bolt::cl::control c_gpu(oclgpu._queue);  // construct control structure from the queue.

    //  Create a new command queue for a different device, but use the same context as was provided
    //  by the default control device
    ::cl::Context myContext = bolt::cl::control::getDefault( ).context( );
    std::vector< cl::Device > devices = myContext.getInfo< CL_CONTEXT_DEVICES >();
    ::cl::CommandQueue myQueue( myContext, devices[ 0 ] );
    bolt::cl::control c_gpu( myQueue );  // construct control structure from the queue.

    //  Calling the actual functions under test
    std::copy( stdInput.begin( ), stdInput.end( ), stdOutput.begin( ), bolt::cl::square<ArrayType>());
    bolt::cl::copy( c_gpu, boltInput.begin( ), boltInput.end( ), boltOutput.begin( ), bolt::cl::square<ArrayType>());

    ArrayCont::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end() );
    ArrayCont::difference_type boltNumElements = std::distance( boltInput.begin( ), boltInput.end() );

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpStdArray< ArrayType, ArraySize >::cmpArrays( stdOutput, boltOutput );
    // FIXME - releaseOcl(ocl);
}
#if (TEST_CPU_DEVICE == 1)
TYPED_TEST_P( UnaryCopyArrayTest, CPU_DeviceMinusFunction )
{
    typedef std::array< ArrayType, ArraySize > ArrayCont;
    MyOclContext oclcpu = initOcl(CL_DEVICE_TYPE_CPU, 0);
	bolt::cl::control c_cpu(oclcpu._queue);  // construct control structure from the queue.

    //  Calling the actual functions under test
    std::copy( stdInput.begin( ), stdInput.end( ), stdOutput.begin( ), bolt::cl::square<ArrayType>());
    bolt::cl::copy( c_cpu, boltInput.begin( ), boltInput.end( ), boltOutput.begin( ), bolt::cl::square<ArrayType>());

    ArrayCont::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end() );
    ArrayCont::difference_type boltNumElements = std::distance( boltInput.begin( ), boltInput.end() );

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpStdArray< ArrayType, ArraySize >::cmpArrays( stdOutput, boltOutput );
    // FIXME - releaseOcl(ocl);
}
#endif

#if (TEST_CPU_DEVICE == 1)
REGISTER_TYPED_TEST_CASE_P( CopyArrayTest, Normal, GPU_DeviceNormal, 
                                           MultipliesFunction, GPU_DeviceMultipliesFunction,
                                           MinusFunction, GPU_DeviceMinusFunction, CPU_DeviceNormal, CPU_DeviceMultipliesFunction, CPU_DeviceMinusFunction);
REGISTER_TYPED_TEST_CASE_P( UnaryCopyArrayTest, Normal, GPU_DeviceNormal, 
                                           MultipliesFunction, GPU_DeviceMultipliesFunction,
                                           MinusFunction, GPU_DeviceMinusFunction, CPU_DeviceNormal, CPU_DeviceMultipliesFunction, CPU_DeviceMinusFunction);
#else
REGISTER_TYPED_TEST_CASE_P( CopyArrayTest, Normal, GPU_DeviceNormal, 
                                           MultipliesFunction, GPU_DeviceMultipliesFunction,
                                           MinusFunction, GPU_DeviceMinusFunction );
REGISTER_TYPED_TEST_CASE_P( UnaryCopyArrayTest, Normal, GPU_DeviceNormal, 
                                           MultipliesFunction, GPU_DeviceMultipliesFunction,
                                           MinusFunction, GPU_DeviceMinusFunction );
#endif


/////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Fixture classes are now defined to enable googletest to process value parameterized tests
//  ::testing::TestWithParam< int > means that GetParam( ) returns int values, which i use for array size

class CopyIntegerVector: public ::testing::TestWithParam< int >
{
public:

    CopyIntegerVector( ): stdInput( GetParam( ) ), boltInput( GetParam( ) ),
                               stdOutput( GetParam( ) ), boltOutput( GetParam( ) )
    {
        std::generate(stdInput.begin(), stdInput.end(), rand);

        boltInput = stdInput;
        stdOutput = stdInput;
        boltOutput = stdInput;
    }

protected:
    std::vector< int > stdInput, boltInput, stdOutput, boltOutput;
};

//  ::testing::TestWithParam< int > means that GetParam( ) returns int values, which i use for array size
class CopyFloatVector: public ::testing::TestWithParam< int >
{
public:
    CopyFloatVector( ): stdInput( GetParam( ) ), boltInput( GetParam( ) ),
                             stdOutput( GetParam( ) ), boltOutput( GetParam( ) )
    {
        std::generate(stdInput.begin(), stdInput.end(), rand);
        boltInput = stdInput;
        stdOutput = stdInput;
        boltOutput = stdInput;
    }

protected:
    std::vector< float > stdInput, boltInput, stdOutput, boltOutput;
};

#if (TEST_DOUBLE == 1)
//  ::testing::TestWithParam< int > means that GetParam( ) returns int values, which i use for array size
class CopyDoubleVector: public ::testing::TestWithParam< int >
{
public:
    CopyDoubleVector( ): stdInput( GetParam( ) ), boltInput( GetParam( ) ),
                              stdOutput( GetParam( ) ), boltOutput( GetParam( ) )
    {
        std::generate(stdInput.begin(), stdInput.end(), rand);
        boltInput = stdInput;
        stdOutput = stdInput;
        boltOutput = stdInput;
    }

protected:
    std::vector< double > stdInput, boltInput, stdOutput, boltOutput;
};
#endif

//  ::testing::TestWithParam< int > means that GetParam( ) returns int values, which i use for array size
class CopyIntegerDeviceVector: public ::testing::TestWithParam< int >
{
public:
    // Create an std and a bolt vector of requested size, and initialize all the elements to 1
    CopyIntegerDeviceVector( ): stdInput( GetParam( ) ), boltInput( GetParam( ) ),
                                     stdOutput( GetParam( ) ), boltOutput( GetParam( ) )
    {
        std::generate(stdInput.begin(), stdInput.end(), rand);
        //boltInput = stdInput;      
        //FIXME - The above should work but the below loop is used. 
        for (int i=0; i< GetParam( ); i++)
        {
            boltInput[i] = stdInput[i];
            boltOutput[i] = stdInput[i];
            stdOutput[i] = stdInput[i];
        }
    }

protected:
    std::vector< int > stdInput, stdOutput;
    bolt::cl::device_vector< int > boltInput, boltOutput;
};

//  ::testing::TestWithParam< int > means that GetParam( ) returns int values, which i use for array size
class CopyFloatDeviceVector: public ::testing::TestWithParam< int >
{
public:
    // Create an std and a bolt vector of requested size, and initialize all the elements to 1
    CopyFloatDeviceVector( ): stdInput( GetParam( ) ), boltInput( static_cast<size_t>( GetParam( ) ) ),
                                   stdOutput( GetParam( ) ), boltOutput( GetParam( ) )
    {
        std::generate(stdInput.begin(), stdInput.end(), rand);
        //boltInput = stdInput;      
        //FIXME - The above should work but the below loop is used. 
        for (int i=0; i< GetParam( ); i++)
        {
            boltInput[i] = stdInput[i];
            boltOutput[i] = stdInput[i];
            stdOutput[i] = stdInput[i];
        }
    }

protected:
    std::vector< float > stdInput, stdOutput;
    bolt::cl::device_vector< float > boltInput, boltOutput;
};

#if (TEST_DOUBLE == 1)
//  ::testing::TestWithParam< int > means that GetParam( ) returns int values, which i use for array size
class CopyDoubleDeviceVector: public ::testing::TestWithParam< int >
{
public:
    // Create an std and a bolt vector of requested size, and initialize all the elements to 1
    CopyDoubleDeviceVector( ): stdInput( GetParam( ) ), boltInput( static_cast<size_t>( GetParam( ) ) )
    {
        std::generate(stdInput.begin(), stdInput.end(), rand);
        //boltInput = stdInput;      
        //FIXME - The above should work but the below loop is used. 
        for (int i=0; i< GetParam( ); i++)
        {
            boltInput[i] = stdInput[i];
            boltOutput[i] = stdInput[i];
            stdOutput[i] = stdInput[i];
        }
    }

protected:
    std::vector< double > stdInput, stdOutput;
    bolt::cl::device_vector< double > boltInput, boltOutput;
};
#endif

//  ::testing::TestWithParam< int > means that GetParam( ) returns int values, which i use for array size
class CopyIntegerNakedPointer: public ::testing::TestWithParam< int >
{
public:
    //  Create an std and a bolt vector of requested size, and initialize all the elements to 1
    CopyIntegerNakedPointer( ): stdInput( new int[ GetParam( ) ] ), boltInput( new int[ GetParam( ) ] ), 
                                     stdOutput( new int[ GetParam( ) ] ), boltOutput( new int[ GetParam( ) ] )
    {}

    virtual void SetUp( )
    {
        size_t size = GetParam( );

        std::generate(stdInput, stdInput + size, rand);
        for (int i = 0; i<size; i++)
        {
            boltInput[i] = stdInput[i];
            boltOutput[i] = stdInput[i];
            stdOutput[i] = stdInput[i];
        }
    };

    virtual void TearDown( )
    {
        delete [] stdInput;
        delete [] boltInput;
        delete [] stdOutput;
        delete [] boltOutput;
};

protected:
     int* stdInput;
     int* boltInput;
     int* stdOutput;
     int* boltOutput;

};

//  ::testing::TestWithParam< int > means that GetParam( ) returns int values, which i use for array size
class CopyFloatNakedPointer: public ::testing::TestWithParam< int >
{
public:
    //  Create an std and a bolt vector of requested size, and initialize all the elements to 1
    CopyFloatNakedPointer( ): stdInput( new float[ GetParam( ) ] ), boltInput( new float[ GetParam( ) ] ), 
                                   stdOutput( new float[ GetParam( ) ] ), boltOutput( new float[ GetParam( ) ] )
    {}

    virtual void SetUp( )
    {
        size_t size = GetParam( );

        std::generate(stdInput, stdInput + size, rand);
        for (int i = 0; i<size; i++)
        {
            boltInput[i] = stdInput[i];
            boltOutput[i] = stdInput[i];
            stdOutput[i] = stdInput[i];
        }
    };

    virtual void TearDown( )
    {
        delete [] stdInput;
        delete [] boltInput;
        delete [] stdOutput;
        delete [] boltOutput;
    };

protected:
     float* stdInput;
     float* boltInput;
     float* stdOutput;
     float* boltOutput;
};

#if (TEST_DOUBLE ==1 )
//  ::testing::TestWithParam< int > means that GetParam( ) returns int values, which i use for array size
class CopyDoubleNakedPointer: public ::testing::TestWithParam< int >
{
public:
    //  Create an std and a bolt vector of requested size, and initialize all the elements to 1
    CopyDoubleNakedPointer( ): stdInput( new double[ GetParam( ) ] ), boltInput( new double[ GetParam( ) ] ),
                                    stdOutput( new double[ GetParam( ) ] ), boltOutput( new double[ GetParam( ) ] )
    {}

    virtual void SetUp( )
    {
        size_t size = GetParam( );

        std::generate(stdInput, stdInput + size, rand);

        for( int i=0; i < size; i++ )
        {
            boltInput[ i ] = stdInput[ i ];
            boltOutput[i] = stdInput[i];
            stdOutput[i] = stdInput[i];
        }
    };

    virtual void TearDown( )
    {
        delete [] stdInput;
        delete [] boltInput;
        delete [] stdOutput;
        delete [] boltOutput;
    };

protected:
     double* stdInput;
     double* boltInput;
     double* stdOutput;
     double* boltOutput;
};
#endif

TEST_P( CopyIntegerVector, Normal )
{
    
    //typedef std::iterator_traits<std::vector<int>::iterator>::value_type T;
    //  Calling the actual functions under test
    std::copy( stdInput.begin( ), stdInput.end( ), stdOutput.begin( ), stdOutput.begin( ), bolt::cl::plus<int>());
    bolt::cl::copy( boltInput.begin( ), boltInput.end( ), boltOutput.begin( ), boltOutput.begin( ), bolt::cl::plus<int>());

    std::vector< int >::iterator::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end( ) );
    std::vector< int >::iterator::difference_type boltNumElements = std::distance( boltInput.begin( ), boltInput.end( ) );

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdOutput, boltOutput );

    std::copy( stdInput.begin( ), stdInput.end( ), stdOutput.begin( ), bolt::cl::negate<int>());
    bolt::cl::copy( boltInput.begin( ), boltInput.end( ), boltOutput.begin( ), bolt::cl::negate<int>());

    stdNumElements = std::distance( stdInput.begin( ), stdInput.end( ) );
    boltNumElements = std::distance( boltInput.begin( ), boltInput.end( ) );

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdOutput, boltOutput );
}

TEST_P( CopyFloatVector, Normal )
{
    //  Calling the actual functions under test
    std::copy( stdInput.begin( ), stdInput.end( ), stdOutput.begin( ), stdOutput.begin( ), bolt::cl::plus<float>());
    bolt::cl::copy( boltInput.begin( ), boltInput.end( ), boltOutput.begin( ), boltOutput.begin( ), bolt::cl::plus<float>());

    std::vector< float >::iterator::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end( ) );
    std::vector< float >::iterator::difference_type boltNumElements = std::distance( boltInput.begin( ), boltInput.end( ) );

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdInput, boltInput );

    std::copy( stdInput.begin( ), stdInput.end( ), stdOutput.begin( ), bolt::cl::negate<float>());
    bolt::cl::copy( boltInput.begin( ), boltInput.end( ), boltOutput.begin( ), bolt::cl::negate<float>());

    stdNumElements = std::distance( stdInput.begin( ), stdInput.end( ) );
    boltNumElements = std::distance( boltInput.begin( ), boltInput.end( ) );

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdOutput, boltOutput );

}
#if (TEST_DOUBLE == 1)
TEST_P( CopyDoubleVector, Inplace )
{
    //  Calling the actual functions under test
    std::copy( stdInput.begin( ), stdInput.end( ), stdOutput.begin( ), stdOutput.begin( ), bolt::cl::plus<double>());
    bolt::cl::copy( boltInput.begin( ), boltInput.end( ), boltOutput.begin( ), boltOutput.begin( ), bolt::cl::plus<double>());

    std::vector< double >::iterator::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end( ) );
    std::vector< double >::iterator::difference_type boltNumElements = std::distance( boltInput.begin( ), boltInput.end( ) );

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdOutput, boltOutput );

    std::copy( stdInput.begin( ), stdInput.end( ), stdOutput.begin( ), bolt::cl::negate<double>());
    bolt::cl::copy( boltInput.begin( ), boltInput.end( ), boltOutput.begin( ), bolt::cl::negate<double>());

    stdNumElements = std::distance( stdInput.begin( ), stdInput.end( ) );
    boltNumElements = std::distance( boltInput.begin( ), boltInput.end( ) );

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdOutput, boltOutput );

}
#endif
#if (TEST_DEVICE_VECTOR == 1)
TEST_P( CopyIntegerDeviceVector, Inplace )
{
    //  Calling the actual functions under test
    std::copy( stdInput.begin( ), stdInput.end( ), stdOutput.begin( ), stdOutput.begin( ), bolt::cl::plus<int>());
    bolt::cl::copy( boltInput.begin( ), boltInput.end( ), boltOutput.begin( ), boltOutput.begin( ), bolt::cl::plus<int>());

    std::vector< int >::iterator::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end( ) );
    std::vector< int >::iterator::difference_type boltNumElements = std::distance( boltInput.begin( ), boltInput.end( ) );

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdOutput, boltOutput );

    //UNARY copy Test
    //  Calling the actual functions under test
    std::copy( stdInput.begin( ), stdInput.end( ), stdOutput.begin( ), bolt::cl::negate<int>());
    bolt::cl::copy( boltInput.begin( ), boltInput.end( ), boltOutput.begin( ), bolt::cl::negate<int>());

    stdNumElements = std::distance( stdInput.begin( ), stdInput.end( ) );
    boltNumElements = std::distance( boltInput.begin( ), boltInput.end( ) );

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdOutput, boltOutput );
}
TEST_P( CopyFloatDeviceVector, Inplace )
{
    //  Calling the actual functions under test
    std::copy( stdInput.begin( ), stdInput.end( ), stdOutput.begin( ), stdOutput.begin( ), bolt::cl::plus<float>());
    bolt::cl::copy( boltInput.begin( ), boltInput.end( ), boltOutput.begin( ), boltOutput.begin( ), bolt::cl::plus<float>());

    std::vector< float >::iterator::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end( ) );
    std::vector< float >::iterator::difference_type boltNumElements = std::distance( boltInput.begin( ), boltInput.end( ) );

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdOutput, boltOutput );

    //UNARY copy Test
    //  Calling the actual functions under test
    std::copy( stdInput.begin( ), stdInput.end( ), stdOutput.begin( ), bolt::cl::negate<float>());
    bolt::cl::copy( boltInput.begin( ), boltInput.end( ), boltOutput.begin( ), bolt::cl::negate<float>());

    stdNumElements = std::distance( stdInput.begin( ), stdInput.end( ) );
    boltNumElements = std::distance( boltInput.begin( ), boltInput.end( ) );

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdOutput, boltOutput );
}
#if (TEST_DOUBLE == 1)
TEST_P( CopyDoubleDeviceVector, Inplace )
{
    //  Calling the actual functions under test
    std::copy( stdInput.begin( ), stdInput.end( ), stdOutput.begin( ), stdOutput.begin( ), bolt::cl::plus<double>());
    bolt::cl::copy( boltInput.begin( ), boltInput.end( ), boltOutput.begin( ), boltOutput.begin( ), bolt::cl::plus<double>());

    std::vector< double >::iterator::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end( ) );
    std::vector< double >::iterator::difference_type boltNumElements = std::distance( boltInput.begin( ), boltInput.end( ) );

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdOutput, boltOutput );

    //UNARY copy Test
    //  Calling the actual functions under test
    std::copy( stdInput.begin( ), stdInput.end( ), stdOutput.begin( ), bolt::cl::negate<double>());
    bolt::cl::copy( boltInput.begin( ), boltInput.end( ), boltOutput.begin( ), bolt::cl::negate<double>());

    stdNumElements = std::distance( stdInput.begin( ), stdInput.end( ) );
    boltNumElements = std::distance( boltInput.begin( ), boltInput.end( ) );

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdOutput, boltOutput );
}
#endif
#endif

TEST_P( CopyIntegerNakedPointer, Inplace )
{
    size_t endIndex = GetParam( );

    //  Calling the actual functions under test
    stdext::checked_array_iterator< int* > wrapStdInput( stdInput, endIndex );
    stdext::checked_array_iterator< int* > wrapStdOutput( stdOutput, endIndex );
    stdext::checked_array_iterator< int* > wrapBoltInput( boltInput, endIndex );
    stdext::checked_array_iterator< int* > wrapBoltOutput( boltOutput, endIndex );

    std::copy( wrapStdInput, wrapStdInput + endIndex, wrapStdOutput, wrapStdOutput, bolt::cl::plus<int>());
    bolt::cl::copy( wrapBoltInput, wrapBoltInput+endIndex, wrapBoltOutput, wrapBoltOutput, bolt::cl::plus<int>());

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdOutput, boltOutput, endIndex );
    
    //UNARY copy Test
    std::copy( wrapStdInput, wrapStdInput + endIndex, wrapStdOutput, bolt::cl::negate<int>());
    bolt::cl::copy( wrapBoltInput, wrapBoltInput+endIndex, wrapBoltOutput, bolt::cl::negate<int>());

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdOutput, boltOutput, endIndex );
}

TEST_P( CopyFloatNakedPointer, Inplace )
{
    size_t endIndex = GetParam( );

    //  Calling the actual functions under test
    stdext::checked_array_iterator< float* > wrapStdInput( stdInput, endIndex );
    stdext::checked_array_iterator< float* > wrapStdOutput( stdOutput, endIndex );
    stdext::checked_array_iterator< float* > wrapBoltInput( boltInput, endIndex );
    stdext::checked_array_iterator< float* > wrapBoltOutput( boltOutput, endIndex );

    std::copy( wrapStdInput, wrapStdInput + endIndex, wrapStdOutput, wrapStdOutput, bolt::cl::plus<float>());
    bolt::cl::copy( wrapBoltInput, wrapBoltInput+endIndex, wrapBoltOutput, wrapBoltOutput, bolt::cl::plus<float>());

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdOutput, boltOutput, endIndex );

    //UNARY copy Test
    std::copy( wrapStdInput, wrapStdInput + endIndex, wrapStdOutput, bolt::cl::negate<float>());
    bolt::cl::copy( wrapBoltInput, wrapBoltInput+endIndex, wrapBoltOutput, bolt::cl::negate<float>());

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdOutput, boltOutput, endIndex );
}

#if (TEST_DOUBLE == 1)
TEST_P( CopyDoubleNakedPointer, Inplace )
{
    size_t endIndex = GetParam( );

    //  Calling the actual functions under test
    stdext::checked_array_iterator< double* > wrapStdInput( stdInput, endIndex );
    stdext::checked_array_iterator< double* > wrapStdOutput( stdOutput, endIndex );
    stdext::checked_array_iterator< double* > wrapBoltInput( boltInput, endIndex );
    stdext::checked_array_iterator< double* > wrapBoltOutput( boltOutput, endIndex );

    std::copy( wrapStdInput, wrapStdInput + endIndex, wrapStdOutput, wrapStdOutput, bolt::cl::plus<double>());
    bolt::cl::copy( wrapBoltInput, wrapBoltInput+endIndex, wrapBoltOutput, wrapBoltOutput, bolt::cl::plus<double>());

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdOutput, boltOutput, endIndex );

    //UNARY copy Test
    std::copy( wrapStdInput, wrapStdInput + endIndex, wrapStdOutput, bolt::cl::negate<double>());
    bolt::cl::copy( wrapBoltInput, wrapBoltInput+endIndex, wrapBoltOutput, bolt::cl::negate<double>());

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdOutput, boltOutput, endIndex );
}
#endif
std::array<int, 15> TestValues = {2,4,8,16,32,64,128,256,512,1024,2048,4096,8192,16384,32768};
//Test lots of consecutive numbers, but small range, suitable for integers because they overflow easier
INSTANTIATE_TEST_CASE_P( CopyRange, CopyIntegerVector, ::testing::Range( 0, 1024, 7 ) );
INSTANTIATE_TEST_CASE_P( CopyValues, CopyIntegerVector, ::testing::ValuesIn( TestValues.begin(), TestValues.end() ) );
INSTANTIATE_TEST_CASE_P( CopyRange, CopyFloatVector, ::testing::Range( 0, 1024, 3 ) );
INSTANTIATE_TEST_CASE_P( CopyValues, CopyFloatVector, ::testing::ValuesIn( TestValues.begin(), TestValues.end() ) );
#if (TEST_DOUBLE == 1)
INSTANTIATE_TEST_CASE_P( CopyRange, CopyDoubleVector, ::testing::Range( 0, 1024, 21 ) );
INSTANTIATE_TEST_CASE_P( CopyValues, CopyDoubleVector, ::testing::ValuesIn( TestValues.begin(), TestValues.end() ) );
#endif
INSTANTIATE_TEST_CASE_P( CopyRange, CopyIntegerDeviceVector, ::testing::Range( 0, 1024, 53 ) );
INSTANTIATE_TEST_CASE_P( CopyValues, CopyIntegerDeviceVector, ::testing::ValuesIn( TestValues.begin(), TestValues.end() ) );
INSTANTIATE_TEST_CASE_P( CopyRange, CopyFloatDeviceVector, ::testing::Range( 0, 1024, 53 ) );
INSTANTIATE_TEST_CASE_P( CopyValues, CopyFloatDeviceVector, ::testing::ValuesIn( TestValues.begin(), TestValues.end() ) );
#if (TEST_DOUBLE == 1)
INSTANTIATE_TEST_CASE_P( CopyRange, CopyDoubleDeviceVector, ::testing::Range( 0, 1024, 53 ) );
INSTANTIATE_TEST_CASE_P( CopyValues, CopyDoubleDeviceVector, ::testing::ValuesIn( TestValues.begin(), TestValues.end() ) );
#endif
INSTANTIATE_TEST_CASE_P( CopyRange, CopyIntegerNakedPointer, ::testing::Range( 0, 1024, 13) );
INSTANTIATE_TEST_CASE_P( CopyValues, CopyIntegerNakedPointer, ::testing::ValuesIn( TestValues.begin(), TestValues.end() ) );
INSTANTIATE_TEST_CASE_P( CopyRange, CopyFloatNakedPointer, ::testing::Range( 0, 1024, 13) );
INSTANTIATE_TEST_CASE_P( CopyValues, CopyFloatNakedPointer, ::testing::ValuesIn( TestValues.begin(), TestValues.end() ) );
#if (TEST_DOUBLE == 1)
INSTANTIATE_TEST_CASE_P( CopyRange, CopyDoubleNakedPointer, ::testing::Range( 0, 1024, 13) );
INSTANTIATE_TEST_CASE_P( Copy, CopyDoubleNakedPointer, ::testing::ValuesIn( TestValues.begin(), TestValues.end() ) );
#endif

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

#if (TEST_DOUBLE == 1)
typedef ::testing::Types< 
    std::tuple< double, TypeValue< 1 > >,
    std::tuple< double, TypeValue< 31 > >,
    std::tuple< double, TypeValue< 32 > >,
    std::tuple< double, TypeValue< 63 > >,
    std::tuple< double, TypeValue< 64 > >,
    std::tuple< double, TypeValue< 127 > >,
    std::tuple< double, TypeValue< 128 > >,
    std::tuple< double, TypeValue< 129 > >,
    std::tuple< double, TypeValue< 1000 > >,
    std::tuple< double, TypeValue< 1053 > >,
    std::tuple< double, TypeValue< 4096 > >,
    std::tuple< double, TypeValue< 4097 > >,
    std::tuple< double, TypeValue< 65535 > >,
    std::tuple< double, TypeValue< 65536 > >
> DoubleTests;
#endif 
BOLT_FUNCTOR(UDD,
struct UDD { 
    int a; 
    int b;

    bool operator() (const UDD& lhs, const UDD& rhs) { 
        return ((lhs.a+lhs.b) > (rhs.a+rhs.b));
    } 
    bool operator < (const UDD& other) const { 
        return ((a+b) < (other.a+other.b));
    }
    bool operator > (const UDD& other) const { 
        return ((a+b) > (other.a+other.b));
    }
    bool operator == (const UDD& other) const { 
        return ((a+b) == (other.a+other.b));
    }
    UDD() 
        : a(0),b(0) { } 
    UDD(int _in) 
        : a(_in), b(_in +1)  { } 
}; 
);
BOLT_CREATE_TYPENAME(bolt::cl::less<UDD>);
BOLT_CREATE_TYPENAME(bolt::cl::greater<UDD>);
BOLT_CREATE_TYPENAME(bolt::cl::plus<UDD>);

typedef ::testing::Types< 
    std::tuple< UDD, TypeValue< 1 > >,
    std::tuple< UDD, TypeValue< 31 > >,
    std::tuple< UDD, TypeValue< 32 > >,
    std::tuple< UDD, TypeValue< 63 > >,
    std::tuple< UDD, TypeValue< 64 > >,
    std::tuple< UDD, TypeValue< 127 > >,
    std::tuple< UDD, TypeValue< 128 > >,
    std::tuple< UDD, TypeValue< 129 > >,
    std::tuple< UDD, TypeValue< 1000 > >,
    std::tuple< UDD, TypeValue< 1053 > >,
    std::tuple< UDD, TypeValue< 4096 > >,
    std::tuple< UDD, TypeValue< 4097 > >,
    std::tuple< UDD, TypeValue< 65535 > >,
    std::tuple< UDD, TypeValue< 65536 > >
> UDDTests;

INSTANTIATE_TYPED_TEST_CASE_P( Integer, CopyArrayTest, IntegerTests );
INSTANTIATE_TYPED_TEST_CASE_P( Float, CopyArrayTest, FloatTests );
#if (TEST_DOUBLE == 1)
INSTANTIATE_TYPED_TEST_CASE_P( Double, CopyArrayTest, DoubleTests );
#endif 
//INSTANTIATE_TYPED_TEST_CASE_P( UDDTest, SortArrayTest, UDDTests );

INSTANTIATE_TYPED_TEST_CASE_P( Integer, UnaryCopyArrayTest, IntegerTests );
INSTANTIATE_TYPED_TEST_CASE_P( Float, UnaryCopyArrayTest, FloatTests );
#if (TEST_DOUBLE == 1)
INSTANTIATE_TYPED_TEST_CASE_P( Double, UnaryCopyArrayTest, DoubleTests );
#endif 
//INSTANTIATE_TYPED_TEST_CASE_P( UDDTest, SortArrayTest, UDDTests );

int main(int argc, char* argv[])
{
    //  Register our minidump generating logic
    bolt::miniDumpSingleton::enableMiniDumps( );

    //	Define MEMORYREPORT on windows platfroms to enable debug memory heap checking
#if defined( MEMORYREPORT ) && defined( _WIN32 )
    TCHAR logPath[ MAX_PATH ];
    ::GetCurrentDirectory( MAX_PATH, logPath );
    ::_tcscat_s( logPath, _T( "\\MemoryReport.txt") );

    //	We leak the handle to this file, on purpose, so that the ::_CrtSetReportFile() can output it's memory 
    //	statistics on app shutdown
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

    //	By looking at the memory leak report that is generated by this debug heap, there is a number with 
    //	{} brackets that indicates the incremental allocation number of that block.  If you wish to set
    //	a breakpoint on that allocation number, put it in the _CrtSetBreakAlloc() call below, and the heap
    //	will issue a bp on the request, allowing you to look at the call stack
    //	::_CrtSetBreakAlloc( 1833 );

#endif /* MEMORYREPORT */

    ::testing::InitGoogleTest( &argc, &argv[ 0 ] );

    ////  Set the standard OpenCL wait behavior to help debugging
    //bolt::cl::control& myControl = bolt::cl::control::getDefault( );
    //myControl.waitMode( bolt::cl::control::NiceWait );

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
#endif