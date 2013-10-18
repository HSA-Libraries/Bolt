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
#include <vector>
//#include <array>

//#include "bolt/cl/scan.h"
#include "bolt/cl/transform_scan.h"
#include "bolt/unicode.h"
#include "bolt/miniDump.h"

#include <gtest/gtest.h>
//#include <boost/shared_array.hpp>
#define TEST_DOUBLE 1

#include <boost/program_options.hpp>
namespace po = boost::program_options;

/////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Below are helper routines to compare the results of two arrays for googletest
//  They return an assertion object that googletest knows how to track

/******************************************************************************
 *  Double x4
 *****************************************************************************/
#if(TEST_DOUBLE == 1)
BOLT_FUNCTOR(uddtD4,
struct uddtD4
{
    double a;
    double b;
    double c;
    double d;

    bool operator==(const uddtD4& rhs) const
    {
        bool equal = true;
        double th = 0.0000000001;
        if (rhs.a < th && rhs.a > -th)
            equal = ( (1.0*a - rhs.a) < th && (1.0*a - rhs.a) > -th) ? equal : false;
        else
            equal = ( (1.0*a - rhs.a)/rhs.a < th && (1.0*a - rhs.a)/rhs.a > -th) ? equal : false;
        if (rhs.b < th && rhs.b > -th)
            equal = ( (1.0*b - rhs.b) < th && (1.0*b - rhs.b) > -th) ? equal : false;
        else
            equal = ( (1.0*b - rhs.b)/rhs.b < th && (1.0*b - rhs.b)/rhs.b > -th) ? equal : false;
        if (rhs.c < th && rhs.c > -th)
            equal = ( (1.0*c - rhs.c) < th && (1.0*c - rhs.c) > -th) ? equal : false;
        else
            equal = ( (1.0*c - rhs.c)/rhs.c < th && (1.0*c - rhs.c)/rhs.c > -th) ? equal : false;
        if (rhs.d < th && rhs.d > -th)
            equal = ( (1.0*d - rhs.d) < th && (1.0*d - rhs.d) > -th) ? equal : false;
        else
            equal = ( (1.0*d - rhs.d)/rhs.d < th && (1.0*d - rhs.d)/rhs.d > -th) ? equal : false;
        return equal;
    }
    uddtD4 operator-() const
    {
        uddtD4 r;
        r.a = -a;
        r.b = -b;
        r.c = -c;
        r.d = -d;
        return r;
    }
    uddtD4 operator*(const uddtD4& rhs)
    {
        uddtD4 r;
        r.a = a*a;
        r.b = b*b;
        r.c = c*c;
        r.d = d*d;
        return r;
    }
};
);

BOLT_CREATE_TYPENAME( bolt::cl::device_vector< uddtD4 >::iterator );
BOLT_CREATE_CLCODE( bolt::cl::device_vector< uddtD4 >::iterator, bolt::cl::deviceVectorIteratorTemplate );

BOLT_FUNCTOR(MultD4,
struct MultD4
{
    uddtD4 operator()(const uddtD4 &lhs, const uddtD4 &rhs) const
    {
        uddtD4 _result;
        _result.a = lhs.a*rhs.a;
        _result.b = lhs.b*rhs.b;
        _result.c = lhs.c*rhs.c;
        _result.d = lhs.d*rhs.d;
        return _result;
    };
};
);
uddtD4 identityMultD4 = { 1.0, 1.0, 1.0, 1.0 };
uddtD4 initialMultD4  = { 1.00001, 1.000003, 1.0000005, 1.00000007 };

BOLT_FUNCTOR(NegateD4,
    struct NegateD4
    {
        uddtD4 operator()(const uddtD4& rhs) const
        {
            uddtD4 ret;
            ret.a = -rhs.a;
            ret.b = -rhs.b;
            ret.c = -rhs.c;
            ret.d = -rhs.d;
            return ret;
        }
    };
);
NegateD4 nD4;

BOLT_FUNCTOR(SquareD4,
    struct SquareD4
    {
        uddtD4 operator()(const uddtD4& rhs) const
        {
            uddtD4 ret;
            ret.a = rhs.a*rhs.a;
            ret.b = rhs.b*rhs.b;
            ret.c = rhs.c*rhs.c;
            ret.d = rhs.d*rhs.d;
            return ret;
        }
    };
);
SquareD4 sD4;
#endif

/******************************************************************************
 *  Integer x2
 *****************************************************************************/
BOLT_FUNCTOR(uddtI2,
struct uddtI2
{
    int a;
    int b;

    bool operator==(const uddtI2& rhs) const
    {
        bool equal = true;
        equal = ( a == rhs.a ) ? equal : false;
        equal = ( b == rhs.b ) ? equal : false;
        return equal;
    }
    uddtI2 operator-() const
    {
        uddtI2 r;
        r.a = -a;
        r.b = -b;
        return r;
    }
    uddtI2 operator*(const uddtI2& rhs)
    {
        uddtI2 r;
        r.a = a*a;
        r.b = b*b;
        return r;
    }
};
);

BOLT_CREATE_TYPENAME( bolt::cl::device_vector< uddtI2 >::iterator );
BOLT_CREATE_CLCODE( bolt::cl::device_vector< uddtI2 >::iterator, bolt::cl::deviceVectorIteratorTemplate );

BOLT_FUNCTOR(AddI2,
struct AddI2
{
    uddtI2 operator()(const uddtI2 &lhs, const uddtI2 &rhs) const
    {
        uddtI2 _result;
        _result.a = lhs.a+rhs.a;
        _result.b = lhs.b+rhs.b;
        return _result;
    };
};
);
uddtI2 identityAddI2 = {  0, 0 };
uddtI2 initialAddI2  = { -1, 2 };

BOLT_FUNCTOR(NegateI2,
    struct NegateI2
    {
        uddtI2 operator()(const uddtI2& rhs) const
        {
            uddtI2 ret;
            ret.a = -rhs.a;
            ret.b = -rhs.b;
            return ret;
        }
    };
);
NegateI2 nI2;

BOLT_FUNCTOR(SquareI2,
    struct SquareI2
    {
        uddtI2 operator()(const uddtI2& rhs) const
        {
            uddtI2 ret;
            ret.a = rhs.a*rhs.a;
            ret.b = rhs.b*rhs.b;
            return ret;
        }
    };
);
SquareI2 sI2;

/******************************************************************************
 *  Heterogeneous Unary Operator

 *****************************************************************************/
#if (TEST_DOUBLE == 1)
BOLT_FUNCTOR(squareD4I2,
    struct squareD4I2
    {
        uddtI2 operator()(const uddtD4& rhs) const
        {
            uddtI2 ret;
            ret.a = (int) (rhs.a*rhs.b);
            ret.b = (int) (rhs.c*rhs.d);
            return ret;
        }
    };
);
squareD4I2 sD4I2;


/******************************************************************************
 *  Mixed float and int
 *****************************************************************************/

BOLT_FUNCTOR(uddtM3,
struct uddtM3
{
    int a;
    float        b;
    double       c;

    bool operator==(const uddtM3& rhs) const
    {
        bool equal = true;
        float ths = 0.00001f;
        double thd = 0.0000000001;
        equal = ( a == rhs.a ) ? equal : false;
        if (rhs.b < ths && rhs.b > -ths)
            equal = ( (1.0*b - rhs.b) < ths && (1.0*b - rhs.b) > -ths) ? equal : false;
        else
            equal = ( (1.0*b - rhs.b)/rhs.b < ths && (1.0*b - rhs.b)/rhs.b > -ths) ? equal : false;
        if (rhs.c < thd && rhs.c > -thd)
            equal = ( (1.0*c - rhs.c) < thd && (1.0*c - rhs.c) > -thd) ? equal : false;
        else
            equal = ( (1.0*c - rhs.c)/rhs.c < thd && (1.0*c - rhs.c)/rhs.c > -thd) ? equal : false;
        return equal;
    }
    uddtM3 operator-() const
    {
        uddtM3 r;
        r.a = -a;
        r.b = -b;
        r.c = -c;
        return r;
    }
    uddtM3 operator*(const uddtM3& rhs)
    {
        uddtM3 r;
        r.a = a*a;
        r.b = b*b;
        r.c = c*c;
        return r;
    }
};
);

BOLT_CREATE_TYPENAME( bolt::cl::device_vector< uddtM3 >::iterator );
BOLT_CREATE_CLCODE( bolt::cl::device_vector< uddtM3 >::iterator, bolt::cl::deviceVectorIteratorTemplate );



BOLT_FUNCTOR(MixM3,
struct MixM3
{
    uddtM3 operator()(const uddtM3 &lhs, const uddtM3 &rhs) const
    {
        uddtM3 _result;
        _result.a = lhs.a^rhs.a;
        _result.b = lhs.b+rhs.b;
        _result.c = lhs.c*rhs.c;
        return _result;
    };
};
);
uddtM3 identityMixM3 = { 0, 0.f, 0.0 };
uddtM3 initialMixM3  = { 2, 3.f, 1.000001 };

BOLT_FUNCTOR(NegateM3,
    struct NegateM3
    {
        uddtM3 operator()(const uddtM3& rhs) const
        {
            uddtM3 ret;
            ret.a = -rhs.a;
            ret.b = -rhs.b;
            ret.c = -rhs.c;
            return ret;
        }
    };
);
NegateM3 nM3;

BOLT_FUNCTOR(SquareM3,
    struct SquareM3
    {
        uddtM3 operator()(const uddtM3& rhs) const
        {
            uddtM3 ret;
            ret.a = rhs.a*rhs.a;
            ret.b = rhs.b*rhs.b;
            ret.c = rhs.c*rhs.c;
            return ret;
        }
    };
);
SquareM3 sM3;
#endif


  template<
    typename oType,
    typename BinaryFunction,
    typename T>
oType*
Serial_scan(
    oType *values,
    oType *result,
    unsigned int  num,
    const BinaryFunction binary_op,
    const bool Incl,
    const T &init)
{
    oType  sum, temp;
    if(Incl){
      *result = *values; // assign value
      sum = *values;
    }
    else {
        temp = *values;
       *result = (oType)init;
       sum = binary_op( *result, temp);
    }
    for ( unsigned int i= 1; i<num; i++)
    {
        oType currentValue = *(values + i); // convertible
        if (Incl)
        {
            oType r = binary_op( sum, currentValue);
            *(result + i) = r;
            sum = r;
        }
        else // new segment
        {
            *(result + i) = sum;
            sum = binary_op( sum, currentValue);

        }
    }
    return result;
}


///////////////////////////////////////////////////////////////////////////////

template< typename T >
::testing::AssertionResult cmpArrays( const T ref, const T calc, size_t N )
{
    for( size_t i = 0; i < N; ++i )
    {
        EXPECT_EQ( ref[ i ], calc[ i ] ) << _T( "Where i = " ) << i;
    }

    return ::testing::AssertionSuccess( );
};

template< typename T, size_t N >
::testing::AssertionResult cmpArrays( const T (&ref)[N], const T (&calc)[N] )
{
    for( size_t i = 0; i < N; ++i )
    {
        EXPECT_EQ( ref[ i ], calc[ i ] ) << _T( "Where i = " ) << i;
    }

    return ::testing::AssertionSuccess( );
};

//  Primary class template for std::array types
//  The struct wrapper is necessary to partially specialize the member function
/*
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
    static ::testing::AssertionResult cmpArrays( const std::array< float, N >& ref, const std::array< float, N >& calc)
    {
        for( size_t i = 0; i < N; ++i )
        {
            EXPECT_FLOAT_EQ( ref[ i ], calc[ i ] ) << _T( "Where i = " ) << i;
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

#if TEST_DOUBLE
::testing::AssertionResult cmpArrays( const std::vector< double >& ref, const std::vector< double >& calc )
{
    for( size_t i = 0; i < ref.size( ); ++i )
    {
        EXPECT_DOUBLE_EQ( ref[ i ], calc[ i ] ) << _T( "Where i = " ) << i;
    }

    return ::testing::AssertionSuccess( );
}
#endif
*/
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

class scanStdVectorWithIters:public ::testing::TestWithParam<int>
{
protected:
    int myStdVectSize;
public:
    scanStdVectorWithIters():myStdVectSize(GetParam()){
    }
};

typedef scanStdVectorWithIters TransformScan;
typedef scanStdVectorWithIters TransformScanMultiCore;
TEST_P (TransformScan, InclTransformScanTestFloat)
{
    bolt::cl::device_vector< float > input( myStdVectSize, 2.f);
    bolt::cl::device_vector< float > output( myStdVectSize, 2.f);
    std::vector< float > refInput( myStdVectSize, 2.f);
   // call scan
    bolt::cl::plus<float> aI2;
    bolt::cl::negate<float> nI2;


    bolt::cl::transform_inclusive_scan( input.begin(), input.end(), output.begin(), nI2, aI2 );
    ::std::transform(   refInput.begin(), refInput.end(),  refInput.begin(), nI2); // transform in-place
    ::std::partial_sum( refInput.begin(), refInput.end(), refInput.begin(), aI2); // out-of-place scan
   
    cmpArrays(output, refInput);
    
}

TEST_P (TransformScan, ExclTransformScanTestFloat)
{
    bolt::cl::device_vector< float > input( myStdVectSize, 2.f);
    bolt::cl::device_vector< float > output( myStdVectSize, 2.f);
    std::vector< float > refInput( myStdVectSize, 2.f);
   // call scan
    
    bolt::cl::negate<float> nI2;
    bolt::cl::plus< float > mM3;
    bolt::cl::transform_exclusive_scan( input.begin(), input.end(), output.begin(), nI2, 3.0f, mM3 );
    std::transform(   refInput.begin(), refInput.end(),  refInput.begin(), nI2);
    Serial_scan<float,  bolt::cl::plus< float >, float>(&refInput[0], &refInput[0], myStdVectSize, mM3, false, 3.0f);

    cmpArrays(output, refInput);
    
} 

TEST_P (TransformScanMultiCore, InclTransformScanTestFloat)
{
    bolt::cl::device_vector< float > input( myStdVectSize, 2.f);
    bolt::cl::device_vector< float > output( myStdVectSize, 2.f);
    std::vector< float > refInput( myStdVectSize, 2.f);
   // call scan
    bolt::cl::plus<float> aI2;
    bolt::cl::negate<float> nI2;

    bolt::cl::control ctl = bolt::cl::control::getDefault( );
    ctl.setForceRunMode(bolt::cl::control::MultiCoreCpu);

    bolt::cl::transform_inclusive_scan(ctl, input.begin(), input.end(), output.begin(), nI2, aI2 );
    ::std::transform(   refInput.begin(), refInput.end(),  refInput.begin(), nI2); // transform in-place
    ::std::partial_sum( refInput.begin(), refInput.end(), refInput.begin(), aI2); // out-of-place scan
   
    cmpArrays(output, refInput);
    
} 

TEST_P (TransformScanMultiCore, ExclTransformScanTestFloat)
{
    bolt::cl::device_vector< float > input( myStdVectSize, 2.f);
    bolt::cl::device_vector< float > output( myStdVectSize, 2.f);
    std::vector< float > refInput( myStdVectSize, 2.f);
   // call scan
    
    bolt::cl::negate<float> nI2;
    bolt::cl::plus< float > mM3;

     bolt::cl::control ctl = bolt::cl::control::getDefault( );
    ctl.setForceRunMode(bolt::cl::control::MultiCoreCpu);

    bolt::cl::transform_exclusive_scan( ctl, input.begin(), input.end(), output.begin(), nI2, 3.0f, mM3 );
    std::transform(   refInput.begin(), refInput.end(),  refInput.begin(), nI2);
    Serial_scan<float,  bolt::cl::plus< float >, float>(&refInput[0], &refInput[0], myStdVectSize, mM3, false, 3.0f);

    cmpArrays(output, refInput);
    
} 

//INSTANTIATE_TEST_CASE_P(TransformScanIterFloatLimit, TransformScan, ::testing::Range(1025, 65535, 5111)); 
//INSTANTIATE_TEST_CASE_P(TransformScanIterFloatLimit, TransformScanMultiCore, ::testing::Range(1025, 65535, 5111));

#if(TEST_DOUBLE == 1)
TEST_P (TransformScan, InclTransformScanTestDouble)
{
    bolt::cl::device_vector< double > input( myStdVectSize, 2.5);
    bolt::cl::device_vector< double > output( myStdVectSize, 2.5);
    std::vector< double > refInput( myStdVectSize, 2.5);
   // call scan
    bolt::cl::plus<double> aI2;
    bolt::cl::negate<double> nI2;


    bolt::cl::transform_inclusive_scan( input.begin(), input.end(), output.begin(), nI2, aI2 );
    ::std::transform(   refInput.begin(), refInput.end(),  refInput.begin(), nI2); // transform in-place
    ::std::partial_sum( refInput.begin(), refInput.end(), refInput.begin(), aI2); // out-of-place scan
   
    cmpArrays(output, refInput);
    
}

TEST_P (TransformScan, ExclTransformScanTestDouble)
{
    bolt::cl::device_vector< double > input( myStdVectSize, 2.5);
    bolt::cl::device_vector< double > output( myStdVectSize, 2.5);
    std::vector< double > refInput( myStdVectSize, 2.5);
   // call scan
    
    bolt::cl::negate<double> nI2;
    bolt::cl::plus< double> mM3;
    bolt::cl::transform_exclusive_scan( input.begin(), input.end(), output.begin(), nI2, 3.0, mM3 );
    std::transform(   refInput.begin(), refInput.end(),  refInput.begin(), nI2);
    Serial_scan<double,  bolt::cl::plus< double >, double>(&refInput[0], &refInput[0], myStdVectSize, mM3, false, 3.0);

    cmpArrays(output, refInput);
    
} 

TEST_P (TransformScanMultiCore, InclTransformScanTestDouble)
{
    bolt::cl::device_vector< double > input( myStdVectSize, 2.5);
    bolt::cl::device_vector< double > output( myStdVectSize, 2.5);
    std::vector< double > refInput( myStdVectSize, 2.5);
   // call scan
    bolt::cl::plus<double> aI2;
    bolt::cl::negate<double> nI2;

    bolt::cl::control ctl = bolt::cl::control::getDefault( );
    ctl.setForceRunMode(bolt::cl::control::MultiCoreCpu);

    bolt::cl::transform_inclusive_scan(ctl, input.begin(), input.end(), output.begin(), nI2, aI2 );
    ::std::transform(   refInput.begin(), refInput.end(),  refInput.begin(), nI2); // transform in-place
    ::std::partial_sum( refInput.begin(), refInput.end(), refInput.begin(), aI2); // out-of-place scan
   
    cmpArrays(output, refInput);
    
} 

TEST_P (TransformScanMultiCore, ExclTransformScanTestDouble)
{
    bolt::cl::device_vector< double > input( myStdVectSize, 2.5);
    bolt::cl::device_vector< double > output( myStdVectSize, 2.5);
    std::vector< double > refInput( myStdVectSize, 2.5);
   // call scan
    
    bolt::cl::negate<double> nI2;
    bolt::cl::plus< double > mM3;

     bolt::cl::control ctl = bolt::cl::control::getDefault( );
    ctl.setForceRunMode(bolt::cl::control::MultiCoreCpu);

    bolt::cl::transform_exclusive_scan( ctl, input.begin(), input.end(), output.begin(), nI2, 3.0f, mM3 );
    std::transform(   refInput.begin(), refInput.end(),  refInput.begin(), nI2);
    Serial_scan<double,  bolt::cl::plus< double >, double>(&refInput[0], &refInput[0], myStdVectSize, mM3, false, 3.0f);

    cmpArrays(output, refInput);
    
} 

//INSTANTIATE_TEST_CASE_P(TransformScanIterDoubleLimit, TransformScan, ::testing::Range(1025, 65535, 5111)); 
//INSTANTIATE_TEST_CASE_P(TransformScanIterDoubleLimit, TransformScanMultiCore, ::testing::Range(1025, 65535, 5111));

#endif

TEST_P (TransformScan, InclTransformScanTestUDD)
{
    bolt::cl::device_vector< uddtI2  > input( myStdVectSize, initialAddI2);
    bolt::cl::device_vector< uddtI2  > output( myStdVectSize, initialAddI2);
    std::vector< uddtI2 > refInput( myStdVectSize, initialAddI2);
    // call scan
    AddI2 aI2;
    //bolt::cl::negate<uddtI2>  nI2;


    bolt::cl::transform_inclusive_scan( input.begin(), input.end(), output.begin(), nI2, aI2 );
    ::std::transform(   refInput.begin(), refInput.end(),  refInput.begin(), nI2); // transform in-place
    ::std::partial_sum( refInput.begin(), refInput.end(), refInput.begin(), aI2); // out-of-place scan
   
    cmpArrays(output, refInput);
    
}

TEST_P (TransformScan, ExclTransformScanTestUDD)
{
    bolt::cl::device_vector< uddtI2 > input( myStdVectSize, initialAddI2);
    bolt::cl::device_vector< uddtI2 > output( myStdVectSize, initialAddI2);
    std::vector< uddtI2 > refInput( myStdVectSize, initialAddI2);
    // call scan
    
    AddI2 aI2;

    bolt::cl::transform_exclusive_scan( input.begin(), input.end(), output.begin(), nI2, initialAddI2, aI2 );
    std::transform(   refInput.begin(), refInput.end(),  refInput.begin(), nI2);
    Serial_scan<uddtI2,  AddI2, uddtI2>(&refInput[0], &refInput[0], myStdVectSize, aI2, false, initialAddI2);

    cmpArrays(output, refInput);
    
} 

TEST_P (TransformScanMultiCore, InclTransformScanTestUDD)
{
    
    bolt::cl::device_vector< uddtI2  > input( myStdVectSize, initialAddI2);
    bolt::cl::device_vector< uddtI2  > output( myStdVectSize, initialAddI2);
    std::vector< uddtI2 > refInput( myStdVectSize, initialAddI2);
    // call scan
    AddI2 aI2;

    bolt::cl::control ctl = bolt::cl::control::getDefault( );
    ctl.setForceRunMode(bolt::cl::control::MultiCoreCpu);

    bolt::cl::transform_inclusive_scan( ctl, input.begin(), input.end(), output.begin(), nI2, aI2 );
    ::std::transform(   refInput.begin(), refInput.end(),  refInput.begin(), nI2); // transform in-place
    ::std::partial_sum( refInput.begin(), refInput.end(), refInput.begin(), aI2); // out-of-place scan
   
    cmpArrays(output, refInput);
    
} 

TEST_P (TransformScanMultiCore, ExclTransformScanTestUDD)
{
    bolt::cl::device_vector< uddtI2 > input( myStdVectSize, initialAddI2);
    bolt::cl::device_vector< uddtI2 > output( myStdVectSize, initialAddI2);
    std::vector< uddtI2 > refInput( myStdVectSize, initialAddI2);
    // call scan
    
    AddI2 aI2;

    bolt::cl::control ctl = bolt::cl::control::getDefault( );
    ctl.setForceRunMode(bolt::cl::control::MultiCoreCpu);

    bolt::cl::transform_exclusive_scan( ctl, input.begin(), input.end(), output.begin(), nI2, initialAddI2, aI2 );
    std::transform(   refInput.begin(), refInput.end(),  refInput.begin(), nI2);
    Serial_scan<uddtI2,  AddI2, uddtI2>(&refInput[0], &refInput[0], myStdVectSize, aI2, false, initialAddI2);

    //Serial_scan<double,  bolt::cl::plus< double >, double>(&refInput[0], &refInput[0], myStdVectSize, mM3, false, 3.0);

    cmpArrays(output, refInput);
    
} 

INSTANTIATE_TEST_CASE_P(TransformScanIterUDDLimit, TransformScan, ::testing::Range(1025, 65535, 5111)); 
INSTANTIATE_TEST_CASE_P(TransformScanIterUDDLimit, TransformScanMultiCore, ::testing::Range(1025, 65535, 5111));

/******************************************************************************
 *  Scan with User Defined Data Types and Operators
 *****************************************************************************/

TEST(NegateScanUserDefined, IncAddInt2)
{
    //setup containers
    int length = (1<<16)+23;
//    bolt::cl::negate< uddtI2 > nI2;
    bolt::cl::device_vector< uddtI2 > input(  length, initialAddI2); //,  CL_MEM_READ_WRITE, true  );
    bolt::cl::device_vector< uddtI2 > output( length, identityAddI2); //, CL_MEM_READ_WRITE, false );
    std::vector< uddtI2 > refInput( length, initialAddI2 );
    std::vector< uddtI2 > refOutput( length );

    // call transform_scan
    AddI2 aI2;
    bolt::cl::transform_inclusive_scan( input.begin(), input.end(), output.begin(), nI2, aI2 );
    ::std::transform(   refInput.begin(), refInput.end(),  refInput.begin(), nI2); // transform in-place
    ::std::partial_sum( refInput.begin(), refInput.end(), refOutput.begin(), aI2); // out-of-place scan

    // compare results
    cmpArrays(refOutput, output);
}

TEST(NegateScanUserDefined, SerialIncAddInt2)
{
    //setup containers
    int length = (1<<16)+23;
//    bolt::cl::negate< uddtI2 > nI2;
    bolt::cl::device_vector< uddtI2 > input(  length, initialAddI2); //,  CL_MEM_READ_WRITE, true  );
    bolt::cl::device_vector< uddtI2 > output( length, identityAddI2); //, CL_MEM_READ_WRITE, false );
    std::vector< uddtI2 > refInput( length, initialAddI2 );
    std::vector< uddtI2 > refOutput( length );

    bolt::cl::control ctl = bolt::cl::control::getDefault( );
    ctl.setForceRunMode(bolt::cl::control::SerialCpu);

    // call transform_scan
    AddI2 aI2;
    bolt::cl::transform_inclusive_scan( ctl, input.begin(), input.end(), output.begin(), nI2, aI2 );
    ::std::transform(   refInput.begin(), refInput.end(),  refInput.begin(), nI2); // transform in-place
    ::std::partial_sum( refInput.begin(), refInput.end(), refOutput.begin(), aI2); // out-of-place scan

    // compare results
    cmpArrays(refOutput, output);
}



TEST(NegateScanUserDefined, MultiCoreIncAddInt2)
{
    //setup containers
    int length = (1<<16)+23;
//    bolt::cl::negate< uddtI2 > nI2;
    bolt::cl::device_vector< uddtI2 > input(  length, initialAddI2); //,  CL_MEM_READ_WRITE, true  );
    bolt::cl::device_vector< uddtI2 > output( length, identityAddI2); //, CL_MEM_READ_WRITE, false );
    std::vector< uddtI2 > refInput( length, initialAddI2 );
    std::vector< uddtI2 > refOutput( length );

    bolt::cl::control ctl = bolt::cl::control::getDefault( );
    ctl.setForceRunMode(bolt::cl::control::MultiCoreCpu);

    // call transform_scan
    AddI2 aI2;
    bolt::cl::transform_inclusive_scan( ctl, input.begin(), input.end(), output.begin(), nI2, aI2 );
    ::std::transform(   refInput.begin(), refInput.end(),  refInput.begin(), nI2); // transform in-place
    ::std::partial_sum( refInput.begin(), refInput.end(), refOutput.begin(), aI2); // out-of-place scan

    // compare results
    cmpArrays(refOutput, output);
}



TEST(TransformScanCLtypeTest, ExclTestLong)
{
    //setup containers
    int length = (1<<14);
    bolt::cl::negate< cl_long > nM3;
    bolt::cl::device_vector< cl_long > input(  length, 3 );
    bolt::cl::device_vector< cl_long > output( length, 0);
    std::vector< cl_long > refInput( length, 3 ); 
    std::vector< cl_long > refOutput( length, 0 );

    // call scan
    bolt::cl::multiplies< cl_long > mM3;
    bolt::cl::transform_exclusive_scan( input.begin(), input.end(), output.begin(), nM3, 3, mM3 );
    std::transform(   refInput.begin(), refInput.end(),  refOutput.begin(), nM3);
    Serial_scan<cl_long,  bolt::cl::multiplies< cl_long >, cl_long>(&refOutput[0], &refOutput[0], length, mM3, false, 3);

    // compare results
    cmpArrays(refOutput, output);
}

TEST(TransformScanCLtypeTest, InclTestLong)
{
    //setup containers
    int length = (1<<14);
    bolt::cl::negate< cl_long > nM3;
    bolt::cl::device_vector< cl_long > input(  length, 3 );
    bolt::cl::device_vector< cl_long > output( length, 0);
    std::vector< cl_long > refInput( length, 3 ); 
    std::vector< cl_long > refOutput(length, 0 );

    // call scan
    bolt::cl::multiplies< cl_long > mM3;
    bolt::cl::transform_inclusive_scan( input.begin(), input.end(), output.begin(), nM3, mM3 );
    ::std::transform(   refInput.begin(), refInput.end(),  refOutput.begin(), nM3);
    ::std::partial_sum( refOutput.begin(), refOutput.end(), refOutput.begin(), mM3);

    // compare results
    cmpArrays(refOutput, output);
}


TEST(TransformScanCLtypeTest, ExclTestULong)
{
    //setup containers
    int length = (1<<14);
    bolt::cl::square< cl_ulong > nM3;
    bolt::cl::device_vector< cl_ulong > input(  length, 3 );
    bolt::cl::device_vector< cl_ulong > output( length, 0);
    std::vector< cl_ulong > refInput( length, 3 ); 
    std::vector< cl_ulong > refOutput( length, 0 );

    // call scan
    bolt::cl::multiplies< cl_ulong > mM3;
    bolt::cl::transform_exclusive_scan( input.begin(), input.end(), output.begin(), nM3, 3, mM3 );
    std::transform(   refInput.begin(), refInput.end(),  refOutput.begin(), nM3);
    Serial_scan<cl_ulong,  bolt::cl::multiplies< cl_ulong >, cl_ulong>(&refOutput[0], &refOutput[0], length, mM3, false, 3);

    // compare results
    cmpArrays(refOutput, output);
}

TEST(TransformScanCLtypeTest, InclTestULong)
{
    //setup containers
    int length = (1<<14);
    bolt::cl::square< cl_ulong > nM3;
    bolt::cl::device_vector< cl_ulong > input(  length, 3 );
    bolt::cl::device_vector< cl_ulong > output( length, 0);
    std::vector< cl_ulong > refInput( length, 3 ); 
    std::vector< cl_ulong > refOutput(length, 0 );

    // call scan
    bolt::cl::multiplies< cl_ulong > mM3;
    bolt::cl::transform_inclusive_scan( input.begin(), input.end(), output.begin(), nM3, mM3 );
    ::std::transform(   refInput.begin(), refInput.end(),  refOutput.begin(), nM3);
    ::std::partial_sum( refOutput.begin(), refOutput.end(), refOutput.begin(), mM3);

    // compare results
    cmpArrays(refOutput, output);
}



TEST(TransformScanCLtypeTest, ExclTestShort)
{
    //setup containers
    int length = (1<<14);
    bolt::cl::negate< cl_short > nM3;
    bolt::cl::device_vector< cl_short > input(  length, 3 );
    bolt::cl::device_vector< cl_short > output( length, 0);
    std::vector< cl_short > refInput( length, 3 ); 
    std::vector< cl_short > refOutput( length, 0 );

    // call scan
    bolt::cl::multiplies< cl_short > mM3;
    bolt::cl::transform_exclusive_scan( input.begin(), input.end(), output.begin(), nM3, 3, mM3 );
    std::transform(   refInput.begin(), refInput.end(),  refOutput.begin(), nM3);
    Serial_scan<cl_short,  bolt::cl::multiplies< cl_short >, cl_short>(&refOutput[0], &refOutput[0], length, mM3, false, 3);

    // compare results
    cmpArrays(refOutput, output);
}

TEST(TransformScanCLtypeTest, InclTestShort)
{
    //setup containers
    int length = (1<<14);
    bolt::cl::negate< cl_short > nM3;
    bolt::cl::device_vector< cl_short > input(  length, 3 );
    bolt::cl::device_vector< cl_short > output( length, 0);
    std::vector< cl_short > refInput( length, 3 ); 
    std::vector< cl_short > refOutput(length, 0 );

    // call scan
    bolt::cl::multiplies< cl_short > mM3;
    bolt::cl::transform_inclusive_scan( input.begin(), input.end(), output.begin(), nM3, mM3 );
    ::std::transform(   refInput.begin(), refInput.end(),  refOutput.begin(), nM3);
    ::std::partial_sum( refOutput.begin(), refOutput.end(), refOutput.begin(), mM3);

    // compare results
    cmpArrays(refOutput, output);
}


TEST(TransformScanCLtypeTest, ExclTestUShort)
{
    //setup containers
    int length = 1024;
    bolt::cl::square< cl_ushort > nM3;
    bolt::cl::device_vector< cl_ushort > input(  length, 1 );
    bolt::cl::device_vector< cl_ushort > output( length, 0);
    std::vector< cl_ushort> refInput( length, 1 ); 
    std::vector< cl_ushort > refOutput( length, 0 );

    // call scan

    bolt::cl::control ctl = bolt::cl::control::getDefault( );
    ctl.setForceRunMode(bolt::cl::control::SerialCpu);


    bolt::cl::plus< cl_ushort > mM3;
    bolt::cl::transform_exclusive_scan( input.begin(), input.end(), output.begin(), nM3, 1, mM3 );

    std::transform(   refInput.begin(), refInput.end(),  refOutput.begin(), nM3);
    Serial_scan<cl_ushort,  bolt::cl::plus< cl_ushort >,cl_ushort>(&refOutput[0], &refOutput[0], length, mM3, false, 1);

    // compare results
    cmpArrays(refOutput, output);
}

TEST(TransformScanCLtypeTest, InclTestUShort)
{
    //setup containers
    int length = 1024;
    bolt::cl::square< unsigned short > nM3;
    bolt::cl::device_vector< unsigned short > input(  length, 1 );
    bolt::cl::device_vector< unsigned short > output( length, 0);
    std::vector< unsigned short > refInput( length, 1 ); 
    std::vector< unsigned short > refOutput(length, 0 );

    // call scan
    bolt::cl::plus< unsigned short > mM3;
    bolt::cl::transform_inclusive_scan( input.begin(), input.end(), output.begin(), nM3, mM3 );

    ::std::transform(   refInput.begin(), refInput.end(),  refOutput.begin(), nM3);
    ::std::partial_sum( refOutput.begin(), refOutput.end(), refOutput.begin(), mM3);

    // compare results
    cmpArrays(refOutput, output);
}

#if(TEST_DOUBLE == 1)
TEST(NegateScanUserDefined, IncMultiplyDouble4)
{
    //setup containers
    int length = (1<<16)+11;
//    bolt::cl::negate< uddtD4 > nD4;
    bolt::cl::device_vector< uddtD4 > input(  length, initialMultD4); //,  CL_MEM_READ_WRITE, true  );
    bolt::cl::device_vector< uddtD4 > output( length, identityMultD4); //, CL_MEM_READ_WRITE, false );
    std::vector< uddtD4 > refInput( length, initialMultD4 );
    std::vector< uddtD4 > refOutput( length );

    // call transform_scan
    MultD4 mD4;
    bolt::cl::transform_inclusive_scan( input.begin(), input.end(), output.begin(), nD4, mD4 );
    ::std::transform(   refInput.begin(), refInput.end(),  refInput.begin(), nD4); // transform in-place
    ::std::partial_sum( refInput.begin(), refInput.end(), refOutput.begin(), mD4); // out-of-place scan

    // compare results
    cmpArrays(refOutput, output);
}

TEST(NegateScanUserDefined, SerialIncMultiplyDouble4)
{
    //setup containers
    int length = (1<<16)+11;
//    bolt::cl::negate< uddtD4 > nD4;
    bolt::cl::device_vector< uddtD4 > input(  length, initialMultD4); //,  CL_MEM_READ_WRITE, true  );
    bolt::cl::device_vector< uddtD4 > output( length, identityMultD4); //, CL_MEM_READ_WRITE, false );
    std::vector< uddtD4 > refInput( length, initialMultD4 );
    std::vector< uddtD4 > refOutput( length );

   
    bolt::cl::control ctl = bolt::cl::control::getDefault( );
    ctl.setForceRunMode(bolt::cl::control::SerialCpu);

    // call transform_scan
    MultD4 mD4;
    bolt::cl::transform_inclusive_scan( ctl, input.begin(), input.end(), output.begin(), nD4, mD4 );
    ::std::transform(   refInput.begin(), refInput.end(),  refInput.begin(), nD4); // transform in-place
    ::std::partial_sum( refInput.begin(), refInput.end(), refOutput.begin(), mD4); // out-of-place scan

    // compare results
    cmpArrays(refOutput, output);
}

TEST(NegateScanUserDefined, MultiCoreIncMultiplyDouble4)
{
    //setup containers
    int length = (1<<16)+11;
//    bolt::cl::negate< uddtD4 > nD4;
    bolt::cl::device_vector< uddtD4 > input(  length, initialMultD4); //,  CL_MEM_READ_WRITE, true  );
    bolt::cl::device_vector< uddtD4 > output( length, identityMultD4); //, CL_MEM_READ_WRITE, false );
    std::vector< uddtD4 > refInput( length, initialMultD4 );
    std::vector< uddtD4 > refOutput( length );


    bolt::cl::control ctl = bolt::cl::control::getDefault( );
    ctl.setForceRunMode(bolt::cl::control::MultiCoreCpu);

    // call transform_scan
    MultD4 mD4;
    bolt::cl::transform_inclusive_scan( ctl, input.begin(), input.end(), output.begin(), nD4, mD4 );
    ::std::transform(   refInput.begin(), refInput.end(),  refInput.begin(), nD4); // transform in-place
    ::std::partial_sum( refInput.begin(), refInput.end(), refOutput.begin(), mD4); // out-of-place scan

    // compare results
    cmpArrays(refOutput, output);
}


TEST(NegateScanUserDefined, IncMixedM3)
{
    //setup containers
    int length = (1<<16)+57;
//    bolt::cl::negate< uddtM3 > nM3;
    bolt::cl::device_vector< uddtM3 > input(  length, initialMixM3); //,  CL_MEM_READ_WRITE, true  );
    bolt::cl::device_vector< uddtM3 > output( length, identityMixM3); //, CL_MEM_READ_WRITE, false );
    std::vector< uddtM3 > refInput( length, initialMixM3 );
    std::vector< uddtM3 > refOutput( length );

    // call scan
    MixM3 mM3;
    bolt::cl::transform_inclusive_scan( input.begin(), input.end(), output.begin(), nM3, mM3 );
    ::std::transform(   refInput.begin(), refInput.end(),  refInput.begin(), nM3);
    ::std::partial_sum( refInput.begin(), refInput.end(), refOutput.begin(), mM3);

    // compare results
    cmpArrays(refOutput, output);
}

TEST(NegateScanUserDefined, SerialIncMixedM3)
{
    //setup containers
    int length = (1<<16)+57;
//    bolt::cl::negate< uddtM3 > nM3;
    bolt::cl::device_vector< uddtM3 > input(  length, initialMixM3); //,  CL_MEM_READ_WRITE, true  );
    bolt::cl::device_vector< uddtM3 > output( length, identityMixM3); //, CL_MEM_READ_WRITE, false );
    std::vector< uddtM3 > refInput( length, initialMixM3 );
    std::vector< uddtM3 > refOutput( length );


    bolt::cl::control ctl = bolt::cl::control::getDefault( );
    ctl.setForceRunMode(bolt::cl::control::SerialCpu);

    // call scan
    MixM3 mM3;
    bolt::cl::transform_inclusive_scan( ctl, input.begin(), input.end(), output.begin(), nM3, mM3 );
    ::std::transform(   refInput.begin(), refInput.end(),  refInput.begin(), nM3);
    ::std::partial_sum( refInput.begin(), refInput.end(), refOutput.begin(), mM3);

    // compare results
    cmpArrays(refOutput, output);
}

TEST(NegateScanUserDefined, MultiCoreIncMixedM3)
{
    //setup containers
    int length = (1<<16)+57;
//    bolt::cl::negate< uddtM3 > nM3;
    bolt::cl::device_vector< uddtM3 > input(  length, initialMixM3); //,  CL_MEM_READ_WRITE, true  );
    bolt::cl::device_vector< uddtM3 > output( length, identityMixM3); //, CL_MEM_READ_WRITE, false );
    std::vector< uddtM3 > refInput( length, initialMixM3 );
    std::vector< uddtM3 > refOutput( length );

    bolt::cl::control ctl = bolt::cl::control::getDefault( );
    ctl.setForceRunMode(bolt::cl::control::MultiCoreCpu);

    // call scan
    MixM3 mM3;
    bolt::cl::transform_inclusive_scan( ctl, input.begin(), input.end(), output.begin(), nM3, mM3 );
    ::std::transform(   refInput.begin(), refInput.end(),  refInput.begin(), nM3);
    ::std::partial_sum( refInput.begin(), refInput.end(), refOutput.begin(), mM3);

    // compare results
    cmpArrays(refOutput, output);
}

#endif

/////////////////////////////////////////////////  Exclusive  ///////////////////////////

TEST(NegateScanUserDefined, ExclAddInt2)
{
    //setup containers
    int length = (1<<16)+23;
//    bolt::cl::negate< uddtI2 > nI2;
    bolt::cl::device_vector< uddtI2 > input(  length, initialAddI2); //,  CL_MEM_READ_WRITE, true  );
    bolt::cl::device_vector< uddtI2 > output( length, identityAddI2); //, CL_MEM_READ_WRITE, false );
    std::vector< uddtI2 > refInput( length, initialAddI2 ); //refInput[0] = identityAddI2;
    std::vector< uddtI2 > refOutput( length );

    // call scan
    AddI2 aI2;
    bolt::cl::transform_exclusive_scan( input.begin(), input.end(), output.begin(), nI2, identityAddI2, aI2 );

    std::transform(   refInput.begin(), refInput.end(),  refOutput.begin(), nI2);
    Serial_scan<uddtI2, AddI2, uddtI2>(&refOutput[0], &refOutput[0], length, aI2, false, identityAddI2);

    // compare results
    cmpArrays(refOutput, output);
}

TEST(NegateScanUserDefined, SerialExclAddInt2)
{
    //setup containers
    int length = (1<<16)+23;
//    bolt::cl::negate< uddtI2 > nI2;
    bolt::cl::device_vector< uddtI2 > input(  length, initialAddI2); //,  CL_MEM_READ_WRITE, true  );
    bolt::cl::device_vector< uddtI2 > output( length, identityAddI2); //, CL_MEM_READ_WRITE, false );
    std::vector< uddtI2 > refInput( length, initialAddI2 );// refInput[0] = identityAddI2;
    std::vector< uddtI2 > refOutput( length );

    bolt::cl::control ctl = bolt::cl::control::getDefault( );
    ctl.setForceRunMode(bolt::cl::control::SerialCpu);

    // call scan
    AddI2 aI2;
    bolt::cl::transform_exclusive_scan( ctl, input.begin(), input.end(), output.begin(), nI2, identityAddI2, aI2 );

    std::transform(   refInput.begin(), refInput.end(),  refOutput.begin(), nI2);
    Serial_scan<uddtI2, AddI2, uddtI2>(&refOutput[0], &refOutput[0], length, aI2, false, identityAddI2);

    // compare results
    cmpArrays(refOutput, output);
}

TEST(NegateScanUserDefined, MultiCoreExclAddInt2)
{
    //setup containers
    int length = (1<<16)+23;
//    bolt::cl::negate< uddtI2 > nI2;
    bolt::cl::device_vector< uddtI2 > input(  length, initialAddI2); //,  CL_MEM_READ_WRITE, true  );
    bolt::cl::device_vector< uddtI2 > output( length, identityAddI2); //, CL_MEM_READ_WRITE, false );
    std::vector< uddtI2 > refInput( length, initialAddI2 ); //refInput[0] = identityAddI2;
    std::vector< uddtI2 > refOutput( length );

    bolt::cl::control ctl = bolt::cl::control::getDefault( );
    ctl.setForceRunMode(bolt::cl::control::MultiCoreCpu);

    // call scan
    AddI2 aI2;
    bolt::cl::transform_exclusive_scan( ctl, input.begin(), input.end(), output.begin(), nI2, identityAddI2, aI2 );

    std::transform(   refInput.begin(), refInput.end(),  refOutput.begin(), nI2);
    Serial_scan<uddtI2, AddI2, uddtI2>(&refOutput[0], &refOutput[0], length, aI2, false, identityAddI2);

    // compare results
    cmpArrays(refOutput, output);
}

#if (TEST_DOUBLE == 1)
TEST(NegateScanUserDefined, ExclMultiplyDouble4)
{
    //setup containers
    int length = (1<<16)+11;
//    bolt::cl::negate< uddtD4 > nD4;
    bolt::cl::device_vector< uddtD4 > input(  length, initialMultD4); //,  CL_MEM_READ_WRITE, true  );
    bolt::cl::device_vector< uddtD4 > output( length, identityMultD4); //, CL_MEM_READ_WRITE, false );
    std::vector< uddtD4 > refInput( length, initialMultD4 );// refInput[0] = identityMultD4;
    std::vector< uddtD4 > refOutput( length );

    // call scan
    MultD4 mD4;
    bolt::cl::transform_exclusive_scan( input.begin(), input.end(), output.begin(), nD4, identityMultD4, mD4 );

    std::transform(   refInput.begin(), refInput.end(),  refOutput.begin(), nD4);
    Serial_scan<uddtD4, MultD4, uddtD4>(&refOutput[0], &refOutput[0], length, mD4, false, identityMultD4);


    // compare results
    cmpArrays(refOutput, output);
}

TEST(NegateScanUserDefined, SerialExclMultiplyDouble4)
{
    //setup containers
    int length = (1<<16)+11;
//    bolt::cl::negate< uddtD4 > nD4;
    bolt::cl::device_vector< uddtD4 > input(  length, initialMultD4); //,  CL_MEM_READ_WRITE, true  );
    bolt::cl::device_vector< uddtD4 > output( length, identityMultD4); //, CL_MEM_READ_WRITE, false );
    std::vector< uddtD4 > refInput( length, initialMultD4 );// refInput[0] = identityMultD4;
    std::vector< uddtD4 > refOutput( length );

    bolt::cl::control ctl = bolt::cl::control::getDefault( );
    ctl.setForceRunMode(bolt::cl::control::SerialCpu);

    // call scan
    MultD4 mD4;
    bolt::cl::transform_exclusive_scan( ctl, input.begin(), input.end(), output.begin(), nD4, identityMultD4, mD4 );

    std::transform(   refInput.begin(), refInput.end(),  refOutput.begin(), nD4);
    Serial_scan<uddtD4, MultD4, uddtD4>(&refOutput[0], &refOutput[0], length, mD4, false, identityMultD4);
    // compare results
    cmpArrays(refOutput, output);
}

TEST(NegateScanUserDefined, MultiCoreExclMultiplyDouble4)
{
    //setup containers
    int length = (1<<16)+11;
//    bolt::cl::negate< uddtD4 > nD4;
    bolt::cl::device_vector< uddtD4 > input(  length, initialMultD4); //,  CL_MEM_READ_WRITE, true  );
    bolt::cl::device_vector< uddtD4 > output( length, identityMultD4); //, CL_MEM_READ_WRITE, false );
    std::vector< uddtD4 > refInput( length, initialMultD4 );// refInput[0] = identityMultD4;
    std::vector< uddtD4 > refOutput( length );

    bolt::cl::control ctl = bolt::cl::control::getDefault( );
    ctl.setForceRunMode(bolt::cl::control::MultiCoreCpu);

    // call scan
    MultD4 mD4;
    bolt::cl::transform_exclusive_scan( ctl, input.begin(), input.end(), output.begin(), nD4, identityMultD4, mD4 );

    std::transform(   refInput.begin(), refInput.end(),  refOutput.begin(), nD4);
    Serial_scan<uddtD4, MultD4, uddtD4>(&refOutput[0], &refOutput[0], length, mD4, false, identityMultD4);

    // compare results
    cmpArrays(refOutput, output);
}


TEST(NegateScanUserDefined, ExclMixedM3)
{
    //setup containers
    int length = (1<<16)+57;
//    bolt::cl::negate< uddtM3 > nM3;
    bolt::cl::device_vector< uddtM3 > input(  length, initialMixM3); //,  CL_MEM_READ_WRITE, true  );
    bolt::cl::device_vector< uddtM3 > output( length, identityMixM3); //, CL_MEM_READ_WRITE, false );
    std::vector< uddtM3 > refInput( length, initialMixM3 ); //refInput[0] = identityMixM3;
    std::vector< uddtM3 > refOutput( length );

    // call scan
    MixM3 mM3;
    bolt::cl::transform_exclusive_scan( input.begin(), input.end(), output.begin(), nM3, identityMixM3, mM3 );

    std::transform(   refInput.begin(), refInput.end(),  refOutput.begin(), nM3);
    Serial_scan<uddtM3, MixM3, uddtM3>(&refOutput[0], &refOutput[0], length, mM3, false, identityMixM3);

    // compare results
    cmpArrays(refOutput, output);
}

TEST(NegateScanUserDefined, SerialExclMixedM3)
{
    //setup containers
    int length = (1<<16)+57;
//    bolt::cl::negate< uddtM3 > nM3;
    bolt::cl::device_vector< uddtM3 > input(  length, initialMixM3); //,  CL_MEM_READ_WRITE, true  );
    bolt::cl::device_vector< uddtM3 > output( length, identityMixM3); //, CL_MEM_READ_WRITE, false );
    std::vector< uddtM3 > refInput( length, initialMixM3 ); //refInput[0] = identityMixM3;
    std::vector< uddtM3 > refOutput( length );

    bolt::cl::control ctl = bolt::cl::control::getDefault( );
    ctl.setForceRunMode(bolt::cl::control::SerialCpu);

    // call scan
    MixM3 mM3;
    bolt::cl::transform_exclusive_scan( ctl, input.begin(), input.end(), output.begin(), nM3, identityMixM3, mM3 );

    std::transform(   refInput.begin(), refInput.end(),  refOutput.begin(), nM3);
    Serial_scan<uddtM3, MixM3, uddtM3>(&refOutput[0], &refOutput[0], length, mM3, false, identityMixM3);
    // compare results
    cmpArrays(refOutput, output);
}

TEST(NegateScanUserDefined, MultiCoreExclMixedM3)
{
    //setup containers
    int length = (1<<16)+57;
//    bolt::cl::negate< uddtM3 > nM3;
    bolt::cl::device_vector< uddtM3 > input(  length, initialMixM3); //,  CL_MEM_READ_WRITE, true  );
    bolt::cl::device_vector< uddtM3 > output( length, identityMixM3); //, CL_MEM_READ_WRITE, false );
    std::vector< uddtM3 > refInput( length, initialMixM3 );// refInput[0] = identityMixM3;
    std::vector< uddtM3 > refOutput( length );

    bolt::cl::control ctl = bolt::cl::control::getDefault( );
    ctl.setForceRunMode(bolt::cl::control::MultiCoreCpu);

    // call scan
    MixM3 mM3;
    bolt::cl::transform_exclusive_scan( ctl, input.begin(), input.end(), output.begin(), nM3, identityMixM3, mM3 );

    std::transform(   refInput.begin(), refInput.end(),  refOutput.begin(), nM3);
    Serial_scan<uddtM3, MixM3, uddtM3>(&refOutput[0], &refOutput[0], length, mM3, false, identityMixM3);

    // compare results
    cmpArrays(refOutput, output);
}

TEST(NegateScanUserDefined, SerialExclOffsetTest)
{
    //setup containers
    int length = (1<<15);
//    bolt::cl::negate< uddtM3 > nM3;
    bolt::cl::device_vector< uddtM3 > input(  length, initialMixM3);
    bolt::cl::device_vector< uddtM3 > output( length, identityMixM3);
    std::vector< uddtM3 > refInput( length, initialMixM3 );// refInput[0] = identityMixM3;
    std::vector< uddtM3 > refOutput( length,identityMixM3);

    bolt::cl::control ctl = bolt::cl::control::getDefault( );
    ctl.setForceRunMode(bolt::cl::control::SerialCpu);

    // call scan
    MixM3 mM3;
    bolt::cl::transform_exclusive_scan( ctl, input.begin()+(length/2), input.end()-(length/4), output.begin()+(length/2), nM3, identityMixM3, mM3 );

    std::transform(   refInput.begin()+(length/2), refInput.end()-(length/4),  refOutput.begin()+(length/2), nM3);
    Serial_scan<uddtM3, MixM3, uddtM3>(&refOutput[(length/2)], &refOutput[(length/2)], length-(length/2)-(length/4), mM3, false, identityMixM3);

    // compare results
    cmpArrays(refOutput, output);
}

TEST(NegateScanFloatDefined, CLExclOffsetTestFloat)
{
    //setup containers1ost
    int length = (1<<18);


    bolt::cl::device_vector< float > input( length, 1.f );// refInput[0] = identityMixM3;
    bolt::cl::device_vector< float > output( length,0.f);
    std::vector< float > refInput( length, 1.f );// refInput[0] = identityMixM3;
    std::vector< float > refOutput( length, 0.f);
    
    bolt::cl::control ctl = bolt::cl::control::getDefault( );
    ctl.setForceRunMode(bolt::cl::control::OpenCL);

    // call scan

    bolt::cl::transform_exclusive_scan( ctl, input.begin()+(length/2), input.end()-(length/4), output.begin()+(length/2), bolt::cl::negate<float>(), 3.f, bolt::cl::plus<float>() );


    std::transform(   refInput.begin()+(length/2), refInput.end()-(length/4),  refOutput.begin()+(length/2), bolt::cl::negate<float>());
    Serial_scan<float, bolt::cl::plus<float>, float>(&refOutput[(length/2)], &refOutput[(length/2)], length-(length/2)-(length/4), bolt::cl::plus<float>(), false, 3.f);

    // compare results
    cmpArrays(refOutput, output);
}





TEST(NegateScanUserDefined, CLExclOffsetTest)
{
    //setup containers
    int length = (1<<16);
//    bolt::cl::negate< uddtM3 > nM3;
    bolt::cl::device_vector< uddtM3 > input(  length, initialMixM3);
    bolt::cl::device_vector< uddtM3 > output( length, identityMixM3);
    std::vector< uddtM3 > refInput( length, initialMixM3 );// refInput[0] = identityMixM3;
    std::vector< uddtM3 > refOutput( length, identityMixM3 );

    // call scan
    MixM3 mM3;
    bolt::cl::transform_exclusive_scan( input.begin()+(length/2), input.end()-(length/4), output.begin()+(length/2), nM3, identityMixM3, mM3 );

    std::transform(   refInput.begin()+(length/2), refInput.end()-(length/4),  refOutput.begin()+(length/2), nM3);
    Serial_scan<uddtM3, MixM3, uddtM3>(&refOutput[(length/2)], &refOutput[(length/2)], length-(length/2)-(length/4), mM3, false, identityMixM3);

    // compare results
    cmpArrays(refOutput, output);
}

TEST(NegateScanUserDefined, MulticoreInclOffsetTest)
{
    //setup containers
    int length = (1<<16);
//    bolt::cl::negate< uddtM3 > nM3;
    bolt::cl::device_vector< uddtM3 > input(  length, initialMixM3);
    bolt::cl::device_vector< uddtM3 > output( length, identityMixM3);
    std::vector< uddtM3 > refInput( length, initialMixM3 );
    std::vector< uddtM3 > refOutput( length, identityMixM3);

    bolt::cl::control ctl = bolt::cl::control::getDefault( );
    ctl.setForceRunMode(bolt::cl::control::MultiCoreCpu);

    // call scan
    MixM3 mM3;
    bolt::cl::transform_inclusive_scan( ctl, input.begin()+(length/2), input.end()-(length/4), output.begin()+(length/2), nM3, mM3 );
    ::std::transform(   refInput.begin()+(length/2), refInput.end()-(length/4),  refInput.begin()+(length/2), nM3);
    ::std::partial_sum( refInput.begin()+(length/2), refInput.end()-(length/4), refOutput.begin()+(length/2), mM3);

    // compare results
    cmpArrays(refOutput, output);
}

TEST(NegateScanUserDefined, CLInclOffsetTest)
{
    //setup containers
    int length = (1<<16);
//    bolt::cl::negate< uddtM3 > nM3;
    bolt::cl::device_vector< uddtM3 > input(  length, initialMixM3);
    bolt::cl::device_vector< uddtM3 > output( length, identityMixM3);
    std::vector< uddtM3 > refInput( length, initialMixM3 );
    std::vector< uddtM3 > refOutput( length, identityMixM3 );

    // call scan
     MixM3 mM3;
    bolt::cl::transform_inclusive_scan( input.begin()+(length/2), input.end()-(length/4), output.begin()+(length/2), nM3, mM3 );
    ::std::transform(   refInput.begin()+(length/2), refInput.end()-(length/4),  refInput.begin()+(length/2), nM3);
    ::std::partial_sum( refInput.begin()+(length/2), refInput.end()-(length/4), refOutput.begin()+(length/2), mM3);
    // compare results
    cmpArrays(refOutput, output);
}

TEST(Mixed, IncAddInt2)
{
    //setup containers
    int length = (1<<16)+23;
//    bolt::cl::negate< uddtI2 > nI2;
    uddtD4 initialD4 = {1.234, 2.345, 3.456, 4.567};
    bolt::cl::device_vector< uddtD4 > input(  length, initialD4); //,  CL_MEM_READ_WRITE, true  );
    bolt::cl::device_vector< uddtI2 > output( length, identityAddI2); //, CL_MEM_READ_WRITE, false );
    std::vector< uddtD4 > refInput( length, initialD4 );
    std::vector< uddtI2 > refIntermediate( length, identityAddI2 );
    std::vector< uddtI2 > refOutput( length, identityAddI2 );

    // call transform_scan
    AddI2 aI2;
    bolt::cl::transform_inclusive_scan( input.begin(), input.end(), output.begin(), sD4I2, aI2 );
    ::std::transform(   refInput.begin(), refInput.end(),  refIntermediate.begin(), sD4I2); // transform in-place
    ::std::partial_sum( refIntermediate.begin(), refIntermediate.end(), refOutput.begin(), aI2); // out-of-place scan

    // compare results
    cmpArrays(refOutput, output);
}

TEST(Mixed, SerialIncAddInt2)
{
    //setup containers
    int length = (1<<16)+23;
//    bolt::cl::negate< uddtI2 > nI2;
    uddtD4 initialD4 = {1.234, 2.345, 3.456, 4.567};
    bolt::cl::device_vector< uddtD4 > input(  length, initialD4); //,  CL_MEM_READ_WRITE, true  );
    bolt::cl::device_vector< uddtI2 > output( length, identityAddI2); //, CL_MEM_READ_WRITE, false );
    std::vector< uddtD4 > refInput( length, initialD4 );
    std::vector< uddtI2 > refIntermediate( length, identityAddI2 );
    std::vector< uddtI2 > refOutput( length, identityAddI2 );

    ::cl::Context myContext = bolt::cl::control::getDefault( ).getContext( );
    bolt::cl::control ctl = bolt::cl::control::getDefault( );
    ctl.setForceRunMode(bolt::cl::control::SerialCpu);

    // call transform_scan
    AddI2 aI2;
    bolt::cl::transform_inclusive_scan( ctl, input.begin(), input.end(), output.begin(), sD4I2, aI2 );
    ::std::transform(   refInput.begin(), refInput.end(),  refIntermediate.begin(), sD4I2); // transform in-place
    ::std::partial_sum( refIntermediate.begin(), refIntermediate.end(), refOutput.begin(), aI2); // out-of-place scan

    // compare results
    cmpArrays(refOutput, output);
}

TEST(Mixed, MultiCoreIncAddInt2)
{
    //setup containers
    int length = (1<<16)+23;
//    bolt::cl::negate< uddtI2 > nI2;
    uddtD4 initialD4 = {1.234, 2.345, 3.456, 4.567};
    bolt::cl::device_vector< uddtD4 > input(  length, initialD4); //,  CL_MEM_READ_WRITE, true  );
    bolt::cl::device_vector< uddtI2 > output( length, identityAddI2); //, CL_MEM_READ_WRITE, false );
    std::vector< uddtD4 > refInput( length, initialD4 );
    std::vector< uddtI2 > refIntermediate( length, identityAddI2 );
    std::vector< uddtI2 > refOutput( length, identityAddI2 );

    ::cl::Context myContext = bolt::cl::control::getDefault( ).getContext( );
    bolt::cl::control ctl = bolt::cl::control::getDefault( );
    ctl.setForceRunMode(bolt::cl::control::MultiCoreCpu);

    // call transform_scan
    AddI2 aI2;
    bolt::cl::transform_inclusive_scan( ctl, input.begin(), input.end(), output.begin(), sD4I2, aI2 );
    ::std::transform(   refInput.begin(), refInput.end(),  refIntermediate.begin(), sD4I2); // transform in-place
    ::std::partial_sum( refIntermediate.begin(), refIntermediate.end(), refOutput.begin(), aI2); // out-of-place scan

    // compare results
    cmpArrays(refOutput, output);
}


// Need to test with CPU command Queue
/*
TEST(SwitchDevices, IncAddInt2)
{
    //bolt::cl::control ctrl = bolt::cl::control::getDefault();
    // print device 1
    cl_int err;
    std::string strDeviceName;

    //setup initial values
    AddI2 aI2;
    int length = (1<<16)+23;
    uddtD4 initialD4 = {1.234, 2.345, 3.456, 4.567};
    std::vector< uddtD4 > refInput( length, initialD4 );
    std::vector< uddtI2 > refIntermediate( length, identityAddI2 );
    std::vector< uddtI2 > refOutput( length, identityAddI2 );
    ::std::transform(   refInput.begin(), refInput.end(),  refIntermediate.begin(), sD4I2); // transform in-place
    ::std::partial_sum( refIntermediate.begin(), refIntermediate.end(), refOutput.begin(), aI2); // out-of-place scan

    // for each device
    ::cl::Context context(CL_DEVICE_TYPE_ALL);//bolt::cl::control::getDefault( ).context( ); // get context
    std::vector< cl::Device > devices = context.getInfo< CL_CONTEXT_DEVICES >(); // get devices
    for (int iter = 0; iter < 3; iter++)
    for (int i = 0; i < devices.size(); i++) {

        // setup device/queue
        ::cl::CommandQueue queue( context, devices.at( i ), CL_QUEUE_PROFILING_ENABLE); // select device; make queue
        bolt::cl::control ctrl(queue);
        //bolt::cl::control::getDefault().commandQueue(queue);
        strDeviceName = ctrl.getDevice( ).getInfo< CL_DEVICE_NAME >( &err );
        bolt::cl::V_OPENCL( err, "Device::getInfo< CL_DEVICE_NAME > failed" );
        std::cout << "Testing Device[" << i << "]: " << strDeviceName << std::endl;


        // initialize device vectors
        bolt::cl::device_vector< uddtD4 > input(  length, initialD4,     CL_MEM_READ_WRITE, true,  ctrl );
        bolt::cl::device_vector< uddtI2 > output( length, identityAddI2, CL_MEM_READ_WRITE, false, ctrl );

        // compute on device
        bolt::cl::transform_inclusive_scan( ctrl, input.begin(), input.end(), output.begin(), sD4I2, aI2 );

        // compare results
        cmpArrays(refOutput, output);
    }

}
*/
#endif

/* Failing Test case - Random Access Iterator with Default Path!

TEST(DefaultGPU, NegPlusInt)
{
    //bolt::cl::control ctrl = bolt::cl::control::getDefault();
    // print device 1
    cl_int err;
    int deviceNum = 1;
    std::string strDeviceName = bolt::cl::control::getDefault( ).getDevice( ).getInfo< CL_DEVICE_NAME >( &err );
    bolt::cl::V_OPENCL( err, "Device::getInfo< CL_DEVICE_NAME > failed" );

    //setup initial values
    bolt::cl::negate<int> unary_op;
    bolt::cl::plus<int> binary_op;
    int length = (1<<16);
    int init = -1;
    int identity = 0;

    // reference data
    std::vector< int > refInput( length, init );
    std::vector< int > refIntermediate( length, identity );
    std::vector< int > refOutput( length, identity );

    // perform reference CPU calculation
    ::std::transform (refInput.begin(), refInput.end(),  refIntermediate.begin(), unary_op); // transform in-place
    // out-of-place scan
    ::std::partial_sum( refIntermediate.begin(), refIntermediate.end(), refOutput.begin(), binary_op);

    // device data
    std::vector< int > input( length, init);
    std::vector< int > output( length, identity);

    // calculate on device
    ::cl::Context context(CL_DEVICE_TYPE_ALL);//bolt::cl::control::getDefault( ).context( ); // get context
    std::vector< cl::Device > devices = context.getInfo< CL_CONTEXT_DEVICES >(); // get devices
    // setup device/queue
    ::cl::CommandQueue queue( context, devices.at( deviceNum ), CL_QUEUE_PROFILING_ENABLE); // select device;make queue
    bolt::cl::control ctrl(queue);
    //bolt::cl::control::getDefault().commandQueue(queue);
    strDeviceName = ctrl.getDevice( ).getInfo< CL_DEVICE_NAME >( &err );
    bolt::cl::V_OPENCL( err, "Device::getInfo< CL_DEVICE_NAME > failed" );
    std::cout << "Testing Device[" << deviceNum << "]: " << strDeviceName << std::endl;
    bolt::cl::transform_inclusive_scan( ctrl, input.begin(), input.end(), output.begin(), unary_op, binary_op );


    // compare results
    cmpArrays(refOutput, output);
} */


TEST(SerialCPU, NegPlusInt)
{
    //bolt::cl::control ctrl = bolt::cl::control::getDefault();
    // print device 1
    cl_int err;
    int deviceNum = 1;
    std::string strDeviceName = bolt::cl::control::getDefault( ).getDevice( ).getInfo< CL_DEVICE_NAME >( &err );
    bolt::cl::V_OPENCL( err, "Device::getInfo< CL_DEVICE_NAME > failed" );

    //setup initial values
    bolt::cl::negate<int> unary_op;
    bolt::cl::plus<int> binary_op;
    int length = (1<<16);
    int init = -1;
    int identity = 0;

    // reference data
    std::vector< int > refInput( length, init );
    std::vector< int > refIntermediate( length, identity );
    std::vector< int > refOutput( length, identity );

    // perform reference CPU calculation
    ::std::transform(   refInput.begin(), refInput.end(),  refIntermediate.begin(), unary_op); // transform in-place
    ::std::partial_sum( refIntermediate.begin(), refIntermediate.end(),refOutput.begin(),binary_op);//out-of-place scan

    // device data
    std::vector< int > input( length, init);
    std::vector< int > output( length, identity);


    // calculate on device
    bolt::cl::control ctrl = bolt::cl::control::getDefault( );
    ctrl.setForceRunMode(bolt::cl::control::SerialCpu);

    strDeviceName = ctrl.getDevice( ).getInfo< CL_DEVICE_NAME >( &err );
    bolt::cl::V_OPENCL( err, "Device::getInfo< CL_DEVICE_NAME > failed" );
    std::cout << "Testing Device[" << deviceNum << "]: " << strDeviceName << std::endl;
    bolt::cl::transform_inclusive_scan( ctrl, input.begin(), input.end(), output.begin(), unary_op, binary_op );


    // compare results
    cmpArrays(refOutput, output);
}


TEST(MultiCoreCPU, NegPlusInt)
{
    //bolt::cl::control ctrl = bolt::cl::control::getDefault();
    // print device 1
    cl_int err;
    int deviceNum = 1;
    std::string strDeviceName = bolt::cl::control::getDefault( ).getDevice( ).getInfo< CL_DEVICE_NAME >( &err );
    bolt::cl::V_OPENCL( err, "Device::getInfo< CL_DEVICE_NAME > failed" );

    //setup initial values
    bolt::cl::negate<int> unary_op;
    bolt::cl::plus<int> binary_op;
    int length = (1<<16);
    int init = -1;
    int identity = 0;

    // reference data
    std::vector< int > refInput( length, init );
    std::vector< int > refIntermediate( length, identity );
    std::vector< int > refOutput( length, identity );

    // perform reference CPU calculation
    ::std::transform(   refInput.begin(), refInput.end(),  refIntermediate.begin(), unary_op); // transform in-place
    ::std::partial_sum( refIntermediate.begin(), refIntermediate.end(),refOutput.begin(),binary_op);//out-of-place scan

    // device data
    std::vector< int > input( length, init);
    std::vector< int > output( length, identity);


    bolt::cl::control ctrl = bolt::cl::control::getDefault( );
    ctrl.setForceRunMode(bolt::cl::control::MultiCoreCpu);

    strDeviceName = ctrl.getDevice( ).getInfo< CL_DEVICE_NAME >( &err );
    bolt::cl::V_OPENCL( err, "Device::getInfo< CL_DEVICE_NAME > failed" );
    std::cout << "Testing Device[" << deviceNum << "]: " << strDeviceName << std::endl;
    bolt::cl::transform_inclusive_scan( ctrl, input.begin(), input.end(), output.begin(), unary_op, binary_op );


    // compare results
    cmpArrays(refOutput, output);
}


int _tmain(int argc, _TCHAR* argv[])
{
    //  Register our minidump generating logic
    //bolt::miniDumpSingleton::enableMiniDumps( );

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
            ( "platform,p",     po::value< cl_uint >( &userPlatform )->default_value( 0 ),
                                                        "Specify the platform under test" )
            ( "device,d",       po::value< cl_uint >( &userDevice )->default_value( 0 ),
                                                          "Specify the device under test" )
            ;


        po::variables_map vm;
        po::store( po::parse_command_line( argc, argv, desc ), vm );
        po::notify( vm );

        if( vm.count( "help" ) )
        {
            //This needs to be 'cout' as program-options does not support wcout yet
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
        bolt::cl::control::printPlatforms( );
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
    bolt::cl::V_OPENCL( platforms.front( ).getDevices( CL_DEVICE_TYPE_ALL, &devices ),"Platform::getDevices() failed");

    cl::Context myContext( devices.at( userDevice ) );
    cl::CommandQueue myQueue( myContext, devices.at( userDevice ) );
    bolt::cl::control::getDefault( ).setCommandQueue( myQueue );

    std::string strDeviceName = bolt::cl::control::getDefault( ).getDevice( ).getInfo< CL_DEVICE_NAME >( &err );
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
