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

#define TEST_DOUBLE 1
#define TEST_DEVICE_VECTOR 1
#define TEST_CPU_DEVICE 1
#define TEST_LARGE_BUFFERS 0

#pragma warning(disable: 4244) // Disabling possible loss of data warning
#if defined (_WIN32)
#include <xutility>
#endif

#include "common/stdafx.h"
#include "bolt/unicode.h"
#include "bolt/miniDump.h"
#include "bolt/amp/functional.h"
#include <bolt/amp/copy.h>
#include <gtest/gtest.h>
//#include <boost/shared_array.hpp>
#include <array>
#include "common/test_common.h"

//#define ENABLE_DOUBLE_TEST_CASES
//Add the developer unit test cases here. 

static TestBuffer<1024> test_buffer;

template <typename SrcIterator, typename StencilIterator, typename DstIterator, typename Predicate>
DstIterator ref_copy_if(SrcIterator first, SrcIterator last, StencilIterator stencil, DstIterator result, Predicate pred)
{
    while (first!=last) 
    {
        if (pred(*stencil)) 
        {
            *result = *first;
            ++result;
        }
        ++first;
        ++stencil;
    }
    return result;        
}

template <typename T>
struct is_even
{
    bool operator()(T& x) const restrict (cpu, amp)
    {
        int temp = x;
        return (temp % 2) == 0;
    }
};


template <typename T>
class Copy_if_raw_test : public ::testing::Test {
    public: 
    typedef typename std::tuple_element<0, T>::type src_value_type;
    typedef typename std::tuple_element<1, T>::type stencil_value_type;
    typedef typename std::tuple_element<2, T>::type dst_value_type;
    typedef typename std::tuple_element<3, T>::type Predicate;
    Predicate predicate;

    void test_raw_ptrs()
    {
        int length = 10;
        src_value_type* pInput       = new src_value_type[length];
        test_buffer.init(pInput, length);
        stencil_value_type* pStencil = new src_value_type[length];
        test_buffer.init(pStencil, length);
        dst_value_type* pResult      = new dst_value_type[length];
        test_buffer.init(pResult, length);

        std::vector<src_value_type> ref_inputVec(length);
        test_buffer.init(ref_inputVec);
        std::vector<stencil_value_type> ref_stencilVec(length);
        test_buffer.init(ref_stencilVec);
        std::vector<src_value_type> ref_outputVec(length);
        
        //Test without Stencil
        std::copy_if(ref_inputVec.begin(), ref_inputVec.end(), ref_outputVec.begin(), predicate);
        bolt::amp::copy_if(pInput, pInput+length, pResult, predicate);

        cmpArrays(ref_outputVec, pResult);
        //for (int i=0;i<length; i++)
        //    std::cout << "pResult = " << pResult[i] << "ref_outputVec = " << ref_outputVec[i] << "\n";
        //Test with Stencil
        ref_copy_if(ref_inputVec.begin(), ref_inputVec.end(), ref_stencilVec.begin(), ref_outputVec.begin(), predicate);
        bolt::amp::copy_if(pInput, pInput+length, pStencil, pResult, predicate);        
        //for (int i=0;i<length; i++)
        //    std::cout << "pResult = " << pResult[i] << "ref_outputVec = " << ref_outputVec[i] << "\n";
        cmpArrays(ref_outputVec, pResult);
        
        delete pInput;
        delete pStencil;
        delete pResult;
    }

};


template <typename T>
class Copy_if_Test : public ::testing::Test {
    public: 
    typedef typename std::tuple_element<0, T>::type SrcContainer;
    typedef typename std::tuple_element<1, T>::type StencilContainer;
    typedef typename std::tuple_element<2, T>::type DstContainer;
    typedef typename std::tuple_element<3, T>::type Predicate;
    Predicate predicate;
    
    typedef typename SrcContainer::iterator     SrcIterator;
    typedef typename StencilContainer::iterator StencilIterator;
    typedef typename DstContainer::iterator     DstIterator;

    typedef typename std::iterator_traits<SrcIterator>::value_type      src_value_type;
    typedef typename std::iterator_traits<StencilIterator>::value_type  stencil_value_type;
    typedef typename std::iterator_traits<DstIterator>::value_type      dst_value_type;

    void test_wo_stencil()
    {
        int length = 100;
        std::cout << "Testing Copy_if_Test\n";

        std::vector<src_value_type> ref_inputVec(length);
        test_buffer.init(ref_inputVec);
        std::vector<dst_value_type> ref_outputVec(length);
        test_buffer.fill_value(ref_outputVec, 0);

        SrcContainer inputVec(length); 
        test_buffer.init(inputVec);
        DstContainer outputVec(length); 
        test_buffer.fill_value(outputVec, 0);

        std::copy_if(ref_inputVec.begin(), ref_inputVec.end(), ref_outputVec.begin(), predicate);
        //Call Bolt Now

        bolt::amp::copy_if(inputVec.begin(), inputVec.end(), outputVec.begin(), predicate);
        //for(int i=0; i< length; i++)
        //    std::cout << "i = " << i <<"-- ref =" << ref_outputVec[i] << " out = " << outputVec[i] << "\n";
        cmpArrays(ref_outputVec, outputVec);
    }

    void test_w_stencil( )
    {
        int length = 100;
        std::cout << "Testing Copy_if_Test\n";

        std::vector<src_value_type> ref_inputVec(length);
        test_buffer.init(ref_inputVec);
        std::vector<stencil_value_type> ref_stencilVec(length);
        test_buffer.init(ref_stencilVec);
        std::vector<dst_value_type> ref_outputVec(length);
        test_buffer.fill_value(ref_outputVec, 0);

        SrcContainer inputVec(length); 
        test_buffer.init(inputVec);
        StencilContainer stencilVec(length); 
        test_buffer.init(stencilVec);
        DstContainer outputVec(length); 
        test_buffer.fill_value(outputVec, 0);

        //std::copy_if(inputVec.begin(), inputVec.end(), outputVec.begin(), predicate);
        ref_copy_if(ref_inputVec.begin(), ref_inputVec.end(), ref_stencilVec.begin(), ref_outputVec.begin(), predicate);
        //Call Bolt Now
        bolt::amp::copy_if(inputVec.begin(), inputVec.end(), stencilVec.begin(), outputVec.begin(), predicate);
        cmpArrays(ref_outputVec, outputVec);
    }

};


TYPED_TEST_CASE_P(Copy_if_Test);
TYPED_TEST_CASE_P(Copy_if_raw_test);

TYPED_TEST_P(Copy_if_Test, without_stencil) 
{

    test_wo_stencil();

}

TYPED_TEST_P(Copy_if_Test, with_stencil) 
{

    test_w_stencil();

}

TYPED_TEST_P(Copy_if_raw_test, raw_pointers) 
{

    test_raw_ptrs();

}

REGISTER_TYPED_TEST_CASE_P(Copy_if_Test, without_stencil, with_stencil);

typedef std::tuple<std::vector<int>,          std::vector<int>,        std::vector<int>,        is_even<int> >       INT_VEC_2_INT_VEC_IS_EVEN_T;
typedef std::tuple<std::vector<float>,        std::vector<float>,      std::vector<float>,      is_even<float> >     FLOAT_VEC_2_FLOAT_VEC_IS_EVEN_T;
typedef std::tuple<std::vector<int>,          std::vector<int>,        std::vector<float>,      is_even<int> >       INT_VEC_2_FLOAT_VEC_IS_EVEN_T;
#if defined (ENABLE_DOUBLE_TEST_CASES)
typedef std::tuple<std::vector<double>,       std::vector<double>,     std::vector<float>,      is_even<double> >    DOUBLE_VEC_2_FLOAT_VEC_IS_EVEN_T;
typedef std::tuple<std::vector<float>,        std::vector<float>,      std::vector<double>,     is_even<float> >     FLOAT_VEC_2_DOUBLE_VEC_IS_EVEN_T;
#endif
typedef ::testing::Types<
        INT_VEC_2_INT_VEC_IS_EVEN_T
        ,FLOAT_VEC_2_FLOAT_VEC_IS_EVEN_T
        ,INT_VEC_2_FLOAT_VEC_IS_EVEN_T
#if defined (ENABLE_DOUBLE_TEST_CASES)
        ,DOUBLE_VEC_2_FLOAT_VEC_IS_EVEN_T
        ,FLOAT_VEC_2_DOUBLE_VEC_IS_EVEN_T 
#endif
> STDVectorIsEvenTypes;

typedef std::tuple<bolt::amp::device_vector<int>,       bolt::amp::device_vector<int>,      bolt::amp::device_vector<int>,     is_even<int> >      INT_DV_2_INT_DV_IS_EVEN_T;
typedef std::tuple<bolt::amp::device_vector<float>,     bolt::amp::device_vector<float>,    bolt::amp::device_vector<float>,   is_even<float> >    FLOAT_DV_2_FLOAT_DV_IS_EVEN_T;
typedef std::tuple<bolt::amp::device_vector<int>,       bolt::amp::device_vector<int>,      bolt::amp::device_vector<float>,   is_even<int> >      INT_DV_2_FLOAT_DV_IS_EVEN_T;
#if defined (ENABLE_DOUBLE_TEST_CASES)
typedef std::tuple<bolt::amp::device_vector<double>,    bolt::amp::device_vector<double>,   bolt::amp::device_vector<float>,   is_even<double> >   DOUBLE_DV_2_FLOAT_DV_IS_EVEN_T;
typedef std::tuple<bolt::amp::device_vector<float>,     bolt::amp::device_vector<float>,    bolt::amp::device_vector<double>,  is_even<float> >    FLOAT_DV_2_DOUBLE_DV_IS_EVEN_T;
#endif
typedef ::testing::Types<
        INT_DV_2_INT_DV_IS_EVEN_T
        ,FLOAT_DV_2_FLOAT_DV_IS_EVEN_T
        ,INT_DV_2_FLOAT_DV_IS_EVEN_T
#if defined (ENABLE_DOUBLE_TEST_CASES)
        ,DOUBLE_DV_2_FLOAT_DV_IS_EVEN_T
        ,FLOAT_DV_2_DOUBLE_DV_IS_EVEN_T
#endif
         > DeviceVectorIsEvenTypes;

REGISTER_TYPED_TEST_CASE_P(Copy_if_raw_test, raw_pointers);
typedef std::tuple<int,          int,        int,     is_even<int> >                 INT_2_INT_IS_EVEN_T;
typedef std::tuple<float,        float,      float,   is_even<float> >               FLOAT_2_FLOAT_IS_EVEN_T;
typedef std::tuple<int,          int,        float,   is_even<int> >                 INT_2_FLOAT_IS_EVEN_T;
#if defined (ENABLE_DOUBLE_TEST_CASES)
typedef std::tuple<double,       double,     float,   is_even<double> >              DOUBLE_2_FLOAT_IS_EVEN_T;
typedef std::tuple<float,        float,      double,  is_even<float> >               FLOAT_2_DOUBLE_IS_EVEN_T;
#endif
typedef ::testing::Types<
        INT_2_INT_IS_EVEN_T
        ,FLOAT_2_FLOAT_IS_EVEN_T
        ,INT_2_FLOAT_IS_EVEN_T
#if defined (ENABLE_DOUBLE_TEST_CASES)
        ,DOUBLE_2_FLOAT_IS_EVEN_T
        ,FLOAT_2_DOUBLE_IS_EVEN_T
#endif
        > POD_RawPtrIsEvenTypes;



INSTANTIATE_TYPED_TEST_CASE_P(STDVectorTests, Copy_if_Test, STDVectorIsEvenTypes);
INSTANTIATE_TYPED_TEST_CASE_P(DeviceVectorTests, Copy_if_Test, DeviceVectorIsEvenTypes);
INSTANTIATE_TYPED_TEST_CASE_P(RawPtrTests, Copy_if_raw_test, POD_RawPtrIsEvenTypes);


//Add the test team test cases here. 

//Add the EPR test cases here with the EPR number. 


int main(int argc, char* argv[])
{
    //  Register our minidump generating logic
#if defined(_WIN32)
    bolt::miniDumpSingleton::enableMiniDumps( );
#endif

    // Define MEMORYREPORT on windows platfroms to enable debug memory heap checking
#if defined( MEMORYREPORT ) && defined( _WIN32 )
    TCHAR logPath[ MAX_PATH ];
    ::GetCurrentDirectory( MAX_PATH, logPath );
    ::_tcscat_s( logPath, _T( "\\MemoryReport.txt") );

    // We leak the handle to this file, on purpose, so that the ::_CrtSetReportFile() can output it's memory 
    // statistics on app shutdown
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

    // By looking at the memory leak report that is generated by this debug heap, there is a number with 
    // {} brackets that indicates the incremental allocation number of that block.  If you wish to set
    // a breakpoint on that allocation number, put it in the _CrtSetBreakAlloc() call below, and the heap
    // will issue a bp on the request, allowing you to look at the call stack
    // ::_CrtSetBreakAlloc( 1833 );

#endif /* MEMORYREPORT */

    ::testing::InitGoogleTest( &argc, &argv[ 0 ] );

    ////  Set the standard OpenCL wait behavior to help debugging

    bolt::amp::control& myControl = bolt::amp::control::getDefault( );

    int ampRetVal = RUN_ALL_TESTS( );
    

    std::cout << "MultiCore CPU Code path tests\n";
    myControl.setForceRunMode(bolt::amp::control::MultiCoreCpu);
    int MultiCoreRetVal = RUN_ALL_TESTS( );

    std::cout << "Serial Code path tests\n";
    myControl.setForceRunMode(bolt::amp::control::SerialCpu);
    int serialRetVal = RUN_ALL_TESTS( );


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

    return 0;
}

#undef ENABLE_DOUBLE_TEST_CASES