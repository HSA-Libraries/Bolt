#pragma once
#if !defined( BOLT_H )
#define BOLT_H

#define __CL_ENABLE_EXCEPTIONS
#define CL_USE_DEPRECATED_OPENCL_1_1_APIS
#if defined(__APPLE__) || defined(__MACOSX)
    #include <OpenCL/cl.hpp>
#else
    #include <CL/cl.hpp>
#endif

#include <string>

#include <bolt/BoltVersion.h>
#include <bolt/cl/control.h>
#include <bolt/cl/clcode.h>

namespace bolt {
    namespace cl {
        extern std::string fileToString(const std::string &fileName);

        extern ::cl::Kernel compileFunctor(const std::string &kernelCodeString, const std::string kernelName, const std::string compileOptions, const control &c);

        extern void constructAndCompile(::cl::Kernel *masterKernel, const std::string &apiName, const std::string instantiationString, std::string userCode, std::string valueTypeName,  std::string functorTypeName, const control &c);

        void compileKernels( std::vector< ::cl::Kernel >& clKernels, 
                const std::vector< const std::string >& kernelNames, 
                const std::string& fileName,
                const std::string& instantiationString,
                const std::string& userCode,
                const std::string& valueTypeName,
                const std::string& functorTypeName,
                const control& ctl );

        /*! \brief Query the Bolt library for version information
            *  \details Return the major, minor and patch version numbers associated with the Bolt library
            *  \param[out] major Major functionality change
            *  \param[out] minor Minor functionality change
            *  \param[out] patch Bug fixes, documentation changes, no new features introduced
            */
        void getVersion( cl_uint& major, cl_uint& minor, cl_uint& patch );

        /*! \brief Translates an integer OpenCL error code to a std::string at runtime
        *  \param status The OpenCL error code
         * \return The error code stringized
        */
        std::string clErrorStringA( const cl_int& status );

        /*! \brief Translates an integer OpenCL error code to a std::wstring at runtime
        *  \param status The OpenCL error code
         * \return The error code stringized
        */
        std::wstring clErrorStringW( const cl_int& status );

        /*! \brief Helper print function to stringify OpenCL error codes
        *  \param res The OpenCL error code
        *  \param msg A relevant message to be printed out supplied by user
        *  \param lineno Source line number; not currently used
        *  \note std::exception is built to only use narrow text
        */
        inline cl_int V_OpenCL( cl_int res, const std::string& msg, size_t lineno )
        { 
            switch( res )
            {
                case    CL_SUCCESS:
                    break;
                default:
                {
                    std::string tmp;
                    tmp.append( "V_OpenCL< " );
                    tmp.append( clErrorStringA( res ) );
                    tmp.append( " >: " );
                    tmp.append( msg );
                    //std::cout << tmp << std::endl;
                    throw ::cl::Error( res, tmp.c_str( ) );
                }
            }

            return	res;
        }
        #define V_OPENCL( status, message ) V_OpenCL( status, message, __LINE__ )

	};
};

BOLT_CREATE_TYPENAME(int);
BOLT_CREATE_TYPENAME(float);
BOLT_CREATE_TYPENAME(double);

#endif