/***************************************************************************                                                                                     
*   © 2012,2014 Advanced Micro Devices, Inc. All rights reserved.                                     
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
#include <bolt/amp/iterator/constant_iterator.h>
#include <bolt/amp/iterator/counting_iterator.h>
#define TEST_DOUBLE 1
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

//protected:

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
TYPED_TEST_P( TransformArrayTest, SerialInPlaceNegateTransform )
{
    typedef std::array< ArrayType, ArraySize > ArrayCont;

    bolt::amp::control ctl = bolt::amp::control::getDefault( );
    ctl.setForceRunMode(bolt::amp::control::SerialCpu);
    
    //  Calling the actual functions under test
    std::transform( stdInput.begin( ), stdInput.end( ), stdInput.begin( ), std::negate<ArrayType>() );
    bolt::amp::transform(ctl, boltInput.begin( ), boltInput.end( ), boltInput.begin( ),bolt::amp::negate<ArrayType>());

    ArrayCont::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end( ) );
    ArrayCont::difference_type boltNumElements = std::distance( boltInput.begin( ), boltInput.end( ) );

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpStdArray< ArrayType, ArraySize >::cmpArrays( stdInput, boltInput );
}
TYPED_TEST_P( TransformArrayTest, MulticoreInPlaceNegateTransform )
{
    typedef std::array< ArrayType, ArraySize > ArrayCont;

    bolt::amp::control ctl = bolt::amp::control::getDefault( );
    ctl.setForceRunMode(bolt::amp::control::MultiCoreCpu); 

    //  Calling the actual functions under test
    std::transform( stdInput.begin( ), stdInput.end( ), stdInput.begin( ), std::negate<ArrayType>() );
    bolt::amp::transform(ctl, boltInput.begin( ), boltInput.end( ), boltInput.begin( ),bolt::amp::negate<ArrayType>());

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
    std::transform( stdInput1.begin( ), stdInput1.end( ), stdInput2.begin( ), stdOutput.begin(),
                                                                    std::plus< ArrayType >( ) );
    bolt::amp::transform( boltInput1.begin( ), boltInput1.end( ), boltInput2.begin( ), boltOutput.begin(), 
                                                                        bolt::amp::plus< ArrayType >( ) );

    ArrayCont::difference_type stdNumElements = std::distance( stdOutput.begin( ), stdOutput.end() );
    ArrayCont::difference_type boltNumElements = std::distance( boltOutput.begin( ), boltOutput.end() );

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpStdArray< ArrayType, ArraySize >::cmpArrays( stdOutput, boltOutput );
}
TYPED_TEST_P( TransformBinaryArrayTest, SerialBinaryPlusTransform )
{
    typedef std::array< ArrayType, ArraySize > ArrayCont;

    bolt::amp::control ctl = bolt::amp::control::getDefault( );
    ctl.setForceRunMode(bolt::amp::control::SerialCpu); 

    //  Calling the actual functions under test
    std::transform( stdInput1.begin( ), stdInput1.end( ), stdInput2.begin( ), stdOutput.begin(),
                                                                    std::plus< ArrayType >( ) );
    bolt::amp::transform(ctl, boltInput1.begin( ), boltInput1.end( ), boltInput2.begin( ), boltOutput.begin(), 
                                                                            bolt::amp::plus< ArrayType >( ) );

    ArrayCont::difference_type stdNumElements = std::distance( stdOutput.begin( ), stdOutput.end() );
    ArrayCont::difference_type boltNumElements = std::distance( boltOutput.begin( ), boltOutput.end() );

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpStdArray< ArrayType, ArraySize >::cmpArrays( stdOutput, boltOutput );
}
TYPED_TEST_P( TransformBinaryArrayTest, MulticoreBinaryPlusTransform )
{
    typedef std::array< ArrayType, ArraySize > ArrayCont;

    bolt::amp::control ctl = bolt::amp::control::getDefault( );
    ctl.setForceRunMode(bolt::amp::control::MultiCoreCpu); 

    //  Calling the actual functions under test
    std::transform( stdInput1.begin( ), stdInput1.end( ), stdInput2.begin( ), stdOutput.begin(),
                                                                    std::plus< ArrayType >( ) );
    bolt::amp::transform(ctl, boltInput1.begin( ), boltInput1.end( ), boltInput2.begin( ), boltOutput.begin(),
                                                                            bolt::amp::plus< ArrayType >( ) );

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
    std::transform( stdInput1.begin( ), stdInput1.end( ), stdInput2.begin( ), stdOutput.begin(),
                                                                std::multiplies< ArrayType >( ) );
    bolt::amp::transform( boltInput1.begin( ), boltInput1.end( ), boltInput2.begin( ), boltOutput.begin(), 
                                                                bolt::amp::multiplies< ArrayType >( ) );

    ArrayCont::difference_type stdNumElements = std::distance( stdOutput.begin( ), stdOutput.end() );
    ArrayCont::difference_type boltNumElements = std::distance( boltOutput.begin( ), boltOutput.end() );

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpStdArray< ArrayType, ArraySize >::cmpArrays( stdOutput, boltOutput );
}
TYPED_TEST_P( TransformBinaryArrayTest, SerialBinaryMultipliesTransform )
{
    typedef std::array< ArrayType, ArraySize > ArrayCont;

    bolt::amp::control ctl = bolt::amp::control::getDefault( );
    ctl.setForceRunMode(bolt::amp::control::SerialCpu); 

    //  Calling the actual functions under test
    std::transform( stdInput1.begin( ), stdInput1.end( ), stdInput2.begin( ), stdOutput.begin(),
                                                             std::multiplies< ArrayType >( ) );
    bolt::amp::transform(ctl, boltInput1.begin( ), boltInput1.end( ), boltInput2.begin( ), boltOutput.begin(),
                                                                        bolt::amp::multiplies< ArrayType >( ) );

    ArrayCont::difference_type stdNumElements = std::distance( stdOutput.begin( ), stdOutput.end() );
    ArrayCont::difference_type boltNumElements = std::distance( boltOutput.begin( ), boltOutput.end() );

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpStdArray< ArrayType, ArraySize >::cmpArrays( stdOutput, boltOutput );
}
TYPED_TEST_P( TransformBinaryArrayTest, MulticoreBinaryMultipliesTransform )
{
    typedef std::array< ArrayType, ArraySize > ArrayCont;
    
    bolt::amp::control ctl = bolt::amp::control::getDefault( );
    ctl.setForceRunMode(bolt::amp::control::MultiCoreCpu); 

    //  Calling the actual functions under test
    std::transform( stdInput1.begin( ), stdInput1.end( ), stdInput2.begin( ), stdOutput.begin(),
                                                                std::multiplies< ArrayType >( ) );
    bolt::amp::transform(ctl, boltInput1.begin( ), boltInput1.end( ), boltInput2.begin( ), boltOutput.begin(), 
                                                                    bolt::amp::multiplies< ArrayType >( ) );

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
    std::transform( stdInput1.begin( ), stdInput1.end( ), stdInput2.begin( ), stdOutput.begin(), [] (ArrayType lhs, 
                                                                    ArrayType rhs) { return rhs > lhs ? rhs:lhs; } );
    bolt::amp::transform( boltInput1.begin( ), boltInput1.end( ), boltInput2.begin( ), boltOutput.begin(), 
                                                                     bolt::amp::maximum< ArrayType >( ) );

    ArrayCont::difference_type stdNumElements = std::distance( stdOutput.begin( ), stdOutput.end() );
    ArrayCont::difference_type boltNumElements = std::distance( boltOutput.begin( ), boltOutput.end() );

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpStdArray< ArrayType, ArraySize >::cmpArrays( stdOutput, boltOutput );
}
TYPED_TEST_P( TransformBinaryArrayTest, SerialBinaryMaxTransform )
{
    typedef std::array< ArrayType, ArraySize > ArrayCont;
    
    bolt::amp::control ctl = bolt::amp::control::getDefault( );
    ctl.setForceRunMode(bolt::amp::control::SerialCpu); 

    //  Calling the actual functions under test
    std::transform( stdInput1.begin( ), stdInput1.end( ), stdInput2.begin( ), stdOutput.begin(), [] (ArrayType lhs,
                                                                ArrayType rhs) { return rhs > lhs ? rhs:lhs; } );
    bolt::amp::transform(ctl, boltInput1.begin( ), boltInput1.end( ), boltInput2.begin( ), boltOutput.begin(),
                                                                        bolt::amp::maximum< ArrayType >( ) );

    ArrayCont::difference_type stdNumElements = std::distance( stdOutput.begin( ), stdOutput.end() );
    ArrayCont::difference_type boltNumElements = std::distance( boltOutput.begin( ), boltOutput.end() );

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpStdArray< ArrayType, ArraySize >::cmpArrays( stdOutput, boltOutput );
}
TYPED_TEST_P( TransformBinaryArrayTest, MulticoreBinaryMaxTransform )
{
    typedef std::array< ArrayType, ArraySize > ArrayCont;
    
    bolt::amp::control ctl = bolt::amp::control::getDefault( );
    ctl.setForceRunMode(bolt::amp::control::MultiCoreCpu); 

    //  Calling the actual functions under test
    std::transform( stdInput1.begin( ), stdInput1.end( ), stdInput2.begin( ), stdOutput.begin(), [] (ArrayType lhs,
                                                                ArrayType rhs) { return rhs > lhs ? rhs:lhs; } );
    bolt::amp::transform(ctl, boltInput1.begin( ), boltInput1.end( ), boltInput2.begin( ), boltOutput.begin(),
                                                                        bolt::amp::maximum< ArrayType >( ) );

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
    bolt::amp::transform( boltInput.begin( ), boltInput.end( ), boltOutput.begin(), bolt::amp::square< ArrayType >());


    ArrayCont::difference_type stdNumElements = std::distance( stdOutput.begin(), stdOutput.end() );
    ArrayCont::difference_type boltNumElements = std::distance( boltOutput.begin(), boltOutput.end() );

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpStdArray< ArrayType, ArraySize >::cmpArrays( stdOutput, boltOutput );
}
TYPED_TEST_P( TransformOutPlaceArrayTest, SerialOutPlaceSquareTransform )
{
    typedef std::array< ArrayType, ArraySize > ArrayCont;
    
    bolt::amp::control ctl = bolt::amp::control::getDefault( );
    ctl.setForceRunMode(bolt::amp::control::SerialCpu); 

    //  Calling the actual functions under test
    std::transform( stdInput.begin( ), stdInput.end( ), stdOutput.begin(), []( ArrayType x ) {return x*x;} );
    bolt::amp::transform(ctl, boltInput.begin( ), boltInput.end( ), boltOutput.begin(), 
                                                    bolt::amp::square< ArrayType >( ) );


    ArrayCont::difference_type stdNumElements = std::distance( stdOutput.begin(), stdOutput.end() );
    ArrayCont::difference_type boltNumElements = std::distance( boltOutput.begin(), boltOutput.end() );

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpStdArray< ArrayType, ArraySize >::cmpArrays( stdOutput, boltOutput );
}
TYPED_TEST_P( TransformOutPlaceArrayTest, MulticoreOutPlaceSquareTransform )
{
    typedef std::array< ArrayType, ArraySize > ArrayCont;
    
    bolt::amp::control ctl = bolt::amp::control::getDefault( );
    ctl.setForceRunMode(bolt::amp::control::MultiCoreCpu); 

    //  Calling the actual functions under test
    std::transform( stdInput.begin( ), stdInput.end( ), stdOutput.begin(), []( ArrayType x ) {return x*x;} );
    bolt::amp::transform(ctl, boltInput.begin( ), boltInput.end( ), boltOutput.begin(),
                                                    bolt::amp::square< ArrayType >( ) );


    ArrayCont::difference_type stdNumElements = std::distance( stdOutput.begin(), stdOutput.end() );
    ArrayCont::difference_type boltNumElements = std::distance( boltOutput.begin(), boltOutput.end() );

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpStdArray< ArrayType, ArraySize >::cmpArrays( stdOutput, boltOutput );
}


struct UDD
{
    int a; 
    int b;

    bool operator() (const UDD& lhs, const UDD& rhs) const restrict(amp,cpu){
        return ((lhs.a+lhs.b) > (rhs.a+rhs.b));
    } 
    bool operator < (const UDD& other) const restrict(amp,cpu){
        return ((a+b) < (other.a+other.b));
    }
    bool operator > (const UDD& other) const restrict(amp,cpu){
        return ((a+b) > (other.a+other.b));
    }
    bool operator == (const UDD& other) const restrict(amp,cpu) {
        return ((a+b) == (other.a+other.b));
    }

    UDD operator + (const UDD &rhs) const restrict(amp,cpu)
    {
      UDD _result;
      _result.a = a + rhs.a;
      _result.b = b + rhs.b;
      return _result;
    }

	UDD operator-() const restrict(amp, cpu)
    {
        UDD r;
        r.a = -a;
        r.b = -b;
        return r;
    }


    UDD() restrict(amp,cpu)
        : a(0),b(0) { }
    UDD(int _in) restrict(amp,cpu)
        : a(_in), b(_in +1)  { }
};



REGISTER_TYPED_TEST_CASE_P( TransformArrayTest, InPlaceNegateTransform, SerialInPlaceNegateTransform,
                                                                   MulticoreInPlaceNegateTransform );
REGISTER_TYPED_TEST_CASE_P( TransformOutPlaceArrayTest, OutPlaceSquareTransform, SerialOutPlaceSquareTransform, 
                                                                           MulticoreOutPlaceSquareTransform );
REGISTER_TYPED_TEST_CASE_P( TransformBinaryArrayTest, BinaryPlusTransform, SerialBinaryPlusTransform,
                            MulticoreBinaryPlusTransform, BinaryMultipliesTransform, 
                            SerialBinaryMultipliesTransform, MulticoreBinaryMultipliesTransform,
                            BinaryMaxTransform, SerialBinaryMaxTransform, MulticoreBinaryMaxTransform);


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

class TransformIntegerDeviceVector: public ::testing::TestWithParam< int >
{
public:
    // Create an std and a bolt vector of requested size, and initialize all the elements to 1
    TransformIntegerDeviceVector( ): stdInput( GetParam( ), 1 ), boltInput(static_cast<size_t>(GetParam( )), 1 )
    {
        std::generate(stdInput.begin(), stdInput.end(), rand);
        //boltInput = stdInput;      
        //FIXME - The above should work but the below loop is used. 
        for (int i=0; i< GetParam( ); i++)
        {
             boltInput[i] = stdInput[i];
          
        }
    }

protected:
    std::vector< int > stdInput;
    bolt::amp::device_vector< int > boltInput;
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


class TransformFloatDeviceVector: public ::testing::TestWithParam< int >
{
public:
    // Create an std and a bolt vector of requested size, and initialize all the elements to 1
    TransformFloatDeviceVector( ): stdInput( GetParam( ), 1.0f ), boltInput( static_cast<size_t>( GetParam( ) ), 1.0f )
    {
        std::generate(stdInput.begin(), stdInput.end(), rand);
        //boltInput = stdInput;      
        //FIXME - The above should work but the below loop is used. 
        for (int i=0; i< GetParam( ); i++)
        {
            boltInput[i] = stdInput[i];
        }

    }

protected:
    std::vector< float > stdInput;
    bolt::amp::device_vector< float > boltInput;
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


#if (TEST_DOUBLE == 1)
//  ::testing::TestWithParam< int > means that GetParam( ) returns int values, which i use for array size
class TransformDoubleDeviceVector: public ::testing::TestWithParam< int >
{
public:
    // Create an std and a bolt vector of requested size, and initialize all the elements to 1
    TransformDoubleDeviceVector( ): stdInput( GetParam( ), 1.0 ), boltInput( static_cast<size_t>( GetParam( ) ), 1.0 )
    {
        std::generate(stdInput.begin(), stdInput.end(), rand);
        //boltInput = stdInput;      
        //FIXME - The above should work but the below loop is used. 
        for (int i=0; i< GetParam( ); i++)
        {
            boltInput[i] = stdInput[i];
        }
    }

protected:
    std::vector< double > stdInput;
    bolt::amp::device_vector< double > boltInput;
};
#endif


TEST( HostIntVector, OffsetTransform )
{
    int length = 1024;

    std::vector<int> stdInput( length ,1);
    std::vector<int> boltInput(stdInput.begin(),stdInput.end());

    int offset = 100;

    std::transform(  stdInput.begin( ) + offset,  stdInput.end( ), stdInput.begin() + offset, std::negate<int>() );
    bolt::amp::transform( boltInput.begin( ) + offset, boltInput.end( ), boltInput.begin( ) + offset, bolt::amp::negate<int>() );

    cmpArrays( stdInput, boltInput);

}

TEST( HostIntVector, SerialOffsetTransform  )
{
    int length = 1024;

    std::vector<int> stdInput( length ,1);
    std::vector<int> boltInput(stdInput.begin(),stdInput.end());

    int offset = 100;

    bolt::amp::control ctl = bolt::amp::control::getDefault( );
    ctl.setForceRunMode(bolt::amp::control::SerialCpu); 

    std::transform(  stdInput.begin( ) + offset,  stdInput.end( ), stdInput.begin() + offset, std::negate<int>() );
    bolt::amp::transform(ctl, boltInput.begin( ) + offset, boltInput.end( ), boltInput.begin( ) + offset, bolt::amp::negate<int>() );

    cmpArrays( stdInput, boltInput);

}

TEST( HostIntVector, MulticoreOffsetTransform  )
{
    int length = 1024;

    std::vector<int> stdInput( length ,1);
    std::vector<int> boltInput(stdInput.begin(),stdInput.end());

    int offset = 100;

    bolt::amp::control ctl = bolt::amp::control::getDefault( );
    ctl.setForceRunMode(bolt::amp::control::MultiCoreCpu); 

    std::transform(  stdInput.begin( ) + offset,  stdInput.end( ), stdInput.begin() + offset, std::negate<int>() );
    bolt::amp::transform(ctl, boltInput.begin( ) + offset, boltInput.end( ), boltInput.begin( ) + offset, bolt::amp::negate<int>() );

    cmpArrays( stdInput, boltInput);

}

TEST( DVIntVector, OffsetTransform  )
{
    int length = 1024;

    std::vector<int> stdInput( length ,1);
    bolt::amp::device_vector<int> boltInput(stdInput.begin(),stdInput.end());

    int offset = 100;

    std::transform(  stdInput.begin( ) + offset,  stdInput.end( ), stdInput.begin() + offset, std::negate<int>() );
    bolt::amp::transform( boltInput.begin( ) + offset, boltInput.end( ), boltInput.begin( ) + offset, bolt::amp::negate<int>() );

    cmpArrays( stdInput, boltInput);
    
}

TEST( DVIntVector, SerialOffsetTransform  )
{
    int length = 1024;

    std::vector<int> stdInput( length ,1);
    bolt::amp::device_vector<int> boltInput(stdInput.begin(),stdInput.end());

    int offset = 100;

    bolt::amp::control ctl = bolt::amp::control::getDefault( );
    ctl.setForceRunMode(bolt::amp::control::SerialCpu); 

    std::transform(  stdInput.begin( ) + offset,  stdInput.end( ), stdInput.begin() + offset, std::negate<int>() );
    bolt::amp::transform( ctl, boltInput.begin( ) + offset, boltInput.end( ), boltInput.begin( ) + offset, bolt::amp::negate<int>() );

    cmpArrays( stdInput, boltInput);
    
}

TEST( DVIntVector, MulticoreOffsetTransform  )
{
    int length = 1024;

    std::vector<int> stdInput( length ,1);
    bolt::amp::device_vector<int> boltInput(stdInput.begin(),stdInput.end());

    int offset = 100;

    bolt::amp::control ctl = bolt::amp::control::getDefault( );
    ctl.setForceRunMode(bolt::amp::control::MultiCoreCpu); 

    std::transform(  stdInput.begin( ) + offset,  stdInput.end( ), stdInput.begin() + offset, std::negate<int>() );
    bolt::amp::transform( ctl, boltInput.begin( ) + offset, boltInput.end( ), boltInput.begin( ) + offset, bolt::amp::negate<int>() );

    cmpArrays( stdInput, boltInput);
    
}

TEST_P( TransformIntegerVector, InplaceTransform )
{
    //  Calling the actual functions under test
    std::transform( stdInput.begin(), stdInput.end(), stdInput.begin(), std::negate<int>());
    bolt::amp::transform( boltInput.begin( ), boltInput.end( ), boltInput.begin( ), bolt::amp::negate<int>() );



    std::vector< int >::iterator::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end() );
    std::vector< int >::iterator::difference_type boltNumElements = std::distance( boltInput.begin( ),boltInput.end());

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdInput, boltInput );
}
TEST_P( TransformIntegerVector, SerialInplaceTransform )
{
    bolt::amp::control ctl = bolt::amp::control::getDefault( );
    ctl.setForceRunMode(bolt::amp::control::SerialCpu); 

    //  Calling the actual functions under test
    std::transform( stdInput.begin(), stdInput.end(), stdInput.begin(), std::negate<int>());
    bolt::amp::transform(ctl, boltInput.begin( ), boltInput.end( ), boltInput.begin( ), bolt::amp::negate<int>() );



    std::vector< int >::iterator::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end() );
    std::vector< int >::iterator::difference_type boltNumElements = std::distance( boltInput.begin( ),boltInput.end());

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdInput, boltInput );
}
TEST_P( TransformIntegerVector, MulticoreInplaceTransform )
{
    bolt::amp::control ctl = bolt::amp::control::getDefault( );
    ctl.setForceRunMode(bolt::amp::control::MultiCoreCpu); 

    //  Calling the actual functions under test
    std::transform( stdInput.begin(), stdInput.end(), stdInput.begin(), std::negate<int>());
    bolt::amp::transform(ctl, boltInput.begin( ), boltInput.end( ), boltInput.begin( ), bolt::amp::negate<int>() );



    std::vector< int >::iterator::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end() );
    std::vector< int >::iterator::difference_type boltNumElements = std::distance( boltInput.begin( ),boltInput.end());

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdInput, boltInput );
}


TEST_P( TransformIntegerDeviceVector, InplaceTransform )
{
    //  Calling the actual functions under test
    std::transform( stdInput.begin( ), stdInput.end( ), stdInput.begin( ), std::negate<int>() );
    bolt::amp::transform( boltInput.begin( ), boltInput.end( ), boltInput.begin( ), bolt::amp::negate<int>() );
       
    std::vector< int >::iterator::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end());
    std::vector< int >::iterator::difference_type boltNumElements = std::distance(boltInput.begin(),boltInput.end());

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdInput, boltInput );
}
TEST_P( TransformIntegerDeviceVector, SerialInplaceTransform )
{
    bolt::amp::control ctl = bolt::amp::control::getDefault( );
    ctl.setForceRunMode(bolt::amp::control::SerialCpu); 

    //  Calling the actual functions under test
    std::transform( stdInput.begin( ), stdInput.end( ), stdInput.begin( ), std::negate<int>() );
    bolt::amp::transform(ctl, boltInput.begin( ), boltInput.end( ), boltInput.begin( ), bolt::amp::negate<int>() );
       
    std::vector< int >::iterator::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end());
    std::vector< int >::iterator::difference_type boltNumElements = std::distance(boltInput.begin(),boltInput.end());

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdInput, boltInput );
}
TEST_P( TransformIntegerDeviceVector, MulticoreInplaceTransform )
{
    bolt::amp::control ctl = bolt::amp::control::getDefault( );
    ctl.setForceRunMode(bolt::amp::control::MultiCoreCpu); 

    //  Calling the actual functions under test
    std::transform( stdInput.begin( ), stdInput.end( ), stdInput.begin( ), std::negate<int>() );
    bolt::amp::transform(ctl, boltInput.begin( ), boltInput.end( ), boltInput.begin( ), bolt::amp::negate<int>() );
       
    std::vector< int >::iterator::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end());
    std::vector< int >::iterator::difference_type boltNumElements = std::distance(boltInput.begin(),boltInput.end());

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


    std::vector< float >::iterator::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end());
    std::vector< float >::iterator::difference_type boltNumElements = std::distance(boltInput.begin(),boltInput.end());

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdInput, boltInput );
}
TEST_P( TransformFloatVector, SerialInplaceTransform )
{
    bolt::amp::control ctl = bolt::amp::control::getDefault( );
    ctl.setForceRunMode(bolt::amp::control::SerialCpu); 

    //  Calling the actual functions under test
    std::transform( stdInput.begin( ), stdInput.end( ), stdInput.begin( ), std::negate<float>() );
    bolt::amp::transform(ctl, boltInput.begin( ), boltInput.end( ), boltInput.begin( ), bolt::amp::negate<float>() );


    std::vector< float >::iterator::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end());
    std::vector< float >::iterator::difference_type boltNumElements = std::distance(boltInput.begin(),boltInput.end());

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdInput, boltInput );
}
TEST_P( TransformFloatVector, MulticoreInplaceTransform )
{
    bolt::amp::control ctl = bolt::amp::control::getDefault( );
    ctl.setForceRunMode(bolt::amp::control::MultiCoreCpu); 

    //  Calling the actual functions under test
    std::transform( stdInput.begin( ), stdInput.end( ), stdInput.begin( ), std::negate<float>() );
    bolt::amp::transform(ctl, boltInput.begin( ), boltInput.end( ), boltInput.begin( ), bolt::amp::negate<float>() );


    std::vector< float >::iterator::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end());
    std::vector< float >::iterator::difference_type boltNumElements = std::distance(boltInput.begin(),boltInput.end());

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdInput, boltInput );
}

TEST_P( TransformFloatDeviceVector, InplaceTransform )
{
    //  Calling the actual functions under test
    std::transform( stdInput.begin( ), stdInput.end( ), stdInput.begin( ), std::negate<float>() );
    bolt::amp::transform( boltInput.begin( ), boltInput.end( ), boltInput.begin( ), bolt::amp::negate<float>() );


    std::vector< float >::iterator::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end());
    std::vector< float >::iterator::difference_type boltNumElements = std::distance(boltInput.begin(),boltInput.end());

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdInput, boltInput );
}
TEST_P( TransformFloatDeviceVector, SerialInplaceTransform )
{
    bolt::amp::control ctl = bolt::amp::control::getDefault( );
    ctl.setForceRunMode(bolt::amp::control::SerialCpu); 

    //  Calling the actual functions under test
    std::transform( stdInput.begin( ), stdInput.end( ), stdInput.begin( ), std::negate<float>() );
    bolt::amp::transform(ctl, boltInput.begin( ), boltInput.end( ), boltInput.begin( ), bolt::amp::negate<float>() );


    std::vector< float >::iterator::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end());
    std::vector< float >::iterator::difference_type boltNumElements = std::distance(boltInput.begin(),boltInput.end());

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdInput, boltInput );
}
TEST_P( TransformFloatDeviceVector, MulticoreInplaceTransform )
{
    bolt::amp::control ctl = bolt::amp::control::getDefault( );
    ctl.setForceRunMode(bolt::amp::control::MultiCoreCpu); 

    //  Calling the actual functions under test
    std::transform( stdInput.begin( ), stdInput.end( ), stdInput.begin( ), std::negate<float>() );
    bolt::amp::transform(ctl, boltInput.begin( ), boltInput.end( ), boltInput.begin( ), bolt::amp::negate<float>() );


    std::vector< float >::iterator::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end());
    std::vector< float >::iterator::difference_type boltNumElements = std::distance(boltInput.begin(),boltInput.end());

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


    std::vector< float >::iterator::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end());
    std::vector< float >::iterator::difference_type boltNumElements = std::distance(boltInput.begin(),boltInput.end());

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdInput, boltInput );
}
TEST_P( TransformFloatVector, SerialInplaceSquareTransform )
{
    bolt::amp::control ctl = bolt::amp::control::getDefault( );
    ctl.setForceRunMode(bolt::amp::control::SerialCpu); 

    //  Calling the actual functions under test
    std::transform( stdInput.begin( ), stdInput.end( ), stdInput.begin( ), [](float x){return x*x;} );
    bolt::amp::transform(ctl, boltInput.begin( ), boltInput.end( ), boltInput.begin( ), bolt::amp::square<float>() );


    std::vector< float >::iterator::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end());
    std::vector< float >::iterator::difference_type boltNumElements = std::distance(boltInput.begin(),boltInput.end());

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdInput, boltInput );
}
TEST_P( TransformFloatVector, MulticoreInplaceSquareTransform )
{
    bolt::amp::control ctl = bolt::amp::control::getDefault( );
    ctl.setForceRunMode(bolt::amp::control::MultiCoreCpu); 

    //  Calling the actual functions under test
    std::transform( stdInput.begin( ), stdInput.end( ), stdInput.begin( ), [](float x){return x*x;} );
    bolt::amp::transform(ctl, boltInput.begin( ), boltInput.end( ), boltInput.begin( ), bolt::amp::square<float>() );


    std::vector< float >::iterator::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end());
    std::vector< float >::iterator::difference_type boltNumElements = std::distance(boltInput.begin(),boltInput.end());

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdInput, boltInput );
}

TEST_P( TransformFloatDeviceVector, InplaceSquareTransform )
{
    //  Calling the actual functions under test
    std::transform( stdInput.begin( ), stdInput.end( ), stdInput.begin( ), [](float x){return x*x;} );
    bolt::amp::transform( boltInput.begin( ), boltInput.end( ), boltInput.begin( ), bolt::amp::square<float>() );


    std::vector< float >::iterator::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end());
    std::vector< float >::iterator::difference_type boltNumElements = std::distance(boltInput.begin(),boltInput.end());

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdInput, boltInput );
}
TEST_P( TransformFloatDeviceVector, SerialInplaceSquareTransform )
{
    bolt::amp::control ctl = bolt::amp::control::getDefault( );
    ctl.setForceRunMode(bolt::amp::control::SerialCpu); 

    //  Calling the actual functions under test
    std::transform( stdInput.begin( ), stdInput.end( ), stdInput.begin( ), [](float x){return x*x;} );
    bolt::amp::transform(ctl, boltInput.begin( ), boltInput.end( ), boltInput.begin( ), bolt::amp::square<float>() );


    std::vector< float >::iterator::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end());
    std::vector< float >::iterator::difference_type boltNumElements = std::distance(boltInput.begin(),boltInput.end());

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdInput, boltInput );
}
TEST_P( TransformFloatDeviceVector, MulticoreInplaceSquareTransform )
{
    bolt::amp::control ctl = bolt::amp::control::getDefault( );
    ctl.setForceRunMode(bolt::amp::control::MultiCoreCpu); 

    //  Calling the actual functions under test
    std::transform( stdInput.begin( ), stdInput.end( ), stdInput.begin( ), [](float x){return x*x;} );
    bolt::amp::transform(ctl, boltInput.begin( ), boltInput.end( ), boltInput.begin( ), bolt::amp::square<float>() );


    std::vector< float >::iterator::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end());
    std::vector< float >::iterator::difference_type boltNumElements = std::distance(boltInput.begin(),boltInput.end());

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


    std::vector< float >::iterator::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end());
    std::vector< float >::iterator::difference_type boltNumElements = std::distance(boltInput.begin(),boltInput.end());

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdInput, boltInput );
}
TEST_P( TransformFloatVector, SerialInplaceCubeTransform )
{
    bolt::amp::control ctl = bolt::amp::control::getDefault( );
    ctl.setForceRunMode(bolt::amp::control::SerialCpu); 

    //  Calling the actual functions under test
    std::transform( stdInput.begin( ), stdInput.end( ), stdInput.begin( ), [](float x){return x*x*x;} );
    bolt::amp::transform(ctl, boltInput.begin( ), boltInput.end( ), boltInput.begin( ), bolt::amp::cube<float>() );


    std::vector< float >::iterator::difference_type stdNumElements = std::distance( stdInput.begin( ),stdInput.end());
    std::vector< float >::iterator::difference_type boltNumElements = std::distance(boltInput.begin(),boltInput.end());

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdInput, boltInput );
}
TEST_P( TransformFloatVector, MulticoreInplaceCubeTransform )
{
    bolt::amp::control ctl = bolt::amp::control::getDefault( );
    ctl.setForceRunMode(bolt::amp::control::MultiCoreCpu); 

    //  Calling the actual functions under test
    std::transform( stdInput.begin( ), stdInput.end( ), stdInput.begin( ), [](float x){return x*x*x;} );
    bolt::amp::transform(ctl, boltInput.begin( ), boltInput.end( ), boltInput.begin( ), bolt::amp::cube<float>() );


    std::vector< float >::iterator::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end());
    std::vector< float >::iterator::difference_type boltNumElements = std::distance(boltInput.begin(),boltInput.end());

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdInput, boltInput );
}

TEST_P( TransformFloatDeviceVector, InplaceCubeTransform )
{
    //  Calling the actual functions under test
    std::transform( stdInput.begin( ), stdInput.end( ), stdInput.begin( ), [](float x){return x*x*x;} );
    bolt::amp::transform( boltInput.begin( ), boltInput.end( ), boltInput.begin( ), bolt::amp::cube<float>() );


    std::vector< float >::iterator::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end());
    std::vector< float >::iterator::difference_type boltNumElements = std::distance(boltInput.begin(),boltInput.end());

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdInput, boltInput );
}
TEST_P( TransformFloatDeviceVector, SerialInplaceCubeTransform )
{
    bolt::amp::control ctl = bolt::amp::control::getDefault( );
    ctl.setForceRunMode(bolt::amp::control::SerialCpu); 

    //  Calling the actual functions under test
    std::transform( stdInput.begin( ), stdInput.end( ), stdInput.begin( ), [](float x){return x*x*x;} );
    bolt::amp::transform(ctl, boltInput.begin( ), boltInput.end( ), boltInput.begin( ), bolt::amp::cube<float>() );


    std::vector< float >::iterator::difference_type stdNumElements = std::distance( stdInput.begin( ),stdInput.end());
    std::vector< float >::iterator::difference_type boltNumElements = std::distance(boltInput.begin(),boltInput.end());

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdInput, boltInput );
}
TEST_P( TransformFloatDeviceVector, MulticoreInplaceCubeTransform )
{
    bolt::amp::control ctl = bolt::amp::control::getDefault( );
    ctl.setForceRunMode(bolt::amp::control::MultiCoreCpu); 

    //  Calling the actual functions under test
    std::transform( stdInput.begin( ), stdInput.end( ), stdInput.begin( ), [](float x){return x*x*x;} );
    bolt::amp::transform(ctl, boltInput.begin( ), boltInput.end( ), boltInput.begin( ), bolt::amp::cube<float>() );


    std::vector< float >::iterator::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end());
    std::vector< float >::iterator::difference_type boltNumElements = std::distance(boltInput.begin(),boltInput.end());

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdInput, boltInput );
}


#if(TEST_DOUBLE == 1)
TEST_P( TransformDoubleVector, InplaceTransform )
{
    //  Calling the actual functions under test
    std::transform( stdInput.begin( ), stdInput.end( ), stdInput.begin( ), std::negate<double>() );
    bolt::amp::transform( boltInput.begin( ), boltInput.end( ), boltInput.begin( ), bolt::amp::negate<double>() );


    std::vector< int >::iterator::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end() );
    std::vector< int >::iterator::difference_type boltNumElements = std::distance( boltInput.begin(),boltInput.end());

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdInput, boltInput );
}
TEST_P( TransformDoubleVector, SerialInplaceTransform )
{
    bolt::amp::control ctl = bolt::amp::control::getDefault( );
    ctl.setForceRunMode(bolt::amp::control::SerialCpu); 

    //  Calling the actual functions under test
    std::transform( stdInput.begin( ), stdInput.end( ), stdInput.begin( ), std::negate<double>() );
    bolt::amp::transform(ctl, boltInput.begin( ), boltInput.end( ), boltInput.begin( ), bolt::amp::negate<double>() );


    std::vector< int >::iterator::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end() );
    std::vector< int >::iterator::difference_type boltNumElements = std::distance( boltInput.begin(),boltInput.end());

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdInput, boltInput );
}
TEST_P( TransformDoubleVector, MulticoreInplaceTransform )
{
    bolt::amp::control ctl = bolt::amp::control::getDefault( );
    ctl.setForceRunMode(bolt::amp::control::MultiCoreCpu); 

    //  Calling the actual functions under test
    std::transform( stdInput.begin( ), stdInput.end( ), stdInput.begin( ), std::negate<double>() );
    bolt::amp::transform(ctl, boltInput.begin( ), boltInput.end( ), boltInput.begin( ), bolt::amp::negate<double>());


    std::vector< int >::iterator::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end() );
    std::vector< int >::iterator::difference_type boltNumElements = std::distance( boltInput.begin(),boltInput.end());

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdInput, boltInput );
}


TEST_P( TransformDoubleDeviceVector, InplaceTransform )
{
    //  Calling the actual functions under test
    std::transform( stdInput.begin( ), stdInput.end( ), stdInput.begin( ), std::negate<double>() );
    bolt::amp::transform( boltInput.begin( ), boltInput.end( ), boltInput.begin( ), bolt::amp::negate<double>() );


    std::vector< int >::iterator::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end() );
    std::vector< int >::iterator::difference_type boltNumElements = std::distance( boltInput.begin(),boltInput.end());

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdInput, boltInput );
}
TEST_P( TransformDoubleDeviceVector, SerialInplaceTransform )
{
    bolt::amp::control ctl = bolt::amp::control::getDefault( );
    ctl.setForceRunMode(bolt::amp::control::SerialCpu); 

    //  Calling the actual functions under test
    std::transform( stdInput.begin( ), stdInput.end( ), stdInput.begin( ), std::negate<double>() );
    bolt::amp::transform(ctl, boltInput.begin( ), boltInput.end( ), boltInput.begin( ), bolt::amp::negate<double>() );


    std::vector< int >::iterator::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end() );
    std::vector< int >::iterator::difference_type boltNumElements = std::distance( boltInput.begin(),boltInput.end());

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdInput, boltInput );
}
TEST_P( TransformDoubleDeviceVector, MulticoreInplaceTransform )
{
    bolt::amp::control ctl = bolt::amp::control::getDefault( );
    ctl.setForceRunMode(bolt::amp::control::MultiCoreCpu); 

    //  Calling the actual functions under test
    std::transform( stdInput.begin( ), stdInput.end( ), stdInput.begin( ), std::negate<double>() );
    bolt::amp::transform(ctl, boltInput.begin( ), boltInput.end( ), boltInput.begin( ), bolt::amp::negate<double>());


    std::vector< int >::iterator::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end() );
    std::vector< int >::iterator::difference_type boltNumElements = std::distance( boltInput.begin(),boltInput.end());

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdInput, boltInput );
}

#endif

TEST( TransformDeviceVector, UDDOutOfPlaceTransform)
{
  int length = 1<<8;
  std::vector<UDD> hVectorA( length ), hVectorB( length ), hVectorO( length );
  std::fill( hVectorA.begin(), hVectorA.end(), 1024 );
  std::fill( hVectorB.begin(), hVectorB.end(), 1024 );
  std::fill( hVectorB.begin(), hVectorB.end(), 0 );

  bolt::amp::device_vector<UDD> dVectorA(hVectorA.begin(), hVectorA.end()),
    dVectorB(hVectorB.begin(), hVectorB.end()),
    dVectorO(hVectorO.begin(), hVectorO.end());

  std::transform( hVectorA.begin(), hVectorA.end(), hVectorB.begin(), hVectorO.begin(), std::plus< UDD >( ) );
  bolt::amp::transform( dVectorA.begin(),
    dVectorA.end(),
    dVectorB.begin(),
    dVectorO.begin(),
    bolt::amp::plus< UDD >( ) );
  
  cmpArrays(hVectorO, dVectorO);

}

TEST( TransformDeviceVector, SerialUDDOutOfPlaceTransform)
{
  int length = 1<<8;
  std::vector<UDD> hVectorA( length ), hVectorB( length ), hVectorO( length );
  std::fill( hVectorA.begin(), hVectorA.end(), 1024 );
  std::fill( hVectorB.begin(), hVectorB.end(), 1024 );
  std::fill( hVectorB.begin(), hVectorB.end(), 0 );

  bolt::amp::device_vector<UDD> dVectorA(hVectorA.begin(), hVectorA.end()),
    dVectorB(hVectorB.begin(), hVectorB.end()),
    dVectorO(hVectorO.begin(), hVectorO.end());

  bolt::amp::control ctl = bolt::amp::control::getDefault( );
  ctl.setForceRunMode(bolt::amp::control::SerialCpu); 

  std::transform( hVectorA.begin(), hVectorA.end(), hVectorB.begin(), hVectorO.begin(), std::plus< UDD >( ) );
  bolt::amp::transform( ctl, dVectorA.begin(),
    dVectorA.end(),
    dVectorB.begin(),
    dVectorO.begin(),
    bolt::amp::plus< UDD >( ) );
  
  cmpArrays(hVectorO, dVectorO);

}


TEST( TransformDeviceVector, MulticoreUDDOutOfPlaceTransform)
{
  int length = 1<<8;
  std::vector<UDD> hVectorA( length ), hVectorB( length ), hVectorO( length );
  std::fill( hVectorA.begin(), hVectorA.end(), 1024 );
  std::fill( hVectorB.begin(), hVectorB.end(), 1024 );
  std::fill( hVectorB.begin(), hVectorB.end(), 0 );

  bolt::amp::device_vector<UDD> dVectorA(hVectorA.begin(), hVectorA.end()),
    dVectorB(hVectorB.begin(), hVectorB.end()),
    dVectorO(hVectorO.begin(), hVectorO.end());

  bolt::amp::control ctl = bolt::amp::control::getDefault( );
  ctl.setForceRunMode(bolt::amp::control::MultiCoreCpu); 

  std::transform( hVectorA.begin(), hVectorA.end(), hVectorB.begin(), hVectorO.begin(), std::plus< UDD >( ) );
  bolt::amp::transform( ctl, dVectorA.begin(),
    dVectorA.end(),
    dVectorB.begin(),
    dVectorO.begin(),
    bolt::amp::plus< UDD >( ) );
  
  cmpArrays(hVectorO, dVectorO);

}


TEST( TransformDeviceVector, OutOfPlaceTransform)
{
  int length = 1<<8;
  std::vector<int> hVectorA( length ), hVectorB( length ), hVectorO( length );
  std::fill( hVectorA.begin(), hVectorA.end(), 1024 );
  std::fill( hVectorB.begin(), hVectorB.end(), 1024 );
  std::fill( hVectorB.begin(), hVectorB.end(), 0 );

  bolt::amp::device_vector<int> dVectorA(hVectorA.begin(), hVectorA.end()),
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

TEST( TransformDeviceVector, SerialOutOfPlaceTransform)
{
  int length = 1<<8;
  std::vector<int> hVectorA( length ), hVectorB( length ), hVectorO( length );
  std::fill( hVectorA.begin(), hVectorA.end(), 1024 );
  std::fill( hVectorB.begin(), hVectorB.end(), 1024 );
  std::fill( hVectorB.begin(), hVectorB.end(), 0 );

  bolt::amp::device_vector<int> dVectorA(hVectorA.begin(), hVectorA.end()),
    dVectorB(hVectorB.begin(), hVectorB.end()),
    dVectorO(hVectorO.begin(), hVectorO.end());
  
    bolt::amp::control ctl = bolt::amp::control::getDefault( );
    ctl.setForceRunMode(bolt::amp::control::SerialCpu); 

  std::transform( hVectorA.begin(), hVectorA.end(), hVectorB.begin(), hVectorO.begin(), std::plus< int >( ) );
  bolt::amp::transform(ctl, dVectorA.begin(),
    dVectorA.end(),
    dVectorB.begin(),
    dVectorO.begin(),
    bolt::amp::plus< int >( ) );
  
  cmpArrays(hVectorO, dVectorO);



}
TEST( TransformDeviceVector, MulticoreOutOfPlaceTransform)
{
  int length = 1<<8;
  std::vector<int> hVectorA( length ), hVectorB( length ), hVectorO( length );
  std::fill( hVectorA.begin(), hVectorA.end(), 1024 );
  std::fill( hVectorB.begin(), hVectorB.end(), 1024 );
  std::fill( hVectorB.begin(), hVectorB.end(), 0 );

  bolt::amp::device_vector<int> dVectorA(hVectorA.begin(), hVectorA.end()),
    dVectorB(hVectorB.begin(), hVectorB.end()),
    dVectorO(hVectorO.begin(), hVectorO.end());

    bolt::amp::control ctl = bolt::amp::control::getDefault( );
    ctl.setForceRunMode(bolt::amp::control::MultiCoreCpu); 

  std::transform( hVectorA.begin(), hVectorA.end(), hVectorB.begin(), hVectorO.begin(), std::plus< int >( ) );
  bolt::amp::transform(ctl, dVectorA.begin(),
    dVectorA.end(),
    dVectorB.begin(),
    dVectorO.begin(),
    bolt::amp::plus< int >( ) );
  
  cmpArrays(hVectorO, dVectorO);
}

TEST( TransformStdVector, OutOfPlaceTransform)
{
  int length = 1<<8;
  std::vector<int> hVectorA( length ), hVectorB( length ), hVectorO( length );
  std::fill( hVectorA.begin(), hVectorA.end(), 1024 );
  std::fill( hVectorB.begin(), hVectorB.end(), 1024 );
  std::fill( hVectorB.begin(), hVectorB.end(), 0 );

     std::vector<int> SVectorA(hVectorA.begin(), hVectorA.end()),
    SVectorB(hVectorB.begin(), hVectorB.end()),
    SVectorO(hVectorO.begin(), hVectorO.end());

  std::transform( hVectorA.begin(), hVectorA.end(), hVectorB.begin(), hVectorO.begin(), std::plus< int >( ) );
  bolt::amp::transform( SVectorA.begin(),
    SVectorA.end(),
    SVectorB.begin(),
    SVectorO.begin(),
    bolt::amp::plus< int >( ) );
  
  cmpArrays(hVectorO, SVectorO);
 }
TEST( TransformStdVector, SerialOutOfPlaceTransform)
{
  int length = 1<<8;
  std::vector<int> hVectorA( length ), hVectorB( length ), hVectorO( length );
  std::fill( hVectorA.begin(), hVectorA.end(), 1024 );
  std::fill( hVectorB.begin(), hVectorB.end(), 1024 );
  std::fill( hVectorB.begin(), hVectorB.end(), 0 );

     std::vector<int> SVectorA(hVectorA.begin(), hVectorA.end()),
    SVectorB(hVectorB.begin(), hVectorB.end()),
    SVectorO(hVectorO.begin(), hVectorO.end());
     
    bolt::amp::control ctl = bolt::amp::control::getDefault( );
    ctl.setForceRunMode(bolt::amp::control::SerialCpu); 

  std::transform( hVectorA.begin(), hVectorA.end(), hVectorB.begin(), hVectorO.begin(), std::plus< int >( ) );
  bolt::amp::transform(ctl, SVectorA.begin(),
    SVectorA.end(),
    SVectorB.begin(),
    SVectorO.begin(),
    bolt::amp::plus< int >( ) );
  
  cmpArrays(hVectorO, SVectorO);
 }
TEST( TransformStdVector, MulticoreOutOfPlaceTransform)
{
  int length = 1<<8;
  std::vector<int> hVectorA( length ), hVectorB( length ), hVectorO( length );
  std::fill( hVectorA.begin(), hVectorA.end(), 1024 );
  std::fill( hVectorB.begin(), hVectorB.end(), 1024 );
  std::fill( hVectorB.begin(), hVectorB.end(), 0 );

     std::vector<int> SVectorA(hVectorA.begin(), hVectorA.end()),
    SVectorB(hVectorB.begin(), hVectorB.end()),
    SVectorO(hVectorO.begin(), hVectorO.end());
     
    bolt::amp::control ctl = bolt::amp::control::getDefault( );
    ctl.setForceRunMode(bolt::amp::control::MultiCoreCpu); 

  std::transform( hVectorA.begin(), hVectorA.end(), hVectorB.begin(), hVectorO.begin(), std::plus< int >( ) );
  bolt::amp::transform(ctl, SVectorA.begin(),
    SVectorA.end(),
    SVectorB.begin(),
    SVectorO.begin(),
    bolt::amp::plus< int >( ) );
  
  cmpArrays(hVectorO, SVectorO);
 }

TEST( DVIntVector, OffsetIntTest )
{
    int length = 1024;

    std::vector<int> stdInput( length ,1);
    bolt::amp::device_vector<int> boltInput(stdInput.begin(),stdInput.end());

    int offset = 100;

    std::transform(  stdInput.begin( ) + offset,  stdInput.end( ), stdInput.begin() + offset, std::negate<int>() );
    bolt::amp::transform( boltInput.begin( ) + offset, boltInput.end( ), boltInput.begin( ) + offset, bolt::amp::negate<int>() );

    cmpArrays( stdInput, boltInput);
    
}
#if TEST_DOUBLE == 1
TEST( DVIntVector, OffsetDoubleTest )
{
    int length = 1024;

    std::vector<double> stdInput( length ,4.0);
    bolt::amp::device_vector<double> boltInput(stdInput.begin(),stdInput.end());

    int offset = 100;

    std::transform(  stdInput.begin( ) + offset,  stdInput.end( ), stdInput.begin() + offset, std::negate<double>() );
    bolt::amp::transform( boltInput.begin( ) + offset, boltInput.end( ), boltInput.begin( ) + offset, bolt::amp::negate<double>() );

    cmpArrays( stdInput, boltInput);
    
}
#endif
//Teststotestthecountingiterator
TEST(simple1,counting)
{
    bolt::amp::counting_iterator<int> iter(0);
    bolt::amp::counting_iterator<int> iter2=iter+1024;
    std::vector<int> input1(1024);
    std::vector<int> input2(1024);
    std::vector<int> stdOutput(1024);
     std::vector<int> boltOutput(1024);
     for(int i=0 ; i< 1024;i++)
     {
         input1[i] = i;
     }
    input2 = input1;
    std::transform( input1.begin(), input1.end(), input2.begin(), stdOutput.begin(), bolt::amp::plus<int>());
    bolt::amp::transform(iter,iter2,input1.begin(),boltOutput.begin(),bolt::amp::plus<int>());
    cmpArrays( stdOutput, boltOutput, 1024 );
}

TEST(simple1,Serial_counting)
{
    bolt::amp::counting_iterator<int> iter(0);
    bolt::amp::counting_iterator<int> iter2=iter+1024;
    std::vector<int> input1(1024);
    std::vector<int> input2(1024);
    std::vector<int> stdOutput(1024);
     std::vector<int> boltOutput(1024);
     for(int i=0 ; i< 1024;i++)
     {
         input1[i] = i;
     }
    input2 = input1;

    bolt::amp::control ctl = bolt::amp::control::getDefault( );
    ctl.setForceRunMode(bolt::amp::control::SerialCpu);

    std::transform( input1.begin(), input1.end(), input2.begin(), stdOutput.begin(), bolt::amp::plus<int>());
    bolt::amp::transform(ctl, iter,iter2,input1.begin(),boltOutput.begin(),bolt::amp::plus<int>());
    cmpArrays( stdOutput, boltOutput, 1024 );
}

TEST(simple1,MultiCore_counting)
{
    bolt::amp::counting_iterator<int> iter(0);
    bolt::amp::counting_iterator<int> iter2=iter+1024;
    std::vector<int> input1(1024);
    std::vector<int> input2(1024);
    std::vector<int> stdOutput(1024);
     std::vector<int> boltOutput(1024);
     for(int i=0 ; i< 1024;i++)
     {
         input1[i] = i;
     }
    input2 = input1;

    bolt::amp::control ctl = bolt::amp::control::getDefault( );
    ctl.setForceRunMode(bolt::amp::control::MultiCoreCpu);

    std::transform( input1.begin(), input1.end(), input2.begin(), stdOutput.begin(), bolt::amp::plus<int>());
    bolt::amp::transform(ctl, iter,iter2,input1.begin(),boltOutput.begin(),bolt::amp::plus<int>());
    cmpArrays( stdOutput, boltOutput, 1024 );
}



//Teststotestthecountingiterator
TEST(simple1,device_counting)
{
    bolt::amp::counting_iterator<int> iter(0);
    bolt::amp::counting_iterator<int> iter2=iter+1024;
    std::vector<int> input1(1024);
    std::vector<int> input2(1024);
    std::vector<int> stdOutput(1024);
    bolt::amp::device_vector<int> boltOutput(1024);
     for(int i=0 ; i< 1024;i++)
     {
         input1[i] = i;
     }
    input2 = input1;

	bolt::amp::device_vector<int> dvinput2(input2.begin(), input2.end());

    std::transform( input1.begin(), input1.end(), input2.begin(), stdOutput.begin(), bolt::amp::plus<int>());
    bolt::amp::transform(iter,iter2,dvinput2.begin(),boltOutput.begin(),bolt::amp::plus<int>());
    cmpArrays( stdOutput, boltOutput);
}

TEST(simple1,Serial_device_counting)
{

    bolt::amp::control ctl = bolt::amp::control::getDefault( );
    ctl.setForceRunMode(bolt::amp::control::SerialCpu);

    bolt::amp::counting_iterator<int> iter(0);
    bolt::amp::counting_iterator<int> iter2=iter+1024;
    std::vector<int> input1(1024);
    std::vector<int> input2(1024);
    std::vector<int> stdOutput(1024);
    bolt::amp::device_vector<int> boltOutput(1024);
     for(int i=0 ; i< 1024;i++)
     {
         input1[i] = i;
     }
    input2 = input1;

	bolt::amp::device_vector<int> dvinput2(input2.begin(), input2.end());

    std::transform( input1.begin(), input1.end(), input2.begin(), stdOutput.begin(), bolt::amp::plus<int>());
    bolt::amp::transform(ctl, iter,iter2,dvinput2.begin(),boltOutput.begin(),bolt::amp::plus<int>());
    cmpArrays( stdOutput, boltOutput);
}

TEST(simple1,MultiCore_device_counting)
{

    bolt::amp::control ctl = bolt::amp::control::getDefault( );
    ctl.setForceRunMode(bolt::amp::control::MultiCoreCpu);

    bolt::amp::counting_iterator<int> iter(0);
    bolt::amp::counting_iterator<int> iter2=iter+1024;
    std::vector<int> input1(1024);
    std::vector<int> input2(1024);
    std::vector<int> stdOutput(1024);
    bolt::amp::device_vector<int> boltOutput(1024);
     for(int i=0 ; i< 1024;i++)
     {
         input1[i] = i;
     }
    input2 = input1;

	bolt::amp::device_vector<int> dvinput2(input2.begin(), input2.end());

    std::transform( input1.begin(), input1.end(), input2.begin(), stdOutput.begin(), bolt::amp::plus<int>());
    bolt::amp::transform(ctl, iter,iter2,dvinput2.begin(),boltOutput.begin(),bolt::amp::plus<int>());
    cmpArrays( stdOutput, boltOutput );
}





//TEST(amp_const_iter_transformBoltampVectFloat, addIterFloatValues){
//    int size = 10;
//    float myConstValueF = 1.125f;
//    bolt::amp::plus<float> addKaro;
//    std::vector<float> stdVect(size);
//    
//    for (int i = 0; i < size; i++){
//        stdVect[i] = (float)i + 1.0f;
//    }
//    bolt::amp::device_vector<float> myDevVect(stdVect.begin(), stdVect.end());
//    
//    for (int i = 0; i<size; ++i){
//        std::cout.setf(std::ios::fixed);
//        std::cout<<std::setprecision(3)<<myDevVect[i]<<" ";
//    }
//
//    bolt::amp::constant_iterator< float > constIter( myConstValueF );
//
//    bolt::amp::transform(myDevVect.begin(), myDevVect.end(), constIter, myDevVect.begin(), addKaro);
//    
//    std::cout<<std::endl<<std::endl;
//    for (int i = 0; i<size; ++i){
//        std::cout.setf(std::ios::fixed);
//        std::cout<<std::setprecision(3)<<myDevVect[i]<<" ";
//    }
//}


//TEST(amp_const_iter_transformBoltampVectFloat, SerialaddIterFloatValues){
//    int size = 10;
//    float myConstValueF = 1.125f;
//    bolt::amp::plus<float> addKaro;
//    std::vector<float> stdVect(size);
//    
//    for (int i = 0; i < size; i++){
//        stdVect[i] = (float)i + 1.0f;
//    }
//    bolt::amp::device_vector<float> myDevVect(stdVect.begin(), stdVect.end());
//    for (int i = 0; i<size; ++i){
//        std::cout.setf(std::ios::fixed);
//        std::cout<<std::setprecision(3)<<myDevVect[i]<<" ";
//    }
//
//    bolt::amp::control ctl = bolt::amp::control::getDefault( );
//    ctl.setForceRunMode(bolt::amp::control::SerialCpu);
//
//    bolt::amp::constant_iterator< float > constIter( myConstValueF );
//
//    bolt::amp::transform(ctl, myDevVect.begin(), myDevVect.end(), constIter, myDevVect.begin(), addKaro);
//    
//    std::cout<<std::endl<<std::endl;
//    for (int i = 0; i<size; ++i){
//        std::cout.setf(std::ios::fixed);
//        std::cout<<std::setprecision(3)<<myDevVect[i]<<" ";
//    }
//}
//
//
//TEST(amp_const_iter_transformBoltampVectFloat, MultiCoreaddIterFloatValues){
//    int size = 10;
//    float myConstValueF = 1.125f;
//    bolt::amp::plus<float> addKaro;
//    std::vector<float> stdVect(size);
//    
//    for (int i = 0; i < size; i++){
//        stdVect[i] = (float)i + 1.0f;
//    }
//    bolt::amp::device_vector<float> myDevVect(stdVect.begin(), stdVect.end());
//    for (int i = 0; i<size; ++i){
//        std::cout.setf(std::ios::fixed);
//        std::cout<<std::setprecision(3)<<myDevVect[i]<<" ";
//    }
//
//    bolt::amp::control ctl = bolt::amp::control::getDefault( );
//    ctl.setForceRunMode(bolt::amp::control::MultiCoreCpu);
//
//    bolt::amp::constant_iterator< float > constIter( myConstValueF );
//
//    bolt::amp::transform(ctl, myDevVect.begin(), myDevVect.end(), constIter, myDevVect.begin(), addKaro);
//    
//    std::cout<<std::endl<<std::endl;
//    for (int i = 0; i<size; ++i){
//        std::cout.setf(std::ios::fixed);
//        std::cout<<std::setprecision(3)<<myDevVect[i]<<" ";
//    }
//}

TEST(amp_const_iter_transformBoltampFloat, addIterFloatValues){
    int size = 10;
    float myConstValueF = 1.125f;
    bolt::amp::plus<float> addKaro;
    std::vector<float> myVect(size);
    
    for (int i = 0; i < size; i++){
        myVect[i] = (float)i + 1.0f;
    }
    
    for (int i = 0; i<size; ++i){
        std::cout.setf(std::ios::fixed);
        std::cout<<std::setprecision(3)<<myVect[i]<<" ";
    }

    bolt::amp::constant_iterator< float > constIter( myConstValueF );

    bolt::amp::transform(myVect.begin(), myVect.end(), constIter, myVect.begin(), addKaro);
    
    std::cout<<std::endl<<std::endl;
    for (int i = 0; i<size; ++i){
        std::cout.setf(std::ios::fixed);
        std::cout<<std::setprecision(3)<<myVect[i]<<" ";
    }
}

TEST(amp_const_iter_transformBoltampFloat, SerialaddIterFloatValues){
    int size = 10;
    float myConstValueF = 1.125f;
    bolt::amp::plus<float> addKaro;
    std::vector<float> myVect(size);
    
    for (int i = 0; i < size; i++){
        myVect[i] = (float)i + 1.0f;
    }
    
    for (int i = 0; i<size; ++i){
        std::cout.setf(std::ios::fixed);
        std::cout<<std::setprecision(3)<<myVect[i]<<" ";
    }

    bolt::amp::control ctl = bolt::amp::control::getDefault( );
    ctl.setForceRunMode(bolt::amp::control::SerialCpu);

    bolt::amp::constant_iterator< float > constIter( myConstValueF );

    bolt::amp::transform(ctl, myVect.begin(), myVect.end(), constIter, myVect.begin(), addKaro);
    
    std::cout<<std::endl<<std::endl;
    for (int i = 0; i<size; ++i){
        std::cout.setf(std::ios::fixed);
        std::cout<<std::setprecision(3)<<myVect[i]<<" ";
    }
}

TEST(amp_const_iter_transformBoltampFloat, MultiCoreaddIterFloatValues){
    int size = 10;
    float myConstValueF = 1.125f;
    bolt::amp::plus<float> addKaro;
    std::vector<float> myVect(size);
    
    for (int i = 0; i < size; i++){
        myVect[i] = (float)i + 1.0f;
    }
    
    for (int i = 0; i<size; ++i){
        std::cout.setf(std::ios::fixed);
        std::cout<<std::setprecision(3)<<myVect[i]<<" ";
    }

    bolt::amp::control ctl = bolt::amp::control::getDefault( );
    ctl.setForceRunMode(bolt::amp::control::MultiCoreCpu);

    bolt::amp::constant_iterator< float > constIter( myConstValueF );

    bolt::amp::transform(ctl, myVect.begin(), myVect.end(), constIter, myVect.begin(), addKaro);
    
    std::cout<<std::endl<<std::endl;
    for (int i = 0; i<size; ++i){
        std::cout.setf(std::ios::fixed);
        std::cout<<std::setprecision(3)<<myVect[i]<<" ";
    }
}


TEST(TransformStdVector, ConstantIterator)
{
  int length = 1 << 8;
  std::vector<int> hVectorA(length), hVectorO(length), hVectorB(length);
  bolt::amp::constant_iterator<int> hB(100);
  bolt::amp::constant_iterator<int> hB2 = hB + length;

  std::fill(hVectorA.begin(), hVectorA.end(), 1024);
  std::fill(hVectorB.begin(), hVectorB.end(), 100);

  std::vector<int> SVectorA(hVectorA.begin(), hVectorA.end()),
    SVectorO(hVectorO.begin(), hVectorO.end());

  //bolt::amp::device_vector<int> hB(hVectorA.begin(), hVectorA.end());

  std::transform(hVectorB.begin(), hVectorB.end(), hVectorA.begin(),  hVectorO.begin(), std::plus< int >());
//  std::transform(hB.begin(), hB.end(), hVectorA.begin(), hVectorO.begin(), std::plus<int>());
  bolt::amp::transform(hB,
                       hB2,
                       SVectorA.begin(),    
                       SVectorO.begin(),
                       bolt::amp::plus< int >());

  cmpArrays(hVectorO, SVectorO);
}



TEST(TransformStdVector, CountingIterator)
{
  int length = 1 << 8;
  std::vector<int> hVectorA(length), hVectorO(length), hVectorB(length);
  bolt::amp::counting_iterator<int> hB(100);
  bolt::amp::counting_iterator<int> hB2 = hB + length;

  std::fill(hVectorA.begin(), hVectorA.end(), 1024);

  for( int i = 0; i < length ; i++ )
  {
      hVectorB[i] = 100 + i;
  }

  std::vector<int> SVectorA(hVectorA.begin(), hVectorA.end()),
    SVectorO(hVectorO.begin(), hVectorO.end());


  std::transform(hVectorB.begin(), hVectorB.end(), hVectorA.begin(),  hVectorO.begin(), std::plus< int >());

  bolt::amp::transform(hB,
                       hB2,
                       SVectorA.begin(),    
                       SVectorO.begin(),
                       bolt::amp::plus< int >());

  cmpArrays(hVectorO, SVectorO);
}


//  Test lots of consecutive numbers, but small range, suitable for integers because they overflow easier
INSTANTIATE_TEST_CASE_P( TransformInPlace, TransformIntegerVector, ::testing::Range( 1, 4096, 54 ) ); //   1 to 2^12
INSTANTIATE_TEST_CASE_P( TransformInPlace,  TransformIntegerDeviceVector, ::testing::Range( 1, 32768, 3276 ) ); // 1 to 2^15

//  Test a huge range, suitable for floating point as they are less prone to overflow (but floating point
// loses granularity at large values)
INSTANTIATE_TEST_CASE_P( TransformInPlace, TransformFloatVector, ::testing::Range( 4096, 65536, 555 ) ); //2^12 to 2^16
INSTANTIATE_TEST_CASE_P( TransformInPlace, TransformFloatDeviceVector, ::testing::Range( 1, 32768, 3276 ) ); // 1 to 2^15

INSTANTIATE_TEST_CASE_P( InplaceSquareTransform, TransformFloatVector, ::testing::Range( 4096, 65536, 555 ) ); //2^12 to 2^16
INSTANTIATE_TEST_CASE_P( InplaceSquareTransform, TransformFloatDeviceVector, ::testing::Range( 0, 1048576, 4096 ) );

INSTANTIATE_TEST_CASE_P( InplaceCubeTransform, TransformFloatVector, ::testing::Range( 4096, 65536, 555 ) ); //2^12 to 2^16
INSTANTIATE_TEST_CASE_P( InplaceCubeTransform, TransformFloatDeviceVector, ::testing::Range( 1, 32768, 3276 ) ); // 1 to 2^15

#if(TEST_DOUBLE == 1)
INSTANTIATE_TEST_CASE_P( TransformInPlace, TransformDoubleVector, ::testing::Range(  65536, 2097152, 55555 ) ); //2^16 to 2^21
INSTANTIATE_TEST_CASE_P( TransformInPlace, TransformDoubleDeviceVector, ::testing::Range( 1, 32768, 3276 ) ); // 1 to 2^15

INSTANTIATE_TEST_CASE_P( InplaceTransform, TransformDoubleVector, ::testing::Range(  65536, 2097152, 55555 ) ); //2^16 to 2^21
INSTANTIATE_TEST_CASE_P( InplaceTransform, TransformDoubleDeviceVector, ::testing::Range( 1, 32768, 3276 ) ); // 1 to 2^15

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


//typedef ::testing::Types< 
//    std::tuple< UDD, TypeValue< 1 > >,
//    std::tuple< UDD, TypeValue< 31 > >,
//    std::tuple< UDD, TypeValue< 32 > >,
//    std::tuple< UDD, TypeValue< 63 > >,
//    std::tuple< UDD, TypeValue< 64 > >,
//    std::tuple< UDD, TypeValue< 127 > >,
//    std::tuple< UDD, TypeValue< 128 > >,
//    std::tuple< UDD, TypeValue< 129 > >,
//    std::tuple< UDD, TypeValue< 1000 > >,
//    std::tuple< UDD, TypeValue< 1053 > >,
//    std::tuple< UDD, TypeValue< 4096 > >,
//    std::tuple< UDD, TypeValue< 4097 > >,
//    std::tuple< UDD, TypeValue< 65535 > >,
//    std::tuple< UDD, TypeValue< 65536 > >
//> UDDTests;



INSTANTIATE_TYPED_TEST_CASE_P( Integer, TransformArrayTest, IntegerTests );
INSTANTIATE_TYPED_TEST_CASE_P( Float, TransformArrayTest, FloatTests );
INSTANTIATE_TYPED_TEST_CASE_P( Integer, TransformBinaryArrayTest, IntegerTests );
INSTANTIATE_TYPED_TEST_CASE_P( Float, TransformBinaryArrayTest, FloatTests );
INSTANTIATE_TYPED_TEST_CASE_P( Integer, TransformOutPlaceArrayTest, IntegerTests );
INSTANTIATE_TYPED_TEST_CASE_P( Float, TransformOutPlaceArrayTest, FloatTests );


TEST( ArrayTest, Transform )
{
    int input1[11] = { 7, 0, 0, 3, 3, 3, -5, -5, -5, -5, 3 }; 
    int input2[11] = { 2, 2, 2, 2, 2, 2,  2,  2,  2,  2, 2 }; 
    int std_out[11], bolt_out[11]; 
 
	std::transform(input1, input1+11, input2,  std_out, std::plus< int >());

    bolt::amp::transform(input1, input1+11, input2, bolt_out, bolt::amp::plus< int >());
 
    // compare results
    cmpArrays<int,11>( std_out, bolt_out );

}

TEST( ArrayTest, Serial_Transform )
{
   
    bolt::amp::control ctl = bolt::amp::control::getDefault( );
    ctl.setForceRunMode(bolt::amp::control::SerialCpu);

    int input1[11] = { 7, 0, 0, 3, 3, 3, -5, -5, -5, -5, 3 }; 
    int input2[11] = { 2, 2, 2, 2, 2, 2,  2,  2,  2,  2, 2 }; 
    int std_out[11], bolt_out[11]; 
   
	std::transform(input1, input1+11, input2,  std_out, std::plus< int >());

    bolt::amp::transform(ctl, input1, input1+11, input2, bolt_out, bolt::amp::plus< int >());
 
    // compare results
    cmpArrays<int,11>( std_out, bolt_out );
}

TEST(  ArrayTest, MultiCore_Transform )
{
   
    bolt::amp::control ctl = bolt::amp::control::getDefault( );
    ctl.setForceRunMode(bolt::amp::control::MultiCoreCpu);

    int input1[11] = { 7, 0, 0, 3, 3, 3, -5, -5, -5, -5, 3 }; 
    int input2[11] = { 2, 2, 2, 2, 2, 2,  2,  2,  2,  2, 2 }; 
    int std_out[11], bolt_out[11]; 
   
	std::transform(input1, input1+11, input2,  std_out, std::plus< int >());

    bolt::amp::transform(ctl, input1, input1+11, input2, bolt_out, bolt::amp::plus< int >());
 
    // compare results
    cmpArrays<int,11>( std_out, bolt_out );
}


//INSTANTIATE_TYPED_TEST_CASE_P( UDDTest, TransformArrayTest, UDDTests );






//TransformIf Test cases....


template <typename T>
struct is_odd
{
    bool operator()(T x) const restrict(amp, cpu)
    {
      if (((int)x) % 2)
		  return true;
	  else
		  return false;
    }
};

struct is_odd_a
{
    bool operator()(UDD x) const restrict(amp, cpu)
    {
      if (x.a % 2)
		  return true;
	  else
		  return false;
    }
};

struct udd_identity
{
    bool operator()(UDD x) const restrict(amp, cpu)
    {
      if (x.a ==1 && x.b == 1)
		  return true;
	  else
		  return false;
    }
};


template <typename T>
struct add_3
{
    T operator()(T x) const restrict(amp, cpu)
    {
      return x+3;
    }
};


template <typename InputIterator, typename OutputIterator, typename Predicate, typename UnaryOperator>
void Serial_transform_if(InputIterator first, InputIterator last, OutputIterator result, UnaryOperator op, Predicate p )
{
	typename std::iterator_traits<InputIterator>::difference_type sz = (last - first);
	for(int i=0; i<sz; i++)
	{
		if(p(first[i]))
			result[i] = op(first[i]);
		else
			result[i] = first[i];
	}

}

template <typename InputIterator, typename OutputIterator, typename Predicate, typename UnaryOperator, typename Stencil>
void Serial_transform_if_stencil(InputIterator first, InputIterator last, Stencil s, OutputIterator result, UnaryOperator op, Predicate p )
{
	typename std::iterator_traits<InputIterator>::difference_type sz = (last - first);
	for(int i=0; i<sz; i++)
	{
		if(p(s[i]))
			result[i] = op(first[i]);
		else
			result[i] = first[i];
	}

}


template <typename InputIterator1, typename InputIterator2, typename OutputIterator, typename Predicate, typename BinaryOperator, typename Stencil>
void Serial_transform_if_binary(InputIterator1 first1, InputIterator1 last1, InputIterator2 first2, Stencil s, OutputIterator result, BinaryOperator op, Predicate p )
{
	typename std::iterator_traits<InputIterator1>::difference_type sz = (last1 - first1);

	for(int i=0; i<sz; i++)
	{
		if(p(s[i]))
			result[i] = op(first1[i], first2[i]);
		else
			result[i] = first1[i];
	}

}


TEST( TransformIfStdVector, SimpleTestUint )
{
    int length = 1024;

    std::vector<unsigned int> stdInput( length );
	for(int i=0; i<length; i++)
		stdInput[i] = rand()%10;

	std::vector<unsigned int> boltOutput( length);
	std::vector<unsigned int> stdOutput( length);

    Serial_transform_if(  stdInput.begin( ),  stdInput.end( ), stdOutput.begin(), add_3<unsigned int>(), is_odd<unsigned int>() );
    bolt::amp::transform_if( stdInput.begin( ), stdInput.end( ), boltOutput.begin(), add_3<unsigned int>(), is_odd<unsigned int>() );
	
    cmpArrays( stdOutput, boltOutput);
    
}


TEST( TransformIfDVVector, SimpleTestUint )
{
    int length = 1024;

    std::vector<unsigned int> stdInput( length );
	for(int i=0; i<length; i++)
		stdInput[i] = rand()%10;

	bolt::amp::device_vector<int> boltInput(stdInput.begin(), stdInput.end());
	bolt::amp::device_vector<int> boltOutput( length);
	std::vector<unsigned int> stdOutput( length);

    Serial_transform_if(  stdInput.begin( ),  stdInput.end( ), stdOutput.begin(), add_3<unsigned int>(), is_odd<unsigned int>() );
    bolt::amp::transform_if( boltInput.begin( ), boltInput.end( ), boltOutput.begin(), add_3<unsigned int>(), is_odd<unsigned int>() );
	
    cmpArrays( stdOutput, boltOutput);
    
}


TEST( TransformIfStdVector, SimpleTest )
{
    int length = 1024;

    std::vector<int> stdInput( length );
	for(int i=0; i<length; i++)
		stdInput[i] = rand()%10;

	std::vector<int> boltOutput( length);
	std::vector<int> stdOutput( length);

    Serial_transform_if(  stdInput.begin( ),  stdInput.end( ), stdOutput.begin(), std::negate<int>(), is_odd<int>() );
    bolt::amp::transform_if( stdInput.begin( ), stdInput.end( ), boltOutput.begin(), bolt::amp::negate<int>(), is_odd<int>() );
	
    cmpArrays( stdOutput, boltOutput);
    
}

TEST( TransformIfDVVector, SimpleTest )
{
    int length = 1024;

    std::vector<int> stdInput( length );
	for(int i=0; i<length; i++)
		stdInput[i] = rand()%10;

	bolt::amp::device_vector<int> boltInput(stdInput.begin(), stdInput.end());
	std::vector<int> stdOutput( length);
	bolt::amp::device_vector<int> boltOutput( length);
  
    Serial_transform_if(  stdInput.begin( ),  stdInput.end( ), stdOutput.begin(), std::negate<int>(), is_odd<int>() );
    bolt::amp::transform_if( boltInput.begin( ), boltInput.end( ), boltOutput.begin(), bolt::amp::negate<int>(), is_odd<int>() );

    cmpArrays( stdOutput, boltOutput);
    
}


TEST( FloatTransformIfStdVector, SimpleTest )
{
    int length = 1024;

    std::vector<float> stdInput( length );
	for(int i=0; i<length; i++)
		stdInput[i] = (float)(rand()%10);

	std::vector<float> boltOutput( length);
	std::vector<float> stdOutput( length);

    Serial_transform_if(  stdInput.begin( ),  stdInput.end( ), stdOutput.begin(), std::negate<float>(), is_odd<float>() );
    bolt::amp::transform_if( stdInput.begin( ), stdInput.end( ), boltOutput.begin(), bolt::amp::negate<float>(), is_odd<float>() );
	
    cmpArrays( stdOutput, boltOutput);
    
}

#if 0
//#if (TEST_DOUBLE==1)
//TEST( DoubleTransformIfStdVector, SimpleTest )
//{
//    int length = 1024;
//
//    std::vector<double> stdInput( length );
//	for(int i=0; i<length; i++)
//		stdInput[i] = (double)(rand()%10);
//
//	std::vector<double> boltOutput( length);
//	std::vector<double> stdOutput( length);
//
//    Serial_transform_if(  stdInput.begin( ),  stdInput.end( ), stdOutput.begin(), std::negate<double>(), is_odd<double>() );
//    bolt::amp::transform_if( stdInput.begin( ), stdInput.end( ), boltOutput.begin(), bolt::amp::negate<double>(), is_odd<double>() );
//	
//    cmpArrays( stdOutput, boltOutput);
//    
//}
//#endif
#endif

TEST( FloatTransformIfDVVector, SimpleTest )
{
    int length = 1024;

    std::vector<float> stdInput( length );
	stdInput[0] = (float)(rand()%10);
	for(int i=0; i<length; i++)
		stdInput[i] = stdInput[i] + 1.5f;

	bolt::amp::device_vector<float> boltInput(stdInput.begin(), stdInput.end());
	std::vector<float> stdOutput( length);
	bolt::amp::device_vector<float> boltOutput( length);
  
    Serial_transform_if(  stdInput.begin( ),  stdInput.end( ), stdOutput.begin(), std::negate<float>(), is_odd<float>() );
    bolt::amp::transform_if( boltInput.begin( ), boltInput.end( ), boltOutput.begin(), bolt::amp::negate<float>(), is_odd<float>() );
	

    cmpArrays( stdOutput, boltOutput);
    
}


TEST( UDDTransformIfStdVector, SimpleTest )
{
    int length = 1024;

    std::vector<UDD> stdInput( length );
	for(int i=0; i<length; i++)
	{
		stdInput[i].a = rand()%10;
		stdInput[i].b = rand()%12;
	}

	std::vector<UDD> boltOutput( length);
	std::vector<UDD> stdOutput( length);

    Serial_transform_if(  stdInput.begin( ),  stdInput.end( ), stdOutput.begin(), std::negate<UDD>(), is_odd_a() );
    bolt::amp::transform_if( stdInput.begin( ), stdInput.end( ), boltOutput.begin(), bolt::amp::negate<UDD>(), is_odd_a() );
	
    cmpArrays( stdOutput, boltOutput);
    
}

TEST( UDDTransformIfDVVector, SimpleTest )
{
    int length = 1024;

    std::vector<UDD> stdInput( length );
	
	for(int i=0; i<length; i++)
	{
		stdInput[i].a = rand()%10;
		stdInput[i].b = rand()%12;
	}

	bolt::amp::device_vector<UDD> boltInput(stdInput.begin(), stdInput.end());
	std::vector<UDD> stdOutput( length);
	bolt::amp::device_vector<UDD> boltOutput( length);
  
    Serial_transform_if(  stdInput.begin( ),  stdInput.end( ), stdOutput.begin(), std::negate<UDD>(), is_odd_a() );
    bolt::amp::transform_if( boltInput.begin( ), boltInput.end( ), boltOutput.begin(), bolt::amp::negate<UDD>(), is_odd_a() );
	

    cmpArrays( stdOutput, boltOutput);
    
}

TEST( SimpleArrayTest, Transform )
{
    int input1[11] = { 7, 0, 0, 3, 3, 3, -5, -5, -5, -5, 3 }; 
    int std_out[11], bolt_out[11]; 
 
	Serial_transform_if(  input1, input1+11, std_out, std::negate<int>(), is_odd<int>() );
    bolt::amp::transform_if(  input1, input1+11, bolt_out, bolt::amp::negate<int>(), is_odd<int>() );

    // compare results
    cmpArrays<int,11>( std_out, bolt_out );

}

TEST( FloatSimpleArrayTest, Transform )
{
    float input1[11] = { 7.5f, 0, 0.6f, 3.3f, 5.6f, 3.0f, -5.6f, -2.5f, -4.0f, -3.2f, 3.2f }; 
    float std_out[11], bolt_out[11]; 
 
	Serial_transform_if(  input1, input1+11, std_out, std::negate<float>(), is_odd<float>() );
    bolt::amp::transform_if(  input1, input1+11, bolt_out, bolt::amp::negate<float>(), is_odd<float>() );

    // compare results
    cmpArrays<float,11>( std_out, bolt_out );

}

TEST( UDDSimpleArrayTest, Transform )
{
    UDD input1[11]; 
	for(int i=0; i<11; i++)
	{
		input1[i].a = rand()%10;
		input1[i].b = rand()%12;
	}

    UDD std_out[11], bolt_out[11]; 
 
	Serial_transform_if(  input1, input1+11, std_out, std::negate<UDD>(), is_odd_a() );
    bolt::amp::transform_if(  input1, input1+11, bolt_out, bolt::amp::negate<UDD>(), is_odd_a());

    // compare results
    cmpArrays<UDD,11>( std_out, bolt_out );

}

TEST( UnaryTransformIfStdVector, SimpleTest )
{
    int length = 1024;

    std::vector<int> stdInput( length );
	std::vector<int> stencil( length );
	for(int i=0; i<length; i++)
	{
		stdInput[i] = rand()%10;
		stencil[i] = rand()%2;
	}

	std::vector<int> boltOutput( length);
	std::vector<int> stdOutput( length);

	Serial_transform_if_stencil( stdInput.begin( ), stdInput.end( ), stencil.begin(), stdOutput.begin(), std::negate<int>(), std::identity<int>());
    bolt::amp::transform_if( stdInput.begin( ), stdInput.end( ), stencil.begin(), boltOutput.begin(), bolt::amp::negate<int>(), bolt::amp::identity<int>());
	
    cmpArrays( stdOutput, boltOutput);
    
}

TEST( UnaryTransformIfDVVector, SimpleTest )
{
    int length = 1024;

    std::vector<int> stdInput( length );
	std::vector<int> stencil( length );
	for(int i=0; i<length; i++)
	{
		stdInput[i] = rand()%10;
		stencil[i] = rand()%2;
	}

	bolt::amp::device_vector<int> boltInput(stdInput.begin( ), stdInput.end( ));
	bolt::amp::device_vector<int> boltStencil(stencil.begin( ), stencil.end( ));
	bolt::amp::device_vector<int> boltOutput( length);
	std::vector<int> stdOutput( length);

    Serial_transform_if_stencil( stdInput.begin( ), stdInput.end( ), stencil.begin(), stdOutput.begin(), std::negate<int>(), std::identity<int>());
    bolt::amp::transform_if( boltInput.begin( ), boltInput.end( ), boltStencil.begin(), boltOutput.begin(), bolt::amp::negate<int>(), bolt::amp::identity<int>() );

    cmpArrays( stdOutput, boltOutput);
    
}


TEST( UDDUnaryTransformIfStdVector, SimpleTest )
{
    int length = 1024;

    std::vector<UDD> stdInput( length );
	std::vector<UDD> stencil( length );
	for(int i=0; i<length; i++)
	{
		stdInput[i].a = rand()%10;
		stdInput[i].b = rand()%12;

		stencil[i].a = rand()%2;
		stencil[i].b = stencil[i].a;
	}

	std::vector<UDD> boltOutput( length);
	std::vector<UDD> stdOutput( length);

	Serial_transform_if_stencil( stdInput.begin( ), stdInput.end( ), stencil.begin(), stdOutput.begin(), std::negate<UDD>(), udd_identity());
    bolt::amp::transform_if( stdInput.begin( ), stdInput.end( ), stencil.begin(), boltOutput.begin(), bolt::amp::negate<UDD>(), udd_identity());
	
    cmpArrays( stdOutput, boltOutput);
    
}

TEST( UDDUnaryTransformIfDVVector, SimpleTest )
{
    int length = 1024;

    std::vector<UDD> stdInput( length );
	std::vector<UDD> stencil( length );
	for(int i=0; i<length; i++)
	{
		stdInput[i].a = rand()%10;
		stdInput[i].b = rand()%12;

		stencil[i].a = rand()%2;
		stencil[i].b = stencil[i].a;
	}

	bolt::amp::device_vector<UDD> boltInput(stdInput.begin( ), stdInput.end( ));
	bolt::amp::device_vector<UDD> boltStencil(stencil.begin( ), stencil.end( ));
	bolt::amp::device_vector<UDD> boltOutput( length);
	std::vector<UDD> stdOutput( length);

    Serial_transform_if_stencil( stdInput.begin( ), stdInput.end( ), stencil.begin(), stdOutput.begin(), std::negate<UDD>(), udd_identity());
    bolt::amp::transform_if( boltInput.begin( ), boltInput.end( ), boltStencil.begin(), boltOutput.begin(), bolt::amp::negate<UDD>(), udd_identity());

    cmpArrays( stdOutput, boltOutput);
    
}



TEST( FloatUnaryTransformIfStdVector, SimpleTest )
{
    int length = 1024;

    std::vector<float> stdInput( length );
	std::vector<float> stencil( length );
	for(int i=0; i<length; i++)
	{
		stdInput[i] = (float) (rand()%10);
		stencil[i] = (float) (rand()%2);
	}

	std::vector<float> boltOutput( length);
	std::vector<float> stdOutput( length);

	Serial_transform_if_stencil( stdInput.begin( ), stdInput.end( ), stencil.begin(), stdOutput.begin(), std::negate<float>(), std::identity<float>());
    bolt::amp::transform_if( stdInput.begin( ), stdInput.end( ), stencil.begin(), boltOutput.begin(), bolt::amp::negate<float>(), bolt::amp::identity<float>());
	
    cmpArrays( stdOutput, boltOutput);
    
}

TEST( FloatUnaryTransformIfDVVector, SimpleTest )
{
    int length = 1024;

    std::vector<float> stdInput( length );
	std::vector<float> stencil( length );
	for(int i=0; i<length; i++)
	{
		stdInput[i] = (float) (rand()%10);
		stencil[i] = (float) (rand()%2);
	}

	bolt::amp::device_vector<float> boltInput(stdInput.begin( ), stdInput.end( ));
	bolt::amp::device_vector<float> boltStencil(stencil.begin( ), stencil.end( ));
	bolt::amp::device_vector<float> boltOutput( length);
	std::vector<float> stdOutput( length);

    Serial_transform_if_stencil( stdInput.begin( ), stdInput.end( ), stencil.begin(), stdOutput.begin(), std::negate<float>(), std::identity<float>());
    bolt::amp::transform_if( boltInput.begin( ), boltInput.end( ), boltStencil.begin(), boltOutput.begin(), bolt::amp::negate<float>(), bolt::amp::identity<float>() );

    cmpArrays( stdOutput, boltOutput);
    
}

TEST( UnaryArrayTest, Transform )
{
    int input1[11] = { 7, 0, 0, 3, 3, 3, -5, -5, -5, -5, 3 }; 
    int input2[11] = { 0, 1, 1, 0, 0, 0,  1,  0,  1,  1, 0 }; 
    int std_out[11], bolt_out[11]; 
 
	Serial_transform_if_stencil(  input1, input1+11, input2, std_out, std::negate<int>(), std::identity<int>() );
    bolt::amp::transform_if(  input1, input1+11, input2, bolt_out, bolt::amp::negate<int>(),  bolt::amp::identity<int>());

    // compare results
    cmpArrays<int,11>( std_out, bolt_out );

}

TEST( FloatUnaryArrayTest, Transform )
{
    float input1[11] = { 7.5f, 0, 0.6f, 3.3f, 5.6f, 3.0f, -5.6f, -2.5f, -4.0f, -3.2f, 3.2f }; 
	float input2[11] = { 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f,  1.0f,  0.0f,  1.0f,  1.0f, 0.0f }; 
    float std_out[11], bolt_out[11]; 
 
	Serial_transform_if_stencil(  input1, input1+11, input2, std_out, std::negate<float>(), std::identity<float>() );
    bolt::amp::transform_if(  input1, input1+11, input2, bolt_out, bolt::amp::negate<float>(), bolt::amp::identity<float>() );

    // compare results
    cmpArrays<float,11>( std_out, bolt_out );

}

TEST( UDDUnaryArrayTest, Transform )
{
    UDD input1[11], input2[11]; 
	for(int i=0; i<11; i++)
	{
		input1[i].a = rand()%10;
		input1[i].b = rand()%12;

		input2[i].a = rand()%2;
		input2[i].b = input2[i].a;
	}

    UDD std_out[11], bolt_out[11]; 
 
	Serial_transform_if_stencil(  input1, input1+11, input2, std_out, std::negate<UDD>(), is_odd_a() );
    bolt::amp::transform_if(  input1, input1+11, input2, bolt_out, bolt::amp::negate<UDD>(), is_odd_a());

    // compare results
    cmpArrays<UDD,11>( std_out, bolt_out );

}


TEST( BinaryTransformIfStdVector, SimpleTest )
{
    int length = 1024;

    std::vector<int> stdInput1( length );
	std::vector<int> stdInput2( length );
	std::vector<int> stencil( length );
	for(int i=0; i<length; i++)
	{
		stdInput1[i] = rand()%10;
		stdInput2[i] = rand()%8;
		stencil[i] = rand()%2;
	}

	std::vector<int> boltOutput( length);
	std::vector<int> stdOutput( length);

    Serial_transform_if_binary( stdInput1.begin( ),  stdInput1.end( ), stdInput2.begin( ), stencil.begin( ), stdOutput.begin(), std::plus<int>(), std::identity<int>() );
    bolt::amp::transform_if( stdInput1.begin( ), stdInput1.end( ), stdInput2.begin( ), stencil.begin(), boltOutput.begin(), bolt::amp::plus<int>(), bolt::amp::identity<int>() );
	
    cmpArrays( stdOutput, boltOutput);
    
}

TEST( BinaryTransformIfDVVector, SimpleTest )
{
    int length = 1024;

    std::vector<int> stdInput1( length );
	std::vector<int> stdInput2( length );
	std::vector<int> stencil( length );
	for(int i=0; i<length; i++)
	{
		stdInput1[i] = rand()%10;
		stdInput2[i] = rand()%8;
		stencil[i] = rand()%2;
	}

	bolt::amp::device_vector<int> dvInput1(stdInput1.begin( ),  stdInput1.end( ));
	bolt::amp::device_vector<int> dvInput2(stdInput2.begin( ),  stdInput2.end( ));
	bolt::amp::device_vector<int> dvInput3(stencil.begin( ),  stencil.end( ));
	bolt::amp::device_vector<int> boltOutput( length);
	std::vector<int> stdOutput( length);

    Serial_transform_if_binary( stdInput1.begin( ),  stdInput1.end( ), stdInput2.begin( ), stencil.begin( ), stdOutput.begin(), std::plus<int>(), std::identity<int>() );
    bolt::amp::transform_if( dvInput1.begin( ), dvInput1.end( ), dvInput2.begin( ), dvInput3.begin(), boltOutput.begin(), bolt::amp::plus<int>(), bolt::amp::identity<int>() );


    cmpArrays( stdOutput, boltOutput);
    
}


TEST( FloatBinaryTransformIfStdVector, SimpleTest )
{
    int length = 1024;

    std::vector<float> stdInput1( length );
	std::vector<float> stdInput2( length );
	std::vector<float> stencil( length );
	for(int i=0; i<length; i++)
	{
		stdInput1[i] =  (float)(rand()%10);
		stdInput2[i] = (float) (rand()%8);
		stencil[i] = (float) (rand()%2);
	}

	std::vector<float> boltOutput( length);
	std::vector<float> stdOutput( length);

    Serial_transform_if_binary( stdInput1.begin( ),  stdInput1.end( ), stdInput2.begin( ), stencil.begin( ), stdOutput.begin(), std::plus<float>(), std::identity<float>() );
    bolt::amp::transform_if( stdInput1.begin( ), stdInput1.end( ), stdInput2.begin( ), stencil.begin(), boltOutput.begin(), bolt::amp::plus<float>(), bolt::amp::identity<float>() );
	
    cmpArrays( stdOutput, boltOutput);
    
}

TEST( FloatBinaryTransformIfDVVector, SimpleTest )
{
    int length = 1024;

    std::vector<float> stdInput1( length );
	std::vector<float> stdInput2( length );
	std::vector<float> stencil( length );
	for(int i=0; i<length; i++)
	{
		stdInput1[i] = (float) ( rand()%10);
		stdInput2[i] = (float) ( rand()%8 );
		stencil[i] =  (float)  ( rand()%2 );
	}

	bolt::amp::device_vector<float> dvInput1(stdInput1.begin( ),  stdInput1.end( ));
	bolt::amp::device_vector<float> dvInput2(stdInput2.begin( ),  stdInput2.end( ));
	bolt::amp::device_vector<float> dvInput3(stencil.begin( ),  stencil.end( ));
	bolt::amp::device_vector<float> boltOutput( length);
	std::vector<float> stdOutput( length);

    Serial_transform_if_binary( stdInput1.begin( ),  stdInput1.end( ), stdInput2.begin( ), stencil.begin( ), stdOutput.begin(), std::plus<float>(), std::identity<float>() );
    bolt::amp::transform_if( dvInput1.begin( ), dvInput1.end( ), dvInput2.begin( ), dvInput3.begin(), boltOutput.begin(), bolt::amp::plus<float>(), bolt::amp::identity<float>() );

    cmpArrays( stdOutput, boltOutput);
    
}


TEST( BinaryArrayTest, Transform )
{
    int input1[11] = { 7, 0, 0, 3, 3, 3, -5, -5, -5, -5, 3 }; 
	int input2[11] = { 6, 4, -3, 3, 5, 13, -5, -15, 9, -10, 0 }; 
    int input3[11] = { 0, 1, 1, 0, 0, 0,  1,  0,  1,  1, 0 }; 
    int std_out[11], bolt_out[11]; 
 
	Serial_transform_if_binary(  input1, input1+11, input2, input3, std_out, std::plus<int>(), std::identity<int>());
    bolt::amp::transform_if(  input1, input1+11, input2, input3, bolt_out, bolt::amp::plus<int>(), bolt::amp::identity<int>() );

    // compare results
    cmpArrays<int,11>( std_out, bolt_out );

}

TEST( FloatBinaryArrayTest, Transform )
{
    float input1[11] = { 7.5f, 0, 0.6f, 3.3f, 5.6f, 3.0f, -5.6f, -2.5f, -4.0f, -3.2f, 3.2f }; 
	float input2[11] = { 6.0f, 4.5, 0.4f, 8.3f, 7.6f, 2.8f, 5.6f, -2.5f, -9.0f, 30.2f, -0.2f }; 
	float input3[11] = { 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f,  1.0f,  0.0f,  1.0f,  1.0f, 0.0f }; 
    float std_out[11], bolt_out[11]; 
 
	Serial_transform_if_binary(  input1, input1+11, input2, input3, std_out, std::plus<float>(), std::identity<float>() );
    bolt::amp::transform_if(  input1, input1+11, input2, input3, bolt_out, bolt::amp::plus<float>(), bolt::amp::identity<float>() );

    // compare results
    cmpArrays<float,11>( std_out, bolt_out );

}

TEST( UDDBinaryArrayTest, Transform )
{
    UDD input1[11], input2[11], input3[11]; 
	for(int i=0; i<11; i++)
	{
		input1[i].a = rand()%10;
		input1[i].b = rand()%12;

		input2[i].a = rand()%15;
		input2[i].b = rand()%9;

		input2[i].a = rand()%2;
		input2[i].b = input2[i].a;
	}

    UDD std_out[11], bolt_out[11]; 
 
	Serial_transform_if_binary(  input1, input1+11, input2, input3, std_out, std::plus<UDD>(), is_odd_a() );
    bolt::amp::transform_if(  input1, input1+11, input2, input3, bolt_out, bolt::amp::plus<UDD>(), is_odd_a());

    // compare results
    cmpArrays<UDD,11>( std_out, bolt_out );

}


TEST( UDDBinaryTransformIfStdVector, SimpleTest )
{
    int length = 1024;

    std::vector<UDD> stdInput1( length );
	std::vector<UDD> stdInput2( length );
	std::vector<UDD> stencil( length );
	for(int i=0; i<length; i++)
	{
		stdInput1[i].a = rand()%10;
		stdInput1[i].b = rand()%12;

		stdInput2[i].a = rand()%13;
		stdInput2[i].b = rand()%15;

		stencil[i].a = rand()%2;
		stencil[i].b = stencil[i].a;
	}

	std::vector<UDD> boltOutput( length);
	std::vector<UDD> stdOutput( length);

    Serial_transform_if_binary( stdInput1.begin( ),  stdInput1.end( ), stdInput2.begin( ), stencil.begin( ), stdOutput.begin(), std::plus<UDD>(), udd_identity() );
    bolt::amp::transform_if( stdInput1.begin( ), stdInput1.end( ), stdInput2.begin( ), stencil.begin(), boltOutput.begin(), bolt::amp::plus<UDD>(), udd_identity() );
	
    cmpArrays( stdOutput, boltOutput);
    
}

TEST( UDDBinaryTransformIfDVVector, SimpleTest )
{
    int length = 1024;

    std::vector<UDD> stdInput1( length );
	std::vector<UDD> stdInput2( length );
	std::vector<UDD> stencil( length );
	for(int i=0; i<length; i++)
	{
		stdInput1[i].a = rand()%10;
		stdInput1[i].b = rand()%12;

		stdInput2[i].a = rand()%13;
		stdInput2[i].b = rand()%15;

		stencil[i].a = rand()%2;
		stencil[i].b = stencil[i].a;
	}


	bolt::amp::device_vector<UDD> dvInput1(stdInput1.begin( ),  stdInput1.end( ));
	bolt::amp::device_vector<UDD> dvInput2(stdInput2.begin( ),  stdInput2.end( ));
	bolt::amp::device_vector<UDD> dvInput3(stencil.begin( ),  stencil.end( ));
	bolt::amp::device_vector<UDD> boltOutput( length);
	std::vector<UDD> stdOutput( length);

    Serial_transform_if_binary( stdInput1.begin( ),  stdInput1.end( ), stdInput2.begin( ), stencil.begin( ), stdOutput.begin(), std::plus<UDD>(), udd_identity() );
    bolt::amp::transform_if( dvInput1.begin( ), dvInput1.end( ), dvInput2.begin( ), dvInput3.begin(), boltOutput.begin(), bolt::amp::plus<UDD>(), udd_identity() );

    cmpArrays( stdOutput, boltOutput);
    
}




int main(int argc, char* argv[])
{
    ::testing::InitGoogleTest( &argc, &argv[ 0 ] );

    //    Set the standard OpenCL wait behavior to help debugging
    bolt::amp::control& myControl = bolt::amp::control::getDefault( );
    myControl.setWaitMode( bolt::amp::control::NiceWait );
    myControl.setForceRunMode( bolt::amp::control::Automatic );  // choose tbb


    int retVal = RUN_ALL_TESTS( );

	myControl.setForceRunMode( bolt::amp::control::SerialCpu );  // choose tbb
    retVal = RUN_ALL_TESTS( );

    myControl.setForceRunMode( bolt::amp::control::MultiCoreCpu );  // choose tbb
    retVal = RUN_ALL_TESTS( );
    
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