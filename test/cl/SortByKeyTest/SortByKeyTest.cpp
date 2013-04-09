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
#define GOOGLE_TEST 1
#if (GOOGLE_TEST == 1)

#include "common/stdafx.h"
#include "common/myocl.h"

#include <bolt/cl/sort_by_key.h>
#include <bolt/miniDump.h>
#include <bolt/unicode.h>

#include <gtest/gtest.h>
#include <boost/shared_array.hpp>
#include <array>
#include <algorithm>
/////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Below are helper routines to compare the results of two arrays for googletest
//  They return an assertion object that googletest knows how to track
//This is a compare routine for naked pointers.
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

template <typename T>
struct stdSortData {
    int key;
    T value;

    bool operator() (const stdSortData& lhs, const stdSortData& rhs) {
        return (lhs.key < rhs.key);
    }
    stdSortData& operator = (const stdSortData& other) {
        key = other.key;
        value = other.value;
        return (*this);
    }
    /*stdSortData& operator() () {
        key = rand();
        value = rand();
        return (*this);
    }*/
    bool operator < (const stdSortData& other) const {
        return (key < (other.key));
    }
    bool operator > (const stdSortData& other) const {
        return (key > other.key);
    }
    bool operator == (const stdSortData& other) const {
        return (key == other.key);
    }
    stdSortData()
        : key(0),value(0) { }

};
//  A very generic template that takes two container, and compares their values assuming a vector interface
template< typename stdType, typename B, typename C >
::testing::AssertionResult cmpArraysSortByKey( const std::vector< stdSortData<stdType> >& ref, const B& key, const C& value )
{
    for( size_t i = 0; i < ref.size( ); ++i )
    {
        EXPECT_EQ( ref[ i ].key, key[ i ] ) << _T( "Where i = " ) << i;
        //EXPECT_EQ( ref[ i ].value, value[ i ] ) << _T( "Where i = " ) << i;
    }
    return ::testing::AssertionSuccess( );
}

template< typename ArrayTuple >
class SortArrayTest: public ::testing::Test
{
public:
    SortArrayTest( ): m_Errors( 0 )
    {}

    virtual void SetUp( )
    {
        std::generate(stdValues.begin(), stdValues.end(), stdValues);
        boltValues = stdValues;
        std::generate(stdKeys.begin(), stdKeys.end(), rand);
        boltKeys = stdKeys;
    };

    virtual void TearDown( )
    {};

    virtual ~SortArrayTest( )
    {}

protected:
    typedef typename std::tuple_element< 0, ArrayTuple >::type ArrayType;
    static const size_t ArraySize = typename std::tuple_element< 1, ArrayTuple >::type::value;
    typename std::array< stdSortData<ArrayType>, ArraySize > stdValues;
    typename std::array< ArrayType, ArraySize > boltValues;
    typename std::array< ArrayType, ArraySize > boltKeys;
    int m_Errors;
};

TYPED_TEST_CASE_P( SortArrayTest );



TYPED_TEST_P( SortArrayTest, Normal )
{
    typedef std::array< ArrayType, ArraySize > ArrayCont;

    //  Calling the actual functions under test
    std::sort( stdKeys.begin( ), stdKeys.end( ));
    std::sort( stdValues.begin( ), stdValues.end( ));
    bolt::cl::sort_by_key( c_gpu, boltKeys.begin( ), boltKeys.end( ), boltValues.begin( ) );

    ArrayCont::difference_type stdKeyElements = std::distance( stdKeys.begin( ), stdKeys.end() );
    ArrayCont::difference_type boltKeyElements = std::distance( boltKeys.begin( ), boltKeys.end() );
    ArrayCont::difference_type stdValueElements = std::distance( stdValues.begin( ), stdValues.end() );
    ArrayCont::difference_type boltValueElements = std::distance( boltValues.begin( ), boltValues.end() );

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdKeyElements, boltKeyElements );
    EXPECT_EQ( stdValueElements, boltValueElements );

    //  Loop through the array and compare all the values with each other
    cmpStdArray< ArrayType, ArraySize >::cmpArrays( stdKeys, boltKeys );
    cmpStdArray< ArrayType, ArraySize >::cmpArrays( stdValues, boltValues );
}

TYPED_TEST_P( SortArrayTest, GPU_DeviceNormal )
{
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

    typedef std::array< ArrayType, ArraySize > ArrayCont;
    //  Calling the actual functions under test
    std::sort( stdKeys.begin( ), stdKeys.end( ));
    std::sort( stdValues.begin( ), stdValues.end( ));
    bolt::cl::sort_by_key( c_gpu, boltKeys.begin( ), boltKeys.end( ), boltValues.begin( ) );

    ArrayCont::difference_type stdKeyElements = std::distance( stdKeys.begin( ), stdKeys.end() );
    ArrayCont::difference_type boltKeyElements = std::distance( boltKeys.begin( ), boltKeys.end() );
    ArrayCont::difference_type stdValueElements = std::distance( stdValues.begin( ), stdValues.end() );
    ArrayCont::difference_type boltValueElements = std::distance( boltValues.begin( ), boltValues.end() );

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdKeyElements, boltKeyElements );
    EXPECT_EQ( stdValueElements, boltValueElements );

    //  Loop through the array and compare all the values with each other
    cmpStdArray< ArrayType, ArraySize >::cmpArrays( stdKeys, boltKeys );
    cmpStdArray< ArrayType, ArraySize >::cmpArrays( stdValues, boltValues );
}

#if (TEST_CPU_DEVICE == 1)
TYPED_TEST_P( SortArrayTest, CPU_DeviceNormal )
{

    MyOclContext oclcpu = initOcl(CL_DEVICE_TYPE_CPU, 0);
	bolt::cl::control c_cpu(oclcpu._queue);  // construct control structure from the queue.

    typedef std::array< ArrayType, ArraySize > ArrayCont;

    //  Calling the actual functions under test
    std::sort( stdKeys.begin( ), stdKeys.end( ));
    std::sort( stdValues.begin( ), stdValues.end( ));
    bolt::cl::sort_by_key( c_cpu, boltKeys.begin( ), boltKeys.end( ), boltValues.begin( ) );

    ArrayCont::difference_type stdKeyElements = std::distance( stdKeys.begin( ), stdKeys.end() );
    ArrayCont::difference_type boltKeyElements = std::distance( boltKeys.begin( ), boltKeys.end() );
    ArrayCont::difference_type stdValueElements = std::distance( stdValues.begin( ), stdValues.end() );
    ArrayCont::difference_type boltValueElements = std::distance( boltValues.begin( ), boltValues.end() );

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdKeyElements, boltKeyElements );
    EXPECT_EQ( stdValueElements, boltValueElements );

    //  Loop through the array and compare all the values with each other
    cmpStdArray< ArrayType, ArraySize >::cmpArrays( stdKeys, boltKeys );
    cmpStdArray< ArrayType, ArraySize >::cmpArrays( stdValues, boltValues );
}
#endif

TYPED_TEST_P( SortArrayTest, GreaterFunction )
{
    typedef std::array< ArrayType, ArraySize > ArrayCont;

    //  Calling the actual functions under test
    std::sort( stdKeys.begin( ), stdKeys.end( ));
    std::sort( stdValues.begin( ), stdValues.end( ));
    bolt::cl::sort_by_key( boltValues.begin( ), boltKeys.end( ), boltKeys.begin( ), bolt::cl::greater< ArrayType >( ) );

    ArrayCont::difference_type stdKeyElements = std::distance( stdKeys.begin( ), stdKeys.end() );
    ArrayCont::difference_type boltKeyElements = std::distance( boltKeys.begin( ), boltKeys.end() );
    ArrayCont::difference_type stdValueElements = std::distance( stdValues.begin( ), stdValues.end() );
    ArrayCont::difference_type boltValueElements = std::distance( boltValues.begin( ), boltValues.end() );

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdKeyElements, boltKeyElements );
    EXPECT_EQ( stdValueElements, boltValueElements );

    //  Loop through the array and compare all the values with each other
    cmpStdArray< ArrayType, ArraySize >::cmpArrays( stdKeys, boltKeys );
    cmpStdArray< ArrayType, ArraySize >::cmpArrays( stdValues, boltValues );

}

TYPED_TEST_P( SortArrayTest, GPU_DeviceGreaterFunction )
{
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

    typedef std::array< ArrayType, ArraySize > ArrayCont;

    //  Calling the actual functions under test
    std::sort( stdKeys.begin( ), stdKeys.end( ));
    std::sort( stdValues.begin( ), stdValues.end( ));
    bolt::cl::sort_by_key( c_gpu, boltKeys.begin( ), boltKeys.end( ), boltValues.begin( ), bolt::cl::greater< ArrayType >( ) );

    ArrayCont::difference_type stdKeyElements = std::distance( stdKeys.begin( ), stdKeys.end() );
    ArrayCont::difference_type boltKeyElements = std::distance( boltKeys.begin( ), boltKeys.end() );
    ArrayCont::difference_type stdValueElements = std::distance( stdValues.begin( ), stdValues.end() );
    ArrayCont::difference_type boltValueElements = std::distance( boltValues.begin( ), boltValues.end() );

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdKeyElements, boltKeyElements );
    EXPECT_EQ( stdValueElements, boltValueElements );

    //  Loop through the array and compare all the values with each other
    cmpStdArray< ArrayType, ArraySize >::cmpArrays( stdKeys, boltKeys );
    cmpStdArray< ArrayType, ArraySize >::cmpArrays( stdValues, boltValues );
}
#if (TEST_CPU_DEVICE == 1)
TYPED_TEST_P( SortArrayTest, CPU_DeviceGreaterFunction )
{

    MyOclContext oclcpu = initOcl(CL_DEVICE_TYPE_CPU, 0);
	bolt::cl::control c_cpu(oclcpu._queue);  // construct control structure from the queue.

    typedef std::array< ArrayType, ArraySize > ArrayCont;

    //  Calling the actual functions under test
    std::sort( stdKeys.begin( ), stdKeys.end( ));
    std::sort( stdValues.begin( ), stdValues.end( ));
    bolt::cl::sort_by_key( c_cpu, boltKeys.begin( ), boltKeys.end( ), boltValues.begin( ), bolt::cl::greater< ArrayType >( ) );

    ArrayCont::difference_type stdKeyElements = std::distance( stdKeys.begin( ), stdKeys.end() );
    ArrayCont::difference_type boltKeyElements = std::distance( boltKeys.begin( ), boltKeys.end() );
    ArrayCont::difference_type stdValueElements = std::distance( stdValues.begin( ), stdValues.end() );
    ArrayCont::difference_type boltValueElements = std::distance( boltValues.begin( ), boltValues.end() );

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdKeyElements, boltKeyElements );
    EXPECT_EQ( stdValueElements, boltValueElements );

    //  Loop through the array and compare all the values with each other
    cmpStdArray< ArrayType, ArraySize >::cmpArrays( stdKeys, boltKeys );
    cmpStdArray< ArrayType, ArraySize >::cmpArrays( stdValues, boltValues );
}
#endif
TYPED_TEST_P( SortArrayTest, LessFunction )
{
    typedef std::array< ArrayType, ArraySize > ArrayCont;

    //  Calling the actual functions under test
    std::sort( stdKeys.begin( ), stdKeys.end( ));
    std::sort( stdValues.begin( ), stdValues.end( ));
    bolt::cl::sort_by_key( boltKeys.begin( ), boltKeys.end( ), boltValues.begin( ), bolt::cl::less< ArrayType >( ) );

    ArrayCont::difference_type stdKeyElements = std::distance( stdKeys.begin( ), stdKeys.end() );
    ArrayCont::difference_type boltKeyElements = std::distance( boltKeys.begin( ), boltKeys.end() );
    ArrayCont::difference_type stdValueElements = std::distance( stdValues.begin( ), stdValues.end() );
    ArrayCont::difference_type boltValueElements = std::distance( boltValues.begin( ), boltValues.end() );

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdKeyElements, boltKeyElements );
    EXPECT_EQ( stdValueElements, boltValueElements );

    //  Loop through the array and compare all the values with each other
    cmpStdArray< ArrayType, ArraySize >::cmpArrays( stdKeys, boltKeys );
    cmpStdArray< ArrayType, ArraySize >::cmpArrays( stdValues, boltValues );


}

TYPED_TEST_P( SortArrayTest, GPU_DeviceLessFunction )
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
    typedef std::array< ArrayType, ArraySize > ArrayCont;

    //  Calling the actual functions under test
    std::sort( stdKeys.begin( ), stdKeys.end( ));
    std::sort( stdValues.begin( ), stdValues.end( ));
    bolt::cl::sort_by_key( c_gpu, boltKeys.begin( ), boltKeys.end( ), boltValues.begin( ), bolt::cl::less< ArrayType >( ) );

    ArrayCont::difference_type stdKeyElements = std::distance( stdKeys.begin( ), stdKeys.end() );
    ArrayCont::difference_type boltKeyElements = std::distance( boltKeys.begin( ), boltKeys.end() );
    ArrayCont::difference_type stdValueElements = std::distance( stdValues.begin( ), stdValues.end() );
    ArrayCont::difference_type boltValueElements = std::distance( boltValues.begin( ), boltValues.end() );

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdKeyElements, boltKeyElements );
    EXPECT_EQ( stdValueElements, boltValueElements );

    //  Loop through the array and compare all the values with each other
    cmpStdArray< ArrayType, ArraySize >::cmpArrays( stdKeys, boltKeys );
    cmpStdArray< ArrayType, ArraySize >::cmpArrays( stdValues, boltValues );
}
#if (TEST_CPU_DEVICE == 1)
TYPED_TEST_P( SortArrayTest, CPU_DeviceLessFunction )
{
    typedef std::array< ArrayType, ArraySize > ArrayCont;
    MyOclContext oclcpu = initOcl(CL_DEVICE_TYPE_CPU, 0);
	bolt::cl::control c_cpu(oclcpu._queue);  // construct control structure from the queue.

    //  Calling the actual functions under test
    std::sort( stdKeys.begin( ), stdKeys.end( ));
    std::sort( stdValues.begin( ), stdValues.end( ));
    bolt::cl::sort_by_key( c_cpu, boltKeys.begin( ), boltKeys.end( ), boltValues.begin( ), bolt::cl::less< ArrayType >( ) );

    ArrayCont::difference_type stdKeyElements = std::distance( stdKeys.begin( ), stdKeys.end() );
    ArrayCont::difference_type boltKeyElements = std::distance( boltKeys.begin( ), boltKeys.end() );
    ArrayCont::difference_type stdValueElements = std::distance( stdValues.begin( ), stdValues.end() );
    ArrayCont::difference_type boltValueElements = std::distance( boltValues.begin( ), boltValues.end() );

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdKeyElements, boltKeyElements );
    EXPECT_EQ( stdValueElements, boltValueElements );

    //  Loop through the array and compare all the values with each other
    cmpStdArray< ArrayType, ArraySize >::cmpArrays( stdKeys, boltKeys );
    cmpStdArray< ArrayType, ArraySize >::cmpArrays( stdValues, boltValues );
}
#endif

#if (TEST_CPU_DEVICE == 1)
REGISTER_TYPED_TEST_CASE_P( SortArrayTest, Normal, GPU_DeviceNormal,
                                           GreaterFunction, GPU_DeviceGreaterFunction,
                                           LessFunction, GPU_DeviceLessFunction, CPU_DeviceNormal, CPU_DeviceGreaterFunction, CPU_DeviceLessFunction);
#else
REGISTER_TYPED_TEST_CASE_P( SortArrayTest, Normal, GPU_DeviceNormal,
                                           GreaterFunction, GPU_DeviceGreaterFunction,
                                           LessFunction, GPU_DeviceLessFunction );
#endif


/////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Fixture classes are now defined to enable googletest to process value parameterized tests
//  ::testing::TestWithParam< int > means that GetParam( ) returns int values, which i use for array size

class SortIntegerVector: public ::testing::TestWithParam< int >
{
public:

    SortIntegerVector( ): stdValues( GetParam( ) ), boltValues( GetParam( ) ), boltKeys( GetParam( ) )
    {
        for (int i=0;i<GetParam( );i++)
        {
            stdValues[i].key = i+3;
            stdValues[i].value = rand();
            boltValues[i] = stdValues[i].value;
            boltKeys[i] = stdValues[i].key;
        }
    }

protected:
    std::vector< stdSortData<int> > stdValues;
    std::vector< int > boltValues, boltKeys;
};

//  ::testing::TestWithParam< int > means that GetParam( ) returns int values, which i use for array size
class SortFloatVector: public ::testing::TestWithParam< int >
{
public:
    SortFloatVector( ): stdValues( GetParam( ) ), boltValues( GetParam( ) ), boltKeys( GetParam( ) )
    {
        for (int i=0;i<GetParam( );i++)
        {
            stdValues[i].key = i+3;
            stdValues[i].value = (float)rand();
            boltValues[i] = stdValues[i].value;
            boltKeys[i] = stdValues[i].key;
        }
    }

protected:
    std::vector< stdSortData<float> > stdValues;
    std::vector< float > boltValues;
    std::vector< int > boltKeys;
};

#if (TEST_DOUBLE == 1)
//  ::testing::TestWithParam< int > means that GetParam( ) returns int values, which i use for array size
class SortDoubleVector: public ::testing::TestWithParam< int >
{
public:
    SortDoubleVector( ): stdValues( GetParam( ) ), boltValues( GetParam( ) ), boltKeys( GetParam( ) )
    {
        for (int i=0;i<GetParam( );i++)
        {
            stdValues[i].key = i+3;
            stdValues[i].value = (double)rand();
            boltValues[i] = stdValues[i].value;
            boltKeys[i] = stdValues[i].key;
        }
    }

protected:
    std::vector< stdSortData<double> > stdValues;
    std::vector< double > boltValues;
    std::vector< int > boltKeys;
};
#endif

//  ::testing::TestWithParam< int > means that GetParam( ) returns int values, which i use for array size
class SortIntegerDeviceVector: public ::testing::TestWithParam< int >
{
public:
    // Create an std and a bolt vector of requested size, and initialize all the elements to 1
    SortIntegerDeviceVector( ): stdValues( GetParam( ) ), boltValues( static_cast<size_t>( GetParam( ) ) ), boltKeys( static_cast<size_t>( GetParam( ) ) )
    {
        for (int i=0;i<GetParam( );i++)
        {
            stdValues[i].key = i+3;
            stdValues[i].value = rand();
            boltValues[i] = stdValues[i].value;
            boltKeys[i] = stdValues[i].key;
        }
    }

protected:
    std::vector< stdSortData<int> > stdValues;
    bolt::cl::device_vector< int > boltValues, boltKeys;
};

//  ::testing::TestWithParam< int > means that GetParam( ) returns int values, which i use for array size
/*class SortUDDDeviceVector: public ::testing::TestWithParam< int >
{
public:
    // Create an std and a bolt vector of requested size, and initialize all the elements to 1
    SortUDDDeviceVector( ): stdValues( GetParam( ) ), boltValues( static_cast<size_t>( GetParam( ) ) )
    {
        std::generate(stdValues.begin(), stdValues.end(), rand);
        //boltValues = stdValues;
        //FIXME - The above should work but the below loop is used.
        for (int i=0; i< GetParam( ); i++)
        {
            boltValues[i] = stdValues[i];
        }
    }

protected:
    std::vector< UDD > stdValues;
    bolt::cl::device_vector< UDD > boltValues;
};*/

//  ::testing::TestWithParam< int > means that GetParam( ) returns int values, which i use for array size
class SortFloatDeviceVector: public ::testing::TestWithParam< int >
{
public:
    // Create an std and a bolt vector of requested size, and initialize all the elements to 1
    SortFloatDeviceVector( ): stdValues( GetParam( ) ), boltValues( static_cast<size_t>( GetParam( ) ) ), boltKeys( static_cast<size_t>( GetParam( ) ) )
    {
        for (int i=0;i<GetParam( );i++)
        {
            stdValues[i].key = i+3;
            stdValues[i].value = (float)rand();
            boltValues[i] = stdValues[i].value;
            boltKeys[i] = stdValues[i].key;
        }
    }

protected:
    std::vector< stdSortData<float> > stdValues;
    bolt::cl::device_vector< int > boltKeys;
    bolt::cl::device_vector< float > boltValues;
};

#if (TEST_DOUBLE == 1)
//  ::testing::TestWithParam< int > means that GetParam( ) returns int values, which i use for array size
class SortDoubleDeviceVector: public ::testing::TestWithParam< int >
{
public:
    // Create an std and a bolt vector of requested size, and initialize all the elements to 1
    SortDoubleDeviceVector( ): stdValues( GetParam( ) ), boltValues( static_cast<size_t>( GetParam( ) ) ), stdKeys( GetParam( ) ), boltKeys( static_cast<size_t>( GetParam( ) ) )
    {
        for (int i=0;i<GetParam( );i++)
        {
            stdValues[i].key = i+3;
            stdValues[i].value = (double)rand();
            boltValues[i] = stdValues[i].value;
            boltKeys[i] = stdValues[i].key;
        }
    }

protected:
    std::vector< stdSortData<double> > stdValues;
    bolt::cl::device_vector< int > boltKeys;
    bolt::cl::device_vector< double > boltValues;
};
#endif

//  ::testing::TestWithParam< int > means that GetParam( ) returns int values, which i use for array size
class SortIntegerNakedPointer: public ::testing::TestWithParam< int >
{
public:
    //  Create an std and a bolt vector of requested size, and initialize all the elements to 1
    SortIntegerNakedPointer( ): stdValues( new stdSortData<int>[ GetParam( ) ] ), boltValues( new int[ GetParam( ) ] ), boltKeys( new int[ GetParam( ) ] )
    {}

    virtual void SetUp( )
    {
        for (int i=0;i<GetParam( );i++)
        {
            stdValues[i].key = i+3;
            stdValues[i].value = rand();
            boltValues[i] = stdValues[i].value;
            boltKeys[i] = stdValues[i].key;
        }
    };

    virtual void TearDown( )
    {
        delete [] stdValues;
        delete [] boltValues;
        delete [] boltKeys;
    };

protected:
     stdSortData<int> *stdValues;
     int *boltValues, *boltKeys;
};

//  ::testing::TestWithParam< int > means that GetParam( ) returns int values, which i use for array size
class SortFloatNakedPointer: public ::testing::TestWithParam< int >
{
public:
    //  Create an std and a bolt vector of requested size, and initialize all the elements to 1
    SortFloatNakedPointer( ): stdValues( new stdSortData<float>[ GetParam( ) ] ), boltValues( new float[ GetParam( ) ] ), boltKeys( new int[ GetParam( ) ] )
    {}

    virtual void SetUp( )
    {
        for (int i=0;i<GetParam( );i++)
        {
            stdValues[i].key = i+3;
            stdValues[i].value = (float)rand();
            boltValues[i] = stdValues[i].value;
            boltKeys[i] = stdValues[i].key;
        }

    };

    virtual void TearDown( )
    {
        delete [] stdValues;
        delete [] boltValues;
        delete [] boltKeys;
    };

protected:
     stdSortData<float>* stdValues;
     float* boltValues;
     int   *boltKeys;
};

#if (TEST_DOUBLE ==1 )
//  ::testing::TestWithParam< int > means that GetParam( ) returns int values, which i use for array size
class SortDoubleNakedPointer: public ::testing::TestWithParam< int >
{
public:
    //  Create an std and a bolt vector of requested size, and initialize all the elements to 1
    SortDoubleNakedPointer( ): stdValues( new stdSortData<double>[ GetParam( ) ] ), boltValues( new double[ GetParam( ) ] ),  stdKeys( new double[ GetParam( ) ] ), boltKeys( new int[ GetParam( ) ] )
    {}

    virtual void SetUp( )
    {
        for (int i=0;i<GetParam( );i++)
        {
            stdValues[i].key = i+3;
            stdValues[i].value = (double)rand();
            boltValues[i] = stdValues[i].value;
            boltKeys[i] = stdValues[i].key;
        }
    };

    virtual void TearDown( )
    {
        delete [] stdValues;
        delete [] boltValues;
        delete [] boltKeys;
    };

protected:
     stdSortData<double>* stdValues;
     double* boltValues;
     int   *boltKeys;
};
#endif

TEST_P( SortIntegerVector, Normal )
{
    //  Calling the actual functions under test
    std::sort( stdValues.begin( ), stdValues.end( ));
    bolt::cl::sort_by_key( boltKeys.begin( ), boltKeys.end( ), boltValues.begin( ));

    std::vector< stdSortData<int> >::iterator::difference_type stdValueElements = std::distance( stdValues.begin( ), stdValues.end() );
    std::vector< int >::iterator::difference_type boltValueElements = std::distance( boltValues.begin( ), boltValues.end() );

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdValueElements, boltValueElements );

    //  Loop through the array and compare all the values with each other
    cmpArraysSortByKey( stdValues, boltKeys, boltValues );
}

// Come Back here
TEST_P( SortFloatVector, Normal )
{
    //  Calling the actual functions under test
    std::sort( stdValues.begin( ), stdValues.end( ));
    bolt::cl::sort_by_key( boltKeys.begin( ), boltKeys.end( ), boltValues.begin( ));

    std::vector< stdSortData<float> >::iterator::difference_type stdValueElements = std::distance( stdValues.begin( ), stdValues.end() );
    std::vector< float >::iterator::difference_type boltValueElements = std::distance( boltValues.begin( ), boltValues.end() );

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdValueElements, boltValueElements );

    //  Loop through the array and compare all the values with each other
    cmpArraysSortByKey( stdValues, boltKeys, boltValues );
}
#if (TEST_DOUBLE == 1)
TEST_P( SortDoubleVector, Normal )
{
    //  Calling the actual functions under test
    std::sort( stdValues.begin( ), stdValues.end( ));
    bolt::cl::sort_by_key( boltKeys.begin( ), boltKeys.end( ), boltValues.begin( ));

    std::vector< stdSortData<double> >::iterator::difference_type stdValueElements = std::distance( stdValues.begin( ), stdValues.end() );
    std::vector< double >::iterator::difference_type boltValueElements = std::distance( boltValues.begin( ), boltValues.end() );

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdValueElements, boltValueElements );

    //  Loop through the array and compare all the values with each other
    cmpArraysSortByKey( stdValues, boltKeys, boltValues );
}
#endif
#if (TEST_DEVICE_VECTOR == 1)
TEST_P( SortIntegerDeviceVector, Inplace )
{
    //  Calling the actual functions under test
    std::sort( stdValues.begin( ), stdValues.end( ));
    bolt::cl::sort_by_key( boltKeys.begin( ), boltKeys.end( ), boltValues.begin( ));

    std::vector< int >::iterator::difference_type stdValueElements = std::distance( stdValues.begin( ), stdValues.end() );
    std::vector< int >::iterator::difference_type boltValueElements = std::distance( boltValues.begin( ), boltValues.end() );

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdValueElements, boltValueElements );

    //  Loop through the array and compare all the values with each other
    cmpArraysSortByKey( stdValues, boltKeys, boltValues );
}
TEST_P( SortFloatDeviceVector, Inplace )
{
    //  Calling the actual functions under test
    std::sort( stdValues.begin( ), stdValues.end( ));
    bolt::cl::sort_by_key( boltKeys.begin( ), boltKeys.end( ), boltValues.begin( ));

    std::vector< float >::iterator::difference_type stdValueElements = std::distance( stdValues.begin( ), stdValues.end() );
    std::vector< float >::iterator::difference_type boltValueElements = std::distance( boltValues.begin( ), boltValues.end() );

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdValueElements, boltValueElements );

    //  Loop through the array and compare all the values with each other
    cmpArraysSortByKey( stdValues, boltKeys, boltValues );
}
#if (TEST_DOUBLE == 1)
TEST_P( SortDoubleDeviceVector, Inplace )
{
    //  Calling the actual functions under test
    std::sort( stdValues.begin( ), stdValues.end( ));
    bolt::cl::sort_by_key( boltKeys.begin( ), boltKeys.end( ), boltValues.begin( ));

    std::vector< double >::iterator::difference_type stdValueElements = std::distance( stdValues.begin( ), stdValues.end() );
    std::vector< double >::iterator::difference_type boltValueElements = std::distance( boltValues.begin( ), boltValues.end() );

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdValueElements, boltValueElements );

    //  Loop through the array and compare all the values with each other
    cmpArraysSortByKey( stdValues, boltKeys, boltValues );
}
#endif
#endif

TEST_P( SortIntegerNakedPointer, Inplace )
{
    size_t endIndex = GetParam( );

    //  Calling the actual functions under test
    stdext::checked_array_iterator< stdSortData<int>* > wrapstdValues( stdValues, endIndex );
    std::sort( wrapstdValues, wrapstdValues + endIndex );
    stdext::checked_array_iterator< int* > wrapboltValues( boltValues, endIndex );
    stdext::checked_array_iterator< int* > wrapboltKeys( boltKeys, endIndex );
    bolt::cl::sort_by_key( wrapboltKeys, wrapboltKeys + endIndex , wrapboltValues);

    //TODO - fix this testcase
    //Loop through the array and compare all the values with each other
    //cmpArraysSortByKey( wrapstdValues, wrapboltKeys, wrapboltValues );
}

TEST_P( SortFloatNakedPointer, Inplace )
{
    size_t endIndex = GetParam( );

    //  Calling the actual functions under test
    stdext::checked_array_iterator< stdSortData<float>* > wrapstdValues( stdValues, endIndex );
    std::sort( wrapstdValues, wrapstdValues + endIndex );
    stdext::checked_array_iterator< float* > wrapboltValues( boltValues, endIndex );
    stdext::checked_array_iterator< int* > wrapboltKeys( boltKeys, endIndex );
    bolt::cl::sort_by_key( wrapboltKeys, wrapboltKeys + endIndex , wrapboltValues);

    //TODO - fix this testcase
    //Loop through the array and compare all the values with each other
    //cmpArraysSortByKey( wrapstdValues, wrapboltKeys, wrapboltValues );
}

#if (TEST_DOUBLE == 1)
TEST_P( SortDoubleNakedPointer, Inplace )
{
    size_t endIndex = GetParam( );
    //  Calling the actual functions under test
    stdext::checked_array_iterator< double* > wrapstdValues( stdValues, endIndex );
    stdext::checked_array_iterator< double* > wrapstdKeys( stdValues, endIndex );
    std::sort( wrapstdValues, wrapstdValues + endIndex );
    std::sort( wrapstdKeys, wrapstdKeys + endIndex );

    stdext::checked_array_iterator< double* > wrapboltValues( boltValues, endIndex );
    stdext::checked_array_iterator< double* > wrapboltKeys( boltKeys, endIndex );
    bolt::cl::sort_by_key( wrapboltKeys, wrapboltKeys + endIndex , wrapboltValues);

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdValues, boltValues, endIndex );
    cmpArrays( stdKeys, boltKeys, endIndex );
}
#endif
std::array<int, 15> TestValues = {2,4,8,16,32,64,128,256,512,1024,2048,4096,8192,16384,32768};

//Test lots of consecutive numbers, but small range, suitable for integers because they overflow easier
//INSTANTIATE_TEST_CASE_P( SortRange, SortIntegerVector, ::testing::Range( 0, 1024, 7 ) );
INSTANTIATE_TEST_CASE_P( SortValues, SortIntegerVector, ::testing::ValuesIn( TestValues.begin(), TestValues.end() ) );
//INSTANTIATE_TEST_CASE_P( SortRange, SortFloatVector, ::testing::Range( 0, 1024, 3 ) );
INSTANTIATE_TEST_CASE_P( SortValues, SortFloatVector, ::testing::ValuesIn( TestValues.begin(), TestValues.end() ) );
#if (TEST_DOUBLE == 1)
//INSTANTIATE_TEST_CASE_P( SortRange, SortDoubleVector, ::testing::Range( 0, 1024, 21 ) );
INSTANTIATE_TEST_CASE_P( SortValues, SortDoubleVector, ::testing::ValuesIn( TestValues.begin(), TestValues.end() ) );
#endif
//INSTANTIATE_TEST_CASE_P( SortRange, SortIntegerDeviceVector, ::testing::Range( 0, 1024, 53 ) );
INSTANTIATE_TEST_CASE_P( SortValues, SortIntegerDeviceVector, ::testing::ValuesIn( TestValues.begin(), TestValues.end() ) );
//INSTANTIATE_TEST_CASE_P( SortRange, SortFloatDeviceVector, ::testing::Range( 0, 1024, 53 ) );
INSTANTIATE_TEST_CASE_P( SortValues, SortFloatDeviceVector, ::testing::ValuesIn( TestValues.begin(), TestValues.end() ) );
#if (TEST_DOUBLE == 1)
INSTANTIATE_TEST_CASE_P( SortRange, SortDoubleDeviceVector, ::testing::Range( 0, 1024, 53 ) );
INSTANTIATE_TEST_CASE_P( SortValues, SortDoubleDeviceVector, ::testing::ValuesIn( TestValues.begin(), TestValues.end() ) );
#endif
//INSTANTIATE_TEST_CASE_P( SortRange, SortIntegerNakedPointer, ::testing::Range( 0, 1024, 13) );
INSTANTIATE_TEST_CASE_P( SortValues, SortIntegerNakedPointer, ::testing::ValuesIn( TestValues.begin(), TestValues.end() ) );
//INSTANTIATE_TEST_CASE_P( SortRange, SortFloatNakedPointer, ::testing::Range( 0, 1024, 13) );
INSTANTIATE_TEST_CASE_P( SortValues, SortFloatNakedPointer, ::testing::ValuesIn( TestValues.begin(), TestValues.end() ) );
#if (TEST_DOUBLE == 1)
//INSTANTIATE_TEST_CASE_P( SortRange, SortDoubleNakedPointer, ::testing::Range( 0, 1024, 13) );
INSTANTIATE_TEST_CASE_P( Sort, SortDoubleNakedPointer, ::testing::ValuesIn( TestValues.begin(), TestValues.end() ) );
#endif
/*
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

INSTANTIATE_TYPED_TEST_CASE_P( Integer, SortArrayTest, IntegerTests );
INSTANTIATE_TYPED_TEST_CASE_P( Float, SortArrayTest, FloatTests );
#if (TEST_DOUBLE == 1)
INSTANTIATE_TYPED_TEST_CASE_P( Double, SortArrayTest, DoubleTests );
#endif
INSTANTIATE_TYPED_TEST_CASE_P( UDDTest, SortArrayTest, UDDTests );
*/
int main(int argc, char* argv[])
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
    std::cout << "Test Completed. Press Enter to exit.\n .... ";
    //getchar();
	return retVal;
}
#else
//BOLT Header files
#include <bolt/cl/clcode.h>
#include <bolt/cl/device_vector.h>
#include <bolt/cl/sort.h>
#include <bolt/cl/functional.h>
#include <bolt/cl/control.h>


//STD Header files
#include <iostream>
#include <algorithm>  // for testing against STL functions.
#include <vector>
#include "common/myocl.h"

// A Data structure defining a less than operator

template <typename T>
struct MyType {
    T a;

    bool operator() (const MyType& lhs, const MyType& rhs) {
        return (lhs.a > rhs.a);
    }
    bool operator < (const MyType& other) const {
        return (a < other.a);
    }
    bool operator > (const MyType& other) const {
        return (a > other.a);
    }
    MyType(const MyType &other)
        : a(other.a) { }
    MyType()
        : a(0) { }
    MyType(T& _in)
        : a(_in) { }
};


BOLT_CREATE_TYPENAME(MyType<int>);
BOLT_CREATE_CLCODE(MyType<int>, "template <typename T> struct MyType { T a; \n\nbool operator() (const MyType& lhs, const MyType& rhs) { return (lhs.a > rhs.a); } \n\nbool operator < (const MyType& other) const { return (a < other.a); }\n\n bool operator > (const MyType& other) const { return (a > other.a);} \n\n };");
BOLT_CREATE_TYPENAME(MyType<float>);
BOLT_CREATE_CLCODE(MyType<float>, "template <typename T> struct MyType { T a; \n\nbool operator() (const MyType& lhs, const MyType& rhs) { return (lhs.a > rhs.a); } \n\nbool operator < (const MyType& other) const { return (a < other.a); } \n\n bool operator > (const MyType& other) const { return (a > other.a);} \n\n };");
BOLT_CREATE_TYPENAME(MyType<double>);
BOLT_CREATE_CLCODE(MyType<double>, "template <typename T> struct MyType { T a; \n\nbool operator() (const MyType& lhs, const MyType& rhs) { return (lhs.a > rhs.a); } \n\nbool operator < (const MyType& other) const { return (a < other.a); }\n\n bool operator > (const MyType& other) const { return (a > other.a);} \n\n };");

BOLT_CREATE_TYPENAME(bolt::cl::less< MyType<int> >);
// A Data structure defining a Functor
template <typename T>
struct MyFunctor{
    T a;
    T b;

    bool operator() (const MyFunctor& lhs, const MyFunctor& rhs) {
        return (lhs.a > rhs.a);
    }
    bool operator < (const MyFunctor& other) const {
        return (a < other.a);
    }
    bool operator > (const MyFunctor& other) const {
        return (a > other.a);
    }
    MyFunctor(const MyFunctor &other)
        : a(other.a), b(other.b) { }
    MyFunctor()
        : a(0), b(0) { }
    MyFunctor(T& _in)
        : a(_in), b(_in) { }
};
BOLT_CREATE_TYPENAME(MyFunctor<int>);
BOLT_CREATE_CLCODE(MyFunctor<int>, "template<typename T> struct MyFunctor { T a; T b; \n\nbool operator() (const MyFunctor& lhs, const MyFunctor& rhs) { return (lhs.a > rhs.a); }   \n\nbool operator < (const MyFunctor& other) const { return (a < other.a); }   \n\nbool operator > (const MyFunctor& other) const { return (a > other.a);}  \n\n}; \n\n");
BOLT_CREATE_TYPENAME(MyFunctor<float>);
BOLT_CREATE_CLCODE(MyFunctor<float>, "template<typename T> struct MyFunctor { T a; T b; \n\nbool operator() (const MyFunctor& lhs, const MyFunctor& rhs) { return (lhs.a > rhs.a); }   \n\nbool operator < (const MyFunctor& other) const { return (a < other.a); }   \n\nbool operator > (const MyFunctor& other) const { return (a > other.a);}  \n\n}; \n\n");
BOLT_CREATE_TYPENAME(MyFunctor<double>);
BOLT_CREATE_CLCODE(MyFunctor<double>, "template<typename T> struct MyFunctor { T a; T b; \n\nbool operator() (const MyFunctor& lhs, const MyFunctor& rhs) { return (lhs.a > rhs.a); }   \n\nbool operator < (const MyFunctor& other) const { return (a < other.a); }   \n\nbool operator > (const MyFunctor& other) const { return (a > other.a);}  \n\n}; \n\n");

template <typename T>
bool FUNCTION (T &i,T &j) { return (i<j); }

template <typename stdType>
void UserDefinedLambdaSortTestOfLength(size_t length)
{
    std::vector<stdType> stdValues(length);
    std::vector<stdType> boltValues(length);
    std::vector<stdType>::iterator it;
    auto func = [](const stdType & a, const stdType & b) {  return a < b;  };

    size_t i;
    for (i=0;i<length;i++)
    {
        boltValues[i]= (stdType)(length - i +2);
        stdValues[i]= (stdType)(length - i +2);
    }

    bolt::cl::sort_by_key(boltValues.begin(), boltValues.end(), func," [](const stdType & a, const stdType & b) {  return a < b;  };");
    std::sort(stdValues.begin(), stdValues.end(),func);
    for (i=0; i<length; i++)
    {
        if(stdValues[i] == boltValues[i])
            continue;
        else
            break;
    }
    if (i==length)
        std::cout << "Test Passed" <<std::endl;
    else
        std::cout << "Test Failed i = " << i <<std::endl;
}

#if 0
template <typename stdType>
void UserDefinedFunctionSortTestOfLength(size_t length)
{
    std::vector<stdType> stdValues(length);
    std::vector<stdType> boltValues(length);

    typedef bool (*MyFunction)(stdType &i,stdType &j);
    size_t i;
    for (i=0;i<length;i++)
    {
        boltValues[i] = (stdType)(length - i +2);
        stdValues[i]  = (stdType)(length - i +2);
    }
    MyFunction function = FUNCTION<stdType>;
    std::string functionString("bool FUNCTION(" + std::string(typeid(stdType).name()) + " in1, " + std::string(typeid(stdType).name()) + " in2) { return (in1 < in2); }");
    bolt::cl::sort_by_key(boltValues.begin(), boltValues.end(), functionString);
    std::sort(stdValues.begin(), stdValues.end(), function);
    for (i=0; i<length; i++)
    {
        if(stdValues[i] == boltValues[i])
            continue;
        else
            break;
    }
    if (i==length)
        std::cout << "Test Passed" <<std::endl;
    else
        std::cout << "Test Failed i = " << i <<std::endl;
}
#endif

BOLT_CREATE_TYPENAME(bolt::cl::greater< MyType<int> >);
BOLT_CREATE_TYPENAME(bolt::cl::greater< MyType<float> >);
BOLT_CREATE_TYPENAME(bolt::cl::greater< MyType<double> >);
BOLT_CREATE_TYPENAME(bolt::cl::greater< MyFunctor<int> >);
BOLT_CREATE_TYPENAME(bolt::cl::greater< MyFunctor<float> >);
BOLT_CREATE_TYPENAME(bolt::cl::greater< MyFunctor<double> >);

template <typename stdType>
void UserDefinedBoltFunctorSortTestOfLength(size_t length)
{
    typedef MyFunctor<stdType> myfunctor;

    std::vector<myfunctor> stdValues(length);
    std::vector<myfunctor> boltValues(length);

    size_t i;
    for (i=0;i<length;i++)
    {
        boltValues[i].a = (stdType)(i +2);
        stdValues[i].a  = (stdType)(i +2);
    }

    bolt::cl::sort_by_key(boltValues.begin(), boltValues.end(),bolt::cl::greater<myfunctor>());
    std::sort(stdValues.begin(), stdValues.end(),bolt::cl::greater<myfunctor>());

    for (i=0; i<length; i++)
    {
        if (stdValues[i].a == boltValues[i].a)
            continue;
        else
            break;
    }
    if (i==length)
        std::cout << "Test Passed" <<std::endl;
    else
        std::cout << "Test Failed i = " << i <<std::endl;
}

template <typename stdType>
void UserDefinedFunctorSortTestOfLength(size_t length)
{
    typedef MyFunctor<stdType> myfunctor;

    std::vector<myfunctor> stdValues(length);
    std::vector<myfunctor> boltValues(length);

    size_t i;
    for (i=0;i<length;i++)
    {
        boltValues[i].a = (stdType)(i +2);
        stdValues[i].a  = (stdType)(i +2);
    }

    bolt::cl::sort_by_key(boltValues.begin(), boltValues.end(),myfunctor());
    std::sort(stdValues.begin(), stdValues.end(),myfunctor());

    for (i=0; i<length; i++)
    {
        if (stdValues[i].a == boltValues[i].a)
            continue;
        else
            break;
    }
    if (i==length)
        std::cout << "Test Passed" <<std::endl;
    else
        std::cout << "Test Failed i = " << i <<std::endl;
}

template <typename stdType>
void UserDefinedObjectSortTestOfLength(size_t length)
{
    typedef MyType<stdType> mytype;

    std::vector<mytype> stdValues(length);
    std::vector<mytype> boltValues(length);

    size_t i;
    for (i=0;i<length;i++)
    {
        boltValues[i].a = (stdType)(i +2);
        stdValues[i].a  = (stdType)(i +2);
    }

    bolt::cl::sort_by_key(boltValues.begin(), boltValues.end(),bolt::cl::greater<mytype>());
    std::sort(stdValues.begin(), stdValues.end(),bolt::cl::greater<mytype>());
    for (i=0; i<length; i++)
    {
        if(stdValues[i].a == boltValues[i].a)
            continue;
        else
            break;
    }
    if (i==length)
        std::cout << "Test Passed" <<std::endl;
    else
        std::cout << "Test Failed i = " << i <<std::endl;
}

template <typename T>
void BasicSortTestOfLength(size_t length)
{
    std::vector<T> stdValues(length);
    std::vector<T> boltValues(length);

    size_t i;
    for (i=0;i<length/2;i++)
    {
        boltValues[2*i]= (T)(length - i +2);
        boltValues[2*i + 1] = boltValues[2*i];
        stdValues[2*i]= (T)(length - i +2);
        stdValues[2*i + 1] = stdValues[2*i];
    }

    bolt::cl::sort_by_key(boltValues.begin(), boltValues.end());
    std::sort(stdValues.begin(), stdValues.end());
    for (i=0; i<length; i++)
    {
        std::cout << i << " : " << stdValues[i] << " , " << boltValues[i] << std::endl;
    }
    for (i=0; i<length; i++)
    {
        if(stdValues[i] == boltValues[i])
            continue;
        else
            break;
    }
    if (i==length)
        std::cout << "Test Passed" <<std::endl;
    else
        std::cout << "Test Failed i = " << i <<std::endl;
}

template <typename T>
void BasicSortTestOfLengthWithDeviceVector(size_t length)
{
    std::vector<T> stdValues(length);
    bolt::cl::device_vector<T> boltValues(length);

    size_t i;
    for (i=0;i<length;i++)
    {
        boltValues[i]= (T)(length - i +2);
        stdValues[i]= (T)(length - i +2);
    }

    bolt::cl::sort_by_key(boltValues.begin(), boltValues.end());
    std::sort(stdValues.begin(), stdValues.end());
    for (i=0; i<length; i++)
    {
        if(stdValues[i] == boltValues[i])
            continue;
        else
            break;
    }
    if (i==length)
        std::cout << "Test Passed" <<std::endl;
    else
        std::cout << "Test Failed i = " << i <<std::endl;
}

template <typename T>
void BasicSortTestWithBoltFunctorOfLengthWithDeviceVector(size_t length)
{
    std::vector<T> stdValues(length);
    bolt::cl::device_vector<T> boltValues(length);

    size_t i;
    for (i=0;i<length;i++)
    {
        boltValues[i]= (T)(i +2);
        stdValues[i]= (T)(i +2);
    }

    bolt::cl::sort_by_key(boltValues.begin(), boltValues.end(), bolt::cl::greater<T>());
    std::sort(stdValues.begin(), stdValues.end(), bolt::cl::greater<T>());
    for (i=0; i<length; i++)
    {
        if(stdValues[i] == boltValues[i])
            continue;
        else
            break;
    }
    if (i==length)
        std::cout << "Test Passed" <<std::endl;
    else
        std::cout << "Test Failed i = " << i <<std::endl;
}


template <typename stdType>
void UDDSortTestOfLengthWithDeviceVector(size_t length)
{
    typedef MyType<stdType> mytype;
    std::vector<mytype> stdValues(length);
    bolt::cl::device_vector<mytype> boltValues(length);

    size_t i;
    for (i=0;i<length;i++)
    {
        //FIX ME - This should work
        //boltValues[i].a = (stdType)(length - i +2);
        stdValues[i].a  = (stdType)(length - i +2);
    }

    bolt::cl::sort_by_key(boltValues.begin(), boltValues.end());
    std::sort(stdValues.begin(), stdValues.end());
    for (i=0; i<length; i++)
    {
         //FIX ME - stdValues should be changed to boltValues
        if(stdValues[i].a == stdValues[i].a)
            continue;
        else
            break;
    }
    if (i==length)
        std::cout << "Test Passed" <<std::endl;
    else
        std::cout << "Test Failed i = " << i <<std::endl;
}

template <typename stdType>
void UDDSortTestWithBoltFunctorOfLengthWithDeviceVector(size_t length)
{
    typedef MyType<stdType> mytype;
    std::vector<mytype> stdValues(length);
    bolt::cl::device_vector<mytype> boltValues(length);

    size_t i;
    for (i=0;i<length;i++)
    {
        //FIX ME - This should work
        //boltValues[i].a = (stdType)(length - i +2);
        stdValues[i].a  = (stdType)(length - i +2);
    }

    bolt::cl::sort_by_key(boltValues.begin(), boltValues.end(),bolt::cl::less<mytype>());
    std::sort(stdValues.begin(), stdValues.end(),bolt::cl::less<mytype>());
    for (i=0; i<length; i++)
    {
         //FIX ME - stdValues should be changed to boltValues
        if(stdValues[i].a == stdValues[i].a)
            continue;
        else
            break;
    }
    if (i==length)
        std::cout << "Test Passed" <<std::endl;
    else
        std::cout << "Test Failed i = " << i <<std::endl;
}

void TestWithBoltControl(int length)
{

	MyOclContext ocl = initOcl(CL_DEVICE_TYPE_CPU, 0);
	bolt::cl::control c(ocl._queue);  // construct control structure from the queue.
    //c.debug(bolt::cl::control::debug::Compile + bolt::cl::control::debug::SaveCompilerTemps);
    typedef MyFunctor<int> myfunctor;
    typedef MyType<int> mytype;

    std::vector<int> boltValues(length);
    std::vector<int> stdValues(length);
    std::vector<myfunctor> myFunctorboltValues(length);
    std::vector<mytype> myTypeboltValues(length);
    //bolt::cl::device_vector<int> dvInput(c,length);
    size_t i;

    for (i=0;i<length;i++)
    {
        boltValues[i]= (int)(length - i +2);
        stdValues[i]= (int)(length - i +2);
    }
    for (i=0;i<length;i++)
    {
        myFunctorboltValues[i].a= (int)(length - i +2);
        myTypeboltValues[i].a= (int)(length - i +2);
    }

    //BasicSortTestOfLengthWithDeviceVector
    bolt::cl::device_vector<int> dvInput( boltValues.begin(), boltValues.end(), CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE, c);
    bolt::cl::sort_by_key(c, dvInput.begin(), dvInput.end());

    std::sort(stdValues.begin(),stdValues.end());
    for (i=0;i<length;i++)
    {
        if(dvInput[i] == stdValues[i])
        {
            continue;
        }
        else
            break;
    }
    if(i==length)
        std::cout << "Test Passed\n";
    else
        std::cout << "Test Failed. i = "<< i << std::endl;
    //Device Vector with greater functor
    bolt::cl::sort_by_key(c, dvInput.begin(), dvInput.end(),bolt::cl::greater<int>());
    //UserDefinedBoltFunctorSortTestOfLength
    bolt::cl::sort_by_key(c, myFunctorboltValues.begin(), myFunctorboltValues.end(),bolt::cl::greater<myfunctor>());
    //UserDefinedBoltFunctorSortTestOfLength
    bolt::cl::sort_by_key(c, myTypeboltValues.begin(), myTypeboltValues.end(),bolt::cl::greater<mytype>());
    return;
}

int main(int argc, char* argv[])
{

    //UDDSortTestOfLengthWithDeviceVector<int>(256);
    BasicSortTestOfLength<int>(68);
    //BasicSortTestOfLength<int>(111);
    //BasicSortTestOfLength<int>(65);
    //BasicSortTestOfLength<int>(63);
    //BasicSortTestOfLength<int>(31);
#if 0
		std::vector<int> input(1024);
		std::generate(input.begin(), input.end(), rand);
		bolt::cl::sort_by_key( input.begin(), input.end(), bolt::cl::greater<int>());

	int a[10] = {2, 9, 3, 7, 5, 6, 3, 8, 3, 4};
	bolt::cl::sort_by_key( a, a+10, bolt::cl::greater<int>());
    //Test the non Power Of 2 Buffer size
	//The following two commented codes does not run. It will throw and cl::Error exception
    //BasicSortTestOfLengthWithDeviceVector<int>(254);
    //BasicSortTestWithBoltFunctorOfLengthWithDeviceVector<int>(254);
    UserDefinedFunctorSortTestOfLength<int>(254);
    UserDefinedBoltFunctorSortTestOfLength<int>(254);
    UserDefinedObjectSortTestOfLength<int>(254);
    BasicSortTestOfLength<int>(254);
    TestWithBoltControl();
#endif
    //The following two are not working because device_vector support is not there.
    //UDDSortTestOfLengthWithDeviceVector<int>(256);
    //UDDSortTestWithBoltFunctorOfLengthWithDeviceVector<int>(256);

#define TEST_ALL 0
#define TEST_DOUBLE 0

#if (TEST_ALL == 1)
    std::cout << "Testing BasicSortTestWithBoltFunctorOfLengthWithDeviceVector\n";
    BasicSortTestWithBoltFunctorOfLengthWithDeviceVector<int>(8);
    BasicSortTestWithBoltFunctorOfLengthWithDeviceVector<int>(16);
    BasicSortTestWithBoltFunctorOfLengthWithDeviceVector<int>(32);
    BasicSortTestWithBoltFunctorOfLengthWithDeviceVector<int>(64);
    BasicSortTestWithBoltFunctorOfLengthWithDeviceVector<int>(128);
    BasicSortTestWithBoltFunctorOfLengthWithDeviceVector<int>(256);
    BasicSortTestWithBoltFunctorOfLengthWithDeviceVector<int>(512);
    BasicSortTestWithBoltFunctorOfLengthWithDeviceVector<int>(1024);
    BasicSortTestWithBoltFunctorOfLengthWithDeviceVector<int>(2048);
    //BasicSortTestWithBoltFunctorOfLengthWithDeviceVector<int>(1048576);
    BasicSortTestWithBoltFunctorOfLengthWithDeviceVector<float>(8);
    BasicSortTestWithBoltFunctorOfLengthWithDeviceVector<float>(16);
    BasicSortTestWithBoltFunctorOfLengthWithDeviceVector<float>(32);
    BasicSortTestWithBoltFunctorOfLengthWithDeviceVector<float>(64);
    BasicSortTestWithBoltFunctorOfLengthWithDeviceVector<float>(128);
    BasicSortTestWithBoltFunctorOfLengthWithDeviceVector<float>(256);
    BasicSortTestWithBoltFunctorOfLengthWithDeviceVector<float>(512);
    BasicSortTestWithBoltFunctorOfLengthWithDeviceVector<float>(1024);
    BasicSortTestWithBoltFunctorOfLengthWithDeviceVector<float>(2048);
    //BasicSortTestWithBoltFunctorOfLengthWithDeviceVector<float>(1048576);
#if (TEST_DOUBLE == 1)
    BasicSortTestWithBoltFunctorOfLengthWithDeviceVector<double>(8);
    BasicSortTestWithBoltFunctorOfLengthWithDeviceVector<double>(16);
    BasicSortTestWithBoltFunctorOfLengthWithDeviceVector<double>(32);
    BasicSortTestWithBoltFunctorOfLengthWithDeviceVector<double>(64);
    BasicSortTestWithBoltFunctorOfLengthWithDeviceVector<double>(128);
    BasicSortTestWithBoltFunctorOfLengthWithDeviceVector<double>(256);
    BasicSortTestWithBoltFunctorOfLengthWithDeviceVector<double>(512);
    BasicSortTestWithBoltFunctorOfLengthWithDeviceVector<double>(1024);
    BasicSortTestWithBoltFunctorOfLengthWithDeviceVector<double>(2048);
    //BasicSortTestWithBoltFunctorOfLengthWithDeviceVector<double>(1048576);
#endif
#endif


#if (TEST_ALL == 1)
//#if 0
    std::cout << "Testing BasicSortTestOfLengthWithDeviceVector\n";
    BasicSortTestOfLengthWithDeviceVector<int>(8);
    BasicSortTestOfLengthWithDeviceVector<int>(16);
    BasicSortTestOfLengthWithDeviceVector<int>(32);
    BasicSortTestOfLengthWithDeviceVector<int>(64);
	BasicSortTestOfLengthWithDeviceVector<int>(128);
    BasicSortTestOfLengthWithDeviceVector<int>(256);
    BasicSortTestOfLengthWithDeviceVector<int>(512);
    BasicSortTestOfLengthWithDeviceVector<int>(1024);
    BasicSortTestOfLengthWithDeviceVector<int>(2048);
    //BasicSortTestOfLengthWithDeviceVector<int>(1048576);
    BasicSortTestOfLengthWithDeviceVector<float>(8);
    BasicSortTestOfLengthWithDeviceVector<float>(16);
    BasicSortTestOfLengthWithDeviceVector<float>(32);
    BasicSortTestOfLengthWithDeviceVector<float>(64);
	BasicSortTestOfLengthWithDeviceVector<float>(128);
    BasicSortTestOfLengthWithDeviceVector<float>(256);
    BasicSortTestOfLengthWithDeviceVector<float>(512);
    BasicSortTestOfLengthWithDeviceVector<float>(1024);
    BasicSortTestOfLengthWithDeviceVector<float>(2048);
    //BasicSortTestOfLengthWithDeviceVector<float>(1048576);
#if (TEST_DOUBLE == 1)
    BasicSortTestOfLengthWithDeviceVector<double>(8);
    BasicSortTestOfLengthWithDeviceVector<double>(16);
    BasicSortTestOfLengthWithDeviceVector<double>(32);
    BasicSortTestOfLengthWithDeviceVector<double>(64);
	BasicSortTestOfLengthWithDeviceVector<double>(128);
    BasicSortTestOfLengthWithDeviceVector<double>(256);
    BasicSortTestOfLengthWithDeviceVector<double>(512);
    BasicSortTestOfLengthWithDeviceVector<double>(1024);
    BasicSortTestOfLengthWithDeviceVector<double>(2048);
    //BasicSortTestOfLengthWithDeviceVector<double>(1048576);
#endif
#endif

#if (TEST_ALL == 1)
    std::cout << "Testing UserDefinedBoltFunctorSortTestOfLength\n";
    UserDefinedBoltFunctorSortTestOfLength<int>(8);
    UserDefinedBoltFunctorSortTestOfLength<int>(16);
    UserDefinedBoltFunctorSortTestOfLength<int>(32);
    UserDefinedBoltFunctorSortTestOfLength<int>(64);
    UserDefinedBoltFunctorSortTestOfLength<int>(128);
    UserDefinedBoltFunctorSortTestOfLength<int>(256);
    UserDefinedBoltFunctorSortTestOfLength<int>(512);
    UserDefinedBoltFunctorSortTestOfLength<int>(1024);
    UserDefinedBoltFunctorSortTestOfLength<int>(2048);
    UserDefinedBoltFunctorSortTestOfLength<int>(1048576);
    UserDefinedBoltFunctorSortTestOfLength<float>(8);
    UserDefinedBoltFunctorSortTestOfLength<float>(16);
    UserDefinedBoltFunctorSortTestOfLength<float>(32);
    UserDefinedBoltFunctorSortTestOfLength<float>(64);
    UserDefinedBoltFunctorSortTestOfLength<float>(128);
    UserDefinedBoltFunctorSortTestOfLength<float>(256);
    UserDefinedBoltFunctorSortTestOfLength<float>(512);
    UserDefinedBoltFunctorSortTestOfLength<float>(1024);
    UserDefinedBoltFunctorSortTestOfLength<float>(2048);
    UserDefinedBoltFunctorSortTestOfLength<float>(1048576);
#if (TEST_DOUBLE == 1)
    UserDefinedBoltFunctorSortTestOfLength<double>(8);
    UserDefinedBoltFunctorSortTestOfLength<double>(16);
    UserDefinedBoltFunctorSortTestOfLength<double>(32);
    UserDefinedBoltFunctorSortTestOfLength<double>(64);
    UserDefinedBoltFunctorSortTestOfLength<double>(128);
    UserDefinedBoltFunctorSortTestOfLength<double>(256);
    UserDefinedBoltFunctorSortTestOfLength<double>(512);
    UserDefinedBoltFunctorSortTestOfLength<double>(1024);
    UserDefinedBoltFunctorSortTestOfLength<double>(2048);
    UserDefinedBoltFunctorSortTestOfLength<double>(1048576);
#endif
#endif

#if (TEST_ALL == 1)
    std::cout << "Testing UserDefinedFunctorSortTestOfLength\n";
    UserDefinedFunctorSortTestOfLength<int>(8);
    UserDefinedFunctorSortTestOfLength<int>(16);
    UserDefinedFunctorSortTestOfLength<int>(32);
    UserDefinedFunctorSortTestOfLength<int>(64);
    UserDefinedFunctorSortTestOfLength<int>(128);
    UserDefinedFunctorSortTestOfLength<int>(256);
    UserDefinedFunctorSortTestOfLength<int>(512);
    UserDefinedFunctorSortTestOfLength<int>(1024);
    UserDefinedFunctorSortTestOfLength<int>(2048);
    UserDefinedFunctorSortTestOfLength<int>(1048576);
    UserDefinedFunctorSortTestOfLength<float>(8);
    UserDefinedFunctorSortTestOfLength<float>(16);
    UserDefinedFunctorSortTestOfLength<float>(32);
    UserDefinedFunctorSortTestOfLength<float>(64);
    UserDefinedFunctorSortTestOfLength<float>(128);
    UserDefinedFunctorSortTestOfLength<float>(256);
    UserDefinedFunctorSortTestOfLength<float>(512);
    UserDefinedFunctorSortTestOfLength<float>(1024);
    UserDefinedFunctorSortTestOfLength<float>(2048);
    UserDefinedFunctorSortTestOfLength<float>(1048576);

#if (TEST_DOUBLE == 1)
    UserDefinedFunctorSortTestOfLength<double>(8);
    UserDefinedFunctorSortTestOfLength<double>(16);
    UserDefinedFunctorSortTestOfLength<double>(32);
    UserDefinedFunctorSortTestOfLength<double>(64);
    UserDefinedFunctorSortTestOfLength<double>(128);
    UserDefinedFunctorSortTestOfLength<double>(256);
    UserDefinedFunctorSortTestOfLength<double>(512);
    UserDefinedFunctorSortTestOfLength<double>(1024);
    UserDefinedFunctorSortTestOfLength<double>(2048);
    UserDefinedFunctorSortTestOfLength<double>(1048576);
#endif
#endif

#if 0
    UserDefinedFunctionSortTestOfLength<int>(256);
    UserDefinedFunctionSortTestOfLength<int>(512);
    UserDefinedFunctionSortTestOfLength<int>(1024);
    UserDefinedFunctionSortTestOfLength<int>(2048);
    UserDefinedFunctionSortTestOfLength<int>(1048576);
    UserDefinedFunctionSortTestOfLength<float>(256);
    UserDefinedFunctionSortTestOfLength<float>(512);
    UserDefinedFunctionSortTestOfLength<float>(1024);
    UserDefinedFunctionSortTestOfLength<float>(2048);
    UserDefinedFunctionSortTestOfLength<float>(1048576);
#if (TEST_DOUBLE == 1)
    UserDefinedFunctionSortTestOfLength<double>(256);
    UserDefinedFunctionSortTestOfLength<double>(512);
    UserDefinedFunctionSortTestOfLength<double>(1024);
    UserDefinedFunctionSortTestOfLength<double>(2048);
    UserDefinedFunctionSortTestOfLength<double>(1048576);
#endif
#endif

#if (TEST_ALL == 1)
    std::cout << "Testing UserDefinedObjectSortTestOfLength\n";
    UserDefinedObjectSortTestOfLength<int>(8);
    UserDefinedObjectSortTestOfLength<int>(16);
    UserDefinedObjectSortTestOfLength<int>(32);
    UserDefinedObjectSortTestOfLength<int>(64);
    UserDefinedObjectSortTestOfLength<int>(128);
    UserDefinedObjectSortTestOfLength<int>(256);
    UserDefinedObjectSortTestOfLength<int>(512);
    UserDefinedObjectSortTestOfLength<int>(1024);
    UserDefinedObjectSortTestOfLength<int>(2048);
    UserDefinedObjectSortTestOfLength<int>(1048576);
    UserDefinedObjectSortTestOfLength<float>(8);
    UserDefinedObjectSortTestOfLength<float>(16);
    UserDefinedObjectSortTestOfLength<float>(32);
    UserDefinedObjectSortTestOfLength<float>(64);
    UserDefinedObjectSortTestOfLength<float>(128);
    UserDefinedObjectSortTestOfLength<float>(256);
    UserDefinedObjectSortTestOfLength<float>(512);
    UserDefinedObjectSortTestOfLength<float>(1024);
    UserDefinedObjectSortTestOfLength<float>(2048);
    UserDefinedObjectSortTestOfLength<float>(1048576);
#if (TEST_DOUBLE == 1)
    UserDefinedObjectSortTestOfLength<double>(8);
    UserDefinedObjectSortTestOfLength<double>(16);
    UserDefinedObjectSortTestOfLength<double>(32);
    UserDefinedObjectSortTestOfLength<double>(64);
    UserDefinedObjectSortTestOfLength<double>(128);
    UserDefinedObjectSortTestOfLength<double>(256);
    UserDefinedObjectSortTestOfLength<double>(512);
    UserDefinedObjectSortTestOfLength<double>(1024);
    UserDefinedObjectSortTestOfLength<double>(2048);
    UserDefinedObjectSortTestOfLength<double>(1048576);
#endif
#endif

#if (TEST_ALL == 1)
    std::cout << "Testing BasicSortTestOfLength\n";
    BasicSortTestOfLength<int>(256);
    BasicSortTestOfLength<int>(512);
    BasicSortTestOfLength<int>(1024);
    BasicSortTestOfLength<int>(2048);
    BasicSortTestOfLength<int>(1048576);
    BasicSortTestOfLength<float>(256);
    BasicSortTestOfLength<float>(512);
    BasicSortTestOfLength<float>(1024);
    BasicSortTestOfLength<float>(2048);
    BasicSortTestOfLength<float>(1048576);

#if (TEST_DOUBLE == 1)
    BasicSortTestOfLength<double>(256);
    BasicSortTestOfLength<double>(512);
    BasicSortTestOfLength<double>(1024);
    BasicSortTestOfLength<double>(2048);
    BasicSortTestOfLength<double>(1048576);
#endif
#endif

    std::cout << "Test Completed" << std::endl;
    getchar();
	return 0;
}
#endif
