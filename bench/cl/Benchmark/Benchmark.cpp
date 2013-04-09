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

/******************************************************************************
 *  Benchmark Bolt Functions
 *****************************************************************************/


#define BOLT_PROFILER_ENABLED
#define BOLT_BENCH_DEVICE_VECTOR_FLAGS CL_MEM_READ_WRITE

#include "bolt/AsyncProfiler.h"
AsyncProfiler aProfiler("default");

/******************************************************************************
 *  Functions Enumerated
 *****************************************************************************/
enum functionType {
    f_generate,
    f_copy,
    f_unary_transform,
    f_binary_transform,
    f_scan,
    f_transform_scan,
    f_scan_by_key
};
static char *functionNames[] = {
    "Generate",
    "Copy",
    "UnaryTransform",
    "BinaryTransform",
    "Scan",
    "TransformScan",
    "ScanByKey"
};

/******************************************************************************
 *  Data Types Enumerated
 *****************************************************************************/
enum dataType {
    t_int,
    t_vec2,
    t_vec4,
    t_vec8
};
static char *dataTypeNames[] = {
    "int1",
    "vec2",
    "vec4",
    "vec8"
};


#include "bolt/cl/functional.h"
#include "bolt/cl/device_vector.h"
#include "bolt/cl/generate.h"
#include "bolt/cl/copy.h"
#include "bolt/cl/transform.h"
#include "bolt/cl/scan.h"
#include "bolt/cl/transform_scan.h"
#include "bolt/cl/scan_by_key.h"

#include <fstream>
#include <vector>
#include <tchar.h>
#include <algorithm>
#include <iomanip>
#include "bolt/unicode.h"
#include "bolt/countof.h"
#include <boost/program_options.hpp>
namespace po = boost::program_options;

/******************************************************************************
 *  User Defined Data Types - vec2,4,8
 *****************************************************************************/

BOLT_FUNCTOR(vec2,
struct vec2
{
    int a, b;

    bool operator==(const vec2& rhs) const
    {
        bool l_equal = true;
        l_equal = ( a == rhs.a ) ? l_equal : false;
        l_equal = ( b == rhs.b ) ? l_equal : false;
        return l_equal;
    }
};
);

BOLT_CREATE_TYPENAME( bolt::cl::device_vector< vec2 >::iterator );
BOLT_CREATE_CLCODE( bolt::cl::device_vector< vec2 >::iterator, bolt::cl::deviceVectorIteratorTemplate );

BOLT_FUNCTOR(vec4,
struct vec4
{
    int a, b, c, d;

    bool operator==(const vec4& rhs) const
    {
        bool l_equal = true;
        l_equal = ( a == rhs.a ) ? l_equal : false;
        l_equal = ( b == rhs.b ) ? l_equal : false;
        l_equal = ( c == rhs.c ) ? l_equal : false;
        l_equal = ( d == rhs.d ) ? l_equal : false;
        return l_equal;
    }
};
);

BOLT_CREATE_TYPENAME( bolt::cl::device_vector< vec4 >::iterator );
BOLT_CREATE_CLCODE( bolt::cl::device_vector< vec4 >::iterator, bolt::cl::deviceVectorIteratorTemplate );

BOLT_FUNCTOR(vec8,
struct vec8
{
    int a, b, c, d, e, f, g, h;

    bool operator==(const vec8& rhs) const
    {
        bool l_equal = true;
        l_equal = ( a == rhs.a ) ? l_equal : false;
        l_equal = ( b == rhs.b ) ? l_equal : false;
        l_equal = ( c == rhs.c ) ? l_equal : false;
        l_equal = ( d == rhs.d ) ? l_equal : false;
        l_equal = ( e == rhs.e ) ? l_equal : false;
        l_equal = ( f == rhs.f ) ? l_equal : false;
        l_equal = ( g == rhs.g ) ? l_equal : false;
        l_equal = ( h == rhs.h ) ? l_equal : false;
        return l_equal;
    }
};
);

BOLT_CREATE_TYPENAME( bolt::cl::device_vector< vec8 >::iterator );
BOLT_CREATE_CLCODE( bolt::cl::device_vector< vec8 >::iterator, bolt::cl::deviceVectorIteratorTemplate );

/******************************************************************************
 *  User Defined Binary Functions - vec2,4,8plus
 *****************************************************************************/

BOLT_FUNCTOR(vec2plus,
struct vec2plus
{
    vec2 operator()(const vec2 &lhs, const vec2 &rhs) const
    {
        vec2 l_result;
        l_result.a = lhs.a+rhs.a;
        l_result.b = lhs.b+rhs.b;
        return l_result;
    };
}; 
);

BOLT_FUNCTOR(vec4plus,
struct vec4plus
{
    vec4 operator()(const vec4 &lhs, const vec4 &rhs) const
    {
        vec4 l_result;
        l_result.a = lhs.a+rhs.a;
        l_result.b = lhs.b+rhs.b;
        l_result.c = lhs.c+rhs.c;
        l_result.d = lhs.d+rhs.d;
        return l_result;
    };
}; 
);

BOLT_FUNCTOR(vec8plus,
struct vec8plus
{
    vec8 operator()(const vec8 &lhs, const vec8 &rhs) const
    {
        vec8 l_result;
        l_result.a = lhs.a+rhs.a;
        l_result.b = lhs.b+rhs.b;
        l_result.c = lhs.c+rhs.c;
        l_result.d = lhs.d+rhs.d;
        l_result.e = lhs.e+rhs.e;
        l_result.f = lhs.f+rhs.f;
        l_result.g = lhs.g+rhs.g;
        l_result.h = lhs.h+rhs.h;
        return l_result;
    };
}; 
);


/******************************************************************************
 *  User Defined Unary Functions vec2,4,8square
 *****************************************************************************/

BOLT_FUNCTOR(vec2square,
struct vec2square
{
    vec2 operator()(const vec2 &rhs) const
    {
        vec2 l_result;
        l_result.a = rhs.a*rhs.a;
        l_result.b = rhs.b*rhs.b;
        return l_result;
    };
}; 
);

BOLT_FUNCTOR(vec4square,
struct vec4square
{
    vec4 operator()(const vec4 &rhs) const
    {
        vec4 l_result;
        l_result.a = rhs.a*rhs.a;
        l_result.b = rhs.b*rhs.b;
        l_result.c = rhs.c*rhs.c;
        l_result.d = rhs.d*rhs.d;
        return l_result;
    };
}; 
);

BOLT_FUNCTOR(vec8square,
struct vec8square
{
    vec8 operator()(const vec8 &rhs) const
    {
        vec8 l_result;
        l_result.a = rhs.a*rhs.a;
        l_result.b = rhs.b*rhs.b;
        l_result.c = rhs.c*rhs.c;
        l_result.d = rhs.d*rhs.d;
        l_result.e = rhs.e*rhs.e;
        l_result.f = rhs.f*rhs.f;
        l_result.g = rhs.g*rhs.g;
        l_result.h = rhs.h*rhs.h;
        return l_result;
    };
}; 
);

/******************************************************************************
 *  User Defined Binary Predicates equal
 *****************************************************************************/

BOLT_FUNCTOR(vec2equal,
struct vec2equal
{
    bool operator()(const vec2 &lhs, const vec2 &rhs) const
    {
        return lhs == rhs;
    };
}; 
);

BOLT_FUNCTOR(vec4equal,
struct vec4equal
{
    bool operator()(const vec4 &lhs, const vec4 &rhs) const
    {
        return lhs == rhs;
    };
}; 
);

BOLT_FUNCTOR(vec8equal,
struct vec8equal
{
    bool operator()(const vec8 &lhs, const vec8 &rhs) const
    {
        return lhs == rhs;
    };
}; 
);

/******************************************************************************
 *  User Defined Binary Predicates less than
 *****************************************************************************/

BOLT_FUNCTOR(vec2less,
struct vec2less
{
    bool operator()(const vec2 &lhs, const vec2 &rhs) const
    {
        if (lhs.a < rhs.a) return true;
        if (lhs.b < rhs.b) return true;
        return false;
    };
}; 
);

BOLT_FUNCTOR(vec4less,
struct vec4less
{
    bool operator()(const vec4 &lhs, const vec4 &rhs) const
    {
        if (lhs.a < rhs.a) return true;
        if (lhs.b < rhs.b) return true;
        if (lhs.c < rhs.c) return true;
        if (lhs.d < rhs.d) return true;
        return false;
    };
}; 
);

BOLT_FUNCTOR(vec8less,
struct vec8less
{
    bool operator()(const vec8 &lhs, const vec8 &rhs) const
    {
        if (lhs.a < rhs.a) return true;
        if (lhs.b < rhs.b) return true;
        if (lhs.c < rhs.c) return true;
        if (lhs.d < rhs.d) return true;
        if (lhs.e < rhs.e) return true;
        if (lhs.f < rhs.f) return true;
        if (lhs.g < rhs.g) return true;
        if (lhs.h < rhs.h) return true;
        return false;
    };
}; 
);

/******************************************************************************
 *  User Defined Binary Predicates vec2,4,8square
 *****************************************************************************/

BOLT_FUNCTOR(intgen,
struct intgen
{
    int operator()() const
    {
        int v = 1;
        return v;
    };
}; 
);

BOLT_FUNCTOR(vec2gen,
struct vec2gen
{
    vec2 operator()() const
    {
        vec2 v = { 2, 3 };
        return v;
    };
}; 
);

BOLT_FUNCTOR(vec4gen,
struct vec4gen
{
    vec4 operator()() const
    {
        vec4 v = { 4, 5, 6, 7 };
        return v;
    };
}; 
);

BOLT_FUNCTOR(vec8gen,
struct vec8gen
{
    vec8 operator()() const
    {
        vec8 v = { 8, 9, 10, 11, 12, 13, 14, 15 };
        return v;
    };
}; 
);

/******************************************************************************
 *  Initializers
 *****************************************************************************/
vec2 v2init = { 1, 1 };
vec2 v2iden = { 0, 0 };
vec4 v4init = { 1, 1, 1, 1 };
vec4 v4iden = { 0, 0, 0, 0 };
vec8 v8init = { 1, 1, 1, 1, 1, 1, 1, 1 };
vec8 v8iden = { 0, 0, 0, 0, 0, 0, 0, 0 };



/******************************************************************************
 *
 *  Execute Function Type
 *
 *****************************************************************************/
template<
    typename VectorType,
    typename Generator,
    typename UnaryFunction,
    typename BinaryFunction,
    typename BinaryPredEq,
    typename BinaryPredLt >
void executeFunctionType(
    bolt::cl::control& ctrl,
    VectorType input1,
    VectorType input2,
    VectorType input3,
    VectorType output,
    Generator generator,
    UnaryFunction unaryFunct,
    BinaryFunction binaryFunct,
    BinaryPredEq binaryPredEq,
    BinaryPredLt binaryPredLt,
    size_t function,
    size_t iterations
    )
{
    for (size_t iter = 0; iter < iterations+1; iter++)
    {
        switch(function)
        {
            
        case f_generate: // generate
            std::cerr << "(" << iter << ") " << functionNames[f_generate] << std::endl;
            bolt::cl::generate(
                ctrl, input1.begin(), input1.end(), generator );
            break;

        case f_copy: // copy
            std::cerr << "(" << iter << ") " << functionNames[f_copy] << std::endl;
            bolt::cl::copy(
                ctrl, input1.begin(), input1.end(), output.begin() );
            break;

        case f_unary_transform: // unary transform
            std::cerr << "(" << iter << ") " << functionNames[f_unary_transform] << std::endl;
            bolt::cl::transform(
                ctrl, input1.begin(), input1.end(), output.begin(), unaryFunct );
            break;

        case f_binary_transform: // binary transform
            std::cerr << "(" << iter << ") " << functionNames[f_binary_transform] << std::endl;
            bolt::cl::transform(
                ctrl, input1.begin(), input1.end(), input2.begin(), output.begin(), binaryFunct );
            break;

        case f_scan: // scan
            std::cerr << "(" << iter << ") " << functionNames[f_scan] << std::endl;
            bolt::cl::inclusive_scan(
                ctrl, input1.begin(), input1.end(), output.begin(), binaryFunct );
            break;

        case f_transform_scan: // transform_scan
            std::cerr << "(" << iter << ") " << functionNames[f_transform_scan] << std::endl;
            bolt::cl::transform_inclusive_scan(
                ctrl, input1.begin(), input1.end(), output.begin(), unaryFunct, binaryFunct );
            break;

        case f_scan_by_key: // scan_by_key
            std::cerr << "(" << iter << ") " << functionNames[f_scan_by_key] << std::endl;
            bolt::cl::inclusive_scan_by_key(
                ctrl, input1.begin(), input1.end(), input2.begin(), output.begin(), binaryPredEq, binaryFunct );
            break;
        
        default:
            //std::cerr << "Unsupported function=" << function << std::endl;
            iter = iterations; // skip to end
            break;
        } // switch
    } // for iterations
}


/******************************************************************************
 *
 *  Determine types
 *
 *****************************************************************************/
void executeFunction(
    bolt::cl::control& ctrl,
    size_t vecType,
    bool hostMemory,
    size_t length,
    size_t routine,
    size_t iterations )
{
    if (vecType == t_int)
    {
        intgen                  generator;
        bolt::cl::square<int>   unaryFunct;
        bolt::cl::plus<int>     binaryFunct;
        bolt::cl::equal_to<int> binaryPredEq;
        bolt::cl::less<int>     binaryPredLt;

        if (hostMemory) {
            std::vector<int> input1(length, 1);
            std::vector<int> input2(length, 1);
            std::vector<int> input3(length, 1);
            std::vector<int> output(length, 0);
            executeFunctionType( ctrl, input1, input2, input3, output,
                generator, unaryFunct, binaryFunct, binaryPredEq, binaryPredLt, routine, iterations);
        }
        else
        {
            bolt::cl::device_vector<int> input1(length, 1, BOLT_BENCH_DEVICE_VECTOR_FLAGS, true, ctrl);
            bolt::cl::device_vector<int> input2(length, 1, BOLT_BENCH_DEVICE_VECTOR_FLAGS, true, ctrl);
            bolt::cl::device_vector<int> input3(length, 1, BOLT_BENCH_DEVICE_VECTOR_FLAGS, true, ctrl);
            bolt::cl::device_vector<int> output(length, 0, BOLT_BENCH_DEVICE_VECTOR_FLAGS, true, ctrl);
            executeFunctionType( ctrl, input1, input2, input3, output,
                generator, unaryFunct, binaryFunct, binaryPredEq, binaryPredLt, routine, iterations);
        }
    }
    else if (vecType == t_vec2)
    {
        vec2gen     generator;
        vec2square  unaryFunct;
        vec2plus    binaryFunct;
        vec2equal   binaryPredEq;
        vec2less    binaryPredLt;

        if (hostMemory) {
            std::vector<vec2> input1(length, v2init);
            std::vector<vec2> input2(length, v2init);
            std::vector<vec2> input3(length, v2init);
            std::vector<vec2> output(length, v2iden);
            executeFunctionType( ctrl, input1, input2, input3, output,
                generator, unaryFunct, binaryFunct, binaryPredEq, binaryPredLt, routine, iterations);
        }
        else
        {
            bolt::cl::device_vector<vec2> input1(length, v2init, BOLT_BENCH_DEVICE_VECTOR_FLAGS, true, ctrl);
            bolt::cl::device_vector<vec2> input2(length, v2init, BOLT_BENCH_DEVICE_VECTOR_FLAGS, true, ctrl);
            bolt::cl::device_vector<vec2> input3(length, v2init, BOLT_BENCH_DEVICE_VECTOR_FLAGS, true, ctrl);
            bolt::cl::device_vector<vec2> output(length, v2iden, BOLT_BENCH_DEVICE_VECTOR_FLAGS, true, ctrl);
            executeFunctionType( ctrl, input1, input2, input3, output,
                generator, unaryFunct, binaryFunct, binaryPredEq, binaryPredLt, routine, iterations);
        }
    }
    else if (vecType == t_vec4)
    {
        vec4gen     generator;
        vec4square  unaryFunct;
        vec4plus    binaryFunct;
        vec4equal   binaryPredEq;
        vec4less    binaryPredLt;

        if (hostMemory) {
            std::vector<vec4> input1(length, v4init);
            std::vector<vec4> input2(length, v4init);
            std::vector<vec4> input3(length, v4init);
            std::vector<vec4> output(length, v4iden);
            executeFunctionType( ctrl, input1, input2, input3, output,
                generator, unaryFunct, binaryFunct, binaryPredEq, binaryPredLt, routine, iterations);
        }
        else
        {
            bolt::cl::device_vector<vec4> input1(length, v4init, BOLT_BENCH_DEVICE_VECTOR_FLAGS, true, ctrl);
            bolt::cl::device_vector<vec4> input2(length, v4init, BOLT_BENCH_DEVICE_VECTOR_FLAGS, true, ctrl);
            bolt::cl::device_vector<vec4> input3(length, v4init, BOLT_BENCH_DEVICE_VECTOR_FLAGS, true, ctrl);
            bolt::cl::device_vector<vec4> output(length, v4iden, BOLT_BENCH_DEVICE_VECTOR_FLAGS, true, ctrl);
            executeFunctionType( ctrl, input1, input2, input3, output,
                generator, unaryFunct, binaryFunct, binaryPredEq, binaryPredLt, routine, iterations);
        }
    }
    else if (vecType == t_vec8)
    {
        vec8gen     generator;
        vec8square  unaryFunct;
        vec8plus    binaryFunct;
        vec8equal   binaryPredEq;
        vec8less    binaryPredLt;

        if (hostMemory) {
            std::vector<vec8> input1(length, v8init);
            std::vector<vec8> input2(length, v8init);
            std::vector<vec8> input3(length, v8init);
            std::vector<vec8> output(length, v8iden);
            executeFunctionType( ctrl, input1, input2, input3, output,
                generator, unaryFunct, binaryFunct, binaryPredEq, binaryPredLt, routine, iterations);
        }
        else
        {
            bolt::cl::device_vector<vec8> input1(length, v8init, BOLT_BENCH_DEVICE_VECTOR_FLAGS, true, ctrl);
            bolt::cl::device_vector<vec8> input2(length, v8init, BOLT_BENCH_DEVICE_VECTOR_FLAGS, true, ctrl);
            bolt::cl::device_vector<vec8> input3(length, v8init, BOLT_BENCH_DEVICE_VECTOR_FLAGS, true, ctrl);
            bolt::cl::device_vector<vec8> output(length, v8iden, BOLT_BENCH_DEVICE_VECTOR_FLAGS, true, ctrl);
            executeFunctionType( ctrl, input1, input2, input3, output,
                generator, unaryFunct, binaryFunct, binaryPredEq, binaryPredLt, routine, iterations);
        }
    }
    else
    {
        std::cerr << "Unsupported vecType=" << vecType << std::endl;
    }

}


/******************************************************************************
 *
 *  Main
 *
 *****************************************************************************/
int _tmain( int argc, _TCHAR* argv[] )
{
    cl_int err = CL_SUCCESS;
    cl_uint userPlatform    = 0;
    cl_uint userDevice      = 0;
    size_t iterations       = 10;
    size_t length           = 1<<26;
    size_t vecType          = 1;
    size_t runMode          = 0;
    size_t routine          = f_scan;
    size_t numThrowAway     = 10;
    std::string filename    = "bench.xml";
    cl_device_type deviceType = CL_DEVICE_TYPE_DEFAULT;
    bool defaultDevice      = true;
    bool print_clInfo       = false;
    bool hostMemory         = true;

    /******************************************************************************
     * Parse Command-line Parameters
     ******************************************************************************/
    try
    {
        // Declare the supported options.
        po::options_description desc( "OpenCL Scan command line options" );
        desc.add_options()
            ( "help,h",			"produces this help message" )
            ( "version,v",		"Print queryable version information from the Bolt CL library" )
            ( "queryOpenCL,q",  "Print queryable platform and device info and return" )
            ( "gpu,g",          "Report only OpenCL GPU devices" )
            ( "cpu,c",          "Report only OpenCL CPU devices" )
            ( "all,a",          "Report all OpenCL devices" )
            ( "deviceMemory",   "Allocate vectors in device memory; default is host memory" )
            ( "platform,p",     po::value< cl_uint >( &userPlatform )->default_value( userPlatform ),
                "Specify the platform under test using the index reported by -q flag" )
            ( "device,d",       po::value< cl_uint >( &userDevice )->default_value( userDevice ),
                "Specify the device under test using the index reported by the -q flag.  "
                "Index is relative with respect to -g, -c or -a flags" )
            ( "length,l",       po::value< size_t >( &length )->default_value( length ),
                "Length of scan array" )
            ( "iterations,i",   po::value< size_t >( &iterations )->default_value( iterations ),
                "Number of samples in timing loop" )
            ( "vecType,t",      po::value< size_t >( &vecType )->default_value( vecType ),
                "Data Type to use: 1-int, 2-int2, 4-int4, 8-int8" )
            ( "runMode,m",      po::value< size_t >( &runMode )->default_value( runMode ),
                "Run Mode: 0-Auto, 1-SerialCPU, 2-MultiCoreCPU, 3-GPU" )
            ( "function",      po::value< size_t >( &routine )->default_value( routine ),
                "Number of samples in timing loop" )
            ( "filename",     po::value< std::string >( &filename )->default_value( filename ),
                "Name of output file" )
            ( "throw-away",   po::value< size_t >( &numThrowAway )->default_value( numThrowAway ),
                "Number of trials to skip averaging" )
            ;

        po::variables_map vm;
        po::store( po::parse_command_line( argc, argv, desc ), vm );
        po::notify( vm );

        if( vm.count( "version" ) )
        {
            cl_uint libMajor, libMinor, libPatch;
            bolt::cl::getVersion( libMajor, libMinor, libPatch );

            const int indent = countOf( "Bolt version: " );
            bolt::tout << std::left << std::setw( indent ) << _T( "Bolt version: " )
                << libMajor << _T( "." )
                << libMinor << _T( "." )
                << libPatch << std::endl;
        }

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

        if( vm.count( "deviceMemory" ) )
        {
            hostMemory = false;
        }
    }
    catch( std::exception& e )
    {
        std::cout << _T( "Scan Benchmark error condition reported:" ) << std::endl << e.what() << std::endl;
        return 1;
    }

    /******************************************************************************
     * Initialize platforms and devices
     ******************************************************************************/
    aProfiler.throwAway( numThrowAway );
    bolt::cl::control ctrl = bolt::cl::control::getDefault();
    
    std::string strDeviceName;
    if (runMode == 1) // serial cpu
    {
        ctrl.setForceRunMode( bolt::cl::control::SerialCpu );
        strDeviceName = "Serial CPU";
    }
    else if (runMode == 2) // multicore cpu
    {
        ctrl.setForceRunMode( bolt::cl::control::MultiCoreCpu );
        strDeviceName = "MultiCore CPU";
    }
    else // gpu || automatic
    {
        // Platform vector contains all available platforms on system
        std::vector< ::cl::Platform > platforms;
        bolt::cl::V_OPENCL( ::cl::Platform::get( &platforms ), "Platform::get() failed" );
        if( print_clInfo ) return 0;

        // Device info
        ::cl::Context myContext = bolt::cl::control::getDefault( ).getContext( );
        std::vector< cl::Device > devices = myContext.getInfo< CL_CONTEXT_DEVICES >();
        ::cl::CommandQueue myQueue( myContext, devices.at( userDevice ) , CL_QUEUE_PROFILING_ENABLE);

        //  Now that the device we want is selected and we have created our own cl::CommandQueue, set it as the
        //  default cl::CommandQueue for the Bolt API
        ctrl.setCommandQueue( myQueue );

        strDeviceName = ctrl.getDevice( ).getInfo< CL_DEVICE_NAME >( &err );
        bolt::cl::V_OPENCL( err, "Device::getInfo< CL_DEVICE_NAME > failed" );
    }
    std::cout << "Device: " << strDeviceName << std::endl;

    /******************************************************************************
     * Select then Execute Function
     ******************************************************************************/
    executeFunction(
        ctrl,
        vecType,
        hostMemory,
        length,
        routine,
        iterations+numThrowAway
        );

    /******************************************************************************
     * Print Results
     ******************************************************************************/
    aProfiler.end();
    std::ofstream outFile( filename.c_str() );
    aProfiler.writeSum( outFile );
    outFile.close();
    return 0;
}