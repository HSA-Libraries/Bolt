/***************************************************************************                                                                                     
*   � 2012,2014 Advanced Micro Devices, Inc. All rights reserved.                                     
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
#define TEST_CPU_DEVICE 0
#define TEST_MULTICORE_TBB_SORT 1
#define TEST_LARGE_BUFFERS 1
#define TEST_OFFSET_BUFFERS 1 // OFFSET BUffers not working in C++ AMP
#define GOOGLE_TEST 1
#define BKND amp 
#define SORT_FUNC sort

#if (GOOGLE_TEST == 1)
#include <gtest/gtest.h>
#include "common/stdafx.h"
#include <bolt/amp/sort.h>
#include <bolt/miniDump.h>
//#include <bolt/unicode.h>
#include <bolt/amp/functional.h>
#include "common/test_common.h"
//#include <boost/shared_array.hpp>
#include <array>
#include <algorithm>
/////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Below are helper routines to compare the results of two arrays for googletest
//  They return an assertion object that googletest knows how to track
//This is a compare routine for naked pointers.

#if ( TEST_DOUBLE == 1)
// UDD which contains four doubles
//BOLT_FUNCTOR(uddtD4,
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
};
//);
// Functor for UDD. Adds all four double elements and returns true if lhs_sum > rhs_sum
//BOLT_FUNCTOR(AddD4,
struct AddD4
{
    bool operator()(const uddtD4 &lhs, const uddtD4 &rhs) const restrict(amp, cpu)
    {

        if( ( lhs.a + lhs.b + lhs.c + lhs.d ) > ( rhs.a + rhs.b + rhs.c + rhs.d) )
            return true;
        return false;
    };
}; 
//);
//BOLT_CREATE_TYPENAME( bolt::BKND::device_vector< AddD4 >::iterator );
//BOLT_CREATE_CLCODE( bolt::BKND::device_vector< AddD4 >::iterator, bolt::BKND::deviceVectorIteratorTemplate );

uddtD4 identityAddD4 = { 1.0, 1.0, 1.0, 1.0 };
uddtD4 initialAddD4  = { 1.00001, 1.000003, 1.0000005, 1.00000007 };
//BOLT_CREATE_TYPENAME( bolt::BKND::device_vector< uddtD4 >::iterator );
//BOLT_CREATE_CLCODE( bolt::BKND::device_vector< uddtD4 >::iterator, bolt::BKND::deviceVectorIteratorTemplate );

TEST(Sort_large_power, StdclLong)  
{
        // test length
        int length = 33554432;

        std::vector<long> bolt_source(length);
        std::vector<long> std_source(length);

        // populate source vector with random ints
        for (int j = 0; j < length; j++)
        {
            bolt_source[j] = (long)rand();
            std_source[j] = bolt_source[j];
        }
    
        // perform sort
        std::sort(std_source.begin(), std_source.end());
        bolt::BKND::SORT_FUNC(bolt_source.begin(), bolt_source.end());

        // GoogleTest Comparison
        cmpArrays(std_source, bolt_source);

} 

TEST(Sort, StdclLong)  
{
        // test length
        int length = (1<<8);

        std::vector<long> bolt_source(length);
        std::vector<long> std_source(length);

        // populate source vector with random ints
        for (int j = 0; j < length; j++)
        {
            bolt_source[j] = (long)rand();
            std_source[j] = bolt_source[j];
        }
    
        // perform sort
        std::sort(std_source.begin(), std_source.end());
        bolt::BKND::SORT_FUNC(bolt_source.begin(), bolt_source.end());

        // GoogleTest Comparison
        cmpArrays(std_source, bolt_source);

} 

TEST(Sort, Serial_StdclLong)  
{
        // test length
        int length = (1<<8);

        std::vector<long> bolt_source(length);
        std::vector<long> std_source(length);

        // populate source vector with random ints
        for (int j = 0; j < length; j++)
        {
            bolt_source[j] = (long)rand();
            std_source[j] = bolt_source[j];
        }
    
        bolt::BKND::control ctl = bolt::BKND::control::getDefault( );
        ctl.setForceRunMode(bolt::BKND::control::SerialCpu); 

        // perform sort
        std::SORT_FUNC(std_source.begin(), std_source.end());
        bolt::BKND::SORT_FUNC(ctl, bolt_source.begin(), bolt_source.end());

        // GoogleTest Comparison
        cmpArrays(std_source, bolt_source);

} 


TEST(Sort, MultiCore_StdclLong)  
{
        // test length
        int length = (1<<8);

        std::vector<long> bolt_source(length);
        std::vector<long> std_source(length);

        // populate source vector with random ints
        for (int j = 0; j < length; j++)
        {
            bolt_source[j] = (long)rand();
            std_source[j] = bolt_source[j];
        }
    
        bolt::BKND::control ctl = bolt::BKND::control::getDefault( );
        ctl.setForceRunMode(bolt::BKND::control::MultiCoreCpu); 

        // perform sort
        std::SORT_FUNC(std_source.begin(), std_source.end());
        bolt::BKND::SORT_FUNC(ctl, bolt_source.begin(), bolt_source.end());

        // GoogleTest Comparison
        cmpArrays(std_source, bolt_source);

} 

TEST(Sort, DevclLong)  
{
        // test length
        int length = (1<<8);

        bolt::BKND::device_vector<long> bolt_source(length);
        std::vector<long> std_source(length);

        // populate source vector with random ints
        for (int j = 0; j < length; j++)
        {
            bolt_source[j] = (long)rand();
            std_source[j] = bolt_source[j];
        }
    
        // perform sort
        std::SORT_FUNC(std_source.begin(), std_source.end());
        bolt::BKND::SORT_FUNC(bolt_source.begin(), bolt_source.end());

        // GoogleTest Comparison
        cmpArrays(std_source, bolt_source);

} 

TEST(Sort, Serial_DevclLong)  
{
        // test length
        int length = (1<<8);

        bolt::BKND::device_vector<long> bolt_source(length);
        std::vector<long> std_source(length);

        // populate source vector with random ints
        for (int j = 0; j < length; j++)
        {
            bolt_source[j] = (long)rand();
            std_source[j] = bolt_source[j];
        }
    
        bolt::BKND::control ctl = bolt::BKND::control::getDefault( );
        ctl.setForceRunMode(bolt::BKND::control::SerialCpu); 

        // perform sort
        std::SORT_FUNC(std_source.begin(), std_source.end());
        bolt::BKND::SORT_FUNC(ctl, bolt_source.begin(), bolt_source.end());

        // GoogleTest Comparison
        cmpArrays(std_source, bolt_source);

}

TEST(Sort, MultiCore_DevclLong)  
{
        // test length
        int length = (1<<8);

        bolt::BKND::device_vector<long> bolt_source(length);
        std::vector<long> std_source(length);

        // populate source vector with random ints
        for (int j = 0; j < length; j++)
        {
            bolt_source[j] = (long)rand();
            std_source[j] = bolt_source[j];
        }
    
        bolt::BKND::control ctl = bolt::BKND::control::getDefault( );
        ctl.setForceRunMode(bolt::BKND::control::MultiCoreCpu); 

        // perform sort
        std::SORT_FUNC(std_source.begin(), std_source.end());
        bolt::BKND::SORT_FUNC(ctl, bolt_source.begin(), bolt_source.end());

        // GoogleTest Comparison
        cmpArrays(std_source, bolt_source);

}

TEST(SortUDD, AddDouble4)
{
    //setup containers
    int length = (1<<8);
    bolt::BKND::device_vector< uddtD4 > input(  length, initialAddD4, true  );
    std::vector< uddtD4 > refInput( length, initialAddD4 );

    // call sort
    AddD4 ad4gt;
    bolt::BKND::SORT_FUNC(input.begin(), input.end(), ad4gt);
    std::SORT_FUNC( refInput.begin(), refInput.end(), ad4gt );

    // compare results
    cmpArrays(refInput, input);
}

TEST(SortUDD, GPUAddDouble4)
{
    //setup containers
    int length = (1<<8);
    bolt::BKND::device_vector< uddtD4 > input(  length, initialAddD4, true  );
    std::vector< uddtD4 > refInput( length, initialAddD4 );

    // call sort
    AddD4 ad4gt;
    bolt::BKND::SORT_FUNC(input.begin(), input.end(), ad4gt );
    std::SORT_FUNC( refInput.begin(), refInput.end(), ad4gt );

    // compare results
    cmpArrays(refInput, input);
}
#endif

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
class SortArrayTest: public ::testing::Test
{
public:
    SortArrayTest( ): m_Errors( 0 )
    {}

    virtual void SetUp( )
    {
        std::generate(stdInput.begin(), stdInput.end(), rand);
        boltInput = stdInput;
        stdOffsetIn = stdInput;
        boltOffsetIn = stdInput;
    };

    virtual void TearDown( )
    {};

    virtual ~SortArrayTest( )
    {}

protected:
    typedef typename std::tuple_element< 0, ArrayTuple >::type ArrayType;
    static const size_t ArraySize = typename std::tuple_element< 1, ArrayTuple >::type::value;
    std::array< ArrayType, ArraySize > stdInput, boltInput, stdOffsetIn, boltOffsetIn;
    int m_Errors;
};

TYPED_TEST_CASE_P( SortArrayTest );


TYPED_TEST_P( SortArrayTest, Normal )
{
    typedef std::array< ArrayType, ArraySize > ArrayCont;
    //  Calling the actual functions under test
    std::SORT_FUNC( stdInput.begin( ), stdInput.end( ) );
    bolt::BKND::SORT_FUNC( boltInput.begin( ), boltInput.end( ) );

    ArrayCont::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end() );
    ArrayCont::difference_type boltNumElements = std::distance( boltInput.begin( ), boltInput.end() );

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpStdArray< ArrayType, ArraySize >::cmpArrays( stdInput, boltInput );

    //  OFFSET Calling the actual functions under test
    size_t startIndex = 17; //Some aribitrary offset position
    size_t endIndex   = ArraySize -17; //Some aribitrary offset position
    if( (( startIndex > ArraySize ) || ( endIndex < 0 ) )  || (startIndex > endIndex) )
    {
        std::cout <<"\nSkipping NormalOffset Test for size "<< ArraySize << "\n";
    }    
    else
    {
        std::SORT_FUNC( stdOffsetIn.begin( ) + startIndex, stdOffsetIn.begin( ) + endIndex );
        bolt::BKND::SORT_FUNC( boltOffsetIn.begin( ) + startIndex, boltOffsetIn.begin( ) + endIndex );

        ArrayCont::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end( ) );
        ArrayCont::difference_type boltNumElements = std::distance( boltInput.begin( ), boltInput.end( ) );

        //  Both collections should have the same number of elements
        EXPECT_EQ( stdNumElements, boltNumElements );

        //  Loop through the array and compare all the values with each other
        cmpStdArray< ArrayType, ArraySize >::cmpArrays( stdInput, boltInput );
    }
}

#if (TEST_CPU_DEVICE == 1)
TYPED_TEST_P( SortArrayTest, CPU_DeviceNormal )
{
    typedef std::array< ArrayType, ArraySize > ArrayCont;

    MyOclContext oclcpu = initOcl(CL_DEVICE_TYPE_CPU, 0);
    bolt::BKND::control c_cpu(oclcpu._queue);  // construct control structure from the queue.

    //  Calling the actual functions under test
    std::SORT_FUNC( stdInput.begin( ), stdInput.end( ));
    bolt::BKND::SORT_FUNC( c_cpu, boltInput.begin( ), boltInput.end( ) );

    ArrayCont::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end() );
    ArrayCont::difference_type boltNumElements = std::distance( boltInput.begin( ), boltInput.end() );

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpStdArray< ArrayType, ArraySize >::cmpArrays( stdInput, boltInput );
    
    //OFFSET Test cases
    //  Calling the actual functions under test
    size_t startIndex = 17; //Some aribitrary offset position
    size_t endIndex   = ArraySize -17; //Some aribitrary offset position
    if( (( startIndex > ArraySize ) || ( endIndex < 0 ) )  || (startIndex > endIndex) )
    {
        std::cout <<"\nSkipping NormalOffset Test for size "<< ArraySize << "\n";
    }    
    else
    {
        std::SORT_FUNC( stdOffsetIn.begin( ) + startIndex, stdOffsetIn.begin( ) + endIndex );
        bolt::BKND::SORT_FUNC( c_cpu, boltOffsetIn.begin( ) + startIndex, boltOffsetIn.begin( ) + endIndex );

        ArrayCont::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end( ) );
        ArrayCont::difference_type boltNumElements = std::distance( boltInput.begin( ), boltInput.end( ) );

        //  Both collections should have the same number of elements
        EXPECT_EQ( stdNumElements, boltNumElements );

        //  Loop through the array and compare all the values with each other
        cmpStdArray< ArrayType, ArraySize >::cmpArrays( stdInput, boltInput );
    }
}
#endif

TYPED_TEST_P( SortArrayTest, GreaterFunction )
{
    typedef std::array< ArrayType, ArraySize > ArrayCont;

    //  Calling the actual functions under test
    std::SORT_FUNC( stdInput.begin( ), stdInput.end( ), std::greater< ArrayType >() );
    
    bolt::BKND::SORT_FUNC( boltInput.begin( ), boltInput.end( ), bolt::BKND::greater< ArrayType >( ) );

    ArrayCont::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end() );
    ArrayCont::difference_type boltNumElements = std::distance( boltInput.begin( ), boltInput.end() );

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpStdArray< ArrayType, ArraySize >::cmpArrays( stdInput, boltInput );
#if (TEST_OFFSET_BUFFERS == 1)    
    //OFFSET Test cases
    //  Calling the actual functions under test
    size_t startIndex = 17; //Some aribitrary offset position
    size_t endIndex   = ArraySize -17; //Some aribitrary offset position
    if( (( startIndex > ArraySize ) || ( endIndex < 0 ) )  || (startIndex > endIndex) )
    {
        std::cout <<"\nSkipping NormalOffset Test for size "<< ArraySize << "\n";
    }    
    else
    {
        std::SORT_FUNC( stdOffsetIn.begin( ) + startIndex, stdOffsetIn.begin( ) + endIndex, std::greater< ArrayType >() );
        bolt::BKND::SORT_FUNC( boltOffsetIn.begin( ) + startIndex, boltOffsetIn.begin( ) + endIndex, bolt::BKND::greater< ArrayType >( )  );

        ArrayCont::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end( ) );
        ArrayCont::difference_type boltNumElements = std::distance( boltInput.begin( ), boltInput.end( ) );

        //  Both collections should have the same number of elements
        EXPECT_EQ( stdNumElements, boltNumElements );

        //  Loop through the array and compare all the values with each other
        cmpStdArray< ArrayType, ArraySize >::cmpArrays( stdInput, boltInput );
    }
#endif
}

TYPED_TEST_P( SortArrayTest, LessFunction )
{
    typedef std::array< ArrayType, ArraySize > ArrayCont;

    //  Calling the actual functions under test
    std::SORT_FUNC( stdInput.begin( ), stdInput.end( ), std::less<ArrayType>());
    bolt::BKND::SORT_FUNC( boltInput.begin( ), boltInput.end( ), bolt::BKND::less<ArrayType>() );

    ArrayCont::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end() );
    ArrayCont::difference_type boltNumElements = std::distance( boltInput.begin( ), boltInput.end() );

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpStdArray< ArrayType, ArraySize >::cmpArrays( stdInput, boltInput );
#if (TEST_OFFSET_BUFFERS == 1)
    //OFFSET Test cases
    //  Calling the actual functions under test
    size_t startIndex = 17; //Some aribitrary offset position
    size_t endIndex   = ArraySize -17; //Some aribitrary offset position
    if( (( startIndex > ArraySize ) || ( endIndex < 0 ) )  || (startIndex > endIndex) )
    {
        std::cout <<"\nSkipping NormalOffset Test for size "<< ArraySize << "\n";
    }    
    else
    {
        std::SORT_FUNC( stdOffsetIn.begin( ) + startIndex, stdOffsetIn.begin( ) + endIndex, std::less< ArrayType >() );
        bolt::BKND::SORT_FUNC( boltOffsetIn.begin( ) + startIndex, boltOffsetIn.begin( ) + endIndex, bolt::BKND::less< ArrayType >( )  );

        ArrayCont::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end( ) );
        ArrayCont::difference_type boltNumElements = std::distance( boltInput.begin( ), boltInput.end( ) );

        //  Both collections should have the same number of elements
        EXPECT_EQ( stdNumElements, boltNumElements );

        //  Loop through the array and compare all the values with each other
        cmpStdArray< ArrayType, ArraySize >::cmpArrays( stdInput, boltInput );
    }
#endif
}


REGISTER_TYPED_TEST_CASE_P( SortArrayTest, Normal,  
                                           GreaterFunction,
                                           LessFunction );


/////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Fixture classes are now defined to enable googletest to process value parameterized tests
//  ::testing::TestWithParam< int > means that GetParam( ) returns int values, which i use for array size



class SortIntegerVector: public ::testing::TestWithParam< int >
{
public:

    static int gen_int_random(void)
    {
        int toggle = 0;
        if(toggle == 0)
        {
            toggle = 1;
            return -rand();
        }
        else
        {
            toggle = 0;
            return rand();
        }
    }
    SortIntegerVector( ): stdInput( GetParam( ) ), boltInput( GetParam( ) )
    {
        std::generate(stdInput.begin(), stdInput.end(), gen_int_random);
        boltInput = stdInput;
    }

protected:
    std::vector< int > stdInput, boltInput;
};

//  ::testing::TestWithParam< int > means that GetParam( ) returns int values, which i use for array size
class SortFloatVector: public ::testing::TestWithParam< int >
{
public:
    static float gen_float_random(void)
    {
        int toggle = 0;
        if(toggle == 0)
        {
            toggle = 1;
            return (float)-rand();
        }
        else
        {
            toggle = 0;
            return (float)rand();
        }
    }
    SortFloatVector( ): stdInput( GetParam( ) ), boltInput( GetParam( ) )
    {
        std::generate(stdInput.begin(), stdInput.end(), gen_float_random);
        boltInput = stdInput;    
    }

protected:
    std::vector< float > stdInput, boltInput;
};

#if (TEST_DOUBLE == 1)
//  ::testing::TestWithParam< int > means that GetParam( ) returns int values, which i use for array size
class SortDoubleVector: public ::testing::TestWithParam< int >
{
public:
    static double gen_double_random(void)
    {
        int toggle = 0;
        if(toggle == 0)
        {
            toggle = 1;
            return (double)-rand();
        }
        else
        {
            toggle = 0;
            return (double)rand();
        }
    }
    SortDoubleVector( ): stdInput( GetParam( ) ), boltInput( GetParam( ) )
    {
        std::generate(stdInput.begin(), stdInput.end(), gen_double_random);
        boltInput = stdInput;    
    }

protected:
    std::vector< double > stdInput, boltInput;
};
#endif

//  ::testing::TestWithParam< int > means that GetParam( ) returns int values, which i use for array size
class SortIntegerDeviceVector: public ::testing::TestWithParam< int >
{
public:
    static int gen_int_random(void)
    {
        int toggle = 0;
        if(toggle == 0)
        {
            toggle = 1;
            return -rand();
        }
        else
        {
            toggle = 0;
            return rand();
        }
    }
    // Create an std and a bolt vector of requested size, and initialize all the elements to 1
    SortIntegerDeviceVector( ): stdInput( GetParam( ) ), boltInput( static_cast<size_t>( GetParam( ) ) ), 
                                stdOffsetIn( GetParam( ) ), boltOffsetIn( static_cast<size_t>( GetParam( ) ) ), ArraySize ( GetParam( ) )
    {
        std::generate(stdInput.begin(), stdInput.end(), gen_int_random);
        //boltInput = stdInput;      
        //FIXME - The above should work but the below loop is used. 
        for (int i=0; i< GetParam( ); i++)
        {
            boltInput[i] = stdInput[i];
            boltOffsetIn[i] = stdInput[i];
            stdOffsetIn[i] = stdInput[i];
        }
    }

protected:
    std::vector< int > stdInput, stdOffsetIn;
    bolt::BKND::device_vector< int > boltInput, boltOffsetIn;
    const int ArraySize;
};

//  ::testing::TestWithParam< int > means that GetParam( ) returns int values, which i use for array size
class SortFloatDeviceVector: public ::testing::TestWithParam< int >
{
public:
    static float gen_float_random(void)
    {
        int toggle = 0;
        if(toggle == 0)
        {
            toggle = 1;
            return (float)-rand();
        }
        else
        {
            toggle = 0;
            return (float)rand();
        }
    }
    // Create an std and a bolt vector of requested size, and initialize all the elements to 1
    SortFloatDeviceVector( ): stdInput( GetParam( ) ), boltInput( static_cast<size_t>( GetParam( ) ) ), 
                              stdOffsetIn( GetParam( ) ), boltOffsetIn( static_cast<size_t>( GetParam( ) ) )
    {
        std::generate(stdInput.begin(), stdInput.end(), gen_float_random);
        //boltInput = stdInput;      
        //FIXME - The above should work but the below loop is used. 
        for (int i=0; i< GetParam( ); i++)
        {
            boltInput[i] = stdInput[i];
            boltOffsetIn[i] = stdInput[i];
            stdOffsetIn[i] = stdInput[i];
        }
    }

protected:
    std::vector< float > stdInput, stdOffsetIn;
    bolt::BKND::device_vector< float > boltInput, boltOffsetIn;
};

#if (TEST_DOUBLE == 1)
//  ::testing::TestWithParam< int > means that GetParam( ) returns int values, which i use for array size
class SortDoubleDeviceVector: public ::testing::TestWithParam< int >
{
public:
    static double gen_double_random(void)
    {
        int toggle = 0;
        if(toggle == 0)
        {
            toggle = 1;
            return (double)-rand();
        }
        else
        {
            toggle = 0;
            return (double)rand();
        }
    }
    // Create an std and a bolt vector of requested size, and initialize all the elements to 1
    SortDoubleDeviceVector( ): stdInput( GetParam( ) ), boltInput( static_cast<size_t>( GetParam( ) ) ),
                               stdOffsetIn( GetParam( ) ), boltOffsetIn( static_cast<size_t>( GetParam( ) ) )
    {
        std::generate(stdInput.begin(), stdInput.end(), gen_double_random);
        //boltInput = stdInput;      
        //FIXME - The above should work but the below loop is used. 
        for (int i=0; i< GetParam( ); i++)
        {
            boltInput[i] = stdInput[i];
            boltOffsetIn[i] = stdInput[i];
            stdOffsetIn[i] = stdInput[i];
        }
    }

protected:
    std::vector< double > stdInput, stdOffsetIn;
    bolt::BKND::device_vector< double > boltInput, boltOffsetIn;
};
#endif


/********* Test case to reproduce SuiCHi bugs ******************/
//BOLT_FUNCTOR(UDD,
struct UDD { 
    int a; 
    int b;

    bool operator() (const UDD& lhs, const UDD& rhs) const restrict(cpu, amp){ 
        return ((lhs.a+lhs.b) > (rhs.a+rhs.b));
    } 
    bool operator < (const UDD& other) const restrict(cpu, amp){ 
        return ((a+b) < (other.a+other.b));
    }
    bool operator > (const UDD& other) const restrict(cpu, amp){ 
        return ((a+b) > (other.a+other.b));
    }
    bool operator == (const UDD& other) const restrict(cpu, amp){ 
        return ((a+b) == (other.a+other.b));
    }
    UDD() restrict(cpu, amp)
        : a(0),b(0) { } 
    UDD(int _in) restrict(cpu, amp)
        : a(_in), b(_in +1)  { } 
}; 
//);

//BOLT_FUNCTOR(sortBy_UDD_a,
    struct sortBy_UDD_a {
        bool operator() (const UDD& a, const UDD& b) const restrict(cpu, amp)
        { 
            return (a.a>b.a); 
        };
    };
//);

//BOLT_FUNCTOR(sortBy_UDD_b,
    struct sortBy_UDD_b {
        bool operator() (UDD& a, UDD& b) const restrict(cpu, amp)
        { 
            return (a.b>b.b); 
        };
    };
//);
//BOLT_TEMPLATE_REGISTER_NEW_TYPE(bolt::BKND::greater, int, UDD);
//BOLT_TEMPLATE_REGISTER_NEW_TYPE(bolt::BKND::less, int, UDD);
//BOLT_TEMPLATE_REGISTER_NEW_ITERATOR(bolt::BKND::device_vector, int, UDD);

//  ::testing::TestWithParam< int > means that GetParam( ) returns int values, which i use for array size
class SortUDDDeviceVector: public ::testing::TestWithParam< int >
{
public:
    static UDD gen_UDD_random(void)
    {
        UDD temp;
        int toggle = 0;
        if(toggle == 0)
        {
            toggle = 1;
            temp.a = -rand();
            temp.b = rand();
            return temp;
        }
        else
        {
            toggle = 0;
            temp.a = rand();
            temp.b = -rand();
            return temp;
        }
    }
    // Create an std and a bolt vector of requested size, and initialize all the elements to 1
    SortUDDDeviceVector( ): stdInput( GetParam( ) ), boltInput( static_cast<size_t>( GetParam( ) ) ),
                            stdOffsetIn( GetParam( ) ), boltOffsetIn( static_cast<size_t>( GetParam( ) ) )
    {
        std::generate(stdInput.begin(), stdInput.end(), gen_UDD_random);
        //boltInput = stdInput;      
        //FIXME - The above should work but the below loop is used. 
        for (int i=0; i< GetParam( ); i++)
        {
            boltInput[i] = stdInput[i];
            boltOffsetIn[i] = stdInput[i];
            stdOffsetIn[i] = stdInput[i];
        }
    }

protected:
    std::vector< UDD > stdInput,stdOffsetIn;
    bolt::BKND::device_vector< UDD > boltInput,boltOffsetIn;
};

//  ::testing::TestWithParam< int > means that GetParam( ) returns int values, which i use for array size
class SortIntegerNakedPointer: public ::testing::TestWithParam< int >
{
public:
    static int gen_int_random(void)
    {
        int toggle = 0;
        if(toggle == 0)
        {
            toggle = 1;
            return -rand();
        }
        else
        {
            toggle = 0;
            return rand();
        }
    }
    //  Create an std and a bolt vector of requested size, and initialize all the elements to 1
    SortIntegerNakedPointer( ): stdInput( new int[ GetParam( ) ] ), boltInput( new int[ GetParam( ) ] )
    {}

    virtual void SetUp( )
    {
        size_t size = GetParam( );

        std::generate(stdInput, stdInput + size, gen_int_random);
        for (size_t i = 0; i<size; i++)
        {
            boltInput[i] = stdInput[i];
        }
    };

    virtual void TearDown( )
    {
        delete [] stdInput;
        delete [] boltInput;
    };

protected:
     int* stdInput;
     int* boltInput;
};

//  ::testing::TestWithParam< int > means that GetParam( ) returns int values, which i use for array size
class SortFloatNakedPointer: public ::testing::TestWithParam< int >
{
public:
    static float gen_float_random(void)
    {
        int toggle = 0;
        if(toggle == 0)
        {
            toggle = 1;
            return (float)-rand();
        }
        else
        {
            toggle = 0;
            return (float)rand();
        }
    }
    //  Create an std and a bolt vector of requested size, and initialize all the elements to 1
    SortFloatNakedPointer( ): stdInput( new float[ GetParam( ) ] ), boltInput( new float[ GetParam( ) ] )
    {}

    virtual void SetUp( )
    {
        size_t size = GetParam( );

        std::generate(stdInput, stdInput + size, gen_float_random);
        for (size_t i = 0; i<size; i++)
        {
            boltInput[i] = stdInput[i];
        }
    };

    virtual void TearDown( )
    {
        delete [] stdInput;
        delete [] boltInput;
    };

protected:
     float* stdInput;
     float* boltInput;
};

#if (TEST_DOUBLE ==1 )
//  ::testing::TestWithParam< int > means that GetParam( ) returns int values, which i use for array size
class SortDoubleNakedPointer: public ::testing::TestWithParam< int >
{
public:
    static double gen_double_random(void)
    {
        int toggle = 0;
        if(toggle == 0)
        {
            toggle = 1;
            return (double)-rand();
        }
        else
        {
            toggle = 0;
            return (double)rand();
        }
    }
    //  Create an std and a bolt vector of requested size, and initialize all the elements to 1
    SortDoubleNakedPointer( ): stdInput( new double[ GetParam( ) ] ), boltInput( new double[ GetParam( ) ] )
    {}

    virtual void SetUp( )
    {
        size_t size = GetParam( );

        std::generate(stdInput, stdInput + size, gen_double_random);

        for( size_t i=0; i < size; i++ )
        {
            boltInput[ i ] = stdInput[ i ];
        }
    };

    virtual void TearDown( )
    {
        delete [] stdInput;
        delete [] boltInput;
    };

protected:
     double* stdInput;
     double* boltInput;
};
#endif

class SortCountingIterator :public ::testing::TestWithParam<int>{
protected:
     int mySize;
public:
    SortCountingIterator(): mySize(GetParam()){
    }
};

//Sort with Fancy Iterator would result in compilation error!

/* TEST_P(SortCountingIterator, withCountingIterator)
{
    std::vector<int> a(mySize);

    bolt::BKND::counting_iterator<int> first(0);
    bolt::BKND::counting_iterator<int> last = first + mySize;
    
    for(int i=0; i< a.size() ; i++)
    {
        a[i] = i;
    }

    std::sort( a.begin( ), a.end( ));
    bolt::BKND::SORT_FUNC( first, last); // This is logically wrong!

    cmpArrays( a, first);

} */


TEST_P( SortIntegerVector, Normal )
{
    //  Calling the actual functions under test
    std::SORT_FUNC( stdInput.begin( ), stdInput.end( ) );
    bolt::BKND::SORT_FUNC( boltInput.begin( ), boltInput.end( ) );

    std::vector< int >::iterator::difference_type stdNumElements = std::distance( stdInput.begin(),stdInput.end());
    std::vector< int >::iterator::difference_type boltNumElements = std::distance( boltInput.begin(),boltInput.end());

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdInput, boltInput );
}

TEST_P( SortIntegerVector, SerialCPU )
{
    bolt::BKND::control ctl = bolt::BKND::control::getDefault( );
    ctl.setForceRunMode(bolt::BKND::control::SerialCpu);

    //  Calling the actual functions under test
    std::SORT_FUNC( stdInput.begin( ), stdInput.end( ) );
    bolt::BKND::SORT_FUNC(ctl, boltInput.begin( ), boltInput.end( ) );

    std::vector< int >::iterator::difference_type stdNumElements = std::distance( stdInput.begin( ),stdInput.end( ) );
    std::vector< int >::iterator::difference_type boltNumElements = std::distance( boltInput.begin(),boltInput.end());

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdInput, boltInput );
}

TEST_P( SortIntegerVector, MultiCoreCPU )
{
    bolt::BKND::control ctl = bolt::BKND::control::getDefault( );
    ctl.setForceRunMode(bolt::BKND::control::MultiCoreCpu);

    //  Calling the actual functions under test
    std::SORT_FUNC( stdInput.begin( ), stdInput.end( ) );
    bolt::BKND::SORT_FUNC(ctl, boltInput.begin( ), boltInput.end( ) );

    std::vector< int >::iterator::difference_type stdNumElements = std::distance( stdInput.begin( ),stdInput.end( ) );
    std::vector< int >::iterator::difference_type boltNumElements = std::distance( boltInput.begin(),boltInput.end());

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdInput, boltInput );
}

// Come Back here
TEST_P( SortFloatVector, Normal )
{
    //  Calling the actual functions under test
    std::SORT_FUNC( stdInput.begin( ), stdInput.end( ) );
    bolt::BKND::SORT_FUNC( boltInput.begin( ), boltInput.end( ) );

    std::vector< float >::iterator::difference_type stdNumElements = std::distance( stdInput.begin( ),stdInput.end());
    std::vector< float >::iterator::difference_type boltNumElements = std::distance(boltInput.begin(),boltInput.end());

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdInput, boltInput );
}

TEST_P( SortFloatVector, SerialCPU)
{
    bolt::BKND::control ctl = bolt::BKND::control::getDefault( );
    ctl.setForceRunMode(bolt::BKND::control::SerialCpu);

    //  Calling the actual functions under test
    std::SORT_FUNC( stdInput.begin( ), stdInput.end( ) );
    bolt::BKND::SORT_FUNC( ctl, boltInput.begin( ), boltInput.end( ) );

    std::vector< float >::iterator::difference_type stdNumElements = std::distance( stdInput.begin( ),stdInput.end());
    std::vector< float >::iterator::difference_type boltNumElements = std::distance(boltInput.begin(),boltInput.end());

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdInput, boltInput );
}

TEST_P( SortFloatVector, MultiCoreCPU)
{
    bolt::BKND::control ctl = bolt::BKND::control::getDefault( );
    ctl.setForceRunMode(bolt::BKND::control::MultiCoreCpu);

    //  Calling the actual functions under test
    std::SORT_FUNC( stdInput.begin( ), stdInput.end( ) );
    bolt::BKND::SORT_FUNC( ctl, boltInput.begin( ), boltInput.end( ) );

    std::vector< float >::iterator::difference_type stdNumElements = std::distance( stdInput.begin( ),stdInput.end());
    std::vector< float >::iterator::difference_type boltNumElements = std::distance(boltInput.begin(),boltInput.end());

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdInput, boltInput );
}

#if (TEST_DOUBLE == 1)
TEST_P( SortDoubleVector, Inplace )
{
    //  Calling the actual functions under test
    std::SORT_FUNC( stdInput.begin( ), stdInput.end( ) );
    bolt::BKND::SORT_FUNC( boltInput.begin( ), boltInput.end( ) );

    std::vector< double >::iterator::difference_type stdNumElements = std::distance( stdInput.begin( ),stdInput.end());
    std::vector< double >::iterator::difference_type boltNumElements =std::distance(boltInput.begin(),boltInput.end());

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdInput, boltInput );
}

TEST_P( SortDoubleVector, SerialInplace )
{
    
    bolt::BKND::control ctl = bolt::BKND::control::getDefault( );
    ctl.setForceRunMode(bolt::BKND::control::SerialCpu);
    //  Calling the actual functions under test
    std::SORT_FUNC( stdInput.begin( ), stdInput.end( ) );
    bolt::BKND::SORT_FUNC( ctl, boltInput.begin( ), boltInput.end( ) );

    std::vector< double >::iterator::difference_type stdNumElements = std::distance( stdInput.begin( ),stdInput.end());
    std::vector< double >::iterator::difference_type boltNumElements =std::distance(boltInput.begin(),boltInput.end());

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdInput, boltInput );
}

TEST_P( SortDoubleVector, MulticoreInplace )
{
    
    bolt::BKND::control ctl = bolt::BKND::control::getDefault( );
    ctl.setForceRunMode(bolt::BKND::control::MultiCoreCpu);
    //  Calling the actual functions under test
    std::SORT_FUNC( stdInput.begin( ), stdInput.end( ) );
    bolt::BKND::SORT_FUNC( ctl, boltInput.begin( ), boltInput.end( ) );

    std::vector< double >::iterator::difference_type stdNumElements = std::distance( stdInput.begin( ),stdInput.end());
    std::vector< double >::iterator::difference_type boltNumElements =std::distance(boltInput.begin(),boltInput.end());

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdInput, boltInput );
}

#endif
#if (TEST_DEVICE_VECTOR == 1)
TEST_P( SortIntegerDeviceVector, Inplace )
{
    //  Calling the actual functions under test
    std::SORT_FUNC( stdInput.begin( ), stdInput.end( ) );
    bolt::BKND::SORT_FUNC( boltInput.begin( ), boltInput.end( ) );

    std::vector< int >::iterator::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end());
    std::vector< int >::iterator::difference_type boltNumElements = std::distance( boltInput.begin(),boltInput.end());

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdInput, boltInput );
#if (TEST_OFFSET_BUFFERS == 1)
    //OFFSET Test cases
    //  Calling the actual functions under test
    typedef std::vector< int >::value_type valtype;
    int startIndex = 17; //Some aribitrary offset position
    int endIndex   = GetParam( ) -17; //Some aribitrary offset position
    int ArraySize = GetParam( );
    if( (( startIndex > GetParam( ) ) || ( endIndex < 0 ) )  || (startIndex > endIndex) )
    {
        std::cout <<"\nSkipping NormalOffset Test for size "<< GetParam( ) << "\n";
    }    
    else
    {
        std::SORT_FUNC( stdOffsetIn.begin( ) + startIndex, stdOffsetIn.begin( ) + endIndex, std::less< valtype >() );
        bolt::BKND::SORT_FUNC(boltOffsetIn.begin( ) + startIndex, boltOffsetIn.begin( ) + endIndex, bolt::BKND::less< valtype >( )  );

        std::vector< valtype >::iterator::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end( ) );
        std::vector< valtype >::iterator::difference_type boltNumElements = std::distance( boltInput.begin( ), boltInput.end( ) );

        //  Both collections should have the same number of elements
        EXPECT_EQ( stdNumElements, boltNumElements );

        //  Loop through the array and compare all the values with each other
        //cmpStdArray< valtype, ArraySize >::cmpArrays( stdInput, boltInput );
        cmpArrays( stdInput, boltInput );
    }
#endif
}

TEST_P( SortIntegerDeviceVector, SerialInplace )
{
    bolt::BKND::control ctl = bolt::BKND::control::getDefault( );
    ctl.setForceRunMode(bolt::BKND::control::SerialCpu);

    //  Calling the actual functions under test
    std::SORT_FUNC( stdInput.begin( ), stdInput.end( ) );
    bolt::BKND::SORT_FUNC(ctl,  boltInput.begin( ), boltInput.end( ) );

    std::vector< int >::iterator::difference_type stdNumElements = std::distance( stdInput.begin( ),stdInput.end());
    std::vector< int >::iterator::difference_type boltNumElements = std::distance( boltInput.begin( ),boltInput.end());

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdInput, boltInput );
#if (TEST_OFFSET_BUFFERS == 1)
    //OFFSET Test cases
    //  Calling the actual functions under test
    typedef std::vector< int >::value_type valtype;
    int startIndex = 17; //Some aribitrary offset position
    int endIndex   = GetParam( ) -17; //Some aribitrary offset position
    int ArraySize = GetParam( );
    if( (( startIndex > GetParam( ) ) || ( endIndex < 0 ) )  || (startIndex > endIndex) )
    {
        std::cout <<"\nSkipping NormalOffset Test for size "<< GetParam( ) << "\n";
    }    
    else
    {
        std::SORT_FUNC( stdOffsetIn.begin( ) + startIndex, stdOffsetIn.begin( ) + endIndex, std::less< valtype >() );
        bolt::BKND::SORT_FUNC(boltOffsetIn.begin( ) + startIndex, boltOffsetIn.begin( ) + endIndex, bolt::BKND::less< valtype >( )  );

        std::vector< valtype >::iterator::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end( ) );
        std::vector< valtype >::iterator::difference_type boltNumElements = std::distance( boltInput.begin( ), boltInput.end( ) );

        //  Both collections should have the same number of elements
        EXPECT_EQ( stdNumElements, boltNumElements );

        //  Loop through the array and compare all the values with each other
        //cmpStdArray< valtype, ArraySize >::cmpArrays( stdInput, boltInput );
        cmpArrays( stdInput, boltInput );
    }
#endif
}

TEST_P( SortIntegerDeviceVector, MultiCoreInplace )
{
    bolt::BKND::control ctl = bolt::BKND::control::getDefault( );
    ctl.setForceRunMode(bolt::BKND::control::MultiCoreCpu);

    //  Calling the actual functions under test
    std::SORT_FUNC( stdInput.begin( ), stdInput.end( ) );
    bolt::BKND::SORT_FUNC(ctl,  boltInput.begin( ), boltInput.end( ) );

    std::vector< int >::iterator::difference_type stdNumElements = std::distance( stdInput.begin( ),stdInput.end());
    std::vector< int >::iterator::difference_type boltNumElements = std::distance( boltInput.begin( ),boltInput.end());

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdInput, boltInput );
#if (TEST_OFFSET_BUFFERS == 1)
    //OFFSET Test cases
    //  Calling the actual functions under test
    typedef std::vector< int >::value_type valtype;
    int startIndex = 17; //Some aribitrary offset position
    int endIndex   = GetParam( ) -17; //Some aribitrary offset position
    int ArraySize = GetParam( );
    if( (( startIndex > GetParam( ) ) || ( endIndex < 0 ) )  || (startIndex > endIndex) )
    {
        std::cout <<"\nSkipping NormalOffset Test for size "<< GetParam( ) << "\n";
    }    
    else
    {
        std::SORT_FUNC( stdOffsetIn.begin( ) + startIndex, stdOffsetIn.begin( ) + endIndex, std::less< valtype >() );
        bolt::BKND::SORT_FUNC(boltOffsetIn.begin( ) + startIndex, boltOffsetIn.begin( ) + endIndex, bolt::BKND::less< valtype >( )  );

        std::vector< valtype >::iterator::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end( ) );
        std::vector< valtype >::iterator::difference_type boltNumElements = std::distance( boltInput.begin( ), boltInput.end( ) );

        //  Both collections should have the same number of elements
        EXPECT_EQ( stdNumElements, boltNumElements );

        //  Loop through the array and compare all the values with each other
        //cmpStdArray< valtype, ArraySize >::cmpArrays( stdInput, boltInput );
        cmpArrays( stdInput, boltInput );
    }
#endif
}

TEST_P( SortUDDDeviceVector, Inplace )
{
    typedef std::vector< UDD >::value_type valtype;
    //  Calling the actual functions under test
    std::SORT_FUNC( stdInput.begin( ), stdInput.end( ) );
    bolt::BKND::SORT_FUNC( boltInput.begin( ), boltInput.end( ) );

    std::vector< valtype >::iterator::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end());
    std::vector< valtype >::iterator::difference_type boltNumElements = std::distance( boltInput.begin(),boltInput.end());

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdInput, boltInput );
#if (TEST_OFFSET_BUFFERS == 1)
    //OFFSET Test cases
    //  Calling the actual functions under test

    int startIndex = 17; //Some aribitrary offset position
    int endIndex   = GetParam( ) -17; //Some aribitrary offset position
    int ArraySize = GetParam( );
    if( (( startIndex > GetParam( ) ) || ( endIndex < 0 ) )  || (startIndex > endIndex) )
    {
        std::cout <<"\nSkipping NormalOffset Test for size "<< GetParam( ) << "\n";
    }    
    else
    {
        std::SORT_FUNC( stdOffsetIn.begin( ) + startIndex, stdOffsetIn.begin( ) + endIndex, std::less< valtype >() );
        bolt::BKND::SORT_FUNC(boltOffsetIn.begin( ) + startIndex, boltOffsetIn.begin( ) + endIndex, bolt::BKND::less< valtype >( )  );

        std::vector< valtype >::iterator::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end( ) );
        std::vector< valtype >::iterator::difference_type boltNumElements = std::distance( boltInput.begin( ), boltInput.end( ) );

        //  Both collections should have the same number of elements
        EXPECT_EQ( stdNumElements, boltNumElements );

        //  Loop through the array and compare all the values with each other
        //cmpStdArray< valtype, ArraySize >::cmpArrays( stdInput, boltInput );
        cmpArrays( stdInput, boltInput );
    }
#endif
}

TEST_P( SortUDDDeviceVector, SerialInplace )
{
    typedef std::vector< UDD >::value_type valtype;
    bolt::BKND::control ctl = bolt::BKND::control::getDefault( );
    ctl.setForceRunMode(bolt::BKND::control::SerialCpu);

    //  Calling the actual functions under test
    std::SORT_FUNC( stdInput.begin( ), stdInput.end( ) );
    bolt::BKND::SORT_FUNC(ctl,  boltInput.begin( ), boltInput.end( ) );

    std::vector< valtype >::iterator::difference_type stdNumElements = std::distance( stdInput.begin( ),stdInput.end());
    std::vector< valtype >::iterator::difference_type boltNumElements = std::distance( boltInput.begin( ),boltInput.end());

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdInput, boltInput );
#if (TEST_OFFSET_BUFFERS == 1)
    //OFFSET Test cases
    //  Calling the actual functions under test

    int startIndex = 17; //Some aribitrary offset position
    int endIndex   = GetParam( ) -17; //Some aribitrary offset position
    int ArraySize = GetParam( );
    if( (( startIndex > GetParam( ) ) || ( endIndex < 0 ) )  || (startIndex > endIndex) )
    {
        std::cout <<"\nSkipping NormalOffset Test for size "<< GetParam( ) << "\n";
    }    
    else
    {
        std::SORT_FUNC( stdOffsetIn.begin( ) + startIndex, stdOffsetIn.begin( ) + endIndex, std::less< valtype >() );
        bolt::BKND::SORT_FUNC(boltOffsetIn.begin( ) + startIndex, boltOffsetIn.begin( ) + endIndex, bolt::BKND::less< valtype >( )  );

        std::vector< valtype >::iterator::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end( ) );
        std::vector< valtype >::iterator::difference_type boltNumElements = std::distance( boltInput.begin( ), boltInput.end( ) );

        //  Both collections should have the same number of elements
        EXPECT_EQ( stdNumElements, boltNumElements );

        //  Loop through the array and compare all the values with each other
        //cmpStdArray< valtype, ArraySize >::cmpArrays( stdInput, boltInput );
        cmpArrays( stdInput, boltInput );
    }
#endif
}

TEST_P( SortUDDDeviceVector, MultiCoreInplace )
{
    typedef std::vector< UDD >::value_type valtype;
    bolt::BKND::control ctl = bolt::BKND::control::getDefault( );
    ctl.setForceRunMode(bolt::BKND::control::MultiCoreCpu);

    //  Calling the actual functions under test
    std::SORT_FUNC( stdInput.begin( ), stdInput.end( ) );
    bolt::BKND::SORT_FUNC(ctl,  boltInput.begin( ), boltInput.end( ) );

    std::vector< UDD >::iterator::difference_type stdNumElements = std::distance( stdInput.begin( ),stdInput.end());
    std::vector< UDD >::iterator::difference_type boltNumElements = std::distance( boltInput.begin( ),boltInput.end());

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdInput, boltInput );
#if (TEST_OFFSET_BUFFERS == 1)
    //OFFSET Test cases
    //  Calling the actual functions under test
    int startIndex = 17; //Some aribitrary offset position
    int endIndex   = GetParam( ) -17; //Some aribitrary offset position
    int ArraySize = GetParam( );
    if( (( startIndex > GetParam( ) ) || ( endIndex < 0 ) )  || (startIndex > endIndex) )
    {
        std::cout <<"\nSkipping NormalOffset Test for size "<< GetParam( ) << "\n";
    }    
    else
    {
        std::SORT_FUNC( stdOffsetIn.begin( ) + startIndex, stdOffsetIn.begin( ) + endIndex, std::less< valtype >() );
        bolt::BKND::SORT_FUNC(boltOffsetIn.begin( ) + startIndex, boltOffsetIn.begin( ) + endIndex, bolt::BKND::less< valtype >( )  );

        std::vector< valtype >::iterator::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end( ) );
        std::vector< valtype >::iterator::difference_type boltNumElements = std::distance( boltInput.begin( ), boltInput.end( ) );

        //  Both collections should have the same number of elements
        EXPECT_EQ( stdNumElements, boltNumElements );

        //  Loop through the array and compare all the values with each other
        //cmpStdArray< valtype, ArraySize >::cmpArrays( stdInput, boltInput );
        cmpArrays( stdInput, boltInput );
    }
#endif
}

TEST_P( SortFloatDeviceVector, Inplace )
{
    typedef std::vector< float >::value_type valtype;
    //  Calling the actual functions under test
    std::SORT_FUNC( stdInput.begin( ), stdInput.end( ) );
    bolt::BKND::SORT_FUNC( boltInput.begin( ), boltInput.end( ) );

    std::vector< valtype >::iterator::difference_type stdNumElements = std::distance( stdInput.begin( ),stdInput.end());
    std::vector< valtype >::iterator::difference_type boltNumElements = std::distance(boltInput.begin(),boltInput.end());

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdInput, boltInput );
#if (TEST_OFFSET_BUFFERS == 1)
    //OFFSET Test cases
    //  Calling the actual functions under test

    int startIndex = 17; //Some aribitrary offset position
    int endIndex   = GetParam( ) -17; //Some aribitrary offset position
    int ArraySize = GetParam( );
    if( (( startIndex > GetParam( ) ) || ( endIndex < 0 ) )  || (startIndex > endIndex) )
    {
        std::cout <<"\nSkipping NormalOffset Test for size "<< GetParam( ) << "\n";
    }    
    else
    {
        std::SORT_FUNC( stdOffsetIn.begin( ) + startIndex, stdOffsetIn.begin( ) + endIndex, std::less< valtype >() );
        bolt::BKND::SORT_FUNC(boltOffsetIn.begin( ) + startIndex, boltOffsetIn.begin( ) + endIndex, bolt::BKND::less< valtype >( )  );

        std::vector< valtype >::iterator::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end( ) );
        std::vector< valtype >::iterator::difference_type boltNumElements = std::distance( boltInput.begin( ), boltInput.end( ) );

        //  Both collections should have the same number of elements
        EXPECT_EQ( stdNumElements, boltNumElements );

        //  Loop through the array and compare all the values with each other
        //cmpStdArray< valtype, ArraySize >::cmpArrays( stdInput, boltInput );
        cmpArrays( stdInput, boltInput );
    }
#endif
}

TEST_P( SortFloatDeviceVector, SerialInplace )
{
    typedef std::vector< float >::value_type valtype;
    bolt::BKND::control ctl = bolt::BKND::control::getDefault( );
    ctl.setForceRunMode(bolt::BKND::control::SerialCpu);

    //  Calling the actual functions under test
    std::SORT_FUNC( stdInput.begin( ), stdInput.end( ) );
    bolt::BKND::SORT_FUNC( ctl, boltInput.begin( ), boltInput.end( ) );

    std::vector< valtype >::iterator::difference_type stdNumElements = std::distance( stdInput.begin(),stdInput.end());
    std::vector< valtype >::iterator::difference_type boltNumElements = std::distance(boltInput.begin(),boltInput.end());

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdInput, boltInput );
#if (TEST_OFFSET_BUFFERS == 1)
    //OFFSET Test cases
    //  Calling the actual functions under test

    int startIndex = 17; //Some aribitrary offset position
    int endIndex   = GetParam( ) -17; //Some aribitrary offset position
    int ArraySize = GetParam( );
    if( (( startIndex > GetParam( ) ) || ( endIndex < 0 ) )  || (startIndex > endIndex) )
    {
        std::cout <<"\nSkipping NormalOffset Test for size "<< GetParam( ) << "\n";
    }    
    else
    {
        std::SORT_FUNC( stdOffsetIn.begin( ) + startIndex, stdOffsetIn.begin( ) + endIndex, std::less< valtype >() );
        bolt::BKND::SORT_FUNC(boltOffsetIn.begin( ) + startIndex, boltOffsetIn.begin( ) + endIndex, bolt::BKND::less< valtype >( )  );

        std::vector< valtype >::iterator::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end( ) );
        std::vector< valtype >::iterator::difference_type boltNumElements = std::distance( boltInput.begin( ), boltInput.end( ) );

        //  Both collections should have the same number of elements
        EXPECT_EQ( stdNumElements, boltNumElements );

        //  Loop through the array and compare all the values with each other
        //cmpStdArray< valtype, ArraySize >::cmpArrays( stdInput, boltInput );
        cmpArrays( stdInput, boltInput );
    }
#endif
}

TEST_P( SortFloatDeviceVector, MultiCoreInplace )
{
    typedef std::vector< float >::value_type valtype;
    bolt::BKND::control ctl = bolt::BKND::control::getDefault( );
    ctl.setForceRunMode(bolt::BKND::control::MultiCoreCpu);

    //  Calling the actual functions under test
    std::SORT_FUNC( stdInput.begin( ), stdInput.end( ) );
    bolt::BKND::SORT_FUNC( ctl, boltInput.begin( ), boltInput.end( ) );

    std::vector< valtype >::iterator::difference_type stdNumElements = std::distance( stdInput.begin(),stdInput.end());
    std::vector< valtype >::iterator::difference_type boltNumElements = std::distance(boltInput.begin(),boltInput.end());

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdInput, boltInput );
#if (TEST_OFFSET_BUFFERS == 1)
    //OFFSET Test cases
    //  Calling the actual functions under test
    int startIndex = 17; //Some aribitrary offset position
    int endIndex   = GetParam( ) -17; //Some aribitrary offset position
    int ArraySize = GetParam( );
    if( (( startIndex > GetParam( ) ) || ( endIndex < 0 ) )  || (startIndex > endIndex) )
    {
        std::cout <<"\nSkipping NormalOffset Test for size "<< GetParam( ) << "\n";
    }    
    else
    {
        std::SORT_FUNC( stdOffsetIn.begin( ) + startIndex, stdOffsetIn.begin( ) + endIndex, std::less< valtype >() );
        bolt::BKND::SORT_FUNC(boltOffsetIn.begin( ) + startIndex, boltOffsetIn.begin( ) + endIndex, bolt::BKND::less< valtype >( )  );

        std::vector< valtype >::iterator::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end( ) );
        std::vector< valtype >::iterator::difference_type boltNumElements = std::distance( boltInput.begin( ), boltInput.end( ) );

        //  Both collections should have the same number of elements
        EXPECT_EQ( stdNumElements, boltNumElements );

        //  Loop through the array and compare all the values with each other
        //cmpStdArray< valtype, ArraySize >::cmpArrays( stdInput, boltInput );
        cmpArrays( stdInput, boltInput );
    }
#endif
}

#if (TEST_DOUBLE == 1)
TEST_P( SortDoubleDeviceVector, Inplace )
{
    typedef std::vector< double >::value_type valtype;
    //  Calling the actual functions under test
    std::SORT_FUNC( stdInput.begin( ), stdInput.end( ) );
    bolt::BKND::SORT_FUNC( boltInput.begin( ), boltInput.end( ) );

    std::vector< valtype >::iterator::difference_type stdNumElements = std::distance( stdInput.begin(),stdInput.end());
    std::vector< valtype >::iterator::difference_type boltNumElements =std::distance(boltInput.begin(),boltInput.end());

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdInput, boltInput );
#if (TEST_OFFSET_BUFFERS == 1)
    //OFFSET Test cases
    //  Calling the actual functions under test
    int startIndex = 17; //Some aribitrary offset position
    int endIndex   = GetParam( ) -17; //Some aribitrary offset position
    int ArraySize = GetParam( );
    if( (( startIndex > GetParam( ) ) || ( endIndex < 0 ) )  || (startIndex > endIndex) )
    {
        std::cout <<"\nSkipping NormalOffset Test for size "<< GetParam( ) << "\n";
    }    
    else
    {
        std::SORT_FUNC( stdOffsetIn.begin( ) + startIndex, stdOffsetIn.begin( ) + endIndex, std::less< valtype >() );
        bolt::BKND::SORT_FUNC(boltOffsetIn.begin( ) + startIndex, boltOffsetIn.begin( ) + endIndex, bolt::BKND::less< valtype >( )  );

        std::vector< valtype >::iterator::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end( ) );
        std::vector< valtype >::iterator::difference_type boltNumElements = std::distance( boltInput.begin( ), boltInput.end( ) );

        //  Both collections should have the same number of elements
        EXPECT_EQ( stdNumElements, boltNumElements );

        //  Loop through the array and compare all the values with each other
        //cmpStdArray< valtype, ArraySize >::cmpArrays( stdInput, boltInput );
        cmpArrays( stdInput, boltInput );
    }
#endif
}

TEST_P( SortDoubleDeviceVector, SerialInplace )
{
    typedef std::vector< double >::value_type valtype;
    bolt::BKND::control ctl = bolt::BKND::control::getDefault( );
    ctl.setForceRunMode(bolt::BKND::control::SerialCpu);
    //  Calling the actual functions under test
    std::SORT_FUNC( stdInput.begin( ), stdInput.end( ) );
    bolt::BKND::SORT_FUNC( boltInput.begin( ), boltInput.end( ) );

    std::vector< valtype >::iterator::difference_type stdNumElements = std::distance( stdInput.begin(),stdInput.end());
    std::vector< valtype >::iterator::difference_type boltNumElements =std::distance(boltInput.begin(),boltInput.end());

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdInput, boltInput );
#if (TEST_OFFSET_BUFFERS == 1)
    //OFFSET Test cases
    //  Calling the actual functions under test
    int startIndex = 17; //Some aribitrary offset position
    int endIndex   = GetParam( ) -17; //Some aribitrary offset position
    int ArraySize = GetParam( );
    if( (( startIndex > GetParam( ) ) || ( endIndex < 0 ) )  || (startIndex > endIndex) )
    {
        std::cout <<"\nSkipping NormalOffset Test for size "<< GetParam( ) << "\n";
    }    
    else
    {
        std::SORT_FUNC( stdOffsetIn.begin( ) + startIndex, stdOffsetIn.begin( ) + endIndex, std::less< valtype >() );
        bolt::BKND::SORT_FUNC(boltOffsetIn.begin( ) + startIndex, boltOffsetIn.begin( ) + endIndex, bolt::BKND::less< valtype >( )  );

        std::vector< valtype >::iterator::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end( ) );
        std::vector< valtype >::iterator::difference_type boltNumElements = std::distance( boltInput.begin( ), boltInput.end( ) );

        //  Both collections should have the same number of elements
        EXPECT_EQ( stdNumElements, boltNumElements );

        //  Loop through the array and compare all the values with each other
        //cmpStdArray< valtype, ArraySize >::cmpArrays( stdInput, boltInput );
        cmpArrays( stdInput, boltInput );
    }
#endif
}

TEST_P( SortDoubleDeviceVector, MulticoreInplace )
{
    typedef std::vector< double >::value_type valtype;
    bolt::BKND::control ctl = bolt::BKND::control::getDefault( );
    ctl.setForceRunMode(bolt::BKND::control::MultiCoreCpu);
    //  Calling the actual functions under test
    std::SORT_FUNC( stdInput.begin( ), stdInput.end( ) );
    bolt::BKND::SORT_FUNC( boltInput.begin( ), boltInput.end( ) );

    std::vector< valtype >::iterator::difference_type stdNumElements = std::distance( stdInput.begin(),stdInput.end());
    std::vector< valtype >::iterator::difference_type boltNumElements =std::distance(boltInput.begin(),boltInput.end());

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdInput, boltInput );
#if (TEST_OFFSET_BUFFERS == 1)
    //OFFSET Test cases
    //  Calling the actual functions under test
    int startIndex = 17; //Some aribitrary offset position
    int endIndex   = GetParam( ) -17; //Some aribitrary offset position
    int ArraySize = GetParam( );
    if( (( startIndex > GetParam( ) ) || ( endIndex < 0 ) )  || (startIndex > endIndex) )
    {
        std::cout <<"\nSkipping NormalOffset Test for size "<< GetParam( ) << "\n";
    }    
    else
    {
        std::SORT_FUNC( stdOffsetIn.begin( ) + startIndex, stdOffsetIn.begin( ) + endIndex, std::less< valtype >() );
        bolt::BKND::SORT_FUNC(boltOffsetIn.begin( ) + startIndex, boltOffsetIn.begin( ) + endIndex, bolt::BKND::less< valtype >( )  );

        std::vector< valtype >::iterator::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end( ) );
        std::vector< valtype >::iterator::difference_type boltNumElements = std::distance( boltInput.begin( ), boltInput.end( ) );

        //  Both collections should have the same number of elements
        EXPECT_EQ( stdNumElements, boltNumElements );

        //  Loop through the array and compare all the values with each other
        //cmpStdArray< valtype, ArraySize >::cmpArrays( stdInput, boltInput );
        cmpArrays( stdInput, boltInput );
    }
#endif
}

#endif
#endif

TEST_P( SortIntegerNakedPointer, Inplace )
{
    unsigned int endIndex = GetParam( );

    //  Calling the actual functions under test
    stdext::checked_array_iterator< int* > wrapStdInput( stdInput, endIndex );
    std::SORT_FUNC( wrapStdInput, wrapStdInput + endIndex );
    //std::SORT_FUNC( stdInput, stdInput + endIndex );

    stdext::checked_array_iterator< int* > wrapBoltInput( boltInput, endIndex );
    bolt::BKND::SORT_FUNC( wrapBoltInput, wrapBoltInput + endIndex );
    //bolt::BKND::SORT_FUNC( boltInput, boltInput + endIndex );

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdInput, boltInput, endIndex );
}

TEST_P( SortIntegerNakedPointer, SerialInplace )
{
    bolt::BKND::control ctl = bolt::BKND::control::getDefault( );
    ctl.setForceRunMode(bolt::BKND::control::SerialCpu);

    unsigned int endIndex = GetParam( );

    //  Calling the actual functions under test
    stdext::checked_array_iterator< int* > wrapStdInput( stdInput, endIndex );
    std::SORT_FUNC( wrapStdInput, wrapStdInput + endIndex );
    //std::SORT_FUNC( stdInput, stdInput + endIndex );

    stdext::checked_array_iterator< int* > wrapBoltInput( boltInput, endIndex );
    bolt::BKND::SORT_FUNC( ctl, wrapBoltInput, wrapBoltInput + endIndex );
    //bolt::BKND::SORT_FUNC( boltInput, boltInput + endIndex );

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdInput, boltInput, endIndex );
}

TEST_P( SortIntegerNakedPointer, MultiCoreInplace )
{
    bolt::BKND::control ctl = bolt::BKND::control::getDefault( );
    ctl.setForceRunMode(bolt::BKND::control::MultiCoreCpu);

    unsigned int endIndex = GetParam( );

    //  Calling the actual functions under test
    stdext::checked_array_iterator< int* > wrapStdInput( stdInput, endIndex );
    std::SORT_FUNC( wrapStdInput, wrapStdInput + endIndex );
    //std::SORT_FUNC( stdInput, stdInput + endIndex );

    stdext::checked_array_iterator< int* > wrapBoltInput( boltInput, endIndex );
    bolt::BKND::SORT_FUNC( ctl, wrapBoltInput, wrapBoltInput + endIndex );
    //bolt::BKND::SORT_FUNC( boltInput, boltInput + endIndex );

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdInput, boltInput, endIndex );
}


TEST_P( SortFloatNakedPointer, Inplace )
{
    unsigned int endIndex = GetParam( );

    //  Calling the actual functions under test
    stdext::checked_array_iterator< float* > wrapStdInput( stdInput, endIndex );
    std::SORT_FUNC( wrapStdInput, wrapStdInput + endIndex );
    //std::SORT_FUNC( stdInput, stdInput + endIndex );

    stdext::checked_array_iterator< float* > wrapBoltInput( boltInput, endIndex );
    bolt::BKND::SORT_FUNC( wrapBoltInput, wrapBoltInput + endIndex );
    //bolt::BKND::SORT_FUNC( boltInput, boltInput + endIndex );

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdInput, boltInput, endIndex );
}

TEST_P( SortFloatNakedPointer, SerialInplace )
{
    bolt::BKND::control ctl = bolt::BKND::control::getDefault( );
    ctl.setForceRunMode(bolt::BKND::control::SerialCpu);

    unsigned int endIndex = GetParam( );

    //  Calling the actual functions under test
    stdext::checked_array_iterator< float* > wrapStdInput( stdInput, endIndex );
    std::SORT_FUNC( wrapStdInput, wrapStdInput + endIndex );
    //std::SORT_FUNC( stdInput, stdInput + endIndex );

    stdext::checked_array_iterator< float* > wrapBoltInput( boltInput, endIndex );
    bolt::BKND::SORT_FUNC( ctl, wrapBoltInput, wrapBoltInput + endIndex );
    //bolt::BKND::SORT_FUNC( boltInput, boltInput + endIndex );

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdInput, boltInput, endIndex );
}

TEST_P( SortFloatNakedPointer, MultiCoreInplace )
{
    bolt::BKND::control ctl = bolt::BKND::control::getDefault( );
    ctl.setForceRunMode(bolt::BKND::control::MultiCoreCpu);

    unsigned int endIndex = GetParam( );

    //  Calling the actual functions under test
    stdext::checked_array_iterator< float* > wrapStdInput( stdInput, endIndex );
    std::SORT_FUNC( wrapStdInput, wrapStdInput + endIndex );
    //std::SORT_FUNC( stdInput, stdInput + endIndex );

    stdext::checked_array_iterator< float* > wrapBoltInput( boltInput, endIndex );
    bolt::BKND::SORT_FUNC( ctl, wrapBoltInput, wrapBoltInput + endIndex );
    //bolt::BKND::SORT_FUNC( boltInput, boltInput + endIndex );

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdInput, boltInput, endIndex );
}

#if (TEST_DOUBLE == 1)
TEST_P( SortDoubleNakedPointer, Inplace )
{
    unsigned int endIndex = GetParam( );

    //  Calling the actual functions under test
    stdext::checked_array_iterator< double* > wrapStdInput( stdInput, endIndex );
    std::SORT_FUNC( wrapStdInput, wrapStdInput + endIndex );
    //std::SORT_FUNC( stdInput, stdInput + endIndex );

    stdext::checked_array_iterator< double* > wrapBoltInput( boltInput, endIndex );
    bolt::BKND::SORT_FUNC( wrapBoltInput, wrapBoltInput + endIndex );
    //bolt::BKND::SORT_FUNC( boltInput, boltInput + endIndex );

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdInput, boltInput, endIndex );
}

TEST_P( SortDoubleNakedPointer, SerialInplace )
{
    unsigned int endIndex = GetParam( );

    //  Calling the actual functions under test
    stdext::checked_array_iterator< double* > wrapStdInput( stdInput, endIndex );
    std::SORT_FUNC( wrapStdInput, wrapStdInput + endIndex );
    //std::SORT_FUNC( stdInput, stdInput + endIndex );
    
    bolt::BKND::control ctl = bolt::BKND::control::getDefault( );
    ctl.setForceRunMode(bolt::BKND::control::SerialCpu);

    stdext::checked_array_iterator< double* > wrapBoltInput( boltInput, endIndex );
    bolt::BKND::SORT_FUNC( wrapBoltInput, wrapBoltInput + endIndex );
    //bolt::BKND::SORT_FUNC( boltInput, boltInput + endIndex );

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdInput, boltInput, endIndex );
}
TEST_P( SortDoubleNakedPointer, MulticoreInplace )
{
    unsigned int endIndex = GetParam( );

    //  Calling the actual functions under test
    stdext::checked_array_iterator< double* > wrapStdInput( stdInput, endIndex );
    std::SORT_FUNC( wrapStdInput, wrapStdInput + endIndex );
    //std::SORT_FUNC( stdInput, stdInput + endIndex );
    
    bolt::BKND::control ctl = bolt::BKND::control::getDefault( );
    ctl.setForceRunMode(bolt::BKND::control::MultiCoreCpu);

    stdext::checked_array_iterator< double* > wrapBoltInput( boltInput, endIndex );
    bolt::BKND::SORT_FUNC( wrapBoltInput, wrapBoltInput + endIndex );
    //bolt::BKND::SORT_FUNC( boltInput, boltInput + endIndex );

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdInput, boltInput, endIndex );
}


#endif
std::array<int, 15> TestValues = {2,4,8,16,32,64,128,256,512,1024,2048,4096,8192,16384,32768}; // 2 to 2^15
//Test lots of consecutive numbers, but small range, suitable for integers because they overflow easier
INSTANTIATE_TEST_CASE_P( SortRange, SortIntegerVector, ::testing::Range(  1, 4096, 54 ) );  //   1 to 2^22
INSTANTIATE_TEST_CASE_P( SortValues, SortIntegerVector, ::testing::ValuesIn( TestValues.begin(),
                                                                            TestValues.end() ) );
INSTANTIATE_TEST_CASE_P( SortRange, SortFloatVector, ::testing::Range( 4096, 65536, 555 ) ); //2^12 to 2^16	
INSTANTIATE_TEST_CASE_P( SortValues, SortFloatVector, ::testing::ValuesIn( TestValues.begin(), 
                                                                        TestValues.end() ) );
#if (TEST_DOUBLE == 1)
INSTANTIATE_TEST_CASE_P( SortRange, SortDoubleVector, ::testing::Range(65536, 2097152, 55555 ) ); //2^16 to 2^21
INSTANTIATE_TEST_CASE_P( SortValues, SortDoubleVector, ::testing::ValuesIn( TestValues.begin(), 
                                                                            TestValues.end() ) );
#endif
INSTANTIATE_TEST_CASE_P( SortRange, SortIntegerDeviceVector, ::testing::Range( 1, 32768, 3276  ) ); // 1 to 2^15
INSTANTIATE_TEST_CASE_P( SortValues, SortIntegerDeviceVector, ::testing::ValuesIn( TestValues.begin(), 
                                                                                TestValues.end() ) );
INSTANTIATE_TEST_CASE_P( SortRange, SortUDDDeviceVector, ::testing::Range( 1, 32768, 3276  ) ); // 1 to 2^15
INSTANTIATE_TEST_CASE_P( SortValues, SortUDDDeviceVector, ::testing::ValuesIn( TestValues.begin(), 
                                                                                TestValues.end() ) );
INSTANTIATE_TEST_CASE_P( SortRange, SortFloatDeviceVector, ::testing::Range( 1, 32768, 3276  ) ); // 1 to 2^15
INSTANTIATE_TEST_CASE_P( SortValues, SortFloatDeviceVector, ::testing::ValuesIn( TestValues.begin(),
                                                                                TestValues.end()));
#if (TEST_DOUBLE == 1)
INSTANTIATE_TEST_CASE_P( SortRange, SortDoubleDeviceVector, ::testing::Range( 1, 32768, 3276  ) ); // 1 to 2^15
INSTANTIATE_TEST_CASE_P( SortValues, SortDoubleDeviceVector, ::testing::ValuesIn(TestValues.begin(),
                                                                                    TestValues.end()));
#endif
INSTANTIATE_TEST_CASE_P( SortRange, SortIntegerNakedPointer, ::testing::Range( 1, 32768, 3276  )); // 1 to 2^15
INSTANTIATE_TEST_CASE_P( SortValues, SortIntegerNakedPointer, ::testing::ValuesIn( TestValues.begin(),
                                                                                    TestValues.end()));
INSTANTIATE_TEST_CASE_P( SortRange, SortFloatNakedPointer, ::testing::Range( 1, 32768, 3276  ) ); // 1 to 2^15
INSTANTIATE_TEST_CASE_P( SortValues, SortFloatNakedPointer, ::testing::ValuesIn( TestValues.begin(), 
                                                                                TestValues.end() ) );
#if (TEST_DOUBLE == 1)
INSTANTIATE_TEST_CASE_P( SortRange, SortDoubleNakedPointer, ::testing::Range( 1, 32768, 3276  ) ); // 1 to 2^15
INSTANTIATE_TEST_CASE_P( Sort, SortDoubleNakedPointer, ::testing::ValuesIn( TestValues.begin(),
                                                                            TestValues.end() ) );
#endif

typedef ::testing::Types< 
    std::tuple< long, TypeValue< 1 > >,
    std::tuple< long, TypeValue< 31 > >,
    std::tuple< long, TypeValue< 32 > >,
    std::tuple< long, TypeValue< 63 > >,
    std::tuple< long, TypeValue< 64 > >,
    std::tuple< long, TypeValue< 127 > >,
    std::tuple< long, TypeValue< 128 > >,
    std::tuple< long, TypeValue< 129 > >,
    std::tuple< long, TypeValue< 1000 > >,
    std::tuple< long, TypeValue< 1053 > >,
    std::tuple< long, TypeValue< 4096 > >,
    std::tuple< long, TypeValue< 4097 > >,
    std::tuple< long, TypeValue< 8192 > >,
    std::tuple< long, TypeValue< 16384 > >,//13
    std::tuple< long, TypeValue< 32768 > >,//14
    std::tuple< long, TypeValue< 65535 > >,//15
    std::tuple< long, TypeValue< 65536 > >,//16
    std::tuple< long, TypeValue< 131072 > >,//17    
    std::tuple< long, TypeValue< 262144 > >,//18    
    std::tuple< long, TypeValue< 524288 > >,//19    
    std::tuple< long, TypeValue< 1048576 > >,//20    
    std::tuple< long, TypeValue< 2097152 > >//21    
#if (TEST_LARGE_BUFFERS == 1)
    , /*This coma is needed*/
    std::tuple< long, TypeValue< 4194304 > >,//22    
    std::tuple< long, TypeValue< 8388608 > >,//23
    std::tuple< long, TypeValue< 16777216 > >,//24
    std::tuple< long, TypeValue< 33554432 > >,//25
    std::tuple< long, TypeValue< 67108864 > >//26
#endif
> clLongTests;

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
    std::tuple< int, TypeValue< 8192 > >,
    std::tuple< int, TypeValue< 16384 > >,//13
    std::tuple< int, TypeValue< 32768 > >,//14
    std::tuple< int, TypeValue< 65535 > >,//15
    std::tuple< int, TypeValue< 65536 > >,//16
    std::tuple< int, TypeValue< 131072 > >,//17    
    std::tuple< int, TypeValue< 262144 > >,//18    
    std::tuple< int, TypeValue< 524288 > >,//19    
    std::tuple< int, TypeValue< 1048576 > >,//20    
    std::tuple< int, TypeValue< 2097152 > >//21    
#if (TEST_LARGE_BUFFERS == 1)
    , /*This coma is needed*/
    std::tuple< int, TypeValue< 4194304 > >,//22    
    std::tuple< int, TypeValue< 8388608 > >,//23
    std::tuple< int, TypeValue< 16777216 > >,//24
    std::tuple< int, TypeValue< 33554432 > >,//25
    std::tuple< int, TypeValue< 67108864 > >//26
#endif
> IntegerTests;

typedef ::testing::Types< 
    std::tuple< unsigned int, TypeValue< 1 > >,
    std::tuple< unsigned int, TypeValue< 31 > >,
    std::tuple< unsigned int, TypeValue< 32 > >,
    std::tuple< unsigned int, TypeValue< 63 > >,
    std::tuple< unsigned int, TypeValue< 64 > >,
    std::tuple< unsigned int, TypeValue< 127 > >,
    std::tuple< unsigned int, TypeValue< 128 > >,
    std::tuple< unsigned int, TypeValue< 129 > >,
    std::tuple< unsigned int, TypeValue< 1000 > >,
    std::tuple< unsigned int, TypeValue< 1053 > >,
    std::tuple< unsigned int, TypeValue< 4096 > >,
    std::tuple< unsigned int, TypeValue< 4097 > >,
    std::tuple< unsigned int, TypeValue< 8192 > >,
    std::tuple< unsigned int, TypeValue< 16384 > >,//13
    std::tuple< unsigned int, TypeValue< 32768 > >,//14
    std::tuple< unsigned int, TypeValue< 65535 > >,//15
    std::tuple< unsigned int, TypeValue< 65536 > >,//16
    std::tuple< unsigned int, TypeValue< 131072 > >,//17    
    std::tuple< unsigned int, TypeValue< 262144 > >,//18    
    std::tuple< unsigned int, TypeValue< 524288 > >,//19    
    std::tuple< unsigned int, TypeValue< 1048576 > >,//20    
    std::tuple< unsigned int, TypeValue< 2097152 > >//21    
#if (TEST_LARGE_BUFFERS == 1)
    , /*This coma is needed*/
    std::tuple< unsigned int, TypeValue< 4194304 > >,//22    
    std::tuple< unsigned int, TypeValue< 8388608 > >,//23
    std::tuple< unsigned int, TypeValue< 16777216 > >,//24
    std::tuple< unsigned int, TypeValue< 33554432 > >,//25
    std::tuple< unsigned int, TypeValue< 67108864 > >//26
#endif

> UnsignedIntegerTests;

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



template< typename ArrayTuple >
class SortUDDArrayTest: public ::testing::Test
{
public:
    SortUDDArrayTest( ): m_Errors( 0 )
    {}

    virtual void SetUp( )
    {
        std::generate(stdInput.begin(), stdInput.end(), rand);
        boltInput = stdInput;
    };

    virtual void TearDown( )
    {};

    virtual ~SortUDDArrayTest( )
    {}

protected:
    typedef typename std::tuple_element< 0, ArrayTuple >::type ArrayType;
    static const size_t ArraySize = typename std::tuple_element< 1, ArrayTuple >::type::value;
    typename std::array< ArrayType, ArraySize > stdInput, boltInput;
    int m_Errors;
};

TYPED_TEST_CASE_P( SortUDDArrayTest );

TYPED_TEST_P( SortUDDArrayTest, Normal )
{
    typedef std::array< ArrayType, ArraySize > ArrayCont;
    //  Calling the actual functions under test

    std::SORT_FUNC( stdInput.begin( ), stdInput.end( ), UDD() );
    bolt::BKND::SORT_FUNC( boltInput.begin( ), boltInput.end( ), UDD() );

    ArrayCont::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end() );
    ArrayCont::difference_type boltNumElements = std::distance( boltInput.begin( ), boltInput.end() );
    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );
    //  Loop through the array and compare all the values with each other
    cmpStdArray< ArrayType, ArraySize >::cmpArrays( stdInput, boltInput );

    std::SORT_FUNC( stdInput.begin( ), stdInput.end( ), sortBy_UDD_a() );
    bolt::BKND::SORT_FUNC( boltInput.begin( ), boltInput.end( ), sortBy_UDD_a() );

    stdNumElements = std::distance( stdInput.begin( ), stdInput.end() );
    boltNumElements = std::distance( boltInput.begin( ), boltInput.end() );
    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );
    //  Loop through the array and compare all the values with each other
    cmpStdArray< ArrayType, ArraySize >::cmpArrays( stdInput, boltInput );

}

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
    std::tuple< UDD, TypeValue< 4097 > >
    //std::tuple< UDD, TypeValue< 65535 > >, //Strangely the Visual Studio compiler fails to build these test cases
    //std::tuple< UDD, TypeValue< 65536 > >  //They result in a compiler stack overflow. 
> UDDTests;

typedef ::testing::Types< 
    std::tuple< unsigned short, TypeValue< 1 > >,
    std::tuple< unsigned short, TypeValue< 31 > >,
    std::tuple< unsigned short, TypeValue< 32 > >,
    std::tuple< unsigned short, TypeValue< 63 > >,
    std::tuple< unsigned short, TypeValue< 64 > >,
    std::tuple< unsigned short, TypeValue< 127 > >,
    std::tuple< unsigned short, TypeValue< 128 > >,
    std::tuple< unsigned short, TypeValue< 129 > >,
    std::tuple< unsigned short, TypeValue< 1000 > >,
    std::tuple< unsigned short, TypeValue< 1053 > >,
    std::tuple< unsigned short, TypeValue< 4096 > >,
    std::tuple< unsigned short, TypeValue< 4097 > >,
    std::tuple< unsigned short, TypeValue< 65535 > >,
    std::tuple< unsigned short, TypeValue< 65536 > >
> cl_ushortTests;

typedef ::testing::Types< 
    std::tuple< short, TypeValue< 1 > >,
    std::tuple< short, TypeValue< 31 > >,
    std::tuple< short, TypeValue< 32 > >,
    std::tuple< short, TypeValue< 63 > >,
    std::tuple< short, TypeValue< 64 > >,
    std::tuple< short, TypeValue< 127 > >,
    std::tuple< short, TypeValue< 128 > >,
    std::tuple< short, TypeValue< 129 > >,
    std::tuple< short, TypeValue< 1000 > >,
    std::tuple< short, TypeValue< 1053 > >,
    std::tuple< short, TypeValue< 4096 > >,
    std::tuple< short, TypeValue< 4097 > >,
    std::tuple< short, TypeValue< 65535 > >,
    std::tuple< short, TypeValue< 65536 > >
> cl_shortTests;


//INSTANTIATE_TYPED_TEST_CASE_P( ushort, SortArrayTest, cl_ushortTests );
//INSTANTIATE_TYPED_TEST_CASE_P( short, SortArrayTest, cl_shortTests );
INSTANTIATE_TYPED_TEST_CASE_P( Integer, SortArrayTest, IntegerTests );
INSTANTIATE_TYPED_TEST_CASE_P( UnsignedInteger, SortArrayTest, UnsignedIntegerTests );
INSTANTIATE_TYPED_TEST_CASE_P( Float, SortArrayTest, FloatTests );
INSTANTIATE_TYPED_TEST_CASE_P( clLong, SortArrayTest, clLongTests );
#if (TEST_DOUBLE == 1)
INSTANTIATE_TYPED_TEST_CASE_P( Double, SortArrayTest, DoubleTests );
#endif 
REGISTER_TYPED_TEST_CASE_P( SortUDDArrayTest,  Normal);
INSTANTIATE_TYPED_TEST_CASE_P( UDDTest, SortUDDArrayTest, UDDTests );

#if (TEST_DOUBLE ==1)
TEST (sanity_sort__withBoltClDevVectDouble_epr, floatSerial){
	int sizeOfInputBufer = 64; //test case is failing for all values greater than 32
	std::vector<double>  stdVect(0);
	bolt::BKND::device_vector<double>  boltVect(0);

	for (int i = 0 ; i < sizeOfInputBufer; i++){
	    double dValue = rand();
        dValue = dValue/rand();
        dValue = dValue*rand();
	    stdVect.push_back(dValue);
	    boltVect.push_back(dValue);
	}
	std::SORT_FUNC(stdVect.begin(), stdVect.end(), std::greater<double>( ) );
	bolt::BKND::SORT_FUNC(boltVect.begin(), boltVect.end(), bolt::BKND::greater<double>( ) );

	for (int i = 0 ; i < sizeOfInputBufer; i++){
	    EXPECT_DOUBLE_EQ(stdVect[i], boltVect[i]);
	}

}
#endif
TEST (rawArrayTest, floatarray){
	const int sizeOfInputBufer = 8192; //test case is failing for all values greater than 32
	float  stdArray[sizeOfInputBufer];
    float  boltArray[sizeOfInputBufer];
    float  backupArray[sizeOfInputBufer];

	for (int i = 0 ; i < sizeOfInputBufer; i++){
	    float fValue = (float)rand();
        fValue = fValue/rand();
        fValue = fValue*rand()*rand();
        stdArray[i] = boltArray[i] = fValue;
	}
	std::SORT_FUNC( stdArray, stdArray+sizeOfInputBufer, std::greater<float>( ) );
	bolt::BKND::SORT_FUNC( boltArray, boltArray+sizeOfInputBufer, bolt::BKND::greater<float>( ) );

	for (int i = 0 ; i < sizeOfInputBufer; i++)
    {
	    EXPECT_FLOAT_EQ(stdArray[i], boltArray[i]);
	}

    //Offset tests 
	for (int i = 0 ; i < sizeOfInputBufer; i++)
    {
	   stdArray[i] = boltArray[i] = backupArray[i];
	}

	std::SORT_FUNC( stdArray+17, stdArray+sizeOfInputBufer-129, std::greater<float>( ) );
	bolt::BKND::SORT_FUNC( boltArray, boltArray+sizeOfInputBufer, bolt::BKND::greater<float>( ) );

	for (int i = 0 ; i < sizeOfInputBufer; i++)
    {
	    EXPECT_FLOAT_EQ(stdArray[i], boltArray[i]);
	}

}


class sort_withStdVectFloat_2: public ::testing::TestWithParam<int>{
protected:
	int sizeOfInputBuffer;
public:
	sort_withStdVectFloat_2():sizeOfInputBuffer(GetParam()){
	}
};
TEST_P(sort_withStdVectFloat_2, floatSerial_EPR){
		
	std::vector <float> stdVect(sizeOfInputBuffer);
	std::vector <float> boltVect(sizeOfInputBuffer);

	for (int i = 0 ; i < sizeOfInputBuffer; i++){
		boltVect[i] = stdVect[i] = (float)i + 1.0625f;
		
	}
	std::SORT_FUNC(stdVect.begin(), stdVect.end(), std::greater<float>());

	bolt::BKND::SORT_FUNC(boltVect.begin(), boltVect.end(), bolt::BKND::greater<float>());

	for (int i = 0 ; i < sizeOfInputBuffer; ++i){
		
		EXPECT_FLOAT_EQ(stdVect[i], boltVect[i]);
	}
}

INSTANTIATE_TEST_CASE_P(sortDescending, sort_withStdVectFloat_2, ::testing::Range( 1, 1129, 7));  //Passing for each iteration
//test code ends

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

#include "bolt/cl/iterator/counting_iterator.h"
#include <bolt/cl/sort.h>
#include <bolt/cl/functional.h>
#undef NOMINMAX
#include <array>
#include <algorithm>
#include <random>

int random_gen()
{
    return -rand();
}

int main ()
{
    const int ArraySize = 8192;
    typedef std::array< int, ArraySize > ArrayCont;
    ArrayCont stdOffsetIn,stdInput;
    ArrayCont boltOffsetIn,boltInput;
    int minimum = -31000;
    int maximum = 31000;
    std::default_random_engine rnd;
    std::generate(stdInput.begin(), stdInput.end(),  random_gen);
    boltInput = stdInput;
    boltOffsetIn = stdInput;
    stdOffsetIn = stdInput;

    //  Calling the actual functions under test
    std::SORT_FUNC( stdInput.begin( ), stdInput.end( ) );
    bolt::BKND::SORT_FUNC( boltInput.begin( ), boltInput.end( ) );
    for(int i=0;i< ArraySize;i++)
    {
            std::cout << stdInput[i]  << "\n";
    }
    //  Loop through the array and compare all the values with each other
    for(int i=0;i< ArraySize;i++)
    {
        if(stdInput[i] = boltInput[i])
            continue;
        else 
            std::cout << "Failed at i " << i << " -- stdInput[i] " << stdInput[i] << " boltInput[i] = " << boltInput[i] << "\n";
    }
    //  OFFSET Calling the actual functions under test
    size_t startIndex = 17; //Some aribitrary offset position
    size_t endIndex   = ArraySize -17; //Some aribitrary offset position
    if( (( startIndex > ArraySize ) || ( endIndex < 0 ) )  || (startIndex > endIndex) )
    {
        std::cout <<"\nSkipping NormalOffset Test for size "<< ArraySize << "\n";
    }    
    else
    {
        std::SORT_FUNC( stdOffsetIn.begin( ) + startIndex, stdOffsetIn.begin( ) + endIndex );
        bolt::BKND::SORT_FUNC( boltOffsetIn.begin( ) + startIndex, boltOffsetIn.begin( ) + endIndex );

        //  Loop through the array and compare all the values with each other
        for(int i=0;i< ArraySize;i++)
        {
            if(stdOffsetIn[i] = boltOffsetIn[i])
                continue;
            else 
                std::cout << "Failed at i " << i << " -- stdInput[i] " << stdInput[i] << " boltInput[i] = " << boltInput[i] << "\n";
        }
    }


}
#endif