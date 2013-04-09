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
#define TEST_CPU_DEVICE 1

#include <bolt/cl/copy.h>
#include <bolt/cl/functional.h>
#include <bolt/miniDump.h>
#include <bolt/cl/device_vector.h>

#include <gtest/gtest.h>
#include <boost/shared_array.hpp>
#include <array>

#include "common/stdafx.h"
#include "common/myocl.h"

#define BOLT_TEST_MAX_FAILURES 1
#include "test_common.h"

//  Author: kfk
//  Originally, the char b was not an array.  However, testing on my machine showed driver hangs and unit test
//  failures.  Reading the documentation of clEnqueueFillBuffer(), OpenCL seems to like structs of certain 
//  sizes {1, 2, 4, 8, 16, 32, 64, 128}.  Making this struct one of those magic sizes seems to make the driver
//  resets go away.
//  \todo:  Need to research and find a way to handle structs of arbitrary size.
BOLT_FUNCTOR(UserStruct,
struct UserStruct
{
    bool a;
    char b[ 4 ];
    int c;
    float d;
    //double e;

    UserStruct():
        a(true),
        c(3),
        d(4.f)//,
        //e(5.0)
    {
        for( unsigned i = 0; i < 4; ++i )
            b[ i ] = 0;
    }

    bool operator==(const UserStruct& rhs) const
    {
        return
            (a == rhs.a) &&
            //(b == rhs.b) &&
            (c == rhs.c) &&
            (d == rhs.d) //&&
            //(e == rhs.e)
            ;
    }

};
);  // end BOLT_FUNCTOR

/******************************************************************************
 * Tests
 *****************************************************************************/
// device_vector vs std_vector
// copy vs copy_n
// primitive vs struct
// mult64 vs not mult
// zero vs positive

static const int numLengths = 24;
static const int lengths[24] = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
    10, 11, 12, 13, 14, 15, 16, 17, 63, 64,
    65, 1023, 1024, 1025 };
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
            us.b[0] = (char) (rand()%128);
            us.b[1] = (char) (rand()%128);
            us.b[2] = (char) (rand()%128);
            us.b[3] = (char) (rand()%128);
            us.c = (int)  (rand());
            us.d = (float) (1.f*rand());
            //us.e = (double) (1.0*rand()/rand());
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
            us.b[0] = (char) (rand()%128);
            us.b[1] = (char) (rand()%128);
            us.b[2] = (char) (rand()%128);
            us.b[3] = (char) (rand()%128);
            us.c = (int)  (rand());
            us.d = (float) (1.f*rand());
            //us.e = (double) (1.0*rand()/rand());
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
            us.b[0] = (char) (rand()%128);
            us.b[1] = (char) (rand()%128);
            us.b[2] = (char) (rand()%128);
            us.b[3] = (char) (rand()%128);
            us.c = (int)  (rand());
            us.d = (float) (1.f*rand());
            //us.e = (double) (1.0*rand()/rand());
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
            us.b[0] = (char) (rand()%128);
            us.b[1] = (char) (rand()%128);
            us.b[2] = (char) (rand()%128);
            us.b[3] = (char) (rand()%128);
            us.c = (int)  (rand());
            us.d = (float) (1.f*rand());
            //us.e = (double) (1.0*rand()/rand());
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


TEST (copyArrWithDiffTypes, IntAndFloats){
   	int arraySize = 100;
   		
   	int* sourceArr1; 
   	float *sourceFloatArr1;
#if TEST_DOUBLE
   	double *sourceDoubleArr1;
#endif
   	int* destArr1; 
   	float *destFloatArr1;
#if TEST_DOUBLE
   	double *destDoubleArr1;
#endif
   	sourceArr1 = (int *) malloc (arraySize* sizeof (int));
   	destArr1= (int *) malloc (arraySize * sizeof (int));
   
   	sourceFloatArr1 = (float*) malloc (arraySize* sizeof (float));
   	destFloatArr1	= (float *) malloc (arraySize * sizeof(float));
#if TEST_DOUBLE
   	sourceDoubleArr1 = (double *) malloc (arraySize * sizeof(double));
   	destDoubleArr1 = (double *) malloc (arraySize * sizeof(double));
#endif
   
   	for (int i = 0; i < arraySize; i++){
   		sourceArr1[i] = 56535 - i;
   	}
   
   	for (int i = 0; i < arraySize ; i++){
   		sourceFloatArr1[i] = ( float )i  + 0.125f;
   	}
#if TEST_DOUBLE
   	for (int i = 0; i < arraySize ; i++){
   		sourceDoubleArr1[i] = ( double )i  + 0.0009765625;
   	}
#endif
   	//using bolt::cl::control
   
   	bolt::cl::control useThisControl = bolt::cl::control::getDefault();
   	
   	//copying int array as a whole to all there types of arrays :)
   	bolt::cl::copy(useThisControl, sourceArr1, sourceArr1 + arraySize, destArr1);                   //no prob
    //cmpArrays(sourceArr1, destArr1, arraySize);
   	bolt::cl::copy(useThisControl, sourceArr1, sourceArr1 + arraySize, destFloatArr1);				//no prob
    //cmpArrays(sourceArr1, destFloatArr1, arraySize);
#if TEST_DOUBLE
   	bolt::cl::copy(useThisControl, sourceArr1, sourceArr1 + arraySize, destDoubleArr1);				//no prob
#endif
    //cmpArrays(sourceArr1, destDoubleArr1, arraySize);
   
   	//copying float array as a whole to all there types of arrays :)
   	bolt::cl::copy(useThisControl, sourceFloatArr1, sourceFloatArr1 + arraySize, destArr1);			//data loss
   	bolt::cl::copy(useThisControl, sourceFloatArr1, sourceFloatArr1 + arraySize, destFloatArr1);    //no prob
#if TEST_DOUBLE
   	bolt::cl::copy(useThisControl, sourceFloatArr1, sourceFloatArr1 + arraySize, destDoubleArr1);   //no prob
#endif
   
   	//copying double array as a whole to all there types of arrays :)
#if TEST_DOUBLE
   	bolt::cl::copy(useThisControl, sourceDoubleArr1, sourceDoubleArr1 + arraySize, destArr1);		 //data loss
   	bolt::cl::copy(useThisControl, sourceDoubleArr1, sourceDoubleArr1 + arraySize, destFloatArr1);   //data loss
   	bolt::cl::copy(useThisControl, sourceDoubleArr1, sourceDoubleArr1 + arraySize, destDoubleArr1);  //no prob
#endif
   }

TEST (copyIntBoltCLDevVect, withIntAsWhole){ 
    int devVectSize = 10;
    bolt::cl::control& ctrl = bolt::cl::control::getDefault();
    
    bolt::cl::device_vector<int> sourceIntDevVect(10);//, 0, CL_MEM_READ_WRITE, false, ctrl );
    bolt::cl::device_vector<int>   destIntDevVect(10);//, 0, CL_MEM_READ_WRITE, false, ctrl ); 
    
    
    for (int i = 0; i < devVectSize; i++){ 
        sourceIntDevVect[i]	= 56535 - i; 
    } 
    
    //bolt::cl::control& ctrl = bolt::cl::control::getDefault(); 
    
    bolt::cl::copy (ctrl, sourceIntDevVect.begin(), sourceIntDevVect.end(), destIntDevVect.begin()); 
    
    for (int i = 0; i < devVectSize; ++i){ 
        EXPECT_EQ(sourceIntDevVect[i], destIntDevVect[i]);
    }
} 

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
