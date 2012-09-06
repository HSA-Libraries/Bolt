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

/*! \file bolt.h
 *  \brief Main public header file defining global functions for Bolt
 *  \todo 1. Remove requirement for VS2012 - make it work on older version of VS
 *  \todo 2. Stringify cl kernel files and embed in Bolt library
 *  \todo 3. Develop googletest framework for Transform
 *  \todo 4. Develop googletest framework for Transform_reduce
 *  \todo 5. Develop googletest framework for count
 *  \todo 6. Develop googletest framework for reduce
 *  \todo 7. Follow the coding guideline for expanding tabs to spaces, max line char width of 120 chars
 *  \todo 8. Rename the template function calls for each Bolt API, so that they don't all have to have the same name\n
 *  \c bolt::cl::reduce ->\n
 *  \c bolt::cl::detail::reduce_detect_random_access ->\n
 *  \c bolt::cl::detail::reduce_pick_iterator ->\n
 *  \c bolt::cl::detail::reduce_enqueue\n
 *  \todo 9. Move non-public versions of code into the detail namespace, across all function families
 *  \todo 10. Review the the use of parameters to the Bolt API; everything should default to const reference unless there
 *  is a solid reason not to be
 *  \todo 11. Use the typename DVInputIterator for functions which require a device_vector iterator
 *  \todo 12. Add buffer pool for temporary memory allocated by Bolt calls
 *  \todo 13. Make Bolt calls thread-safe (Save cl:program rather than cl::kernel, and call clCreateKernel on each Bolt call)
 *  \todo 14. Review documentation for typos, clarity, etc
 *  \todo 15. Add CPU implementations, i.e. link in external library such as TBB or define our own CPU implementation
 *  \todo 16. Add richer set of API functions (can this be made more specific?)
*/

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