
#define GOOGLE_TEST 0
#if (GOOGLE_TEST == 1)
//#include "stdafx.h"
#include <bolt/cl/scan.h>
#include <bolt/unicode.h>
#include <bolt/miniDump.h>

#include <gtest/gtest.h>

#include <vector>
#include <array>

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

//  Explicit initialization of the C++ static const
template< size_t N >
const size_t TypeValue< N >::value;

//  Test fixture class, used for the Type-parameterized tests
//  Namely, the tests that use std::array and TYPED_TEST_P macros
template< typename ArrayTuple >
class SortArrayTest: public ::testing::Test
{
public:
    SortArrayTest( ): m_Errors( 0 )
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

    virtual ~SortArrayTest( )
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
const size_t SortArrayTest< ArrayTuple >::ArraySize;

TYPED_TEST_CASE_P( SortArrayTest );

TYPED_TEST_P( SortArrayTest, InPlace )
{
    typedef std::array< ArrayType, ArraySize > ArrayCont;

    //  Calling the actual functions under test
    std::sort( stdInput.begin( ), stdInput.end( ));
    bolt::cl::sort( boltInput.begin( ), boltInput.end( ));

    ArrayCont::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end( ) );
    ArrayCont::difference_type boltNumElements = std::distance( boltInput.begin( ), boltInput.end( ) );

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpStdArray< ArrayType, ArraySize >::cmpArrays( stdInput, boltInput );
}

TYPED_TEST_P( SortArrayTest, InPlaceLessFunction )
{
    typedef std::array< ArrayType, ArraySize > ArrayCont;

    //  Calling the actual functions under test
    std::sort( stdInput.begin( ), stdInput.end( ), stdInput.begin( ), std::less< ArrayType >( ) );
    bolt::cl::sort( boltInput.begin( ), boltInput.end( ), boltInput.begin( ), bolt::cl::less< ArrayType >( ) );

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

TYPED_TEST_P( SortArrayTest, InPlaceGreaterFunction )
{
    typedef std::array< ArrayType, ArraySize > ArrayCont;

    //  Calling the actual functions under test
    std::sort( stdInput.begin( ), stdInput.end( ), stdInput.begin( ), std::greater< ArrayType >( ) );
    bolt::cl::sort( boltInput.begin( ), boltInput.end( ), boltInput.begin( ), bolt::cl::greater< ArrayType >( ) );

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


REGISTER_TYPED_TEST_CASE_P( SortArrayTest, InPlace, InPlaceLessFunction, InPlaceGreaterFunction);

/////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Fixture classes are now defined to enable googletest to process value parameterized tests

//  ::testing::TestWithParam< int > means that GetParam( ) returns int values, which i use for array size
class SortIntegerVector: public ::testing::TestWithParam< int >
{
public:
    //  Create an std and a bolt vector of requested size, and initialize all the elements to 1
    SortIntegerVector( ): stdInput( GetParam( )), boltInput( GetParam( ))
    {  
        size_t i, length;
        length = GetParam( );
        for (i=0;i<length;i++)
        {
            stdInput[i] = (int)(length - i + 2);
            boltInput[i] = (int)(length - i + 2);
        }
    }

protected:
    std::vector< int > stdInput, boltInput;
};

//  ::testing::TestWithParam< int > means that GetParam( ) returns int values, which i use for array size
class SortFloatVector: public ::testing::TestWithParam< int >
{
public:
    //  Create an std and a bolt vector of requested size, and initialize all the elements to 1
    SortFloatVector( ): stdInput( GetParam( ), 1.0f ), boltInput( GetParam( ), 1.0f )
    {}

protected:
    std::vector< float > stdInput, boltInput;
};

//  ::testing::TestWithParam< int > means that GetParam( ) returns int values, which i use for array size
class SortDoubleVector: public ::testing::TestWithParam< int >
{
public:
    //  Create an std and a bolt vector of requested size, and initialize all the elements to 1
    SortDoubleVector( ): stdInput( GetParam( ), 1.0 ), boltInput( GetParam( ), 1.0 )
    {}

protected:
    std::vector< double > stdInput, boltInput;
};

TEST_P( SortIntegerVector, InclusiveInplace )
{
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

TEST_P( SortFloatVector, InclusiveInplace )
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

TEST_P( SortDoubleVector, InclusiveInplace )
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

//  Test lots of consecutive numbers, but small range, suitable for integers because they overflow easier
// INSTANTIATE_TEST_CASE_P( Inclusive, ScanIntegerVector, ::testing::Range( 0, 1024, 1 ) );

//  Test a huge range, suitable for floating point as they are less prone to overflow (but floating point loses granularity at large values)
INSTANTIATE_TEST_CASE_P( Inclusive, SortFloatVector, ::testing::Range( 4096, 1048576, 4096 ) );
INSTANTIATE_TEST_CASE_P( Inclusive, SortDoubleVector, ::testing::Range( 0, 1048576, 4096 ) );

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

//INSTANTIATE_TYPED_TEST_CASE_P( Integer, ScanArrayTest, IntegerTests );
//INSTANTIATE_TYPED_TEST_CASE_P( Float, ScanArrayTest, FloatTests );

int main(int argc, _TCHAR* argv[])
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
#include "myocl.h"

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
    std::vector<stdType> stdInput(length);
    std::vector<stdType> boltInput(length);
    std::vector<stdType>::iterator it; 
    auto func = [](const stdType & a, const stdType & b) {  return a < b;  };

    size_t i;
    for (i=0;i<length;i++)
    {
        boltInput[i]= (stdType)(length - i +2);
        stdInput[i]= (stdType)(length - i +2);
    }
    
    bolt::cl::sort(boltInput.begin(), boltInput.end(), func," [](const stdType & a, const stdType & b) {  return a < b;  };");
    std::sort(stdInput.begin(), stdInput.end(),func); 
    for (i=0; i<length; i++)
    {
        if(stdInput[i] == boltInput[i])
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
    std::vector<stdType> stdInput(length);
    std::vector<stdType> boltInput(length);

    typedef bool (*MyFunction)(stdType &i,stdType &j);
    size_t i;
    for (i=0;i<length;i++)
    {
        boltInput[i] = (stdType)(length - i +2);
        stdInput[i]  = (stdType)(length - i +2);
    }
    MyFunction function = FUNCTION<stdType>;
    std::string functionString("bool FUNCTION(" + std::string(typeid(stdType).name()) + " in1, " + std::string(typeid(stdType).name()) + " in2) { return (in1 < in2); }");
    bolt::cl::sort(boltInput.begin(), boltInput.end(), functionString);
    std::sort(stdInput.begin(), stdInput.end(), function);
    for (i=0; i<length; i++)
    {
        if(stdInput[i] == boltInput[i])
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

    std::vector<myfunctor> stdInput(length);
    std::vector<myfunctor> boltInput(length);

    size_t i;
    for (i=0;i<length;i++)
    {
        boltInput[i].a = (stdType)(i +2);
        stdInput[i].a  = (stdType)(i +2);
    }
    
    bolt::cl::sort(boltInput.begin(), boltInput.end(),bolt::cl::greater<myfunctor>());
    std::sort(stdInput.begin(), stdInput.end(),bolt::cl::greater<myfunctor>());

    for (i=0; i<length; i++)
    {
        if (stdInput[i].a == boltInput[i].a)
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

    std::vector<myfunctor> stdInput(length);
    std::vector<myfunctor> boltInput(length);

    size_t i;
    for (i=0;i<length;i++)
    {
        boltInput[i].a = (stdType)(i +2);
        stdInput[i].a  = (stdType)(i +2);
    }
    
    bolt::cl::sort(boltInput.begin(), boltInput.end(),myfunctor());
    std::sort(stdInput.begin(), stdInput.end(),myfunctor());

    for (i=0; i<length; i++)
    {
        if (stdInput[i].a == boltInput[i].a)
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

    std::vector<mytype> stdInput(length);
    std::vector<mytype> boltInput(length);

    size_t i;
    for (i=0;i<length;i++)
    {
        boltInput[i].a = (stdType)(i +2);
        stdInput[i].a  = (stdType)(i +2);
    }
    
    bolt::cl::sort(boltInput.begin(), boltInput.end(),mytype());
    std::sort(stdInput.begin(), stdInput.end(),mytype());
    for (i=0; i<length; i++)
    {
        if(stdInput[i].a == boltInput[i].a)
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
    std::vector<T> stdInput(length);
    std::vector<T> boltInput(length);
    
    size_t i;
    for (i=0;i<length;i++)
    {
        boltInput[i]= (T)(length - i +2);
        stdInput[i]= (T)(length - i +2);
    }
    
    bolt::cl::sort(boltInput.begin(), boltInput.end());
    std::sort(stdInput.begin(), stdInput.end());
    for (i=0; i<length; i++)
    {
        if(stdInput[i] == boltInput[i])
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
    std::vector<T> stdInput(length);
    bolt::cl::device_vector<T> boltInput(length);

    size_t i;
    for (i=0;i<length;i++)
    {
        boltInput[i]= (T)(length - i +2);
        stdInput[i]= (T)(length - i +2);
    }
    
    bolt::cl::sort(boltInput.begin(), boltInput.end());
    std::sort(stdInput.begin(), stdInput.end());
    for (i=0; i<length; i++)
    {
        if(stdInput[i] == boltInput[i])
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
    std::vector<T> stdInput(length);
    bolt::cl::device_vector<T> boltInput(length);

    size_t i;
    for (i=0;i<length;i++)
    {
        boltInput[i]= (T)(i +2);
        stdInput[i]= (T)(i +2);
    }
    
    bolt::cl::sort(boltInput.begin(), boltInput.end(), bolt::cl::greater<T>());
    std::sort(stdInput.begin(), stdInput.end(), bolt::cl::greater<T>());
    for (i=0; i<length; i++)
    {
        if(stdInput[i] == boltInput[i])
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
    std::vector<mytype> stdInput(length);
    bolt::cl::device_vector<mytype> boltInput(length);

    size_t i;
    for (i=0;i<length;i++)
    {
        //FIX ME - This should work
        //boltInput[i].a = (stdType)(length - i +2);
        stdInput[i].a  = (stdType)(length - i +2);
    }
    
    bolt::cl::sort(boltInput.begin(), boltInput.end());
    std::sort(stdInput.begin(), stdInput.end());
    for (i=0; i<length; i++)
    {
         //FIX ME - stdInput should be changed to boltInput
        if(stdInput[i].a == stdInput[i].a)
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
    std::vector<mytype> stdInput(length);
    bolt::cl::device_vector<mytype> boltInput(length);

    size_t i;
    for (i=0;i<length;i++)
    {
        //FIX ME - This should work        
        //boltInput[i].a = (stdType)(length - i +2);
        stdInput[i].a  = (stdType)(length - i +2);
    }
    
    bolt::cl::sort(boltInput.begin(), boltInput.end(),bolt::cl::less<mytype>());
    std::sort(stdInput.begin(), stdInput.end(),bolt::cl::less<mytype>());
    for (i=0; i<length; i++)
    {
         //FIX ME - stdInput should be changed to boltInput
        if(stdInput[i].a == stdInput[i].a)
            continue;
        else
            break;
    }
    if (i==length)
        std::cout << "Test Passed" <<std::endl;
    else 
        std::cout << "Test Failed i = " << i <<std::endl;
}

void TestWithBoltControl()
{

	MyOclContext ocl = initOcl(CL_DEVICE_TYPE_GPU, 0);
	bolt::cl::control c(ocl._queue);  // construct control structure from the queue.
    //c.debug(bolt::cl::control::debug::Compile + bolt::cl::control::debug::SaveCompilerTemps);
    size_t length = 256;
    typedef MyFunctor<int> myfunctor;
    typedef MyType<int> mytype;

    std::vector<int> boltInput(length);
    std::vector<int> stdInput(length);
    std::vector<myfunctor> myFunctorBoltInput(length);
    std::vector<mytype> myTypeBoltInput(length);
    //bolt::cl::device_vector<int> dvInput(c,length);
    size_t i;

    for (i=0;i<length;i++)
    {
        boltInput[i]= (int)(length - i +2);
        stdInput[i]= (int)(length - i +2);
    }
    for (i=0;i<length;i++)
    {
        myFunctorBoltInput[i].a= (int)(length - i +2);
        myTypeBoltInput[i].a= (int)(length - i +2);
    }

    //BasicSortTestOfLengthWithDeviceVector
    bolt::cl::device_vector<int> dvInput( boltInput.begin(), boltInput.end(), CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE, c);
    bolt::cl::sort(c, dvInput.begin(), dvInput.end());

    std::sort(stdInput.begin(),stdInput.end());
    for (i=0;i<length;i++)
    {
        if(dvInput[i] == stdInput[i])
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
    bolt::cl::sort(c, dvInput.begin(), dvInput.end(),bolt::cl::greater<int>());
    //UserDefinedBoltFunctorSortTestOfLength
    bolt::cl::sort(c, myFunctorBoltInput.begin(), myFunctorBoltInput.end(),bolt::cl::greater<myfunctor>());
    //UserDefinedBoltFunctorSortTestOfLength
    bolt::cl::sort(c, myTypeBoltInput.begin(), myTypeBoltInput.end(),bolt::cl::greater<mytype>());
    return;
}

int main(int argc, char* argv[])
{
		std::vector<int> input(1024);
		std::generate(input.begin(), input.end(), rand);	
		bolt::cl::sort( input.begin(), input.end(), bolt::cl::greater<int>());

	int a[10] = {2, 9, 3, 7, 5, 6, 3, 8, 3, 4};
	bolt::cl::sort( a, a+10, bolt::cl::greater<int>());
    //Test the non Power Of 2 Buffer size 
	//The following two commented codes does not run. It will throw and cl::Error exception
    //BasicSortTestOfLengthWithDeviceVector<int>(254);
    //BasicSortTestWithBoltFunctorOfLengthWithDeviceVector<int>(254); 
    UserDefinedFunctorSortTestOfLength<int>(254);
    UserDefinedBoltFunctorSortTestOfLength<int>(254);
    UserDefinedObjectSortTestOfLength<int>(254);
    BasicSortTestOfLength<int>(254);
    TestWithBoltControl();

    //The following two are not working because device_vector support is not there.
    //UDDSortTestOfLengthWithDeviceVector<int>(256);
    //UDDSortTestWithBoltFunctorOfLengthWithDeviceVector<int>(256);

#define TEST_ALL 1
#define TEST_DOUBLE 1

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
