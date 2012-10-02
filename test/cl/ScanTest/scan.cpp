#include "common/stdafx.h"
#include <vector>
#include <array>

#include "bolt/cl/scan.h"
#include "bolt/unicode.h"
#include "bolt/miniDump.h"

#include <gtest/gtest.h>
#include <boost/shared_array.hpp>

#include <boost/program_options.hpp>
namespace po = boost::program_options;

/////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Below are helper routines to compare the results of two arrays for googletest
//  They return an assertion object that googletest knows how to track

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

    //  Calling the actual functions under test
    ArrayCont::iterator stdEnd  = std::partial_sum( stdInput.begin( ), stdInput.end( ), stdInput.begin( ) );
    ArrayCont::iterator boltEnd = bolt::cl::inclusive_scan( boltInput.begin( ), boltInput.end( ), boltInput.begin( ) );

    //  The returned iterator should be at the end of the result range
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
    ArrayCont::iterator stdEnd  = std::partial_sum( stdInput.begin( ), stdInput.end( ), stdInput.begin( ), std::plus< ArrayType >( ) );
    ArrayCont::iterator boltEnd = bolt::cl::inclusive_scan( boltInput.begin( ), boltInput.end( ), boltInput.begin( ), bolt::cl::plus< ArrayType >( ) );

    //  The returned iterator should be at the end of the result range
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
    ArrayCont::iterator stdEnd  = std::partial_sum( stdInput.begin( ), stdInput.end( ), stdInput.begin( ), bolt::cl::maximum< ArrayType >( ) );
    ArrayCont::iterator boltEnd = bolt::cl::inclusive_scan( boltInput.begin( ), boltInput.end( ), boltInput.begin( ), bolt::cl::maximum< ArrayType >( ) );

    //  The returned iterator should be at the end of the result range
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
    ArrayCont::iterator boltEnd = bolt::cl::inclusive_scan( boltInput.begin( ), boltInput.end( ), boltResult.begin( ) );

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

//  ::testing::TestWithParam< int > means that GetParam( ) returns int values, which i use for array size
class ScanIntegerDeviceVector: public ::testing::TestWithParam< int >
{
public:
    //  Create an std and a bolt vector of requested size, and initialize all the elements to 1
    ScanIntegerDeviceVector( ): stdInput( GetParam( ), 1 ), boltInput( static_cast<size_t>( GetParam( ) ), 1 )
    {}

protected:
    std::vector< int > stdInput;
    bolt::cl::device_vector< int > boltInput;
};

//  ::testing::TestWithParam< int > means that GetParam( ) returns int values, which i use for array size
class ScanIntegerNakedPointer: public ::testing::TestWithParam< int >
{
public:
    //  Create an std and a bolt vector of requested size, and initialize all the elements to 1
    ScanIntegerNakedPointer( ): stdInput( new int[ GetParam( ) ] ), boltInput( new int[ GetParam( ) ] )
    {}

    virtual void SetUp( )
    {
        size_t size = GetParam( );
        for( int i=0; i < size; i++ )
        {
            stdInput[ i ] = 1;
            boltInput[ i ] = 1;
        }
    };

    virtual void TearDown( )
    {
        delete [] stdInput;
        delete [] boltInput;
    };

protected:
    //boost::shared_array< int > stdInput;
    //boost::shared_array< int > boltInput;
     int* stdInput;
     int* boltInput;
};

TEST_P( ScanIntegerVector, InclusiveInplace )
{
    //cl_int err = CL_SUCCESS;

    //std::string strDeviceName = bolt::cl::control::getDefault( ).device( ).getInfo< CL_DEVICE_NAME >( &err );
    //bolt::cl::V_OPENCL( err, "Device::getInfo< CL_DEVICE_NAME > failed" );

    //std::cout << "Device under test : " << strDeviceName << std::endl;

    //  Calling the actual functions under test
    std::vector< int >::iterator stdEnd  = std::partial_sum( stdInput.begin( ), stdInput.end( ), stdInput.begin( ) );
    std::vector< int >::iterator boltEnd = bolt::cl::inclusive_scan( boltInput.begin( ), boltInput.end( ), boltInput.begin( ) );

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
    std::vector< float >::iterator boltEnd = bolt::cl::inclusive_scan( boltInput.begin( ), boltInput.end( ), boltInput.begin( ) );

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
    std::vector< double >::iterator boltEnd = bolt::cl::inclusive_scan( boltInput.begin( ), boltInput.end( ), boltInput.begin( ) );

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

TEST_P( ScanIntegerDeviceVector, InclusiveInplace )
{
    //  Calling the actual functions under test
    std::vector< int >::iterator stdEnd  = std::partial_sum( stdInput.begin( ), stdInput.end( ), stdInput.begin( ) );
    bolt::cl::device_vector< int >::iterator boltEnd = bolt::cl::inclusive_scan( boltInput.begin( ), boltInput.end( ), boltInput.begin( ) );

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

TEST_P( ScanIntegerNakedPointer, InclusiveInplace )
{
    size_t endIndex = GetParam( );

    //  Calling the actual functions under test
    stdext::checked_array_iterator< int* > wrapStdInput( stdInput, endIndex );
    stdext::checked_array_iterator< int* > stdEnd = std::partial_sum( wrapStdInput, wrapStdInput + endIndex, wrapStdInput );
    //int* stdEnd = std::partial_sum( stdInput, stdInput + endIndex, stdInput );

    stdext::checked_array_iterator< int* > wrapBoltInput( boltInput, endIndex );
    stdext::checked_array_iterator< int* > boltEnd = bolt::cl::inclusive_scan( wrapBoltInput, wrapBoltInput + endIndex, wrapBoltInput );
    //int* boltEnd = bolt::cl::inclusive_scan( boltInput, boltInput + endIndex, boltInput );

    //  The returned iterator should be one past the 
    EXPECT_EQ( wrapStdInput + endIndex, stdEnd );
    EXPECT_EQ( wrapBoltInput + endIndex, boltEnd );

    size_t stdNumElements = std::distance( wrapStdInput, stdEnd );
    size_t boltNumElements = std::distance( wrapBoltInput, boltEnd );

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdInput, boltInput, endIndex );
}

TEST_P( ScanIntegerVector, ExclusiveOutOfPlace )
{
    //  Declare temporary arrays to store results for out of place computation
    std::vector< int > stdResult( GetParam( ) ), boltResult( GetParam( ) );
    int init = 3;

    //  Emulating a std exclusive scan
    if( stdInput.size( ) )
        stdInput[ 0 ] += init;
    std::vector< int >::iterator stdEnd  = std::partial_sum( stdInput.begin( ), stdInput.end( ), stdResult.begin( ) );
    if( stdInput.size( ) )
        stdInput[ 0 ] -= init;
    stdEnd = std::transform( stdResult.begin( ), stdResult.end( ), stdInput.begin( ), stdResult.begin( ), std::minus< int >( ) );

    //  Calling Bolt exclusive scan
    std::vector< int >::iterator boltEnd = bolt::cl::exclusive_scan( boltInput.begin( ), boltInput.end( ), boltResult.begin( ), init );

    //  The returned iterator should be one past the 
    EXPECT_EQ( stdResult.end( ), stdEnd );
    EXPECT_EQ( boltResult.end( ), boltEnd );

    std::vector< int >::iterator::difference_type stdNumElements = std::distance( stdResult.begin( ), stdEnd );
    std::vector< int >::iterator::difference_type boltNumElements = std::distance( boltResult.begin( ), boltEnd );

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdResult, boltResult );
}

//tring to call out-place scan
TEST_P( ScanFloatVector, intSameValuesSerialOutPlace )
{
    bolt::cl::device_vector< float > boltInput( GetParam( ), 1.0f );
    bolt::cl::device_vector< float > boltOutput( GetParam( ), 1.0f );
    bolt::cl::device_vector< float >::iterator boltEnd = bolt::cl::exclusive_scan( boltInput.begin( ), boltInput.end( ), boltOutput.begin() );

    std::vector< float > stdOutput( GetParam( ), 1.0f );
    std::vector< float >::iterator stdEnd  = bolt::cl::exclusive_scan( stdInput.begin( ), stdInput.end( ), stdOutput.begin( ) );
    
    //  Loop through the array and compare all the values with each other
    cmpArrays( stdOutput, boltOutput );
}

//tring to call in-place scan
TEST_P( ScanFloatVector, intSameValuesSerialInPlace )
{
    bolt::cl::device_vector< float > dvBoltInput( GetParam( ), 1.0f );
    bolt::cl::device_vector< float >::iterator boltEnd = bolt::cl::exclusive_scan( dvBoltInput.begin( ), dvBoltInput.end( ), dvBoltInput.begin() );
    {
        bolt::cl::device_vector< float >::pointer inPlaceData      = dvBoltInput.data( );
    }

    //std::vector< float > stdInput( 1024, 1 );
    std::vector< float >::iterator stdEnd  = bolt::cl::exclusive_scan( stdInput.begin( ), stdInput.end( ), stdInput.begin( ) );
    
    //  Loop through the array and compare all the values with each other
    cmpArrays( stdInput, boltInput );
}

//  Test lots of consecutive numbers, but small range, suitable for integers because they overflow easier
INSTANTIATE_TEST_CASE_P( Inclusive, ScanIntegerVector, ::testing::Range( 0, 1024, 1 ) );
INSTANTIATE_TEST_CASE_P( Inclusive, ScanIntegerDeviceVector, ::testing::Range( 0, 1024, 1 ) );
INSTANTIATE_TEST_CASE_P( Inclusive, ScanIntegerNakedPointer, ::testing::Range( 0, 1024, 1 ) );

//  Test a huge range, suitable for floating point as they are less prone to overflow (but floating point loses granularity at large values)
INSTANTIATE_TEST_CASE_P( Inclusive, ScanFloatVector, ::testing::Range( 4096, 1048576, 4096 ) );
INSTANTIATE_TEST_CASE_P( Inclusive, ScanDoubleVector, ::testing::Range( 4096, 1048576, 4096 ) );

typedef ::testing::Types< 
    std::tuple< int, TypeValue< 1 > >,
    //std::tuple< int, TypeValue< bolt::cl::scanMultiCpuThreshold - 1 > >,
    //std::tuple< int, TypeValue< bolt::cl::scanGpuThreshold - 1 > >,
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
    //std::tuple< float, TypeValue< bolt::cl::scanMultiCpuThreshold - 1 > >,
    //std::tuple< float, TypeValue< bolt::cl::scanGpuThreshold - 1 > >,
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

static const std::streamsize colWidth = 38;

void printExtention( const std::string& str )
{
    std::cout << std::setw( colWidth ) << "" << str << std::endl;
}

class printDeviceFunctor
{
    cl_uint m_numDev;

public:
    printDeviceFunctor( cl_uint initNumDevice ): m_numDev( initNumDevice )
    {}

    void operator()( const cl::Device& dev )
    {
        cl_int err = CL_SUCCESS;

        std::string strDeviceName = dev.getInfo< CL_DEVICE_NAME >( &err );
        bolt::cl::V_OPENCL( err, "Device::getInfo< CL_DEVICE_NAME > failed" );

        std::string strDeviceProfile = dev.getInfo< CL_DEVICE_PROFILE >( &err );
        bolt::cl::V_OPENCL( err, "Device::getInfo< CL_DEVICE_PROFILE > failed" );

        std::string strDeviceVersion = dev.getInfo< CL_DEVICE_VERSION >( &err );
        bolt::cl::V_OPENCL( err, "Device::getInfo< CL_DEVICE_VERSION > failed" );

        std::string strDriverVersion = dev.getInfo< CL_DRIVER_VERSION >( &err );
        bolt::cl::V_OPENCL( err, "Device::getInfo< CL_DRIVER_VERSION > failed" );

        std::string strOpenCLVersion = dev.getInfo< CL_DEVICE_OPENCL_C_VERSION >( &err );
        bolt::cl::V_OPENCL( err, "Device::getInfo< CL_DEVICE_OPENCL_C_VERSION > failed" );

        cl_device_type ulDeviceType = dev.getInfo< CL_DEVICE_TYPE >( &err );
        bolt::cl::V_OPENCL( err, "Device::getInfo< CL_DEVICE_TYPE > failed" );

        cl_uint uiMaxClock = dev.getInfo< CL_DEVICE_MAX_CLOCK_FREQUENCY >( &err );
        bolt::cl::V_OPENCL( err, "Device::getInfo< CL_DEVICE_MAX_CLOCK_FREQUENCY > failed" );

        cl_uint uiMaxUnits = dev.getInfo< CL_DEVICE_MAX_COMPUTE_UNITS >( &err );
        bolt::cl::V_OPENCL( err, "Device::getInfo< CL_DEVICE_MAX_COMPUTE_UNITS > failed" );

        size_t stMaxWorkGroup = dev.getInfo< CL_DEVICE_MAX_WORK_GROUP_SIZE >( &err );
        bolt::cl::V_OPENCL( err, "Device::getInfo< CL_DEVICE_MAX_WORK_GROUP_SIZE > failed" );

        cl_uint uiMaxDim = dev.getInfo< CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS >( &err );
        bolt::cl::V_OPENCL( err, "Device::getInfo< CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS > failed" );

        std::vector< size_t > uiMaxDimSizes = dev.getInfo< CL_DEVICE_MAX_WORK_ITEM_SIZES >( &err );
        bolt::cl::V_OPENCL( err, "Device::getInfo< CL_DEVICE_MAX_WORK_ITEM_SIZES > failed" );

        std::string szDeviceExtensions = dev.getInfo< CL_DEVICE_EXTENSIONS >( &err );
        bolt::cl::V_OPENCL( err, "Device::getInfo< CL_DEVICE_EXTENSIONS > failed" );

        cl_uint uiMemBaseAddrAlign = dev.getInfo< CL_DEVICE_MEM_BASE_ADDR_ALIGN >( &err );
        bolt::cl::V_OPENCL( err, "Device::getInfo< CL_DEVICE_MEM_BASE_ADDR_ALIGN > failed" );

        cl_uint uiMinDataTypeAlignSize = dev.getInfo< CL_DEVICE_MIN_DATA_TYPE_ALIGN_SIZE >( &err );
        bolt::cl::V_OPENCL( err, "Device::getInfo< CL_DEVICE_MIN_DATA_TYPE_ALIGN_SIZE > failed" );

        cl_ulong ulGlobalMemSize = dev.getInfo< CL_DEVICE_GLOBAL_MEM_SIZE >( &err );
        bolt::cl::V_OPENCL( err, "Device::getInfo< CL_DEVICE_GLOBAL_MEM_SIZE > failed" );

        cl_ulong ulMaxMemAllocSize = dev.getInfo< CL_DEVICE_MAX_MEM_ALLOC_SIZE >( &err );
        bolt::cl::V_OPENCL( err, "Device::getInfo< CL_DEVICE_MAX_MEM_ALLOC_SIZE > failed" );

        cl_ulong ulLocalMemSize = dev.getInfo< CL_DEVICE_LOCAL_MEM_SIZE >( &err );
        bolt::cl::V_OPENCL( err, "Device::getInfo< CL_DEVICE_LOCAL_MEM_SIZE > failed" );

        cl_device_local_mem_type ulLocalMemType = dev.getInfo< CL_DEVICE_LOCAL_MEM_TYPE >( &err );
        bolt::cl::V_OPENCL( err, "Device::getInfo< CL_DEVICE_LOCAL_MEM_TYPE > failed" );

        cl_bool bHostUnifiedMem = dev.getInfo< CL_DEVICE_HOST_UNIFIED_MEMORY >( &err );
        bolt::cl::V_OPENCL( err, "Device::getInfo< CL_DEVICE_HOST_UNIFIED_MEMORY > failed" );

        //  Create a vector of extention strings, which we know are seperated by spaces
        std::istringstream splitExtentions( szDeviceExtensions );
        std::vector< std::string > extTokens;
        std::copy( std::istream_iterator< std::string >( splitExtentions ), std::istream_iterator< std::string >( ),
                   std::back_inserter< std::vector< std::string > >( extTokens ) );

        //  Print logic
        std::cout << "Device info [ " << m_numDev++ << " ]" << std::endl;
        std::cout << std::setw( colWidth ) << "CL_DEVICE_NAME : " << strDeviceName << std::endl;
        std::cout << std::setw( colWidth ) << "CL_DEVICE_TYPE : "
            << (CL_DEVICE_TYPE_DEFAULT     & ulDeviceType ? "default"     : "")
            << (CL_DEVICE_TYPE_CPU         & ulDeviceType ? "CPU"         : "")
            << (CL_DEVICE_TYPE_GPU         & ulDeviceType ? "GPU"         : "")
            << (CL_DEVICE_TYPE_ACCELERATOR & ulDeviceType ? "Accelerator" : "")
            << std::endl;

        std::cout << std::setw( colWidth ) << "CL_DEVICE_MAX_CLOCK_FREQUENCY : " << uiMaxClock << std::endl;
        std::cout << std::setw( colWidth ) << "CL_DEVICE_MAX_COMPUTE_UNITS : " << uiMaxUnits << std::endl;
        std::cout << std::setw( colWidth ) << "CL_DEVICE_MAX_WORK_GROUP_SIZE : " << stMaxWorkGroup << std::endl;
        std::cout << std::setw( colWidth ) << "CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS : " << uiMaxDim << std::endl;
        for( size_t wis = 0; wis < uiMaxDimSizes.size( ); ++wis )
        {
            std::stringstream dimString;
            dimString << "Dimension[ " << wis << " ] : ";
            std::cout << std::setw( colWidth ) << dimString.str( ) << uiMaxDimSizes[ wis ] << std::endl;
        }
        std::cout << std::setw( colWidth ) << "CL_DEVICE_GLOBAL_MEM_SIZE : " << ulGlobalMemSize / 1024 / 1024 << " MB" << std::endl;
        std::cout << std::setw( colWidth ) << "CL_DEVICE_MAX_MEM_ALLOC_SIZE : " << ulMaxMemAllocSize / 1024 / 1024 << " MB" << std::endl;
        std::cout << std::setw( colWidth ) << "CL_DEVICE_MEM_BASE_ADDR_ALIGN : " << uiMemBaseAddrAlign << std::endl;
        std::cout << std::setw( colWidth ) << "CL_DEVICE_MIN_DATA_TYPE_ALIGN_SIZE : " << uiMinDataTypeAlignSize << std::endl;
        std::cout << std::setw( colWidth ) << "CL_DEVICE_LOCAL_MEM_SIZE : " << ulLocalMemSize / 1024 << " KB" << std::endl;
        std::cout << std::setw( colWidth ) << "CL_DEVICE_LOCAL_MEM_TYPE : "
            << (CL_LOCAL     & ulLocalMemType ? "local"  : "")
            << (CL_GLOBAL    & ulLocalMemType ? "global" : "")
            << std::endl;
        std::cout << std::setw( colWidth ) << "CL_DEVICE_HOST_UNIFIED_MEMORY : "
            << (1 == bHostUnifiedMem ? "true"  : "")
            << (0 == bHostUnifiedMem ? "false" : "")
            << std::endl;

        std::cout << std::setw( colWidth ) << "CL_DEVICE_PROFILE : " << strDeviceProfile << std::endl;
        std::cout << std::setw( colWidth ) << "CL_DEVICE_OPENCL_C_VERSION : " << strOpenCLVersion << std::endl;
        std::cout << std::setw( colWidth ) << "CL_DEVICE_VERSION : " << strDeviceVersion << std::endl;
        std::cout << std::setw( colWidth ) << "CL_DRIVER_VERSION : " << strDriverVersion << std::endl;
        std::cout << std::setw( colWidth ) << "CL_DEVICE_EXTENSIONS : " << std::endl;
        std::for_each( extTokens.begin( ), extTokens.end( ), printExtention );
        std::cout << std::endl;
    }
};

class printPlatformFunctor
{
    cl_uint m_numPlat;

public:
    printPlatformFunctor( cl_uint initNumPlat ): m_numPlat( initNumPlat )
    {}

    void operator()( const cl::Platform& plat )
    {
        cl_int err = CL_SUCCESS;

        std::string szPlatformProfile = plat.getInfo< CL_PLATFORM_PROFILE >( &err );
        bolt::cl::V_OPENCL( err, "Platform::getInfo< CL_DEVICE_NAME > failed" );

        std::string szPlatformVersion = plat.getInfo< CL_PLATFORM_VERSION >( &err );
        bolt::cl::V_OPENCL( err, "Platform::getInfo< CL_DEVICE_NAME > failed" );

        std::string szPlatformName = plat.getInfo< CL_PLATFORM_NAME >( &err );
        bolt::cl::V_OPENCL( err, "Platform::getInfo< CL_DEVICE_NAME > failed" );

        std::string szPlatformVendor = plat.getInfo< CL_PLATFORM_VENDOR >( &err );
        bolt::cl::V_OPENCL( err, "Platform::getInfo< CL_DEVICE_NAME > failed" );

        std::string szPlatformExtensions = plat.getInfo< CL_PLATFORM_EXTENSIONS >( &err );
        bolt::cl::V_OPENCL( err, "Platform::getInfo< CL_DEVICE_NAME > failed" );

        //  Create a vector of extention strings, which we know are seperated by spaces
        std::istringstream splitExtentions( szPlatformExtensions );
        std::vector< std::string > extTokens;
        std::copy( std::istream_iterator< std::string >( splitExtentions ), std::istream_iterator< std::string >( ),
                   std::back_inserter< std::vector< std::string > >( extTokens ) );
        //std::copy( std::istream_iterator< std::string >( splitExtentions ), std::istream_iterator< std::string >( ),
        //           std::ostream_iterator< std::string >( std::cout, "\n" ) );

        std::cout << "Platform info [ " << m_numPlat++ << " ]" << std::endl;
        std::cout << std::setw( colWidth ) << "CL_PLATFORM_PROFILE : " << szPlatformProfile << std::endl;
        std::cout << std::setw( colWidth ) << "CL_PLATFORM_VERSION : " << szPlatformVersion << std::endl;
        std::cout << std::setw( colWidth ) << "CL_PLATFORM_NAME : " << szPlatformName << std::endl;
        std::cout << std::setw( colWidth ) << "CL_PLATFORM_VENDOR : " << szPlatformVendor<< std::endl;
        std::cout << std::setw( colWidth ) << "CL_PLATFORM_EXTENSIONS : " << std::endl;
        std::for_each( extTokens.begin( ), extTokens.end( ), printExtention );
        std::cout << std::endl;

        // For each device for this specific platform, print all the associated information
        std::vector< cl::Device > devices;
        bolt::cl::V_OPENCL( plat.getDevices( CL_DEVICE_TYPE_ALL, &devices ), "Platform::getDevices() failed" );

        std::for_each( devices.begin( ), devices.end( ), printDeviceFunctor( 0 ) );

    }
};

int _tmain(int argc, _TCHAR* argv[])
{
    //  Register our minidump generating logic
    bolt::miniDumpSingleton::enableMiniDumps( );

    //  Initialize googletest; this removes googletest specific flags from command line
    ::testing::InitGoogleTest( &argc, &argv[ 0 ] );

    bool print_clInfo = false;
    cl_uint userPlatform = 0;
    cl_uint userDevice = 0;
    cl_device_type deviceType = CL_DEVICE_TYPE_DEFAULT;

    try
    {
        // Declare supported options below, describe what they do
        po::options_description desc( "Scan GoogleTest command line options" );
        desc.add_options()
            ( "help,h",         "produces this help message" )
            ( "queryOpenCL,q",  "Print queryable platform and device info and return" )
            ( "platform,p",     po::value< cl_uint >( &userPlatform )->default_value( 0 ),	"Specify the platform under test" )
            ( "device,d",       po::value< cl_uint >( &userDevice )->default_value( 0 ),	"Specify the device under test" )
            //( "gpu,g",         "Force instantiation of all OpenCL GPU device" )
            //( "cpu,c",         "Force instantiation of all OpenCL CPU device" )
            //( "all,a",         "Force instantiation of all OpenCL devices" )
            ;

        ////  All positional options (un-named) should be interpreted as kernelFiles
        //po::positional_options_description p;
        //p.add("kernelFiles", -1);

        //po::variables_map vm;
        //po::store( po::command_line_parser( argc, argv ).options( desc ).positional( p ).run( ), vm );
        //po::notify( vm );

        po::variables_map vm;
        po::store( po::parse_command_line( argc, argv, desc ), vm );
        po::notify( vm );

        if( vm.count( "help" ) )
        {
            //	This needs to be 'cout' as program-options does not support wcout yet
            std::cout << desc << std::endl;
            return 0;
        }

        if( vm.count( "queryOpenCL" ) )
        {
            print_clInfo = true;
        }

        //  The following 3 options are not implemented yet; they are meant to be used with ::clCreateContextFromType()
        if( vm.count( "gpu" ) )
        {
            deviceType	= CL_DEVICE_TYPE_GPU;
        }
        
        if( vm.count( "cpu" ) )
        {
            deviceType	= CL_DEVICE_TYPE_CPU;
        }

        if( vm.count( "all" ) )
        {
            deviceType	= CL_DEVICE_TYPE_ALL;
        }

    }
    catch( std::exception& e )
    {
        std::cout << _T( "Scan GoogleTest error condition reported:" ) << std::endl << e.what() << std::endl;
        return 1;
    }

    //  Query OpenCL for available platforms
    cl_int err = CL_SUCCESS;

    // Platform vector contains all available platforms on system
    std::vector< cl::Platform > platforms;
    //std::cout << "HelloCL!\nGetting Platform Information\n";
    bolt::cl::V_OPENCL( cl::Platform::get( &platforms ), "Platform::get() failed" );

    if( print_clInfo )
    {
        std::for_each( platforms.begin( ), platforms.end( ), printPlatformFunctor( 0 ) );
        return 0;
    }

    //  Do stuff with the platforms
    std::vector<cl::Platform>::iterator i;
    if(platforms.size() > 0)
    {
        for(i = platforms.begin(); i != platforms.end(); ++i)
        {
            if(!strcmp((*i).getInfo<CL_PLATFORM_VENDOR>(&err).c_str(), "Advanced Micro Devices, Inc."))
            {
                break;
            }
        }
    }
    bolt::cl::V_OPENCL( err, "Platform::getInfo() failed" );

    // Device info
    std::vector< cl::Device > devices;
    bolt::cl::V_OPENCL( platforms.front( ).getDevices( CL_DEVICE_TYPE_ALL, &devices ), "Platform::getDevices() failed" );

    cl::Context myContext( devices.at( userDevice ) );

    cl::CommandQueue myQueue( myContext, devices.at( userDevice ) );
    bolt::cl::control::getDefault( ).commandQueue( myQueue );

    //bolt::cl::control myBoltControl( myQueue );

    std::string strDeviceName = bolt::cl::control::getDefault( ).device( ).getInfo< CL_DEVICE_NAME >( &err );
    bolt::cl::V_OPENCL( err, "Device::getInfo< CL_DEVICE_NAME > failed" );

    std::cout << "Device under test : " << strDeviceName << std::endl;

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