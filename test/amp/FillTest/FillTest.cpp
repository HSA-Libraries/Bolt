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

#include "stdafx.h"
#include "bolt/amp/fill.h"
#include "bolt/amp/functional.h"
#include "bolt/amp/device_vector.h"

#define STRUCT 1
#define FILL_GOOGLE_TEST 1

#define TEST_LARGE_BUFFERS 0

#define TEST_DOUBLE 1
#define TEST_CPU_DEVICE 1
#ifndef WIN32
#define _FPCLASS_ND  0x0010 // Linux negative denormal Value
#endif

#if FILL_GOOGLE_TEST

#include <gtest/gtest.h>
#include <type_traits>

#include "common/stdafx.h"
#include "common/test_common.h"
#include "bolt/miniDump.h"

#include <array>
#include <algorithm>



//////////////////////////////////////////////////////////////////////////////////////////////
//These test cases are used to ensure that fill works with structs
//////////////////////////////////////////////////////////////////////////////////////////////

#if STRUCT
int teststruct( int length );
int teststruct_n( int length );
struct samp
{
    int x,y;

};


#endif



#if STRUCT
/*
*
*Fill and Fill_N Host/Device Vector Tests
*
*/

 // Fill 
int teststruct( int length )
{
    int errCnt=0;
    static const int maxErrCnt = 10;
    // function name for reporting
    std::string fName = __FUNCTION__;
    // containers
    std::vector<samp> gold(length);
    std::vector<samp> hv(length);
    //bolt::amp::device_vector<samp>dv(length);
    struct samp s1,temp; /*temp is used to map back the device_vector to host_vector*/
    s1.x=10;
    s1.y=20;
    std::fill(gold.begin(), gold.end(),s1);
    bolt::amp::fill(hv.begin(), hv.end(),s1);
    //bolt::amp::fill(dv.begin(), dv.end(),s1);
	bolt::amp::device_vector<samp> dv(hv.begin(), hv.end());

    //check results

    for (int i=0; i<length ; i++) {
        temp = dv[i];
        if((gold[i].x!=hv[i].x)&&(gold[i].y!=hv[i].y)&&(gold[i].x!=temp.x)&&(gold[i].y!=temp.y)){
            errCnt++;
            if (errCnt < maxErrCnt) 
            {
                std::cout<<"\tMISMATCH"<<std::endl;
            }
            else if (errCnt == maxErrCnt) 
            {
                std::cout << "\tMax error count reached; no more mismatches will be printed...\n";
            }
        }
    }
    if ( errCnt == 0 ) {
        printf(" PASSED %20s Correct for all %6i elements.\n", fName.c_str(), length);
    } else {
        printf("*FAILED %20s Mismatch for %6i elements.\n", fName.c_str(), length);
    };
    fflush(stdout);
    return errCnt;
}

//// Fill_n 
int teststruct_n( int length )
{
    int errCnt=0;
    static const int maxErrCnt = 10;
    // function name for reporting
    std::string fName = __FUNCTION__;
    // containers
    std::vector<samp> gold(length);
    std::vector<samp> hv(length);
    //bolt::amp::device_vector<samp>dv(length);
    struct samp s1,temp; /*temp is used to map back the device_vector to host_vector*/
    s1.x=10;
    s1.y=20;
    std::fill_n(gold.begin(), length,s1);
    bolt::amp::fill_n(hv.begin(), length,s1);
    //bolt::amp::fill_n(dv.begin(), length,s1);
	bolt::amp::device_vector<samp> dv(hv.begin(), hv.end());

    //check results

    for (int i=0; i<length ; i++) {
        temp = dv[i];
        if((gold[i].x!=hv[i].x)&&(gold[i].y!=hv[i].y)&&(gold[i].x!=temp.x)&&(gold[i].y!=temp.y)){
            errCnt++;
            if (errCnt < maxErrCnt) 
            {
                std::cout<<"\tMISMATCH"<<std::endl;
            }
            else if (errCnt == maxErrCnt) 
            {
                std::cout << "\tMax error count reached; no more mismatches will be printed...\n";
            }
        }
    }
    
    if ( errCnt == 0 ) {
        printf(" PASSED %20s Correct for all %6i elements.\n", fName.c_str(), length);
    } else {
        printf("*FAILED %20s Mismatch for %6i elements.\n", fName.c_str(), length);
    };
    fflush(stdout);

    return errCnt;
}


#endif

struct UDD
{
    int a;
    int b;
  
    bool operator == (const UDD& other) const {
        return ((a==other.a) && (b == other.b));
    }
    
    UDD()
        : a(0), b(0) { }
    UDD(int _in)
        : a(_in), b(_in+2){ }
};


class HostIntVector: public ::testing::TestWithParam< int >
{
    
public:
    //  Create an std and a bolt vector of requested size, and initialize all the elements to -1
    HostIntVector( ): stdInput( GetParam( ), -1 ), boltInput( GetParam( ), -1 )  {}

protected:
    std::vector< int > stdInput, boltInput;
};

class HostUnsignedIntVector: public ::testing::TestWithParam< int >
{
    
public:
    //  Create an std and a bolt vector of requested size, and initialize all the elements to 1
    HostUnsignedIntVector( ): stdInput( GetParam( ), 1 ), boltInput( GetParam( ), 1 )  {}

protected:
    std::vector< unsigned int > stdInput, boltInput;
};

class HostFloatVector: public ::testing::TestWithParam< int >
{
    
public:
    //  Create an std and a bolt vector of requested size, and initialize all the elements to -1
    HostFloatVector( ): stdInput( GetParam( ), -1.0 ), boltInput( GetParam( ), -1.0 )  {}

protected:
    std::vector< float > stdInput, boltInput;
};

class HostUDDVector: public ::testing::TestWithParam< int >
{
    
public:
    //  Create an std and a bolt vector of requested size, and initialize all the elements to -1
    HostUDDVector( ): stdInput( GetParam( ), -1), boltInput( GetParam( ), -1)  {}

protected:
    std::vector< UDD > stdInput, boltInput;
};


class DevUDDVector: public ::testing::TestWithParam< int >
{
    
public:
    //  Create an std and a bolt vector of requested size, and initialize all the elements to 1
    DevUDDVector( ): stdInput( GetParam( ), 1 ), boltInput( static_cast<size_t>(GetParam( )), 1 )  {}

protected:
    std::vector< UDD > stdInput;
    bolt::amp::device_vector< UDD > boltInput;
};


class DevIntVector: public ::testing::TestWithParam< int >
{
public:
    //  Create a std and a bolt vector of requested size, and initialize all the elements to 1
    DevIntVector( ): stdInput( GetParam( ), 1 ), boltInput( static_cast<size_t>(GetParam( )), 1  )
    {}

protected:
    std::vector< int > stdInput;
    bolt::amp::device_vector< int > boltInput;
};

class DevUnsignedIntVector: public ::testing::TestWithParam< int >
{
public:
    //  Create an std and a bolt vector of requested size, and initialize all the elements to -1
    DevUnsignedIntVector( ): stdInput( GetParam( ), 1 ), boltInput( static_cast<size_t>(GetParam( )), 1 )
    {}

protected:
    std::vector< unsigned int > stdInput;
    bolt::amp::device_vector< unsigned int > boltInput;
};

class DevFloatVector: public ::testing::TestWithParam< int >
{
public:
    //  Create an std and a bolt vector of requested size, and initialize all the elements to 1
    DevFloatVector( ): stdInput( GetParam( ), 1.0 ), boltInput( static_cast<size_t>(GetParam( )), 1.0 )
    {}

protected:
    std::vector< float > stdInput;
    bolt::amp::device_vector< float > boltInput;
};

#if (TEST_DOUBLE == 1)
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
    //  Create an std and a bolt vector of requested size, and initialize all the elements to 1
    DevDblVector( ): stdInput( GetParam( ), 1.0 ), boltInput( static_cast<size_t>(GetParam( )), 1.0 )
    {}

protected:
    std::vector< double > stdInput;
    bolt::amp::device_vector< double > boltInput;
};
#endif

//////////////////////////////////////////////////////////////////////////////////////////////
//Test Cases for Fill
//////////////////////////////////////////////////////////////////////////////////////////////

template< int N >
class TypeValue
{
public:
    static const int value = N;
};

//template< typename ArrayTuple>
//class FillArrayTest: public ::testing::Test
//{
//public:
//    FillArrayTest( ): m_Errors( 0 ), val(73)
//    { }
//
//    virtual void TearDown( )
//    {};
//
//    virtual ~FillArrayTest( )
//    {}
//
//protected:
//    typedef typename std::tuple_element< 0, ArrayTuple >::type ArrayType;
//    static const size_t ArraySize =  std::tuple_element< 1, ArrayTuple >::type::value;
//    std::array< ArrayType, ArraySize > stdInput, boltInput, stdOffsetIn, boltOffsetIn;
//    int m_Errors;
//    ArrayType val;
//};
//
//TYPED_TEST_CASE_P( FillArrayTest );
//
//
///* #if (TEST_CPU_DEVICE == 1)
//TYPED_TEST_P( FillArrayTest,CPU_DeviceNormal )
//{
//    
//    typedef typename FillArrayTest< gtest_TypeParam_ >::ArrayType ArrayType;    
//    typedef std::array< ArrayType, FillArrayTest< gtest_TypeParam_ >::ArraySize > ArrayCont;
//    
//    
//
//    MyOclContext oclcpu = initOcl(CL_DEVICE_TYPE_CPU, 0);
//    bolt::cl::control c_cpu(oclcpu._queue);  // construct control structure from the queue.
//
//    //  Calling the actual functions under test
//    std::fill( FillArrayTest< gtest_TypeParam_ >::stdInput.begin( ), FillArrayTest< gtest_TypeParam_ >::stdInput.end( ), FillArrayTest< gtest_TypeParam_ >::val );
//    bolt::amp::fill( c_cpu, FillArrayTest< gtest_TypeParam_ >::boltInput.begin( ), FillArrayTest< gtest_TypeParam_ >::boltInput.end( ) , FillArrayTest< gtest_TypeParam_ >::val);
//
//    typename ArrayCont::difference_type stdNumElements = std::distance( FillArrayTest< gtest_TypeParam_ >::stdInput.begin( ), FillArrayTest< gtest_TypeParam_ >::stdInput.end() );
//    typename ArrayCont::difference_type boltNumElements = std::distance( FillArrayTest< gtest_TypeParam_ >::boltInput.begin( ), FillArrayTest< gtest_TypeParam_ >::boltInput.end() );
//
//    //  Both collections should have the same number of elements
//    EXPECT_EQ( stdNumElements, boltNumElements );
//
//    //  Loop through the array and compare all the values with each other
//    //cmpStdArray< ArrayType,  FillArrayTest< gtest_TypeParam_ >::ArraySize >::cmpArrays( FillArrayTest< gtest_TypeParam_ >::stdInput, FillArrayTest< gtest_TypeParam_ >::boltInput );
//    
//    //OFFSET Test cases
//    //  Calling the actual functions under test
//    size_t startIndex = 17; //Some aribitrary offset position
//    size_t endIndex   = FillArrayTest< gtest_TypeParam_ >::ArraySize - 17; //Some aribitrary offset position
//    if( (( startIndex > FillArrayTest< gtest_TypeParam_ >::ArraySize ) || ( endIndex < 0 ) )  || (startIndex > endIndex) )
//    {
//        std::cout <<"\nSkipping NormalOffset Test for size "<< FillArrayTest< gtest_TypeParam_ >::ArraySize << "\n";
//    }    
//    else
//    {
//        std::fill( FillArrayTest< gtest_TypeParam_ >::stdOffsetIn.begin( ) , FillArrayTest< gtest_TypeParam_ >::stdOffsetIn.begin( ) + startIndex, FillArrayTest< gtest_TypeParam_ >::val + 2);
//        std::fill( FillArrayTest< gtest_TypeParam_ >::stdOffsetIn.end( ) - startIndex, FillArrayTest< gtest_TypeParam_ >::stdOffsetIn.end( ), FillArrayTest< gtest_TypeParam_ >::val + 2);
//
//        std::fill( FillArrayTest< gtest_TypeParam_ >::boltOffsetIn.begin( ) , FillArrayTest< gtest_TypeParam_ >::boltOffsetIn.begin( ) + startIndex, FillArrayTest< gtest_TypeParam_ >::val + 2);
//        std::fill( FillArrayTest< gtest_TypeParam_ >::boltOffsetIn.end( ) - startIndex, FillArrayTest< gtest_TypeParam_ >::boltOffsetIn.end( ), FillArrayTest< gtest_TypeParam_ >::val + 2);
//
//
//
//        std::fill( FillArrayTest< gtest_TypeParam_ >::stdOffsetIn.begin( ) + startIndex, FillArrayTest< gtest_TypeParam_ >::stdOffsetIn.begin( ) + endIndex, FillArrayTest< gtest_TypeParam_ >::val );
//        bolt::cl::fill( c_cpu, FillArrayTest< gtest_TypeParam_ >::boltOffsetIn.begin( ) + startIndex, FillArrayTest< gtest_TypeParam_ >::boltOffsetIn.begin( ) + endIndex, FillArrayTest< gtest_TypeParam_ >::val);
//
//        typename ArrayCont::difference_type stdNumElements = std::distance( FillArrayTest< gtest_TypeParam_ >::stdOffsetIn.begin( ), FillArrayTest< gtest_TypeParam_ >::stdOffsetIn.end( ) );
//        typename ArrayCont::difference_type boltNumElements = std::distance(  FillArrayTest< gtest_TypeParam_ >::boltOffsetIn.begin( ),  FillArrayTest< gtest_TypeParam_ >::boltOffsetIn.end( ) );
//
//        //  Both collections should have the same number of elements
//        EXPECT_EQ( stdNumElements, boltNumElements );
//
//        //  Loop through the array and compare all the values with each other
//        cmpStdArray< ArrayType,  FillArrayTest< gtest_TypeParam_ >::ArraySize >::cmpArrays(  FillArrayTest< gtest_TypeParam_ >::stdOffsetIn,  FillArrayTest< gtest_TypeParam_ >::boltOffsetIn );
//    }
//}
//
//REGISTER_TYPED_TEST_CASE_P( FillArrayTest, CPU_DeviceNormal); */
//
//#endif
//
////typedef ::testing::Types< 
////    std::tuple< cl_long, TypeValue< 1 > >,
////    std::tuple< cl_long, TypeValue< 31 > >,
////    std::tuple< cl_long, TypeValue< 32 > >,
////    std::tuple< cl_long, TypeValue< 63 > >,
////    std::tuple< cl_long, TypeValue< 64 > >,
////    std::tuple< cl_long, TypeValue< 127 > >,
////    std::tuple< cl_long, TypeValue< 128 > >,
////    std::tuple< cl_long, TypeValue< 129 > >,
////    std::tuple< cl_long, TypeValue< 1000 > >,
////    std::tuple< cl_long, TypeValue< 1053 > >,
////    std::tuple< cl_long, TypeValue< 4096 > >,
////    std::tuple< cl_long, TypeValue< 4097 > >,
////    std::tuple< cl_long, TypeValue< 8192 > >,
////    std::tuple< cl_long, TypeValue< 16384 > >,//13
////    std::tuple< cl_long, TypeValue< 32768 > >,//14
////    std::tuple< cl_long, TypeValue< 65535 > >,//15
////    std::tuple< cl_long, TypeValue< 65536 > >,//16
////    std::tuple< cl_long, TypeValue< 131072 > >,//17    
////    std::tuple< cl_long, TypeValue< 262144 > >,//18    
////    std::tuple< cl_long, TypeValue< 524288 > >,//19    
////    std::tuple< cl_long, TypeValue< 1048576 > >,//20    
////    std::tuple< cl_long, TypeValue< 2097152 > >//21    
////#if (TEST_LARGE_BUFFERS == 1)
////    , /*This coma is needed*/
////    std::tuple< cl_long, TypeValue< 4194304 > >,//22    
////    std::tuple< cl_long, TypeValue< 8388608 > >,//23
////    std::tuple< cl_long, TypeValue< 16777216 > >,//24
////    std::tuple< cl_long, TypeValue< 33554432 > >,//25
////    std::tuple< cl_long, TypeValue< 67108864 > >//26
////#endif
////> clLongTests; 
//
//typedef ::testing::Types< 
//    std::tuple< int, TypeValue< 1 > >,
//    std::tuple< int, TypeValue< 31 > >,
//    std::tuple< int, TypeValue< 32 > >,
//    std::tuple< int, TypeValue< 63 > >,
//    std::tuple< int, TypeValue< 64 > >,
//    std::tuple< int, TypeValue< 127 > >,
//    std::tuple< int, TypeValue< 128 > >,
//    std::tuple< int, TypeValue< 129 > >,
//    std::tuple< int, TypeValue< 1000 > >,
//    std::tuple< int, TypeValue< 1053 > >,
//    std::tuple< int, TypeValue< 4096 > >,
//    std::tuple< int, TypeValue< 4097 > >,
//    std::tuple< int, TypeValue< 8192 > >,
//    std::tuple< int, TypeValue< 16384 > >,//13
//    std::tuple< int, TypeValue< 32768 > >,//14
//    std::tuple< int, TypeValue< 65535 > >,//15
//    std::tuple< int, TypeValue< 65536 > >,//16
//    std::tuple< int, TypeValue< 131072 > >,//17    
//    std::tuple< int, TypeValue< 262144 > >,//18    
//    std::tuple< int, TypeValue< 524288 > >,//19    
//    std::tuple< int, TypeValue< 1048576 > >,//20    
//    std::tuple< int, TypeValue< 2097152 > >//21    
//#if (TEST_LARGE_BUFFERS == 1)
//    , /*This coma is needed*/
//    std::tuple< int, TypeValue< 4194304 > >,//22    
//    std::tuple< int, TypeValue< 8388608 > >,//23
//    std::tuple< int, TypeValue< 16777216 > >,//24
//    std::tuple< int, TypeValue< 33554432 > >,//25
//    std::tuple< int, TypeValue< 67108864 > >//26
//#endif
//> IntegerTests;
//
//typedef ::testing::Types< 
//    std::tuple< unsigned int, TypeValue< 1 > >,
//    std::tuple< unsigned int, TypeValue< 31 > >,
//    std::tuple< unsigned int, TypeValue< 32 > >,
//    std::tuple< unsigned int, TypeValue< 63 > >,
//    std::tuple< unsigned int, TypeValue< 64 > >,
//    std::tuple< unsigned int, TypeValue< 127 > >,
//    std::tuple< unsigned int, TypeValue< 128 > >,
//    std::tuple< unsigned int, TypeValue< 129 > >,
//    std::tuple< unsigned int, TypeValue< 1000 > >,
//    std::tuple< unsigned int, TypeValue< 1053 > >,
//    std::tuple< unsigned int, TypeValue< 4096 > >,
//    std::tuple< unsigned int, TypeValue< 4097 > >,
//    std::tuple< unsigned int, TypeValue< 8192 > >,
//    std::tuple< unsigned int, TypeValue< 16384 > >,//13
//    std::tuple< unsigned int, TypeValue< 32768 > >,//14
//    std::tuple< unsigned int, TypeValue< 65535 > >,//15
//    std::tuple< unsigned int, TypeValue< 65536 > >,//16
//    std::tuple< unsigned int, TypeValue< 131072 > >,//17    
//    std::tuple< unsigned int, TypeValue< 262144 > >,//18    
//    std::tuple< unsigned int, TypeValue< 524288 > >,//19    
//    std::tuple< unsigned int, TypeValue< 1048576 > >,//20    
//    std::tuple< unsigned int, TypeValue< 2097152 > >//21    
//#if (TEST_LARGE_BUFFERS == 1)
//    , /*This coma is needed*/
//    std::tuple< unsigned int, TypeValue< 4194304 > >,//22    
//    std::tuple< unsigned int, TypeValue< 8388608 > >,//23
//    std::tuple< unsigned int, TypeValue< 16777216 > >,//24
//    std::tuple< unsigned int, TypeValue< 33554432 > >,//25
//    std::tuple< unsigned int, TypeValue< 67108864 > >//26
//#endif
//
//> UnsignedIntegerTests;
//
//typedef ::testing::Types< 
//    std::tuple< float, TypeValue< 1 > >,
//    std::tuple< float, TypeValue< 31 > >,
//    std::tuple< float, TypeValue< 32 > >,
//    std::tuple< float, TypeValue< 63 > >,
//    std::tuple< float, TypeValue< 64 > >,
//    std::tuple< float, TypeValue< 127 > >,
//    std::tuple< float, TypeValue< 128 > >,
//    std::tuple< float, TypeValue< 129 > >,
//    std::tuple< float, TypeValue< 1000 > >,
//    std::tuple< float, TypeValue< 1053 > >,
//    std::tuple< float, TypeValue< 4096 > >,
//    std::tuple< float, TypeValue< 4097 > >,
//    std::tuple< float, TypeValue< 65535 > >,
//    std::tuple< float, TypeValue< 65536 > >
//> FloatTests;
//
//#if (TEST_DOUBLE == 1)
//typedef ::testing::Types< 
//    std::tuple< double, TypeValue< 1 > >,
//    std::tuple< double, TypeValue< 31 > >,
//    std::tuple< double, TypeValue< 32 > >,
//    std::tuple< double, TypeValue< 63 > >,
//    std::tuple< double, TypeValue< 64 > >,
//    std::tuple< double, TypeValue< 127 > >,
//    std::tuple< double, TypeValue< 128 > >,
//    std::tuple< double, TypeValue< 129 > >,
//    std::tuple< double, TypeValue< 1000 > >,
//    std::tuple< double, TypeValue< 1053 > >,
//    std::tuple< double, TypeValue< 4096 > >,
//    std::tuple< double, TypeValue< 4097 > >,
//    std::tuple< double, TypeValue< 65535 > >,
//    std::tuple< double, TypeValue< 65536 > >
//> DoubleTests;
//#endif 
//
//#if (TEST_CPU_DEVICE == 1)
////INSTANTIATE_TYPED_TEST_CASE_P( clLong, FillArrayTest, clLongTests );
//INSTANTIATE_TYPED_TEST_CASE_P( Integer, FillArrayTest, IntegerTests );
//INSTANTIATE_TYPED_TEST_CASE_P( UnsignedInteger, FillArrayTest, UnsignedIntegerTests );
//INSTANTIATE_TYPED_TEST_CASE_P( Float, FillArrayTest, FloatTests );
//#endif
//#if (TEST_DOUBLE == 1)
//INSTANTIATE_TYPED_TEST_CASE_P( Double, FillArrayTest, DoubleTests );
//#endif 


TEST( StdIntVector, OffsetFill )
{
    int length = 1024;

    std::vector<int> stdInput( length );
    for (int i = 0; i < 1024; ++i)
    {
        stdInput[i] = 1;
    }

    std::vector<int> boltInput( stdInput.begin(),stdInput.end() );
    int val = 73, offset = 100;


    std::fill(  stdInput.begin( ) + offset,  stdInput.end( ), val );
    bolt::amp::fill( boltInput.begin( ) + offset, boltInput.end( ), val );

    cmpArrays( stdInput, boltInput );
}

TEST( StdIntVector, SerialOffsetFill )
{
    int length = 1024;

    std::vector<int> stdInput( length );
    for (int i = 0; i < 1024; ++i)
    {
        stdInput[i] = 1;
    }

    std::vector<int> boltInput( stdInput.begin(),stdInput.end() );
    int val = 73, offset = 100;

    bolt::amp::control ctl = bolt::amp::control::getDefault( );
    ctl.setForceRunMode(bolt::amp::control::SerialCpu);

    std::fill(  stdInput.begin( ) + offset,  stdInput.end( ), val );
    bolt::amp::fill(ctl, boltInput.begin( ) + offset, boltInput.end( ), val );

    cmpArrays( stdInput, boltInput );
}

TEST( StdIntVector, MultiCoreOffsetFill )
{
    int length = 1024;

    std::vector<int> stdInput( length );
    for (int i = 0; i < 1024; ++i)
    {
        stdInput[i] = 1;
    }

    std::vector<int> boltInput( stdInput.begin(),stdInput.end() );
    int val = 73, offset = 100;

    bolt::amp::control ctl = bolt::amp::control::getDefault( );
    ctl.setForceRunMode(bolt::amp::control::MultiCoreCpu);

    std::fill(  stdInput.begin( ) + offset,  stdInput.end( ), val );
    bolt::amp::fill(ctl, boltInput.begin( ) + offset, boltInput.end( ), val );

    cmpArrays( stdInput, boltInput );
}

TEST( DVIntVector, OffsetFill )
{
	int length = 1024;

    std::vector<int> stdInput( length ,1);
    bolt::amp::device_vector<int> boltInput(stdInput.begin(),stdInput.end());

	int val = 52, offset = 10;

    std::fill(  stdInput.begin( ) + offset,  stdInput.end( ), val );
    bolt::amp::fill( boltInput.begin( ) + offset, boltInput.end( ), val );

    cmpArrays( stdInput, boltInput);
	
}
 
TEST( DVIntVector, SerialOffsetFill )
{
   
	int length = 1024;

    std::vector<int> stdInput( length ,1);
    bolt::amp::device_vector<int> boltInput(stdInput.begin(),stdInput.end());

	int val = 52, offset = 10;

	bolt::amp::control ctl = bolt::amp::control::getDefault( );
    ctl.setForceRunMode(bolt::amp::control::SerialCpu);

    std::fill(  stdInput.begin( ) + offset,  stdInput.end( ), val );
    bolt::amp::fill( ctl, boltInput.begin( ) + offset, boltInput.end( ), val );

    cmpArrays( stdInput, boltInput);
	
}

TEST( DVIntVector, MultiCoreOffsetFill )
{
   
	int length = 1024;

    std::vector<int> stdInput( length ,1);
    bolt::amp::device_vector<int> boltInput(stdInput.begin(),stdInput.end());

	int val = 52, offset = 10;

	bolt::amp::control ctl = bolt::amp::control::getDefault( );
    ctl.setForceRunMode(bolt::amp::control::MultiCoreCpu);

    std::fill(  stdInput.begin( ) + offset,  stdInput.end( ), val );
    bolt::amp::fill( ctl, boltInput.begin( ) + offset, boltInput.end( ), val );

    cmpArrays( stdInput, boltInput);
	
}

TEST ( StdIntVectorWithSplit, OffsetFill )
{ 
    int length = 1000;
    int splitSize = 250;
    std::vector<int> v(length);
    int val = 3;

    //Set all the elements to the expected result
    for(int i=0;i<length/splitSize;i++)
        for(int count=0; count < splitSize; ++count) {
                     if(i%2 == 0 )
                         v[i*splitSize + count] = val;
                     else
                         v[i*splitSize + count] = val * 3;
        }
    
    std::vector<int> stdv(length);

    //memcpy(stdv.data(), v.data(), splitSize * sizeof(int)); // Copy 250 elements to device vector
    bolt::amp::fill_n(stdv.begin(), splitSize, val);
    bolt::amp::fill(stdv.begin() + splitSize, stdv.begin() + (splitSize * 2), val * 3); // Fill 2nd set of 250 elements
    bolt::amp::fill(stdv.begin() + (splitSize * 2), stdv.begin() + (splitSize * 3), val);// Fill 3rd set of 250 elements

    bolt::amp::fill(stdv.begin() + (splitSize * 3), stdv.end(), val * 3);  // Fill 4th set of 250 elements
    
    for (int i = 0; i < length; ++i){ 
        EXPECT_EQ(stdv[i], v[i]);
    }
} 

TEST (dvIntWithSplit, OffsetFill){ 
    int length = 1000;
    int splitSize = 250;
    int val = 3;

	std::vector<int> stdIn(length);
    //bolt::amp::device_vector<int> dvIn(length);

    //Set all the elements
    for(int i=0;i<length/splitSize;i++)
        for(int count=0; count < splitSize; ++count) {
                     if(i%2 == 0 )
                         stdIn[i*splitSize + count] = val;
                     else
                         stdIn[i*splitSize + count] = val * 3;
        }
    
    bolt::amp::device_vector<int> dvIn(stdIn.begin(), stdIn.end());

	std::vector<int> stdOut(length);
    //bolt::amp::device_vector<int> dvOut(length);
	bolt::amp::device_vector<int> dvOut(stdOut.begin(), stdOut.end());
 //   {
 //       //Using Boost Smart Pointer
 //       bolt::amp::device_vector<int>::pointer dpOut=dvOut.data();
 //       bolt::amp::device_vector<int>::pointer dpIn=dvIn.data();
 //       //memcpy(dpOut.get(), dpIn.get(), splitSize * sizeof(int)); // Copy 250 elements to device vector
 //       bolt::amp::fill_n(dpOut.get(), splitSize, val);
 //   
 //      //bolt::cl::fill(dvOut.begin() + splitSize, dvOut.begin() + (splitSize*2),val*3); //Fill 2nd set of 250 elements
 //      bolt::amp::fill(dpOut.get() + splitSize, dpOut.get() + (splitSize * 2), val * 3);
 //      //bolt::cl::fill(dvOut.begin()+(splitSize * 2),dvOut.begin()+(splitSize*3),val); //Fill 3rd set of 250 elements

 //      bolt::amp::fill(dpOut.get() + (splitSize * 2), dpOut.get() + (splitSize * 3), val);

 //   }

	bolt::amp::fill_n(dvOut.begin(), splitSize, val);
	bolt::amp::fill(dvOut.begin() + splitSize, dvOut.begin() + (splitSize * 2), val * 3);
	bolt::amp::fill(dvOut.begin() + (splitSize * 2), dvOut.begin() + (splitSize * 3), val);

    bolt::amp::fill(dvOut.begin() + (splitSize * 3), dvOut.end(), val * 3);  // Fill 4th set of 250 elements
    for (int i = 0; i < length; ++i){ 
        EXPECT_EQ(dvOut[i], dvIn[i]);
    }
} 


TEST_P( HostUDDVector, Fill )
{
    UDD val(73);
    std::fill(  stdInput.begin( ),  stdInput.end( ), val );
    bolt::amp::fill( boltInput.begin( ), boltInput.end( ), val );

    cmpArrays( stdInput, boltInput );
}

TEST_P( HostUDDVector, SerialFill )
{
    UDD val(73);
    bolt::amp::control ctl = bolt::amp::control::getDefault( );
    ctl.setForceRunMode(bolt::amp::control::SerialCpu);

    std::fill(  stdInput.begin( ),  stdInput.end( ), val );
    bolt::amp::fill( ctl, boltInput.begin( ), boltInput.end( ), val );

    cmpArrays( stdInput, boltInput );
}

TEST_P( HostUDDVector, MultiCoreFill )
{
    UDD val(73);
    bolt::amp::control ctl = bolt::amp::control::getDefault( );
    ctl.setForceRunMode(bolt::amp::control::MultiCoreCpu);

    std::fill(  stdInput.begin( ),  stdInput.end( ), val );
    bolt::amp::fill( ctl, boltInput.begin( ), boltInput.end( ), val );

    cmpArrays( stdInput, boltInput );
}

TEST_P( HostUnsignedIntVector, Fill )
{
    unsigned int val = 73;
    std::fill(  stdInput.begin( ),  stdInput.end( ), val );
    bolt::amp::fill( boltInput.begin( ), boltInput.end( ), val );

    cmpArrays( stdInput, boltInput );
}

TEST_P( HostUnsignedIntVector, SerialFill )
{
    unsigned int val = 73;

    bolt::amp::control ctl = bolt::amp::control::getDefault( );
    ctl.setForceRunMode(bolt::amp::control::SerialCpu);

    std::fill(  stdInput.begin( ),  stdInput.end( ), val );
    bolt::amp::fill( ctl, boltInput.begin( ), boltInput.end( ), val );

    cmpArrays( stdInput, boltInput );
}

TEST_P( HostUnsignedIntVector, MultiCoreFill )
{
    unsigned int val = 73;

    bolt::amp::control ctl = bolt::amp::control::getDefault( );
    ctl.setForceRunMode(bolt::amp::control::MultiCoreCpu);

    std::fill(  stdInput.begin( ),  stdInput.end( ), val );
    bolt::amp::fill( ctl, boltInput.begin( ), boltInput.end( ), val );

    cmpArrays( stdInput, boltInput );
}

TEST_P( HostIntVector, Fill )
{
    int val = 73;
    std::fill(  stdInput.begin( ),  stdInput.end( ), val );
    bolt::amp::fill( boltInput.begin( ), boltInput.end( ), val );

    cmpArrays( stdInput, boltInput );
}

TEST_P( HostIntVector, SerialFill )
{
    int val = 73;

    bolt::amp::control ctl = bolt::amp::control::getDefault( );
    ctl.setForceRunMode(bolt::amp::control::SerialCpu);

    std::fill(  stdInput.begin( ),  stdInput.end( ), val );
    bolt::amp::fill( ctl, boltInput.begin( ), boltInput.end( ), val );

    cmpArrays( stdInput, boltInput );
}

TEST_P( HostIntVector, MultiCoreFill )
{
    int val = 73;

    bolt::amp::control ctl = bolt::amp::control::getDefault( );
    ctl.setForceRunMode(bolt::amp::control::MultiCoreCpu);

    std::fill(  stdInput.begin( ),  stdInput.end( ), val );
    bolt::amp::fill( ctl, boltInput.begin( ), boltInput.end( ), val );

    cmpArrays( stdInput, boltInput );
}

TEST_P( HostFloatVector, Fill )
{
    float val = (float)73.6;

    std::fill(  stdInput.begin( ),  stdInput.end( ), val );
    bolt::amp::fill( boltInput.begin( ), boltInput.end( ), val );

    cmpArrays( stdInput, boltInput );
}

TEST_P( HostFloatVector, SerialFill )
{
    float val = (float)73.6;

    bolt::amp::control ctl = bolt::amp::control::getDefault( );
    ctl.setForceRunMode(bolt::amp::control::SerialCpu);

    std::fill(  stdInput.begin( ),  stdInput.end( ), val );
    bolt::amp::fill(ctl, boltInput.begin( ), boltInput.end( ), val );

    cmpArrays( stdInput, boltInput );
}

TEST_P( HostFloatVector, MultiCoreFill )
{

    float val = (float)73.6;


    bolt::amp::control ctl = bolt::amp::control::getDefault( );
    ctl.setForceRunMode(bolt::amp::control::MultiCoreCpu);

    std::fill(  stdInput.begin( ),  stdInput.end( ), val );
    bolt::amp::fill(ctl, boltInput.begin( ), boltInput.end( ), val );

    cmpArrays( stdInput, boltInput );
}


#if (TEST_DOUBLE == 1)
TEST_P( HostDblVector, Fill )
{
    double val = 78.85;
    std::fill(  stdInput.begin( ),  stdInput.end( ), val );
    bolt::amp::fill( boltInput.begin( ), boltInput.end( ), val );

    cmpArrays( stdInput, boltInput );
}


TEST_P( HostDblVector, CPUFill )
{
    double val = 78.85;
    
    bolt::amp::control ctl = bolt::amp::control::getDefault( );
    ctl.setForceRunMode(bolt::amp::control::SerialCpu);

    std::fill(  stdInput.begin( ),  stdInput.end( ), val );
    bolt::amp::fill( ctl, boltInput.begin( ), boltInput.end( ), val );

    cmpArrays( stdInput, boltInput );
}

TEST_P( HostDblVector, MultiCoreFill )
{
    double val = 78.85;

    bolt::amp::control ctl = bolt::amp::control::getDefault( );
    ctl.setForceRunMode(bolt::amp::control::MultiCoreCpu);

    std::fill(  stdInput.begin( ),  stdInput.end( ), val );
    bolt::amp::fill( ctl, boltInput.begin( ), boltInput.end( ), val );

    cmpArrays( stdInput, boltInput );
}
#endif

TEST_P( DevUDDVector, Fill )
{
    UDD val(73);
    std::fill(  stdInput.begin( ),  stdInput.end( ), val );
    bolt::amp::fill( boltInput.begin( ), boltInput.end( ), val );

    cmpArrays( stdInput, boltInput );
}

TEST_P( DevUDDVector, SerialFill )
{
    UDD val(73);
    bolt::amp::control ctl = bolt::amp::control::getDefault( );
    ctl.setForceRunMode(bolt::amp::control::SerialCpu);

    std::fill(  stdInput.begin( ),  stdInput.end( ), val );
    bolt::amp::fill( ctl, boltInput.begin( ), boltInput.end( ), val );

    cmpArrays( stdInput, boltInput );
}

TEST_P( DevUDDVector, MultiCoreFill )
{
    UDD val(73);
    bolt::amp::control ctl = bolt::amp::control::getDefault( );
    ctl.setForceRunMode(bolt::amp::control::MultiCoreCpu);

    std::fill(  stdInput.begin( ),  stdInput.end( ), val );
    bolt::amp::fill( ctl, boltInput.begin( ), boltInput.end( ), val );

    cmpArrays( stdInput, boltInput );
}

TEST_P( DevUnsignedIntVector, Fill )
{
    unsigned int val = 73;
    std::fill(  stdInput.begin( ),  stdInput.end( ), val );
    bolt::amp::fill( boltInput.begin( ), boltInput.end( ), val );

    cmpArrays( stdInput, boltInput );
}


TEST_P( DevUnsignedIntVector, SerialFill )
{
    unsigned int val = 73;

    bolt::amp::control ctl = bolt::amp::control::getDefault( );
    ctl.setForceRunMode(bolt::amp::control::SerialCpu);

    std::fill(  stdInput.begin( ),  stdInput.end( ), val );
    bolt::amp::fill( ctl, boltInput.begin( ), boltInput.end( ), val );

    cmpArrays( stdInput, boltInput );
}

TEST_P( DevUnsignedIntVector, MultiCoreFill )
{
    unsigned int val = 73;

    bolt::amp::control ctl = bolt::amp::control::getDefault( );
    ctl.setForceRunMode(bolt::amp::control::MultiCoreCpu);

    std::fill(  stdInput.begin( ),  stdInput.end( ), val );
    bolt::amp::fill( ctl, boltInput.begin( ), boltInput.end( ), val );

    cmpArrays( stdInput, boltInput );
}

TEST_P( DevIntVector, Fill )
{
    int val = 73;
    std::fill(  stdInput.begin( ),  stdInput.end( ), val );
    bolt::amp::fill( boltInput.begin( ), boltInput.end( ), val );

    cmpArrays( stdInput, boltInput );
}

TEST_P( DevIntVector, SerialFill )
{
    int val = 73;

    bolt::amp::control ctl = bolt::amp::control::getDefault( );
    ctl.setForceRunMode(bolt::amp::control::SerialCpu);

    std::fill(  stdInput.begin( ),  stdInput.end( ), val );
    bolt::amp::fill( ctl, boltInput.begin( ), boltInput.end( ), val );

    cmpArrays( stdInput, boltInput );
}

TEST_P( DevIntVector, MultiCoreFill )
{
    int val = 73;

    bolt::amp::control ctl = bolt::amp::control::getDefault( );
    ctl.setForceRunMode(bolt::amp::control::MultiCoreCpu);

    std::fill(  stdInput.begin( ),  stdInput.end( ), val );
    bolt::amp::fill( ctl, boltInput.begin( ), boltInput.end( ), val );

    cmpArrays( stdInput, boltInput );
}


TEST_P( DevFloatVector, Fill )
{
    float val = (float)73.7;

    std::fill(  stdInput.begin( ),  stdInput.end( ), val );
    bolt::amp::fill( boltInput.begin( ), boltInput.end( ), val );

    cmpArrays( stdInput, boltInput );
}

TEST_P( DevFloatVector, SerialFill )
{

    float val = (float)73.7;


    bolt::amp::control ctl = bolt::amp::control::getDefault( );
    ctl.setForceRunMode(bolt::amp::control::SerialCpu);

    std::fill(  stdInput.begin( ),  stdInput.end( ), val );
    bolt::amp::fill( ctl, boltInput.begin( ), boltInput.end( ), val );

    cmpArrays( stdInput, boltInput );
}

TEST_P( DevFloatVector, MultiCoreFill )
{

    float val = (float)73.7;


    bolt::amp::control ctl = bolt::amp::control::getDefault( );
    ctl.setForceRunMode(bolt::amp::control::MultiCoreCpu);

    std::fill(  stdInput.begin( ),  stdInput.end( ), val );
    bolt::amp::fill( ctl, boltInput.begin( ), boltInput.end( ), val );

    cmpArrays( stdInput, boltInput );
}


#if (TEST_DOUBLE == 1)
TEST_P( DevDblVector, Fill )
{
    double val = 78.85;
    std::fill(  stdInput.begin( ),  stdInput.end( ), val );
    bolt::amp::fill( boltInput.begin( ), boltInput.end( ), val );
    //  Loop through the array and compare all the values with each other
    cmpArrays( stdInput, boltInput );
}

TEST_P( DevDblVector, SerialFill )
{
    double val = 78.85;

    bolt::amp::control ctl = bolt::amp::control::getDefault( );
    ctl.setForceRunMode(bolt::amp::control::SerialCpu);

    std::fill(  stdInput.begin( ),  stdInput.end( ), val );
    bolt::amp::fill( ctl, boltInput.begin( ), boltInput.end( ), val );
    //  Loop through the array and compare all the values with each other
    cmpArrays( stdInput, boltInput );
}

TEST_P(DevDblVector,  MultiCoreFill )
{
    double val = 78.85;

    bolt::amp::control ctl = bolt::amp::control::getDefault( );
    ctl.setForceRunMode(bolt::amp::control::MultiCoreCpu);

    std::fill(  stdInput.begin( ),  stdInput.end( ), val );
    bolt::amp::fill( ctl, boltInput.begin( ), boltInput.end( ), val );
    //  Loop through the array and compare all the values with each other
    cmpArrays( stdInput, boltInput );
}
#endif

//////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////////
//Test Cases for Fill_N
//////////////////////////////////////////////////////////////////////////////////////////////

TEST_P( HostUDDVector, Fill_n )
{
    UDD val(73);
    size_t size = stdInput.size();

    std::fill_n(  stdInput.begin( ), size, val );
    bolt::amp::fill_n( boltInput.begin( ), size, val );

    cmpArrays( stdInput, boltInput );
}

TEST_P( HostUDDVector, SerialFill_n )
{
    UDD val(73);
    size_t size = stdInput.size();

    bolt::amp::control ctl = bolt::amp::control::getDefault( );
    ctl.setForceRunMode(bolt::amp::control::SerialCpu);

    std::fill_n(  stdInput.begin( ),  size, val );
    bolt::amp::fill_n( ctl, boltInput.begin( ), size, val );

    cmpArrays( stdInput, boltInput );
}

TEST_P( HostUDDVector, MultiCoreFill_n )
{
    UDD val(73);
    size_t size = stdInput.size();

    bolt::amp::control ctl = bolt::amp::control::getDefault( );
    ctl.setForceRunMode(bolt::amp::control::MultiCoreCpu);

    std::fill_n(  stdInput.begin( ),  size, val );
    bolt::amp::fill_n( ctl, boltInput.begin( ), size, val );

    cmpArrays( stdInput, boltInput );
}

TEST_P( HostUnsignedIntVector, Fill_n )
{
    unsigned int val = 73;
    size_t size = stdInput.size();

    std::fill_n(stdInput.begin( ),size,val);
    bolt::amp::fill_n(boltInput.begin( ),size,val);

    cmpArrays(stdInput, boltInput);
}

TEST_P( HostUnsignedIntVector, SerialFill_n )
{
    unsigned int val = 73;
    size_t size = stdInput.size();

    bolt::amp::control ctl = bolt::amp::control::getDefault( );
    ctl.setForceRunMode(bolt::amp::control::SerialCpu);

    std::fill_n(stdInput.begin( ),size,val);
    bolt::amp::fill_n(ctl, boltInput.begin( ),size,val);

    cmpArrays(stdInput, boltInput);
}

TEST_P( HostUnsignedIntVector, MultiCoreFill_n )
{
    unsigned int val = 73;
    size_t size = stdInput.size();

    bolt::amp::control ctl = bolt::amp::control::getDefault( );
    ctl.setForceRunMode(bolt::amp::control::MultiCoreCpu);

    std::fill_n(stdInput.begin( ),size,val);
    bolt::amp::fill_n(ctl, boltInput.begin( ),size,val);

    cmpArrays(stdInput, boltInput);
}

TEST_P( HostIntVector, Fill_n )
{
    int val = 73;
    size_t size = stdInput.size();
    std::fill_n(stdInput.begin( ),size,val);
    bolt::amp::fill_n(boltInput.begin( ),size,val);

    cmpArrays(stdInput, boltInput);
}                       

TEST_P( HostIntVector, SerialFill_n )
{
    int val = 73;
    size_t size = stdInput.size();

    bolt::amp::control ctl = bolt::amp::control::getDefault( );
    ctl.setForceRunMode(bolt::amp::control::SerialCpu);

    std::fill_n(stdInput.begin( ),size,val);
    bolt::amp::fill_n(ctl, boltInput.begin( ),size,val);

    cmpArrays(stdInput, boltInput);
}

TEST_P( HostIntVector, MultiCoreFill_n )
{
    int val = 73;
    size_t size = stdInput.size();

    bolt::amp::control ctl = bolt::amp::control::getDefault( );
    ctl.setForceRunMode(bolt::amp::control::MultiCoreCpu);

    std::fill_n(stdInput.begin( ),size,val);
    bolt::amp::fill_n(ctl, boltInput.begin( ),size,val);

    cmpArrays(stdInput, boltInput);
}

TEST_P( HostFloatVector, Fill_n )
{

    float val = (float) 73.6;

    size_t size = stdInput.size();
    std::fill_n(stdInput.begin( ),size,val);
    bolt::amp::fill_n(boltInput.begin( ),size,val);

    cmpArrays(stdInput, boltInput);
}


TEST_P( HostFloatVector, SerialFill_n )
{

    float val = (float)73.6;

    size_t size = stdInput.size();

    bolt::amp::control ctl = bolt::amp::control::getDefault( );
    ctl.setForceRunMode(bolt::amp::control::SerialCpu);

    std::fill_n(stdInput.begin( ),size,val);
    bolt::amp::fill_n(ctl, boltInput.begin( ),size,val);

    cmpArrays(stdInput, boltInput);
}

TEST_P( HostFloatVector, MultiCoreFill_n )
{

    float val = (float) 73.6;

    size_t size = stdInput.size();

    bolt::amp::control ctl = bolt::amp::control::getDefault( );
    ctl.setForceRunMode(bolt::amp::control::MultiCoreCpu);

    std::fill_n(stdInput.begin( ),size,val);
    bolt::amp::fill_n(ctl, boltInput.begin( ),size,val);

    cmpArrays(stdInput, boltInput);
}


#if (TEST_DOUBLE == 1)
TEST_P( HostDblVector, Fill_n )
{
    double val = 78.85;
    size_t size = stdInput.size();
    std::fill_n(stdInput.begin(),size,val );
    bolt::amp::fill_n(boltInput.begin(),size,val );

    cmpArrays(stdInput,boltInput);
}


TEST_P( HostDblVector, SerialFill_n )
{
    double val = 78.85;
    size_t size = stdInput.size();

    bolt::amp::control ctl = bolt::amp::control::getDefault( );
    ctl.setForceRunMode(bolt::amp::control::SerialCpu);

    std::fill_n(stdInput.begin(),size,val );
    bolt::amp::fill_n(ctl, boltInput.begin(),size,val );

    cmpArrays(stdInput,boltInput);
}

TEST_P(HostDblVector,  MultiCoreFill_n )
{
    double val = 78.85;
    size_t size = stdInput.size();

    bolt::amp::control ctl = bolt::amp::control::getDefault( );
    ctl.setForceRunMode(bolt::amp::control::MultiCoreCpu);

    std::fill_n(stdInput.begin(),size,val );
    bolt::amp::fill_n(ctl, boltInput.begin(),size,val );

    cmpArrays(stdInput,boltInput);
}
#endif


TEST_P( DevUDDVector, Fill_n )
{
    UDD val(73);
    size_t size = stdInput.size();
    std::fill_n(  stdInput.begin( ),  size, val );
    bolt::amp::fill_n( boltInput.begin( ), size, val );

    cmpArrays( stdInput, boltInput );
}

TEST_P( DevUDDVector, SerialFill_n )
{
    UDD val(73);
    size_t size = stdInput.size();
    bolt::amp::control ctl = bolt::amp::control::getDefault( );
    ctl.setForceRunMode(bolt::amp::control::SerialCpu);

    std::fill_n(  stdInput.begin( ),  size, val );
    bolt::amp::fill_n( ctl, boltInput.begin( ), size, val );

    cmpArrays( stdInput, boltInput );
}

TEST_P( DevUDDVector, MultiCoreFill_n )
{
    UDD val(73);
    size_t size = stdInput.size();

    bolt::amp::control ctl = bolt::amp::control::getDefault( );
    ctl.setForceRunMode(bolt::amp::control::MultiCoreCpu);

    std::fill_n(  stdInput.begin( ),  size, val );
    bolt::amp::fill_n( ctl, boltInput.begin( ), size, val );

    cmpArrays( stdInput, boltInput );
}

TEST_P( DevUnsignedIntVector, Fill_n )
{
    unsigned int val = 73;
    size_t size = stdInput.size();
    std::fill_n(stdInput.begin(),size,val);
    bolt::amp::fill_n(boltInput.begin(),size,val);

    cmpArrays(stdInput, boltInput);
}


TEST_P( DevUnsignedIntVector, SerialFill_n )
{
    unsigned int val = 73;
    size_t size = stdInput.size();

    bolt::amp::control ctl = bolt::amp::control::getDefault( );
    ctl.setForceRunMode(bolt::amp::control::SerialCpu);

    std::fill_n(stdInput.begin(),size,val);
    bolt::amp::fill_n(ctl, boltInput.begin(),size,val);

    cmpArrays(stdInput, boltInput);
}

TEST_P( DevUnsignedIntVector, MultiCoreFill_n )
{
    unsigned int val = 73;
    size_t size = stdInput.size();

    bolt::amp::control ctl = bolt::amp::control::getDefault( );
    ctl.setForceRunMode(bolt::amp::control::MultiCoreCpu);

    std::fill_n(stdInput.begin(),size,val);
    bolt::amp::fill_n(ctl, boltInput.begin(),size,val);

    cmpArrays(stdInput, boltInput);
}

TEST_P( DevIntVector, Fill_n )
{
    int val = 73;
    size_t size = stdInput.size();
    std::fill_n(stdInput.begin(),size,val);
    bolt::amp::fill_n(boltInput.begin(),size,val);

    cmpArrays(stdInput, boltInput);
}


TEST_P( DevIntVector, SerialFill_n )
{
    int val = 73;
    size_t size = stdInput.size();

    bolt::amp::control ctl = bolt::amp::control::getDefault( );
    ctl.setForceRunMode(bolt::amp::control::SerialCpu);

    std::fill_n(stdInput.begin(),size,val);
    bolt::amp::fill_n(ctl, boltInput.begin(),size,val);

    cmpArrays(stdInput, boltInput);
}

TEST_P( DevIntVector, MultiCoreFill_n )
{
    int val = 73;
    size_t size = stdInput.size();

    bolt::amp::control ctl = bolt::amp::control::getDefault( );
    ctl.setForceRunMode(bolt::amp::control::MultiCoreCpu);

    std::fill_n(stdInput.begin(),size,val);
    bolt::amp::fill_n(ctl, boltInput.begin(),size,val);

    cmpArrays(stdInput, boltInput);
}

TEST_P( DevFloatVector, Fill_n )
{

    float val = (float) 73.7;

    size_t size = stdInput.size();
    std::fill_n(stdInput.begin(),size,val);
    bolt::amp::fill_n(boltInput.begin(),size,val);

    cmpArrays(stdInput, boltInput);
}


TEST_P( DevFloatVector, SerialFill_n )
{

    float val = (float)73.7;

    size_t size = stdInput.size();

    bolt::amp::control ctl = bolt::amp::control::getDefault( );
    ctl.setForceRunMode(bolt::amp::control::SerialCpu);

    std::fill_n(stdInput.begin(),size,val);
    bolt::amp::fill_n(ctl, boltInput.begin(),size,val);

    cmpArrays(stdInput, boltInput);
}

TEST_P( DevFloatVector, MultiCoreFill_n )
{

    float val = (float)73.7;

    size_t size = stdInput.size();

    bolt::amp::control ctl = bolt::amp::control::getDefault( );
    ctl.setForceRunMode(bolt::amp::control::MultiCoreCpu);

    std::fill_n(stdInput.begin(),size,val);
    bolt::amp::fill_n(ctl, boltInput.begin(),size,val);

    cmpArrays(stdInput, boltInput);
}


#if (TEST_DOUBLE == 1)
TEST_P( DevDblVector, Fill_n )
{
    double val = 78.85;
    size_t size = stdInput.size();
    std::fill_n(stdInput.begin( ),size, val );
    bolt::amp::fill_n(boltInput.begin( ),size,val );
    cmpArrays( stdInput, boltInput );
}

TEST_P( DevDblVector, SerialFill_n )
{
    double val = 78.85;
    size_t size = stdInput.size();

    bolt::amp::control ctl = bolt::amp::control::getDefault( );
    ctl.setForceRunMode(bolt::amp::control::SerialCpu);

    std::fill_n(stdInput.begin( ),size, val );
    bolt::amp::fill_n(ctl, boltInput.begin( ),size,val );
    cmpArrays( stdInput, boltInput );
}

TEST_P(DevDblVector, MultiCoreFill_n )
{
    double val = 78.85;
    size_t size = stdInput.size();

    bolt::amp::control ctl = bolt::amp::control::getDefault( );
    ctl.setForceRunMode(bolt::amp::control::MultiCoreCpu);

    std::fill_n(stdInput.begin( ),size, val );
    bolt::amp::fill_n(ctl, boltInput.begin( ),size,val );
    cmpArrays( stdInput, boltInput );
}
#endif

//////////////////////////////////////////////////////////////////////////////////////////////

INSTANTIATE_TEST_CASE_P( FillSmall, HostIntVector, ::testing::Range(1,256,3));
#if TEST_LARGE_BUFFERS
INSTANTIATE_TEST_CASE_P( FillLarge, HostIntVector, ::testing::Range(1023,1050000,350001));
#endif
INSTANTIATE_TEST_CASE_P( FillLarge, DevIntVector,  ::testing::Range(1024,1050000,350003));
INSTANTIATE_TEST_CASE_P( FillSmall, DevIntVector,  ::testing::Range(2,256,3));


INSTANTIATE_TEST_CASE_P( FillSmall, HostFloatVector, ::testing::Range(1,256,3));
#if TEST_LARGE_BUFFERS
INSTANTIATE_TEST_CASE_P( FillLarge, HostFloatVector, ::testing::Range(1023,1050000,350001));
#endif
INSTANTIATE_TEST_CASE_P( FillSmall, DevFloatVector,  ::testing::Range(2,256,3));
INSTANTIATE_TEST_CASE_P( FillLarge, DevFloatVector,  ::testing::Range(1024,1050000,350003));

INSTANTIATE_TEST_CASE_P( FillSmall, HostUnsignedIntVector, ::testing::Range(1,256,3));
#if TEST_LARGE_BUFFERS
INSTANTIATE_TEST_CASE_P( FillLarge, HostUnsignedIntVector, ::testing::Range(1023,1050000,350001));
#endif
INSTANTIATE_TEST_CASE_P( FillSmall, DevUnsignedIntVector,  ::testing::Range(2,256,3));
INSTANTIATE_TEST_CASE_P( FillLarge, DevUnsignedIntVector,  ::testing::Range(1024,1050000,350003));

INSTANTIATE_TEST_CASE_P( FillSmall, HostUDDVector, ::testing::Range(1,256,3));
#if TEST_LARGE_BUFFERS
INSTANTIATE_TEST_CASE_P( FillLarge, HostUDDVector, ::testing::Range(1023,1050000,350001));
#endif
INSTANTIATE_TEST_CASE_P( FillLarge, DevUDDVector,  ::testing::Range(1024,1050000,350003));
INSTANTIATE_TEST_CASE_P( FillSmall, DevUDDVector,  ::testing::Range(2,256,3));



#if (TEST_DOUBLE == 1)
INSTANTIATE_TEST_CASE_P( FillSmall, HostDblVector, ::testing::Range(3,256,3));
#if TEST_LARGE_BUFFERS
INSTANTIATE_TEST_CASE_P( FillLarge, HostDblVector, ::testing::Range(1025,1050000, 350007 ) );
#endif
INSTANTIATE_TEST_CASE_P( FillSmall, DevDblVector,  ::testing::Range(4, 256, 3 ) );
INSTANTIATE_TEST_CASE_P( FillLarge, DevDblVector,  ::testing::Range(1026, 1050000, 350011 ) );
#endif


#if (TEST_DOUBLE == 1)

TEST (simpleTest, basicDataBoltAMPDevVectAutoConvertCheck)
{ 
    int size =10; 
    int iValue = 48; 
    union ieeeconvert
    {
        float x;
        int y;
    }converter;
    //double dValue = 48.6;
    float fValue = 48.0;
    double dNan = std::numeric_limits<double>::signaling_NaN();
    bolt::amp::device_vector<int> dv(size); 
    std::vector<int> hv(size);
    //bolt::amp::device_vector<double> ddv(size); 
    //std::vector<double> dhv(size);
    bolt::amp::device_vector<float> fdv(size); 
    std::vector<float> fhv(size);    


    ////////////////////////////////////////////////////
    // No casting needed here!
    ////////////////////////////////////////////////////

    bolt::amp::fill(dv.begin(), dv.end(),iValue);  
    std::fill(hv.begin(), hv.end(),iValue); 
    cmpArrays(hv,dv);

    /*std::fill(dhv.begin(), dhv.end(), dValue);
    bolt::amp::fill(ddv.begin(), ddv.end(), dValue); 
    cmpArrays(dhv,ddv);*/

    ////////////////////////////////////////////////////
    // Test cases to verify casting
    ////////////////////////////////////////////////////

    //std::fill(hv.begin(), hv.end(), static_cast< int >( dValue ) ); 
    //bolt::amp::fill(dv.begin(), dv.end(), dValue);
    //cmpArrays(hv,dv);

    std::fill(hv.begin(), hv.end(), static_cast< int >( fValue ) ); 
    bolt::amp::fill(dv.begin(), dv.end(), fValue);
    cmpArrays(hv,dv);

    /*bolt::amp::fill(ddv.begin(), ddv.end(),iValue);
    std::fill(dhv.begin(), dhv.end(),iValue); 
    cmpArrays(dhv,ddv);

    std::fill(dhv.begin(), dhv.end(), fValue); 
    bolt::amp::fill(ddv.begin(), ddv.end(), fValue); 
    cmpArrays(dhv,ddv);*/

    converter.y =_FPCLASS_ND;

    ////////////////////////////////////////////////////
    // This verifies that it works with Denormals 
    ////////////////////////////////////////////////////

    //std::fill(dhv.begin(), dhv.end(), converter.x); 
    //bolt::cl::fill(ddv.begin(), ddv.end(), converter.x);
    //cmpArrays(dhv,ddv);

    std::fill(fhv.begin(), fhv.end(), converter.x); 
    bolt::amp::fill(fdv.begin(), fdv.end(), converter.x);
    cmpArrays(fhv,fdv);

    ////////////////////////////////////////////////////
    // Fill some NANs: It fills, but the test fails
    //                 since you can't compare NANs
    ////////////////////////////////////////////////////

    //std::fill(fhv.begin(), fhv.end(), dNan); 
    //bolt::cl::fill(fdv.begin(), fdv.end(), dNan);
    //cmpArrays(fhv,fdv);

    //std::fill(dhv.begin(), dhv.end(), dNan); 
    //bolt::cl::fill(ddv.begin(), ddv.end(), dNan);
    //cmpArrays(dhv,ddv);
    //std::cout<<dhv[0]<<" Hst"<<ddv[0]<<" Device"<<std::endl;


} 


TEST (SerialsimpleTest, basicDataBoltAMPDevVectAutoConvertCheck)
{ 
    int size =10; 
    int iValue = 48; 
    union ieeeconvert
    {
        float x;
        int y;
    }converter;
    double dValue = 48.6;
    float fValue = 48.0;
    double dNan = std::numeric_limits<double>::signaling_NaN();
    bolt::amp::device_vector<int> dv(size); 
    std::vector<int> hv(size);
    bolt::amp::device_vector<double> ddv(size); 
    std::vector<double> dhv(size);
    bolt::amp::device_vector<float> fdv(size); 
    std::vector<float> fhv(size);    


    ////////////////////////////////////////////////////
    // No casting needed here!
    ////////////////////////////////////////////////////

    bolt::amp::control ctl = bolt::amp::control::getDefault( );
    ctl.setForceRunMode(bolt::amp::control::SerialCpu);

    bolt::amp::fill(ctl, dv.begin(), dv.end(),iValue);  
    std::fill(hv.begin(), hv.end(),iValue); 
    cmpArrays(hv,dv);

    std::fill(dhv.begin(), dhv.end(), dValue);
    bolt::amp::fill(ctl, ddv.begin(), ddv.end(), dValue); 
    cmpArrays(dhv,ddv);

    ////////////////////////////////////////////////////
    // Test cases to verify casting
    ////////////////////////////////////////////////////

    std::fill(hv.begin(), hv.end(), static_cast< int >( dValue ) ); 
    bolt::amp::fill(ctl, dv.begin(), dv.end(), dValue);
    cmpArrays(hv,dv);

    std::fill(hv.begin(), hv.end(), static_cast< int >( fValue ) ); 
    bolt::amp::fill(ctl, dv.begin(), dv.end(), fValue);
    cmpArrays(hv,dv);

    bolt::amp::fill(ctl, ddv.begin(), ddv.end(),iValue);
    std::fill(dhv.begin(), dhv.end(),iValue); 
    cmpArrays(dhv,ddv);

    std::fill(dhv.begin(), dhv.end(), fValue); 
    bolt::amp::fill(ctl, ddv.begin(), ddv.end(), fValue); 
    cmpArrays(dhv,ddv);

    converter.y =_FPCLASS_ND;


    ////////////////////////////////////////////////////
    // This verifies that it works with Denormals 
    ////////////////////////////////////////////////////

    //std::fill(dhv.begin(), dhv.end(), converter.x); 
    //bolt::cl::fill(ddv.begin(), ddv.end(), converter.x);
    //cmpArrays(dhv,ddv);

    std::fill(fhv.begin(), fhv.end(), converter.x); 
    bolt::amp::fill(ctl, fdv.begin(), fdv.end(), converter.x);
    cmpArrays(fhv,fdv);

    ////////////////////////////////////////////////////
    // Fill some NANs: It fills, but the test fails
    //                 since you can't compare NANs
    ////////////////////////////////////////////////////

    //std::fill(fhv.begin(), fhv.end(), dNan); 
    //bolt::cl::fill(fdv.begin(), fdv.end(), dNan);
    //cmpArrays(fhv,fdv);

    //std::fill(dhv.begin(), dhv.end(), dNan); 
    //bolt::cl::fill(ddv.begin(), ddv.end(), dNan);
    //cmpArrays(dhv,ddv);
    //std::cout<<dhv[0]<<" Hst"<<ddv[0]<<" Device"<<std::endl;


} 

TEST (MultiCoresimpleTest, basicDataBoltAMPDevVectAutoConvertCheck)
{ 
    int size =10; 
    int iValue = 48; 
    union ieeeconvert
    {
        float x;
        int y;
    }converter;
    double dValue = 48.6;
    float fValue = 48.0;
    double dNan = std::numeric_limits<double>::signaling_NaN();
    bolt::amp::device_vector<int> dv(size); 
    std::vector<int> hv(size);
    bolt::amp::device_vector<double> ddv(size); 
    std::vector<double> dhv(size);
    bolt::amp::device_vector<float> fdv(size); 
    std::vector<float> fhv(size);    


    ////////////////////////////////////////////////////
    // No casting needed here!
    ////////////////////////////////////////////////////

    bolt::amp::control ctl = bolt::amp::control::getDefault( );
    ctl.setForceRunMode(bolt::amp::control::MultiCoreCpu);

    bolt::amp::fill(ctl, dv.begin(), dv.end(),iValue);  
    std::fill(hv.begin(), hv.end(),iValue); 
    cmpArrays(hv,dv);

    std::fill(dhv.begin(), dhv.end(), dValue);
    bolt::amp::fill(ctl, ddv.begin(), ddv.end(), dValue); 
    cmpArrays(dhv,ddv);

    ////////////////////////////////////////////////////
    // Test cases to verify casting
    ////////////////////////////////////////////////////

    std::fill(hv.begin(), hv.end(), static_cast< int >( dValue ) ); 
    bolt::amp::fill(ctl, dv.begin(), dv.end(), dValue);
    cmpArrays(hv,dv);

    std::fill(hv.begin(), hv.end(), static_cast< int >( fValue ) ); 
    bolt::amp::fill(ctl, dv.begin(), dv.end(), fValue);
    cmpArrays(hv,dv);

    bolt::amp::fill(ctl, ddv.begin(), ddv.end(),iValue);
    std::fill(dhv.begin(), dhv.end(),iValue); 
    cmpArrays(dhv,ddv);

    std::fill(dhv.begin(), dhv.end(), fValue); 
    bolt::amp::fill(ctl, ddv.begin(), ddv.end(), fValue); 
    cmpArrays(dhv,ddv);

    converter.y =_FPCLASS_ND;


    ////////////////////////////////////////////////////
    // This verifies that it works with Denormals 
    ////////////////////////////////////////////////////

    //std::fill(dhv.begin(), dhv.end(), converter.x); 
    //bolt::cl::fill(ddv.begin(), ddv.end(), converter.x);
    //cmpArrays(dhv,ddv);

    std::fill(fhv.begin(), fhv.end(), converter.x); 
    bolt::amp::fill(ctl, fdv.begin(), fdv.end(), converter.x);
    cmpArrays(fhv,fdv);

    ////////////////////////////////////////////////////
    // Fill some NANs: It fills, but the test fails
    //                 since you can't compare NANs
    ////////////////////////////////////////////////////

    //std::fill(fhv.begin(), fhv.end(), dNan); 
    //bolt::cl::fill(fdv.begin(), fdv.end(), dNan);
    //cmpArrays(fhv,fdv);

    //std::fill(dhv.begin(), dhv.end(), dNan); 
    //bolt::cl::fill(ddv.begin(), ddv.end(), dNan);
    //cmpArrays(dhv,ddv);
    //std::cout<<dhv[0]<<" Hst"<<ddv[0]<<" Device"<<std::endl;


} 
#endif



TEST(Fill, AllRunModes)
{
  int length = 1024;
  bolt::amp::control ctlCPU, ctlMCPU;


  std::vector<int> hA(length), dA(length);
  //bolt::amp::device_vector<int> dVA(length);
  bolt::amp::device_vector<int> dVA(dA.begin(), dA.end());

  // Try with SerialCpu runmode
  ctlCPU.setForceRunMode(bolt::amp::control::SerialCpu);  
  std::fill(hA.begin(), hA.end(), 10);
  bolt::amp::fill(ctlCPU, dA.begin(), dA.end(), 10);

  cmpArrays(hA,dA);

  // Try with MultiCoreCpu runmode
  ctlMCPU.setForceRunMode(bolt::amp::control::MultiCoreCpu);  
  std::fill(hA.begin(), hA.end(), 50);
  bolt::amp::fill(ctlMCPU, dA.begin(), dA.end(), 50);

  cmpArrays(hA,dA);

    // Try DV with MultiCoreCpu runmode
  ctlMCPU.setForceRunMode(bolt::amp::control::MultiCoreCpu);  
  std::fill(hA.begin(), hA.end(), 120);
  bolt::amp::fill(ctlMCPU, dVA.begin(), dVA.end(),120);
  cmpArrays(hA,dVA);


}


int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

#else

/******************************************************************************
 * checkResults
 *      compare std:: and bolt::cl:: results
 *      returns number of errors
 *    For testing struct types checkResults is embedded in the test function.
 *****************************************************************************/
template<typename InputIterator1, typename InputIterator2>
int checkResults(std::string &msg, InputIterator1 first1 , InputIterator1 end1 , InputIterator2 first2)
{
    int errCnt = 0;
    static const int maxErrCnt = 10;
    int sz = end1-first1 ;
    for (int i=0; i<sz ; i++) {
        if (first1 [i] != *(first2 + i) ) {
            errCnt++;
            if (errCnt < maxErrCnt) {
                std::cout<<"\tMISMATCH["<<i<<"] " <<msg<< " STL= "<<first1[i]<<"  BOLT=" <<*(first2 + i)<<std::endl;
            } else if (errCnt == maxErrCnt) {
                std::cout << "\tMax error count reached; no more mismatches will be printed...\n";
            }
        };

};

    if ( errCnt == 0 ) {
        printf(" PASSED %20s Correct for all %6i elements.\n", msg.c_str(), sz);
    } else {
        printf("*FAILED %20s Mismatch for %6i /%6i elements.\n", msg.c_str(), sz);
    };
    fflush(stdout);

    return errCnt;
};

/******************************************************************************
 * Pause Program to see Results
 *****************************************************************************/
void waitForEnter()
{
    std::cout << "Press <ENTER> to continue." << std::endl;
    std::cin.clear();
    std::cin.ignore(1, '\n');
}



/******************************************************************************
 * Tests
 *****************************************************************************/
int testFill1DevVec( int length );
int testFill2DevVec( int length );
int testFillN1DevVec( int length );
int testFillN2DevVec( int length );
int testFill1HostVec( int length );
int testFill2HostVec( int length );
int testFillN1HostVec( int length );
int testFillN2HostVec( int length );


/******************************************************************************
 * Main
 *****************************************************************************/
int main(int argc, char* argv[])
{
    // Test several vector lengths
    std::vector<int> lengths;
    lengths.push_back(0);
    lengths.push_back(1);
    lengths.push_back(63);
    lengths.push_back(64);
    lengths.push_back(65);
    lengths.push_back(1023);
    lengths.push_back(1024);
    lengths.push_back(1025);
    lengths.push_back(16384+1);

    int testsPassed = 0;
    int testsFailed = 0;

    // for each test length
    for (int iter = 0; iter < lengths.size(); iter++)
    {
        int i = iter % lengths.size();
        int errorCount = 0;

        /***********************************************************
         * Test Device Vectors
         **********************************************************/

        //Fill 1
        errorCount = testFill1DevVec(lengths[i]);
        if ( errorCount == 0 )
            testsPassed++;
        else
            testsFailed++;

        // Fill 2
        errorCount = testFill2DevVec(lengths[i]);
        if ( errorCount == 0 )
            testsPassed++;
        else
            testsFailed++;

        // Fill_N 1
        errorCount = testFillN1DevVec(lengths[i]);
        if ( errorCount == 0 )
            testsPassed++;
        else
            testsFailed++;

        // Fill_N 2
        errorCount = testFillN2DevVec(lengths[i]);
        if ( errorCount == 0 )
            testsPassed++;
        else
            testsFailed++;


        /***********************************************************
         * Test Host Vectors
         **********************************************************/

        // Fill 1
        errorCount = 0;
        errorCount = testFill1HostVec(lengths[i]);
        if ( errorCount == 0 )
            testsPassed++;
        else
            testsFailed++;

        // Fill 2
        errorCount = testFill2HostVec(lengths[i]);
        if ( errorCount == 0 )
            testsPassed++;
        else
            testsFailed++;

        // Fill_N 1
        errorCount = testFillN1HostVec(lengths[i]);
        if ( errorCount == 0 )
            testsPassed++;
        else
            testsFailed++;

        // Fill_N 2
        errorCount = testFillN2HostVec(lengths[i]);
        if ( errorCount == 0 )
            testsPassed++;
        else
            testsFailed++;

#if STRUCT
        // Fill struct
        errorCount = teststruct(lengths[i]);
        if ( errorCount == 0 )
            testsPassed++;
        else
            testsFailed++;

        errorCount = teststruct_n(lengths[i]);
        if ( errorCount == 0 )
            testsPassed++;
        else
            testsFailed++;
    
#endif
}
    // Print final results
    printf("Final Results:\n");
    printf("%9i Tests Passed\n", testsPassed);
    printf("%9i Tests Failed\n", testsFailed);

    // Wait to exit
    waitForEnter();
    return 1;
}

/***********************************************************
 * Device Vector Functions
 **********************************************************/

// Fill 1
int testFill1DevVec( int length )
{
    // function name for reporting
    std::string fName = __FUNCTION__;
    // containers
    std::vector<float> gold(length);
    bolt::amp::device_vector<float> dv(length);
   //Call Fill
    std::fill(gold.begin(), gold.end(), 3.14159f);
    bolt::amp::fill(dv.begin(), dv.end(), 3.14159f);
    //check results
    return checkResults(fName, gold.begin(), gold.end(), dv.begin());
}

// Fill 2
int testFill2DevVec( int length )
{
    // function name for reporting
    std::string fName = __FUNCTION__;
    // containers
    std::vector<float> gold(length);
    bolt::amp::device_vector<float> dv(length);
    //Call Fill
    std::fill(gold.begin(), gold.end(), 0.f);
    bolt::amp::fill(dv.begin(), dv.end(), 0.f);
    //check results
    return checkResults(fName, gold.begin(), gold.end(), dv.begin());
}

 // Fill_N 1
int testFillN1DevVec( int length )
{
    // function name for reporting
    std::string fName = __FUNCTION__;
    // containers
    std::vector<float> gold(length);
    bolt::amp::device_vector<float> dv(length);
    //Call Fill_N
    std::fill_n(gold.begin(), length, 3.14159f);
    bolt::amp::fill_n(dv.begin(), length, 3.14159f);
    //check results
    return checkResults(fName, gold.begin(), gold.end(), dv.begin());
}

// Fill_N 2
int testFillN2DevVec( int length )
{
    // function name for reporting
    std::string fName = __FUNCTION__;
    // containers
    std::vector<float> gold(length);
    bolt::amp::device_vector<float> dv(length);
    //Call Fill_N
    std::fill_n(gold.begin(), length, 0.f);
    bolt::amp::fill_n(dv.begin(), length, 0.f);
    //check results
    return checkResults(fName, gold.begin(), gold.end(), dv.begin());
}



/***********************************************************
 * Host Vector Functions
 **********************************************************/

// Fill 1
int testFill1HostVec( int length )
{
    // function name for reporting
    std::string fName = __FUNCTION__;
    // containers
    std::vector<float> gold(length);
    std::vector<float> dv(length);

    std::fill(gold.begin(), gold.end(), 3.14159f);
    bolt::amp::fill(dv.begin(), dv.end(), 3.14159f);
    //check results
    return checkResults(fName, gold.begin(), gold.end(), dv.begin());
}

// Fill 2
int testFill2HostVec( int length )
{
    // function name for reporting
    std::string fName = __FUNCTION__;
    // containers
    std::vector<float> gold(length);
    std::vector<float> dv(length);

    std::fill(gold.begin(), gold.end(), 0.f);
    bolt::amp::fill(dv.begin(), dv.end(), 0.f);
    //check results
    return checkResults(fName, gold.begin(), gold.end(), dv.begin());
}

// Fill_N 1
int testFillN1HostVec( int length )
{
    // function name for reporting
    std::string fName = __FUNCTION__;
    // containers
    std::vector<float> gold(length);
    std::vector<float> dv(length);

    std::fill_n(gold.begin(), length, 3.14159f);
    bolt::amp::fill_n(dv.begin(), length, 3.14159f);
    //check results
    return checkResults(fName, gold.begin(), gold.end(), dv.begin());
}

// Fill_N 1
int testFillN2HostVec( int length )
{
    // function name for reporting
    std::string fName = __FUNCTION__;
    // containers
    std::vector<float> gold(length);
    std::vector<float> dv(length);

    std::fill_n(gold.begin(), length, 0.f);
    bolt::amp::fill_n(dv.begin(), length, 0.f);
    //check results
    return checkResults(fName, gold.begin(), gold.end(), dv.begin());
}


#endif
