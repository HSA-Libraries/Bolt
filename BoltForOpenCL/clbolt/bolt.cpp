

//#include "stdafx.h"  // not present in the clbolt dir, but don't really need it 
#include <clbolt/bolt.h>
#include <clbolt/unicode.h>

#include <iostream>
#include <fstream>
#include <streambuf>
#include <direct.h>  //windows CWD for error message
#include <tchar.h>
#include <algorithm>

namespace clbolt {




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
				tstring thisPath( osPath );
				tstring::size_type pos = thisPath.find_last_of( _T( "\\" ) );

				tstring newPath;
				if( pos != tstring::npos )
				{
					tstring exePath	= thisPath.substr( 0, pos + 1 );	// include the \ character

					//	Narrow to wide conversion should always work, but beware of wide to narrow!
					tstring convName( fileName.begin( ), fileName.end( ) );
					newPath = exePath + convName;
				}

				infile.open( newPath.c_str( ) );
			}
#endif
			if (infile.fail() ) {
				TCHAR cCurrentPath[FILENAME_MAX];
				if (_tgetcwd(cCurrentPath, sizeof(cCurrentPath) / sizeof(TCHAR))) {
					tout <<  _T( "CWD=" ) << cCurrentPath << std::endl;
				};
				std::cout << "error: failed to open file '" << fileName << std::endl;
				throw;
			} 
		}

		std::string str((std::istreambuf_iterator<char>(infile)),
			std::istreambuf_iterator<char>());
		return str;
	};



	cl::Kernel compileFunctor(const std::string &kernelCodeString, const std::string kernelName,  std::string compileOptions, const control &c)
	{

		if (c.debug() & control::debug::ShowCode) {
			std::cout << "debug: code=" << std::endl << kernelCodeString;
		}

		cl::Program mainProgram(c.context(), kernelCodeString, false);
		try
		{
			compileOptions += c.compileOptions();
			compileOptions += " -x clc++";

			if (c.compileForAllDevices()) {
				std::vector<cl::Device> devices = c.context().getInfo<CL_CONTEXT_DEVICES>();

				mainProgram.build(devices, compileOptions.c_str());
			} else {
				std::vector<cl::Device> devices;
				devices.push_back(c.device());
				mainProgram.build(devices, compileOptions.c_str());
			};




		} catch(cl::Error e) {
			std::cout << "Code         :\n" << kernelCodeString << std::endl;

			std::vector<cl::Device> devices = c.context().getInfo<CL_CONTEXT_DEVICES>();
			std::for_each(devices.begin(), devices.end(), [&] (cl::Device &d) {
				if (mainProgram.getBuildInfo<CL_PROGRAM_BUILD_STATUS>(d) != CL_SUCCESS) {
					std::cout << "Build status for device: " << d.getInfo<CL_DEVICE_NAME>() << "_"<< d.getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>() << "CU_"<< d.getInfo<CL_DEVICE_MAX_CLOCK_FREQUENCY>() << "Mhz" << "\n";
					std::cout << "    Build Status: " << mainProgram.getBuildInfo<CL_PROGRAM_BUILD_STATUS>(d) << std::endl;
					std::cout << "    Build Options:\t" << mainProgram.getBuildInfo<CL_PROGRAM_BUILD_OPTIONS>(d) << std::endl;
					std::cout << "    Build Log:\t " << mainProgram.getBuildInfo<CL_PROGRAM_BUILD_LOG>(d) << std::endl;
				}
			});
			throw e;
		}

		if ( c.debug() & control::debug::Compile) {
			std::vector<cl::Device> devices = c.context().getInfo<CL_CONTEXT_DEVICES>();
			std::for_each(devices.begin(), devices.end(), [&] (cl::Device &d) {
				std::cout << "debug: Build status for device: " << d.getInfo<CL_DEVICE_NAME>() << "_"<< d.getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>() << "CU_"<< d.getInfo<CL_DEVICE_MAX_CLOCK_FREQUENCY>() << "Mhz" << "\n";
				std::cout << "debug:   Build Status: " << mainProgram.getBuildInfo<CL_PROGRAM_BUILD_STATUS>(d) << std::endl;
				std::cout << "debug:   Build Options:\t" << mainProgram.getBuildInfo<CL_PROGRAM_BUILD_OPTIONS>(d) << std::endl;
				std::cout << "debug:   Build Log:\n " << mainProgram.getBuildInfo<CL_PROGRAM_BUILD_LOG>(d) << std::endl;
			}); 
		};

		return cl::Kernel(mainProgram, kernelName.c_str());
	}



	void constructAndCompile(cl::Kernel *masterKernel, const std::string &apiName, const std::string instantiationString, std::string userCode, std::string valueTypeName,  std::string functorTypeName, const control &c) {

		//FIXME, when this becomes more stable move the kernel code to a string in bolt.cpp
		// Note unfortunate dependency here on relative file path of run directory and location of clbolt dir.
		std::string templateFunctionString = clbolt::fileToString( apiName + "_kernels.cl"); 

		std::string codeStr = userCode + "\n\n" + templateFunctionString +   instantiationString;


		std::string compileOptions = "";
		if (c.debug() & control::debug::SaveCompilerTemps) {
			compileOptions += "-save-temps=BOLT";
		}

		if (c.debug() & control::debug::Compile) {
			std::cout << "debug: compiling algorithm: '" << apiName << "' with valueType='" << valueTypeName << "'" << " ;  functorType='" << functorTypeName << "'" << std::endl;
		}




		*masterKernel = clbolt::compileFunctor(codeStr, apiName + "Instantiated", compileOptions, c);
	};






};

