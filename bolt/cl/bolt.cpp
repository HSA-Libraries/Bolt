/***************************************************************************                                                                                     
*   Copyright 2012 Advanced Micro Devices, Inc.                                     
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


#include <iostream>
#include <fstream>
#include <streambuf>
#include <direct.h>  //windows CWD for error message
#include <tchar.h>
#include <algorithm>
#include <vector>

#include "bolt/cl/bolt.h"
#include "bolt/unicode.h"

//  Include all kernel string objects
#include "bolt/generate_kernels.hpp"
#include "bolt/reduce_kernels.hpp"
#include "bolt/scan_kernels.hpp"
#include "bolt/sort_kernels.hpp"
#include "bolt/transform_kernels.hpp"
#include "bolt/transform_reduce_kernels.hpp"

namespace bolt {
    namespace cl {

    void getVersion( cl_uint& major, cl_uint& minor, cl_uint& patch )
    {
        major	= BoltVersionMajor;
        minor	= BoltVersionMinor;
        patch	= BoltVersionPatch;
    }

    std::string clErrorStringA( const cl_int& status )
    {
        switch( status )
        {
            case CL_INVALID_DEVICE_PARTITION_COUNT:
                return "CL_INVALID_DEVICE_PARTITION_COUNT";
            case CL_INVALID_LINKER_OPTIONS:
                return "CL_INVALID_LINKER_OPTIONS";
            case CL_INVALID_COMPILER_OPTIONS:
                return "CL_INVALID_COMPILER_OPTIONS";
            case CL_INVALID_IMAGE_DESCRIPTOR:
                return "CL_INVALID_IMAGE_DESCRIPTOR";
            case CL_INVALID_PROPERTY:
                return "CL_INVALID_PROPERTY";
            case CL_INVALID_GLOBAL_WORK_SIZE:
                return "CL_INVALID_GLOBAL_WORK_SIZE";
            case CL_INVALID_MIP_LEVEL:
                return "CL_INVALID_MIP_LEVEL";
            case CL_INVALID_BUFFER_SIZE:
                return "CL_INVALID_BUFFER_SIZE";
            case CL_INVALID_GL_OBJECT:
                return "CL_INVALID_GL_OBJECT";
            case CL_INVALID_OPERATION:
                return "CL_INVALID_OPERATION";
            case CL_INVALID_EVENT:
                return "CL_INVALID_EVENT";
            case CL_INVALID_EVENT_WAIT_LIST:
                return "CL_INVALID_EVENT_WAIT_LIST";
            case CL_INVALID_GLOBAL_OFFSET:
                return "CL_INVALID_GLOBAL_OFFSET";
            case CL_INVALID_WORK_ITEM_SIZE:
                return "CL_INVALID_WORK_ITEM_SIZE";
            case CL_INVALID_WORK_GROUP_SIZE:
                return "CL_INVALID_WORK_GROUP_SIZE";
            case CL_INVALID_WORK_DIMENSION:
                return "CL_INVALID_WORK_DIMENSION";
            case CL_INVALID_KERNEL_ARGS:
                return "CL_INVALID_KERNEL_ARGS";
            case CL_INVALID_ARG_SIZE:
                return "CL_INVALID_ARG_SIZE";
            case CL_INVALID_ARG_VALUE:
                return "CL_INVALID_ARG_VALUE";
            case CL_INVALID_ARG_INDEX:
                return "CL_INVALID_ARG_INDEX";
            case CL_INVALID_KERNEL:
                return "CL_INVALID_KERNEL";
            case CL_INVALID_KERNEL_DEFINITION:
                return "CL_INVALID_KERNEL_DEFINITION";
            case CL_INVALID_KERNEL_NAME:
                return "CL_INVALID_KERNEL_NAME";
            case CL_INVALID_PROGRAM_EXECUTABLE:
                return "CL_INVALID_PROGRAM_EXECUTABLE";
            case CL_INVALID_PROGRAM:
                return "CL_INVALID_PROGRAM";
            case CL_INVALID_BUILD_OPTIONS:
                return "CL_INVALID_BUILD_OPTIONS";
            case CL_INVALID_BINARY:
                return "CL_INVALID_BINARY";
            case CL_INVALID_SAMPLER:
                return "CL_INVALID_SAMPLER";
            case CL_INVALID_IMAGE_SIZE:
                return "CL_INVALID_IMAGE_SIZE";
            case CL_INVALID_IMAGE_FORMAT_DESCRIPTOR:
                return "CL_INVALID_IMAGE_FORMAT_DESCRIPTOR";
            case CL_INVALID_MEM_OBJECT:
                return "CL_INVALID_MEM_OBJECT";
            case CL_INVALID_HOST_PTR:
                return "CL_INVALID_HOST_PTR";
            case CL_INVALID_COMMAND_QUEUE:
                return "CL_INVALID_COMMAND_QUEUE";
            case CL_INVALID_QUEUE_PROPERTIES:
                return "CL_INVALID_QUEUE_PROPERTIES";
            case CL_INVALID_CONTEXT:
                return "CL_INVALID_CONTEXT";
            case CL_INVALID_DEVICE:
                return "CL_INVALID_DEVICE";
            case CL_INVALID_PLATFORM:
                return "CL_INVALID_PLATFORM";
            case CL_INVALID_DEVICE_TYPE:
                return "CL_INVALID_DEVICE_TYPE";
            case CL_INVALID_VALUE:
                return "CL_INVALID_VALUE";
            case CL_KERNEL_ARG_INFO_NOT_AVAILABLE:
                return "CL_KERNEL_ARG_INFO_NOT_AVAILABLE";
            case CL_DEVICE_PARTITION_FAILED:
                return "CL_DEVICE_PARTITION_FAILED";
            case CL_LINK_PROGRAM_FAILURE:
                return "CL_LINK_PROGRAM_FAILURE";
            case CL_LINKER_NOT_AVAILABLE:
                return "CL_LINKER_NOT_AVAILABLE";
            case CL_COMPILE_PROGRAM_FAILURE:
                return "CL_COMPILE_PROGRAM_FAILURE";
            case CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST:
                return "CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST";
            case CL_MISALIGNED_SUB_BUFFER_OFFSET:
                return "CL_MISALIGNED_SUB_BUFFER_OFFSET";
            case CL_MAP_FAILURE:
                return "CL_MAP_FAILURE";
            case CL_BUILD_PROGRAM_FAILURE:
                return "CL_BUILD_PROGRAM_FAILURE";
            case CL_IMAGE_FORMAT_NOT_SUPPORTED:
                return "CL_IMAGE_FORMAT_NOT_SUPPORTED";
            case CL_IMAGE_FORMAT_MISMATCH:
                return "CL_IMAGE_FORMAT_MISMATCH";
            case CL_MEM_COPY_OVERLAP:
                return "CL_MEM_COPY_OVERLAP";
            case CL_PROFILING_INFO_NOT_AVAILABLE:
                return "CL_PROFILING_INFO_NOT_AVAILABLE";
            case CL_OUT_OF_HOST_MEMORY:
                return "CL_OUT_OF_HOST_MEMORY";
            case CL_OUT_OF_RESOURCES:
                return "CL_OUT_OF_RESOURCES";
            case CL_MEM_OBJECT_ALLOCATION_FAILURE:
                return "CL_MEM_OBJECT_ALLOCATION_FAILURE";
            case CL_COMPILER_NOT_AVAILABLE:
                return "CL_COMPILER_NOT_AVAILABLE";
            case CL_DEVICE_NOT_AVAILABLE:
                return "CL_DEVICE_NOT_AVAILABLE";
            case CL_DEVICE_NOT_FOUND:
                return "CL_DEVICE_NOT_FOUND";
            case CL_SUCCESS:
                return "CL_SUCCESS";
            default:
                return "Error code not defined";
            break;
        }
    }

    std::wstring clErrorStringW( const cl_int& status )
    {
        switch( status )
        {
            case CL_INVALID_DEVICE_PARTITION_COUNT:
                return L"CL_INVALID_DEVICE_PARTITION_COUNT";
            case CL_INVALID_LINKER_OPTIONS:
                return L"CL_INVALID_LINKER_OPTIONS";
            case CL_INVALID_COMPILER_OPTIONS:
                return L"CL_INVALID_COMPILER_OPTIONS";
            case CL_INVALID_IMAGE_DESCRIPTOR:
                return L"CL_INVALID_IMAGE_DESCRIPTOR";
            case CL_INVALID_PROPERTY:
                return L"CL_INVALID_PROPERTY";
            case CL_INVALID_GLOBAL_WORK_SIZE:
                return L"CL_INVALID_GLOBAL_WORK_SIZE";
            case CL_INVALID_MIP_LEVEL:
                return L"CL_INVALID_MIP_LEVEL";
            case CL_INVALID_BUFFER_SIZE:
                return L"CL_INVALID_BUFFER_SIZE";
            case CL_INVALID_GL_OBJECT:
                return L"CL_INVALID_GL_OBJECT";
            case CL_INVALID_OPERATION:
                return L"CL_INVALID_OPERATION";
            case CL_INVALID_EVENT:
                return L"CL_INVALID_EVENT";
            case CL_INVALID_EVENT_WAIT_LIST:
                return L"CL_INVALID_EVENT_WAIT_LIST";
            case CL_INVALID_GLOBAL_OFFSET:
                return L"CL_INVALID_GLOBAL_OFFSET";
            case CL_INVALID_WORK_ITEM_SIZE:
                return L"CL_INVALID_WORK_ITEM_SIZE";
            case CL_INVALID_WORK_GROUP_SIZE:
                return L"CL_INVALID_WORK_GROUP_SIZE";
            case CL_INVALID_WORK_DIMENSION:
                return L"CL_INVALID_WORK_DIMENSION";
            case CL_INVALID_KERNEL_ARGS:
                return L"CL_INVALID_KERNEL_ARGS";
            case CL_INVALID_ARG_SIZE:
                return L"CL_INVALID_ARG_SIZE";
            case CL_INVALID_ARG_VALUE:
                return L"CL_INVALID_ARG_VALUE";
            case CL_INVALID_ARG_INDEX:
                return L"CL_INVALID_ARG_INDEX";
            case CL_INVALID_KERNEL:
                return L"CL_INVALID_KERNEL";
            case CL_INVALID_KERNEL_DEFINITION:
                return L"CL_INVALID_KERNEL_DEFINITION";
            case CL_INVALID_KERNEL_NAME:
                return L"CL_INVALID_KERNEL_NAME";
            case CL_INVALID_PROGRAM_EXECUTABLE:
                return L"CL_INVALID_PROGRAM_EXECUTABLE";
            case CL_INVALID_PROGRAM:
                return L"CL_INVALID_PROGRAM";
            case CL_INVALID_BUILD_OPTIONS:
                return L"CL_INVALID_BUILD_OPTIONS";
            case CL_INVALID_BINARY:
                return L"CL_INVALID_BINARY";
            case CL_INVALID_SAMPLER:
                return L"CL_INVALID_SAMPLER";
            case CL_INVALID_IMAGE_SIZE:
                return L"CL_INVALID_IMAGE_SIZE";
            case CL_INVALID_IMAGE_FORMAT_DESCRIPTOR:
                return L"CL_INVALID_IMAGE_FORMAT_DESCRIPTOR";
            case CL_INVALID_MEM_OBJECT:
                return L"CL_INVALID_MEM_OBJECT";
            case CL_INVALID_HOST_PTR:
                return L"CL_INVALID_HOST_PTR";
            case CL_INVALID_COMMAND_QUEUE:
                return L"CL_INVALID_COMMAND_QUEUE";
            case CL_INVALID_QUEUE_PROPERTIES:
                return L"CL_INVALID_QUEUE_PROPERTIES";
            case CL_INVALID_CONTEXT:
                return L"CL_INVALID_CONTEXT";
            case CL_INVALID_DEVICE:
                return L"CL_INVALID_DEVICE";
            case CL_INVALID_PLATFORM:
                return L"CL_INVALID_PLATFORM";
            case CL_INVALID_DEVICE_TYPE:
                return L"CL_INVALID_DEVICE_TYPE";
            case CL_INVALID_VALUE:
                return L"CL_INVALID_VALUE";
            case CL_KERNEL_ARG_INFO_NOT_AVAILABLE:
                return L"CL_KERNEL_ARG_INFO_NOT_AVAILABLE";
            case CL_DEVICE_PARTITION_FAILED:
                return L"CL_DEVICE_PARTITION_FAILED";
            case CL_LINK_PROGRAM_FAILURE:
                return L"CL_LINK_PROGRAM_FAILURE";
            case CL_LINKER_NOT_AVAILABLE:
                return L"CL_LINKER_NOT_AVAILABLE";
            case CL_COMPILE_PROGRAM_FAILURE:
                return L"CL_COMPILE_PROGRAM_FAILURE";
            case CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST:
                return L"CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST";
            case CL_MISALIGNED_SUB_BUFFER_OFFSET:
                return L"CL_MISALIGNED_SUB_BUFFER_OFFSET";
            case CL_MAP_FAILURE:
                return L"CL_MAP_FAILURE";
            case CL_BUILD_PROGRAM_FAILURE:
                return L"CL_BUILD_PROGRAM_FAILURE";
            case CL_IMAGE_FORMAT_NOT_SUPPORTED:
                return L"CL_IMAGE_FORMAT_NOT_SUPPORTED";
            case CL_IMAGE_FORMAT_MISMATCH:
                return L"CL_IMAGE_FORMAT_MISMATCH";
            case CL_MEM_COPY_OVERLAP:
                return L"CL_MEM_COPY_OVERLAP";
            case CL_PROFILING_INFO_NOT_AVAILABLE:
                return L"CL_PROFILING_INFO_NOT_AVAILABLE";
            case CL_OUT_OF_HOST_MEMORY:
                return L"CL_OUT_OF_HOST_MEMORY";
            case CL_OUT_OF_RESOURCES:
                return L"CL_OUT_OF_RESOURCES";
            case CL_MEM_OBJECT_ALLOCATION_FAILURE:
                return L"CL_MEM_OBJECT_ALLOCATION_FAILURE";
            case CL_COMPILER_NOT_AVAILABLE:
                return L"CL_COMPILER_NOT_AVAILABLE";
            case CL_DEVICE_NOT_AVAILABLE:
                return L"CL_DEVICE_NOT_AVAILABLE";
            case CL_DEVICE_NOT_FOUND:
                return L"CL_DEVICE_NOT_FOUND";
            case CL_SUCCESS:
                return L"CL_SUCCESS";
            default:
                return L"Error code not defined";
            break;
        }
    }

    std::string fileToString(const std::string &fileName)
    {
        std::ifstream infile (fileName);
        if (infile.fail() ) {
#if defined( _WIN32 )
            TCHAR osPath[ MAX_PATH ];

            //	If loading the .cl file fails from the specified path, then make a last ditch attempt (purely for convenience) to find the .cl file right to the executable,
            //	regardless of what the CWD is
            //	::GetModuleFileName( ) returns TCHAR's (and we define _UNICODE for windows); but the fileName string is char's, 
            //	so we needed to create an abstraction for string/wstring
            if( ::GetModuleFileName( NULL, osPath, MAX_PATH ) )
            {
                bolt::tstring thisPath( osPath );
                bolt::tstring::size_type pos = thisPath.find_last_of( _T( "\\" ) );

                bolt::tstring newPath;
                if( pos != bolt::tstring::npos )
                {
                    bolt::tstring exePath	= thisPath.substr( 0, pos + 1 );	// include the \ character

                    //	Narrow to wide conversion should always work, but beware of wide to narrow!
                    bolt::tstring convName( fileName.begin( ), fileName.end( ) );
                    newPath = exePath + convName;
                }

                infile.open( newPath.c_str( ) );
            }
#endif
            if (infile.fail() ) {
                TCHAR cCurrentPath[FILENAME_MAX];
                if (_tgetcwd(cCurrentPath, sizeof(cCurrentPath) / sizeof(TCHAR))) {
                    bolt::tout <<  _T( "CWD=" ) << cCurrentPath << std::endl;
                };
                std::cout << "error: failed to open file '" << fileName << std::endl;
                throw;
            } 
        }

        std::string str((std::istreambuf_iterator<char>(infile)),
            std::istreambuf_iterator<char>());
        return str;
    };

    ::cl::Kernel compileFunctor(const std::string &kernelCodeString, const std::string kernelName,  std::string compileOptions, const control &c)
    {

        if (c.debug() & control::debug::ShowCode) {
            std::cout << "debug: code=" << std::endl << kernelCodeString;
        }

        ::cl::Program mainProgram(c.context(), kernelCodeString, false);
        try
        {
            compileOptions += c.compileOptions();
            compileOptions += " -x clc++  -cl-std=CL1.2";

            if (c.compileForAllDevices()) {
                std::vector<::cl::Device> devices = c.context().getInfo<CL_CONTEXT_DEVICES>();

                mainProgram.build(devices, compileOptions.c_str());
            } else {
                std::vector<::cl::Device> devices;
                devices.push_back(c.device());
                mainProgram.build(devices, compileOptions.c_str());
            };

        } catch(::cl::Error e) {
            std::cout << "Code         :\n" << kernelCodeString << std::endl;

            std::vector<::cl::Device> devices = c.context().getInfo<CL_CONTEXT_DEVICES>();
            std::for_each(devices.begin(), devices.end(), [&] (::cl::Device &d) {
                if (mainProgram.getBuildInfo<CL_PROGRAM_BUILD_STATUS>(d) != CL_SUCCESS) {
                    std::cout << "Build status for device: " << d.getInfo<CL_DEVICE_NAME>() << "_"<< d.getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>() << "CU_"<< d.getInfo<CL_DEVICE_MAX_CLOCK_FREQUENCY>() << "Mhz" << "\n";
                    std::cout << "    Build Status: " << mainProgram.getBuildInfo<CL_PROGRAM_BUILD_STATUS>(d) << std::endl;
                    std::cout << "    Build Options:\t" << mainProgram.getBuildInfo<CL_PROGRAM_BUILD_OPTIONS>(d) << std::endl;
                    std::cout << "    Build Log:\n " << mainProgram.getBuildInfo<CL_PROGRAM_BUILD_LOG>(d) << std::endl;
                }
            });
            throw;
        }

        if ( c.debug() & control::debug::Compile) {
            std::vector<::cl::Device> devices = c.context().getInfo<CL_CONTEXT_DEVICES>();
            std::for_each(devices.begin(), devices.end(), [&] (::cl::Device &d) {
                std::cout << "debug: Build status for device: " << d.getInfo<CL_DEVICE_NAME>() << "_"<< d.getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>() << "CU_"<< d.getInfo<CL_DEVICE_MAX_CLOCK_FREQUENCY>() << "Mhz" << "\n";
                std::cout << "debug:   Build Status: " << mainProgram.getBuildInfo<CL_PROGRAM_BUILD_STATUS>(d) << std::endl;
                std::cout << "debug:   Build Options:\t" << mainProgram.getBuildInfo<CL_PROGRAM_BUILD_OPTIONS>(d) << std::endl;
                std::cout << "debug:   Build Log:\n " << mainProgram.getBuildInfo<CL_PROGRAM_BUILD_LOG>(d) << std::endl;
            }); 
        };

        return ::cl::Kernel(mainProgram, kernelName.c_str());
    }

    void constructAndCompile(::cl::Kernel *masterKernel, const std::string &apiName, const std::string instantiationString, std::string userCode, std::string valueTypeName,  std::string functorTypeName, const control &c) 
    {
        std::string templateFunctionString = bolt::cl::fileToString( apiName + "_kernels.cl"); 

        std::string codeStr = userCode + "\n\n" + templateFunctionString +   instantiationString;

        std::string compileOptions = "";
        if (c.debug() & control::debug::SaveCompilerTemps) {
            compileOptions += "-save-temps=BOLT";
        }

        if (c.debug() & control::debug::Compile) {
            std::cout << "debug: compiling algorithm: '" << apiName << "' with valueType='" << valueTypeName << "'" << " ;  functorType='" << functorTypeName << "'" << std::endl;
        }

        *masterKernel = bolt::cl::compileFunctor(codeStr, apiName + "Instantiated", compileOptions, c);
    };

    ::cl::Program buildProgram( const std::string& kernelCodeString, std::string compileOptions, const control& ctl )
    {
        if( ctl.debug() & control::debug::ShowCode )
        {
            std::cout << "debug: code=" << std::endl << kernelCodeString;
        }

        ::cl::Program mainProgram( ctl.context( ), kernelCodeString, false );
        try
        {
            compileOptions += ctl.compileOptions( );
            compileOptions += " -x clc++";

            if( ctl.compileForAllDevices( ) )
            {
                std::vector<::cl::Device> devices = ctl.context( ).getInfo<CL_CONTEXT_DEVICES>( );

                mainProgram.build( devices, compileOptions.c_str( ) );
            } 
            else
            {
                std::vector<::cl::Device> devices;
                devices.push_back( ctl.device( ) );
                mainProgram.build( devices, compileOptions.c_str( ) );
            };
        } 
        catch( const ::cl::Error& )
        {
            std::cout << "Code:\n---\n" << kernelCodeString << "\n----\n";

            std::vector<::cl::Device> devices = ctl.context( ).getInfo<CL_CONTEXT_DEVICES>( );
            std::for_each( devices.begin( ), devices.end( ), [&]( ::cl::Device &d )
            {
                if (mainProgram.getBuildInfo<CL_PROGRAM_BUILD_STATUS>(d) != CL_SUCCESS)
                {
                    std::cout << "Build status for device: " << d.getInfo<CL_DEVICE_NAME>() << "_"<< d.getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>() << "CU_"<< d.getInfo<CL_DEVICE_MAX_CLOCK_FREQUENCY>() << "Mhz" << "\n";
                    std::cout << "    Build Status: " << mainProgram.getBuildInfo<CL_PROGRAM_BUILD_STATUS>(d) << std::endl;
                    std::cout << "    Build Options:\t" << mainProgram.getBuildInfo<CL_PROGRAM_BUILD_OPTIONS>(d) << std::endl;
                    std::cout << "    Build Log:\n " << mainProgram.getBuildInfo<CL_PROGRAM_BUILD_LOG>(d) << std::endl;
                }
            });
            throw;
        }

        if( ctl.debug( ) & control::debug::Compile )
        {
            std::vector<::cl::Device> devices = ctl.context( ).getInfo<CL_CONTEXT_DEVICES>( );
            std::for_each(devices.begin(), devices.end(), [&] (::cl::Device &d )
            {
                std::cout << "debug: Build status for device: " << d.getInfo<CL_DEVICE_NAME>() << "_"<< d.getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>() << "CU_"<< d.getInfo<CL_DEVICE_MAX_CLOCK_FREQUENCY>() << "Mhz" << "\n";
                std::cout << "debug:   Build Status: " << mainProgram.getBuildInfo<CL_PROGRAM_BUILD_STATUS>(d) << std::endl;
                std::cout << "debug:   Build Options:\t" << mainProgram.getBuildInfo<CL_PROGRAM_BUILD_OPTIONS>(d) << std::endl;
                std::cout << "debug:   Build Log:\n " << mainProgram.getBuildInfo<CL_PROGRAM_BUILD_LOG>(d) << std::endl;
            }); 
        };

        return mainProgram;
    }

    void compileKernels( std::vector< ::cl::Kernel >& clKernels, 
            const std::vector< const std::string >& kernelNames, 
            const std::string& fileName,
            const std::string& instantiationString,
            const std::string& userCode,
            const std::string& valueTypeName,
            const std::string& functorTypeName,
            const control& ctl )
    {

        //FIXME, when this becomes more stable move the kernel code to a string in bolt.cpp
        // Note unfortunate dependency here on relative file path of run directory and location of bolt::cl dir.
        std::string templateFunctionString = bolt::cl::fileToString( fileName + "_kernels.cl"); 

        std::string codeStr = templateFunctionString + "\n\n" + userCode + "\n\n" + instantiationString;

        std::string compileOptions = "";
        if( ctl.debug() & control::debug::SaveCompilerTemps )
        {
            compileOptions += "-save-temps=BOLT";
        }

        if( ctl.debug() & control::debug::Compile )
        {
            std::cout << "debug: compiling algorithm: '" << fileName << "' with valueType='" << valueTypeName << "'" << " ;  functorType='" << functorTypeName << "'" << std::endl;
        }

        ::cl::Program clProgram = buildProgram( codeStr, compileOptions, ctl );

        for( size_t k = 0; k < kernelNames.size( ); ++k )
        {
            std::string kernelName = kernelNames[ k ] + "Instantiated";
            clKernels.push_back( ::cl::Kernel( clProgram, kernelName.c_str( ) ) );
        }
    };

    void compileKernelsString( std::vector< ::cl::Kernel >& clKernels, 
            const std::vector< const std::string >& kernelNames, 
            //const boltKernel& clKernel,
            const std::string& clKernel, 
            const std::string& instantiationString,
            const std::string& userCode,
            const std::string& valueTypeName,
            const std::string& functorTypeName,
            const control& ctl )
    {

        std::string codeStr = clKernel + "\n\n" + userCode + "\n\n" + instantiationString;

        std::string compileOptions = "";
        if( ctl.debug() & control::debug::SaveCompilerTemps )
        {
            compileOptions += "-save-temps=BOLT";
        }

        if( ctl.debug() & control::debug::Compile )
        {
            std::cout << "debug: compiling" << " with valueType='" << valueTypeName << "'" << " ;  functorType='" << functorTypeName << "'" << std::endl;
        }

        try {
            ::cl::Program clProgram = buildProgram( codeStr, compileOptions, ctl );

            for( size_t k = 0; k < kernelNames.size( ); ++k )
            {
                std::string kernelName = kernelNames[ k ] + "Instantiated";
                clKernels.push_back( ::cl::Kernel( clProgram, kernelName.c_str( ) ) );
            } 
        }  catch(::cl::Error e) {
            std::cerr << "clBuildProgram failed in compileKernelsString.  Code=" << e.err() << "\n";
        }

    };

    void constructAndCompileString( ::cl::Kernel *masterKernel, 
            const std::string& kernelName, 
            //const boltKernel& clKernel, 
            const std::string& clKernel, 
            const std::string& instantiationString, 
            const std::string& userCode, 
            const std::string& valueTypeName, 
            const std::string& functorTypeName, 
            const control &c )
    {
        std::string codeStr = userCode + "\n\n" + clKernel +   instantiationString;

        std::string compileOptions = "";
        if (c.debug() & control::debug::SaveCompilerTemps) {
            compileOptions += "-save-temps=BOLT";
        }

        if (c.debug() & control::debug::Compile) {
            std::cout << "debug: compiling algorithm: '" << kernelName << "' with valueType='" << valueTypeName << "'" << " ;  functorType='" << functorTypeName << "'" << std::endl;
        }

        *masterKernel = bolt::cl::compileFunctor(codeStr, kernelName + "Instantiated", compileOptions, c);
    };


    void wait(const bolt::cl::control &ctl, ::cl::Event &e) 
    {
        const bolt::cl::control::e_WaitMode waitMode = ctl.waitMode();
        if (waitMode == bolt::cl::control::BusyWait) {
            const ::cl::CommandQueue& q = ctl.commandQueue();
            q.flush();
            while (e.getInfo<CL_EVENT_COMMAND_EXECUTION_STATUS>() != CL_COMPLETE) {
                // spin here for fast completion detection...
            };
        } else if ((waitMode == bolt::cl::control::NiceWait) || (waitMode == bolt::cl::control::BalancedWait)) {
            cl_int l_Error = e.wait();
            V_OPENCL( l_Error, "wait call failed" );
        } else if (waitMode == bolt::cl::control::ClFinish) {
            const ::cl::CommandQueue& q = ctl.commandQueue();
            cl_int l_Error = q.finish();
            V_OPENCL( l_Error, "clFinish call failed" );
        }
    };

    }; //namespace bolt::cl
}; // namespace bolt

