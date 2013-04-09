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
#include "bolt/amp/transform.h"
#include "bolt/unicode.h"
#include "bolt/miniDump.h"
#include <gtest/gtest.h>
#include <array>
#include "bolt/amp/functional.h"
#include "common/test_common.h"

#define GTEST_TESTS 1
#if !GTEST_TESTS


    template<typename Container>
    static void printA2(const char * msg, const Container &a, const Container &b, int x_size) 
    {
        std::wcout << msg << std::endl;
        for (int i = 0; i < x_size; i++)
            std::wcout << a[i] << "\t" << b[i] << std::endl;
    };

    static void printA(const char * msg, const int *a, int x_size) 
    {
        std::wcout << msg << std::endl;
        for (int i = 0; i < x_size; i++)
            std::wcout << a[i] << std::endl;
    };



    /*
    * Demonstrates:
        * Use of bolt::transform function
        * Bolt delivers same result as stl::transform
        * Bolt syntax is similar to STL transform
        * Works for both integer arrays and STL vectors
        */
    void simpleTransform1()
    {
        const int aSize = 16;
        int a[aSize] = {4,0,5,5,0,5,5,1,3,1,0,3,1,1,3,5};
        int b[aSize] = {1,9,0,8,6,1,7,7,1,0,1,3,5,7,9,8};
        int out[aSize];
        std::transform(a,a+aSize, out, std::negate<int>());
        bolt::amp::transform(a, a+aSize, out, bolt::negate<int>());
        printA2("Transform Neg - From Pointer", a, out, aSize);

        bolt::amp::transform(a, a+aSize, b, out, bolt::plus<int>());
        printA("\nTransformVaddOut", out, aSize);

        static const int vSz=16;
        std::vector<int> vec(16);
        std::generate(vec.begin(), vec.end(), rand);
        std::vector<int> outVec(16);
        //std::transform(vec.begin(),vec.end(), outVec.begin(), std::negate<int>());
        bolt::amp::transform(vec.begin(),vec.end(), outVec.begin(), bolt::negate<int>());
        std::cout<<"Printing";
        for(unsigned int i=0; i < 16; i++)
            std::cout<<outVec[i]<<std::endl;

    

    #if 0
        // Same as above but using lamda rather than standard "plus" functor:
        // Doesn't compile in Dev11 Preview due to compiler bug, should be fixed in newer rev.
        // FIXME- try with new C++AMP compiler.
        bolt::transform(a, a+aSize, b, out, [&](int x, int y)
        {
            return x+y;
        });
        printA("\nTransformVaddOut-Lambda", out, aSize);
    #endif
    };


    /* Demostrates:
    * Bolt works for template arguments, ie int, float
    */
    template<typename T>
    void simpleTransform2(const int sz) 
    {
        std::vector<T> A(sz);
        std::vector<T> S(sz);
        std::vector<T> B(sz);

        for (int i=0; i < sz; i++) {
            //A[i] = T(i);     // sequential assignment
            A[i] = T(rand())/137;  // something a little more exciting.
        };

        std::transform (A.begin(), A.end(), S.begin(), std::negate<T>());  // single-core CPU version
        bolt::amp::transform(A.begin(), A.end(), B.begin(), bolt::negate<T>()); // bolt version on GPU or mcore CPU.
    
        // Check result:
        const int maxErrCount = 10;
        int errCount = 0;
        for (unsigned x=0; x< S.size(); x++) {
            const T s = S[x];
            const T b = B[x];
            //std::cout << s << "," << b << std::endl;
            if ((s != b) && (++errCount < maxErrCount)) {
                std::cout << "ERROR#" << errCount << " " << s << "!=" << b << std::endl;
            };
        };
    };


    //// Show use of Saxpy Functor object.
    //struct SaxpyFunctor
    //{
       // float _a;
       // SaxpyFunctor(float a) : _a(a) {};

       // float operator() (const float &xx, const float &yy) restrict(cpu,amp)
       // {
          //  return _a * xx + yy;
       // };
    
    //};


    //void transformSaxpy(int aSize)
    //{
       // std::string fName = __FUNCTION__ ;
       // fName += ":";

       // std::vector<float> A(aSize), B(aSize), Z1(aSize), Z0(aSize);

       // for (int i=0; i<aSize; i++) {
          //  A[i] = float(i);
          //  B[i] = 10000.0f + (float)i;
       // }

       // SaxpyFunctor sb(10.0);

       // std::transform(A.begin(), A.end(), B.begin(), Z0.begin(), sb);
       // bolt::amp::transform(A.begin(), A.end(), B.begin(), Z1.begin(), sb);  

       // //checkResults(fName, Z0.begin(), Z0.end(), Z1.begin());

    //};

    void simpleTransform()
    {
        simpleTransform1();
        simpleTransform2<int>(128);
        simpleTransform2<float>(1000);
        simpleTransform2<float>(100000);

       // transformSaxpy(256);
    };


    int _tmain(int argc, _TCHAR* argv[])
    {
        simpleTransform();
        return 0;
    }


#else

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
class TransformArrayTest: public ::testing::Test
{
public:
    TransformArrayTest( ): m_Errors( 0 )
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

    virtual ~TransformArrayTest( )
    {}

protected:
    typedef typename std::tuple_element< 0, ArrayTuple >::type ArrayType;
    static const size_t ArraySize = typename std::tuple_element< 1, ArrayTuple >::type::value;

    typename std::array< ArrayType, ArraySize > stdInput, boltInput;
    int m_Errors;
};

template< typename ArrayTuple >
class TransformBinaryArrayTest: public ::testing::Test
{
public:
    TransformBinaryArrayTest( ): m_Errors( 0 )
    {}

    virtual void SetUp( )
    {
        for( int i=0; i < ArraySize; i++ )
        {
            stdInput1[ i ] = 1;
            stdInput2[ i ] = 1;
            stdOutput[ i ] = 0;
            boltInput1[ i ] = 1;
            boltInput2[ i ] = 1;
            boltOutput[ i ] = 0;
        }
    };

    virtual void TearDown( )
    {};

    virtual ~TransformBinaryArrayTest( )
    {}

protected:
    typedef typename std::tuple_element< 0, ArrayTuple >::type ArrayType;
    static const size_t ArraySize = typename std::tuple_element< 1, ArrayTuple >::type::value;

    typename std::array< ArrayType, ArraySize > stdInput1, stdInput2, stdOutput,boltInput1, boltInput2, boltOutput;
    int m_Errors;
};

template< typename ArrayTuple >
class TransformOutPlaceArrayTest: public ::testing::Test
{
public:
    TransformOutPlaceArrayTest( ): m_Errors( 0 )
    {}

    virtual void SetUp( )
    {
        for( int i=0; i < ArraySize; i++ )
        {
            stdInput[ i ] = 1;
            stdOutput[ i ] = 0;
            boltInput[ i ] = 1;
            boltOutput[ i ] = 0;
        }
    };

    virtual void TearDown( )
    {};

    virtual ~TransformOutPlaceArrayTest( )
    {}

protected:
    typedef typename std::tuple_element< 0, ArrayTuple >::type ArrayType;
    static const size_t ArraySize = typename std::tuple_element< 1, ArrayTuple >::type::value;

    typename std::array< ArrayType, ArraySize > stdInput, stdOutput, boltInput, boltOutput;
    int m_Errors;
};

//  Explicit initialization of the C++ static const
template< typename ArrayTuple >
const size_t TransformArrayTest< ArrayTuple >::ArraySize;

template< typename ArrayTuple >
const size_t TransformBinaryArrayTest< ArrayTuple >::ArraySize;

template< typename ArrayTuple >
const size_t TransformOutPlaceArrayTest< ArrayTuple >::ArraySize;


TYPED_TEST_CASE_P( TransformArrayTest );
TYPED_TEST_CASE_P( TransformBinaryArrayTest );
TYPED_TEST_CASE_P( TransformOutPlaceArrayTest );

TYPED_TEST_P( TransformArrayTest, InPlaceNegateTransform )
{
    typedef std::array< ArrayType, ArraySize > ArrayCont;

    //  Calling the actual functions under test
    std::transform( stdInput.begin( ), stdInput.end( ), stdInput.begin( ), std::negate<ArrayType>() );
    bolt::amp::transform( boltInput.begin( ), boltInput.end( ), boltInput.begin( ), bolt::amp::negate<ArrayType>() );

    ArrayCont::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end( ) );
    ArrayCont::difference_type boltNumElements = std::distance( boltInput.begin( ), boltInput.end( ) );

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpStdArray< ArrayType, ArraySize >::cmpArrays( stdInput, boltInput );
}

TYPED_TEST_P( TransformBinaryArrayTest, BinaryPlusTransform )
{
    typedef std::array< ArrayType, ArraySize > ArrayCont;

    //  Calling the actual functions under test
    std::transform( stdInput1.begin( ), stdInput1.end( ), stdInput2.begin( ), stdOutput.begin(),std::plus< ArrayType >( ) );
    bolt::amp::transform( boltInput1.begin( ), boltInput1.end( ), boltInput2.begin( ), boltOutput.begin(), bolt::amp::plus< ArrayType >( ) );

    ArrayCont::difference_type stdNumElements = std::distance( stdOutput.begin( ), stdOutput.end() );
    ArrayCont::difference_type boltNumElements = std::distance( boltOutput.begin( ), boltOutput.end() );

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpStdArray< ArrayType, ArraySize >::cmpArrays( stdOutput, boltOutput );
}

TYPED_TEST_P( TransformBinaryArrayTest, BinaryMultipliesTransform )
{
    typedef std::array< ArrayType, ArraySize > ArrayCont;

    //  Calling the actual functions under test
    std::transform( stdInput1.begin( ), stdInput1.end( ), stdInput2.begin( ), stdOutput.begin(),std::multiplies< ArrayType >( ) );
    bolt::amp::transform( boltInput1.begin( ), boltInput1.end( ), boltInput2.begin( ), boltOutput.begin(), bolt::amp::multiplies< ArrayType >( ) );

    ArrayCont::difference_type stdNumElements = std::distance( stdOutput.begin( ), stdOutput.end() );
    ArrayCont::difference_type boltNumElements = std::distance( boltOutput.begin( ), boltOutput.end() );

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpStdArray< ArrayType, ArraySize >::cmpArrays( stdOutput, boltOutput );
}

TYPED_TEST_P( TransformBinaryArrayTest, BinaryMaxTransform )
{
    typedef std::array< ArrayType, ArraySize > ArrayCont;

    //  Calling the actual functions under test
    std::transform( stdInput1.begin( ), stdInput1.end( ), stdInput2.begin( ), stdOutput.begin(), [] (ArrayType lhs, ArrayType rhs) { return rhs > lhs ? rhs:lhs; } );
    bolt::amp::transform( boltInput1.begin( ), boltInput1.end( ), boltInput2.begin( ), boltOutput.begin(), bolt::amp::maximum< ArrayType >( ) );

    ArrayCont::difference_type stdNumElements = std::distance( stdOutput.begin( ), stdOutput.end() );
    ArrayCont::difference_type boltNumElements = std::distance( boltOutput.begin( ), boltOutput.end() );

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpStdArray< ArrayType, ArraySize >::cmpArrays( stdOutput, boltOutput );
}

TYPED_TEST_P( TransformOutPlaceArrayTest, OutPlaceSquareTransform )
{
    typedef std::array< ArrayType, ArraySize > ArrayCont;

    //  Calling the actual functions under test
    std::transform( stdInput.begin( ), stdInput.end( ), stdOutput.begin(), []( ArrayType x ) {return x*x;} );
    bolt::amp::transform( boltInput.begin( ), boltInput.end( ), boltOutput.begin(), bolt::amp::square< ArrayType >( ) );


    ArrayCont::difference_type stdNumElements = std::distance( stdOutput.begin(), stdOutput.end() );
    ArrayCont::difference_type boltNumElements = std::distance( boltOutput.begin(), boltOutput.end() );

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpStdArray< ArrayType, ArraySize >::cmpArrays( stdOutput, boltOutput );
}





REGISTER_TYPED_TEST_CASE_P( TransformArrayTest, InPlaceNegateTransform  );
REGISTER_TYPED_TEST_CASE_P( TransformOutPlaceArrayTest, OutPlaceSquareTransform );
REGISTER_TYPED_TEST_CASE_P( TransformBinaryArrayTest, BinaryPlusTransform, BinaryMultipliesTransform, BinaryMaxTransform);


/////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Fixture classes are now defined to enable googletest to process value parameterized tests

//  ::testing::TestWithParam< int > means that GetParam( ) returns int values, which i use for array size
class TransformIntegerVector: public ::testing::TestWithParam< int >
{
public:
    //  Create an std and a bolt vector of requested size, and initialize all the elements to 1
    TransformIntegerVector( ): stdInput( GetParam( ), 1 ), boltInput( GetParam( ), 1 )
    {}

protected:
    std::vector< int > stdInput, boltInput;
};

//  ::testing::TestWithParam< int > means that GetParam( ) returns int values, which i use for array size
class TransformFloatVector: public ::testing::TestWithParam< int >
{
public:
    //  Create an std and a bolt vector of requested size, and initialize all the elements to 1
    TransformFloatVector( ): stdInput( GetParam( ), 1.0f ), boltInput( GetParam( ), 1.0f )
    {}

protected:
    std::vector< float > stdInput, boltInput;
};

//  ::testing::TestWithParam< int > means that GetParam( ) returns int values, which i use for array size
class TransformDoubleVector: public ::testing::TestWithParam< int >
{
public:
    //  Create an std and a bolt vector of requested size, and initialize all the elements to 1
    TransformDoubleVector( ): stdInput( GetParam( ), 1.0 ), boltInput( GetParam( ), 1.0 )
    {}

protected:
    std::vector< double > stdInput, boltInput;
};

TEST_P( TransformIntegerVector, InplaceTransform )
{
    //  Calling the actual functions under test
    std::transform( stdInput.begin(), stdInput.end(), stdInput.begin(), std::negate<int>());
    bolt::amp::transform( boltInput.begin( ), boltInput.end( ), boltInput.begin( ), bolt::amp::negate<int>() );



    std::vector< int >::iterator::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end() );
    std::vector< int >::iterator::difference_type boltNumElements = std::distance( boltInput.begin( ), boltInput.end());

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdInput, boltInput );
}

TEST_P( TransformFloatVector, InplaceTransform )
{
    //  Calling the actual functions under test
    std::transform( stdInput.begin( ), stdInput.end( ), stdInput.begin( ), std::negate<float>() );
    bolt::amp::transform( boltInput.begin( ), boltInput.end( ), boltInput.begin( ), bolt::amp::negate<float>() );


    std::vector< float >::iterator::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end() );
    std::vector< float >::iterator::difference_type boltNumElements = std::distance( boltInput.begin( ), boltInput.end() );

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdInput, boltInput );
}

TEST_P( TransformFloatVector, InplaceSquareTransform )
{
    //  Calling the actual functions under test
  std::transform( stdInput.begin( ), stdInput.end( ), stdInput.begin( ), [](float x){return x*x;} );
    bolt::amp::transform( boltInput.begin( ), boltInput.end( ), boltInput.begin( ), bolt::amp::square<float>() );


    std::vector< float >::iterator::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end() );
    std::vector< float >::iterator::difference_type boltNumElements = std::distance( boltInput.begin( ), boltInput.end() );

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdInput, boltInput );
}

TEST_P( TransformFloatVector, InplaceCubeTransform )
{
    //  Calling the actual functions under test
  std::transform( stdInput.begin( ), stdInput.end( ), stdInput.begin( ), [](float x){return x*x*x;} );
    bolt::amp::transform( boltInput.begin( ), boltInput.end( ), boltInput.begin( ), bolt::amp::cube<float>() );


    std::vector< float >::iterator::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end() );
    std::vector< float >::iterator::difference_type boltNumElements = std::distance( boltInput.begin( ), boltInput.end() );

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdInput, boltInput );
}




TEST_P( TransformDoubleVector, InplaceTransform )
{
    //  Calling the actual functions under test
    std::transform( stdInput.begin( ), stdInput.end( ), stdInput.begin( ), std::negate<double>() );
    bolt::amp::transform( boltInput.begin( ), boltInput.end( ), boltInput.begin( ), bolt::amp::negate<double>() );


    std::vector< int >::iterator::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end() );
    std::vector< int >::iterator::difference_type boltNumElements = std::distance( boltInput.begin( ), boltInput.end() );

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdInput, boltInput );
}

TEST( TransformDeviceVector, OutOfPlaceTransform)
{
  int length = 1<<8;
  std::vector<int> hVectorA( length ), hVectorB( length ), hVectorO( length );
  std::fill( hVectorA.begin(), hVectorA.end(), 1024 );
  std::fill( hVectorB.begin(), hVectorB.end(), 1024 );
  std::fill( hVectorB.begin(), hVectorB.end(), 0 );

  bolt::amp::device_vector<int, concurrency::array> dVectorA(hVectorA.begin(), hVectorA.end()),
    dVectorB(hVectorB.begin(), hVectorB.end()),
    dVectorO(hVectorO.begin(), hVectorO.end());

  std::transform( hVectorA.begin(), hVectorA.end(), hVectorB.begin(), hVectorO.begin(), std::plus< int >( ) );
  bolt::amp::transform( dVectorA.begin(),
    dVectorA.end(),
    dVectorB.begin(),
    dVectorO.begin(),
    bolt::amp::plus< int >( ) );
  
  cmpArrays(hVectorO, dVectorO);



}

//  Test lots of consecutive numbers, but small range, suitable for integers because they overflow easier
INSTANTIATE_TEST_CASE_P( TransformInPlace, TransformIntegerVector, ::testing::Range( 0, 1024, 1 ) );

//  Test a huge range, suitable for floating point as they are less prone to overflow (but floating point loses granularity at large values)
INSTANTIATE_TEST_CASE_P( TransformInPlace, TransformFloatVector, ::testing::Range( 0, 1048576, 4096 ) );
INSTANTIATE_TEST_CASE_P( InplaceSquareTransform, TransformFloatVector, ::testing::Range( 0, 1048576, 4096 ) );
INSTANTIATE_TEST_CASE_P( InplaceCubeTransform, TransformFloatVector, ::testing::Range( 0, 1048576, 4096 ) );

INSTANTIATE_TEST_CASE_P( TransformInPlace, TransformDoubleVector, ::testing::Range( 0, 1048576, 4096 ) );
INSTANTIATE_TEST_CASE_P( InplaceTransform, TransformDoubleVector, ::testing::Range( 0, 1048576, 4096 ) );

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

INSTANTIATE_TYPED_TEST_CASE_P( Integer, TransformArrayTest, IntegerTests );
INSTANTIATE_TYPED_TEST_CASE_P( Float, TransformArrayTest, FloatTests );
INSTANTIATE_TYPED_TEST_CASE_P( Integer, TransformBinaryArrayTest, IntegerTests );
INSTANTIATE_TYPED_TEST_CASE_P( Float, TransformBinaryArrayTest, FloatTests );
INSTANTIATE_TYPED_TEST_CASE_P( Integer, TransformOutPlaceArrayTest, IntegerTests );
INSTANTIATE_TYPED_TEST_CASE_P( Float, TransformOutPlaceArrayTest, FloatTests );


int main(int argc, char* argv[])
{
    ::testing::InitGoogleTest( &argc, &argv[ 0 ] );

    //    Set the standard OpenCL wait behavior to help debugging
    bolt::amp::control& myControl = bolt::amp::control::getDefault( );
    myControl.setWaitMode( bolt::amp::control::NiceWait );
    myControl.setForceRunMode( bolt::amp::control::Automatic );  // choose tbb


    int retVal = RUN_ALL_TESTS( );

#ifdef BUILD_TBB

    bolt::amp::control& myControl = bolt::amp::control::getDefault( );
    myControl.setWaitMode( bolt::amp::control::NiceWait );
    myControl.setForceRunMode( bolt::amp::control::MultiCoreCpu );  // choose tbb


    int retVal = RUN_ALL_TESTS( );

#endif
    
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