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
#include <array>

#include "bolt/cl/iterator/constant_iterator.h"
#include "bolt/cl/iterator/counting_iterator.h"
#include "bolt/cl/iterator/transform_iterator.h"
#include "bolt/cl/transform.h"
#include "bolt/cl/generate.h"
#include "bolt/cl/reduce.h"
#include "bolt/cl/functional.h"
#include "bolt/cl/distance.h"
#include "bolt/miniDump.h"
#include "bolt/unicode.h"

#include <gtest/gtest.h>

#include <boost/program_options.hpp>
#define BCKND cl



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



BOLT_FUNCTOR(square,
    struct square
    {
        int operator() (const int x)  const { return x + 2; }
        typedef int result_type;
    };
);

BOLT_FUNCTOR(square1,
    struct square1
    {
        int operator() (const int x)  const { return x + 2; }
        typedef int result_type;
    };
);

BOLT_FUNCTOR(add_4,
    struct add_4
    {
        int operator() (const int x)  const { return x + 4; }
        typedef int result_type;
    };
);

BOLT_FUNCTOR(add_3,
    struct add_3
    {
        int operator() (const int x)  const { return x + 3; }
        typedef int result_type;
    };
);


int global_id = 0;

int get_global_id(int i)
{
    return global_id++;
}

BOLT_FUNCTOR(gen_input,
    struct gen_input
    {
        int operator() ()  const { return get_global_id(0); }
        typedef int result_type;
    };
);

TEST( TransformIterator, FirstTest)
{
    {
        const int length = 1024;
        std::vector< int > svInVec( length );
        std::vector< int > svOutVec( length );
        bolt::BCKND::device_vector< int > dvInVec( length );
        bolt::BCKND::device_vector< int > dvOutVec( length );

        square sq;
        gen_input gen;
        typedef std::vector< int >::const_iterator                                                          sv_itr;
        typedef bolt::BCKND::device_vector< int >::iterator                                                 dv_itr;
        typedef bolt::BCKND::transform_iterator< square, std::vector< int >::const_iterator>                sv_trf_itr;
        typedef bolt::BCKND::transform_iterator< square, bolt::BCKND::device_vector< int >::iterator>       dv_trf_itr;
    
        /*Create Iterators*/
        sv_trf_itr sv_trf_begin (svInVec.begin(), sq), sv_trf_end (svInVec.end(), sq);
        dv_trf_itr dv_trf_begin (dvInVec.begin(), sq), dv_trf_end (dvInVec.end(), sq);
    
        /*Generate inputs*/
        std::generate(svInVec.begin(), svInVec.end(), gen);    
        bolt::BCKND::generate(dvInVec.begin(), dvInVec.end(), gen);

        sv_trf_itr::difference_type dist1 = bolt::cl::distance(sv_trf_begin, sv_trf_end);
        sv_trf_itr::difference_type dist2 = bolt::cl::distance(dv_trf_begin, dv_trf_end );

        EXPECT_EQ( dist1, dist2 );
        //std::cout << "distance = " << dist1 << "\n" ;

        for(int i =0; i< length; i++)
        {
            int temp1, temp2;
            temp1 = *sv_trf_begin++;
            temp2 = *dv_trf_begin++;
            EXPECT_EQ( temp1, temp2 );
            //std::cout << temp1 << "   " << temp2 << "\n";
        }
        global_id = 0; // Reset the global id counter
    }
}

BOLT_FUNCTOR(UDD, 
struct UDD
{
    int i;
    float f;
  
    bool operator == (const UDD& other) const {
        return ((i == other.i) && (f == other.f));
    }
    
    UDD()
        : i(0), f(0) { }
    UDD(int _in)
        : i(_in), f((float)(_in+2) ){ }
};
);

BOLT_TEMPLATE_REGISTER_NEW_ITERATOR( bolt::cl::device_vector, int, UDD);


BOLT_FUNCTOR(squareUDD,
    struct squareUDD
    {
        float operator() (const UDD& x)  const 
        { 
            return ((float)x.i + x.f);
        }
        typedef float result_type;
    };
);

BOLT_FUNCTOR(gen_input_udd,
    struct gen_input_udd
    {
        UDD operator() ()  const 
       { 
            int i=get_global_id(0);
            UDD temp;
            temp.i = i;
            temp.f = (float)i;
            return temp; 
        }
        typedef int result_type;
    };
);

TEST( TransformIterator, UDDTest)
{
    {
        const int length = 10;
        std::vector< UDD > svInVec( length );
        std::vector< UDD > svOutVec( length );
        bolt::BCKND::device_vector< UDD > dvInVec( length );
        bolt::BCKND::device_vector< UDD > dvOutVec( length );

        squareUDD sqUDD;
        gen_input_udd genUDD;
        /*Type defintions*/
        typedef std::vector< UDD >::const_iterator                                                          sv_itr;
        typedef bolt::BCKND::device_vector< UDD >::iterator                                                 dv_itr;
        typedef bolt::BCKND::transform_iterator< squareUDD, std::vector< UDD >::const_iterator>             sv_trf_itr;
        typedef bolt::BCKND::transform_iterator< squareUDD, bolt::BCKND::device_vector< UDD >::iterator>    dv_trf_itr;
    
        /*Create Iterators*/
        sv_trf_itr sv_trf_begin (svInVec.begin(), sqUDD), sv_trf_end (svInVec.begin(), sqUDD);

        dv_trf_itr dv_trf_begin (dvInVec.begin(), sqUDD), dv_trf_end (dvInVec.begin(), sqUDD);
 
        /*Generate inputs*/
        std::generate(svInVec.begin(), svInVec.end(), genUDD);
        bolt::BCKND::generate(dvInVec.begin(), dvInVec.end(), genUDD);

        int dist1 = static_cast< int >(std::distance(sv_trf_begin, sv_trf_end));
        int dist2 = static_cast< int >(std::distance( dv_trf_begin, dv_trf_end ));

        EXPECT_EQ( dist1, dist2 );
        //std::cout << "distance = " << dist1 << "\n" ;

        for(int i =0; i< length; i++)
        {
            float temp1, temp2; //Return type of the unary function is a float
            temp1 = (float)*sv_trf_begin++;
            temp2 = (float)*dv_trf_begin++;
            EXPECT_FLOAT_EQ( temp1, temp2 );
            //std::cout << temp1 << "   " << temp2 << "\n";
        }
        global_id = 0; // Reset the global id counter
    }
    
}


std::string temp_str = BOLT_CODE_STRING(typedef bolt::cl::transform_iterator< square, bolt::cl::device_vector< int >::iterator > trf_sq_itr;);
BOLT_CREATE_TYPENAME( trf_sq_itr );
BOLT_CREATE_CLCODE  ( trf_sq_itr, ClCode<square>::get() + bolt::cl::deviceTransformIteratorTemplate + temp_str);

//#define BOLT_CREATE_TYPENAME( Type ) \
//    template<> struct TypeName< Type > { static std::string get( ) { return #Type; } };
TEST( TransformIterator, UnaryTransformRoutine)
{
    {
        const int length = 100;
        std::vector< int > svInVec( length );
        std::vector< int > svOutVec( length );
        std::vector< int > stlOut( length );
        bolt::BCKND::device_vector< int > dvInVec( length );
        bolt::BCKND::device_vector< int > dvOutVec( length );

        add_3 add3;
        square sq;
        gen_input gen;
        typedef std::vector< int >::const_iterator                                                          sv_itr;
        typedef bolt::BCKND::device_vector< int >::iterator                                                 dv_itr;
        typedef bolt::BCKND::transform_iterator< square, std::vector< int >::const_iterator>                sv_trf_itr;
        typedef bolt::BCKND::transform_iterator< square, bolt::BCKND::device_vector< int >::iterator>       dv_trf_itr;
    
        /*Create Iterators*/
        sv_trf_itr sv_trf_begin (svInVec.begin(), sq), sv_trf_end (svInVec.end(), sq);
        dv_trf_itr dv_trf_begin (dvInVec.begin(), sq), dv_trf_end (dvInVec.end(), sq);
    
        /*Generate inputs*/
        std::generate(svInVec.begin(), svInVec.end(), gen);
        global_id = 0;
        bolt::BCKND::generate(dvInVec.begin(), dvInVec.end(), gen);

        bolt::cl::transform(sv_trf_begin, sv_trf_end, svOutVec.begin(), add3);
        bolt::cl::transform(dv_trf_begin, dv_trf_end, dvOutVec.begin(), add3);
        ///*Compute expected results*/
        std::transform(sv_trf_begin, sv_trf_end, stlOut.begin(), add3);
        ///*Check the results*/
        cmpArrays(svOutVec, stlOut, length);
        cmpArrays(svOutVec, stlOut, length);
        global_id = 0; // Reset the global id counter
    }
}

std::string temp_str_1 = BOLT_CODE_STRING(typedef bolt::cl::transform_iterator< add_3, bolt::cl::device_vector< int >::iterator > trf_add_3_itr;);
BOLT_CREATE_TYPENAME( trf_add_3_itr );
BOLT_CREATE_CLCODE  ( trf_add_3_itr, ClCode<add_3>::get() + bolt::cl::deviceTransformIteratorTemplate + temp_str_1);

std::string temp_str_2 = BOLT_CODE_STRING(typedef bolt::cl::transform_iterator< add_4, bolt::cl::device_vector< int >::iterator > trf_add_4_itr;);
BOLT_CREATE_TYPENAME( trf_add_4_itr );
BOLT_CREATE_CLCODE  ( trf_add_4_itr, ClCode<add_4>::get() + bolt::cl::deviceTransformIteratorTemplate + temp_str_2);

TEST( TransformIterator, BinaryTransformRoutine)
{
    {
        const int length = 100;
        std::vector< int > svIn1Vec( length );
        std::vector< int > svIn2Vec( length );
        std::vector< int > svOutVec( length );
        std::vector< int > stlOut( length );
        bolt::BCKND::device_vector< int > dvIn1Vec( length );
        bolt::BCKND::device_vector< int > dvIn2Vec( length );
        bolt::BCKND::device_vector< int > dvOutVec( length );

        add_3 add3;
        add_4 add4;
        bolt::cl::plus<int> plus;
        gen_input gen;
        typedef std::vector< int >::const_iterator                                                         sv_itr;
        typedef bolt::BCKND::device_vector< int >::iterator                                                dv_itr;
        typedef bolt::BCKND::transform_iterator< add_3, std::vector< int >::const_iterator>                sv_trf_itr_add3;
        typedef bolt::BCKND::transform_iterator< add_3, bolt::BCKND::device_vector< int >::iterator>       dv_trf_itr_add3;
        typedef bolt::BCKND::transform_iterator< add_4, std::vector< int >::const_iterator>                sv_trf_itr_add4;
        typedef bolt::BCKND::transform_iterator< add_4, bolt::BCKND::device_vector< int >::iterator>       dv_trf_itr_add4;    
        /*Create Iterators*/
        sv_trf_itr_add3 sv_trf_begin1 (svIn1Vec.begin(), add3), sv_trf_end1 (svIn1Vec.end(), add3);
        sv_trf_itr_add4 sv_trf_begin2 (svIn2Vec.begin(), add4);
        dv_trf_itr_add3 dv_trf_begin1 (dvIn1Vec.begin(), add3), dv_trf_end1 (dvIn1Vec.end(), add3);
        dv_trf_itr_add4 dv_trf_begin2 (dvIn2Vec.begin(), add4);

        /*Generate inputs*/
        global_id = 0;
        std::generate(svIn1Vec.begin(), svIn1Vec.end(), gen);
        std::generate(svIn2Vec.begin(), svIn2Vec.end(), gen);
        global_id = 0;
        bolt::BCKND::generate(dvIn1Vec.begin(), dvIn1Vec.end(), gen);
        bolt::BCKND::generate(dvIn2Vec.begin(), dvIn2Vec.end(), gen);

        //bolt::cl::transform(sv_trf_begin1, sv_trf_end1, sv_trf_begin2, svOutVec.begin(), plus);
        //bolt::cl::transform(dv_trf_begin1, dv_trf_end1, dv_trf_begin2, dvOutVec.begin(), plus);
        /*Compute expected results*/
        std::transform(sv_trf_begin1, sv_trf_end1, sv_trf_begin2, stlOut.begin(), plus);
        /*Check the results*/
        //cmpArrays(svOutVec, stlOut, length);
        //cmpArrays(svOutVec, stlOut, length);
        global_id = 0; // Reset the global id counter
    }
}

//TODO add test cases for binary transform when one of the inputs is a transform iterator 
//     and the other is a device_vector iterator
//TODO add test cases for binary transform when one of the inputs is a transform iterator 
//     and the other is a constant iterator
//TODO add test cases for binary transform when one of the inputs is a transform iterator 
//     and the other is a counting iterator

/* /brief List of possible tests
 * Two input transform with first input a constant iterator
 * One input transform with a constant iterator
*/
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
    bolt::cl::V_OPENCL( platforms.front( ).getDevices( CL_DEVICE_TYPE_ALL, &devices ), "Platform::getDevices() failed" );

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
