

//#include "stdafx.h"  // not present in the clbolt dir, but don't really need it 
#include <clbolt/bolt.h>
#include <clbolt/unicode.h>

#include <iostream>
#include <fstream>
#include <streambuf>
#include <direct.h>  //windows CWD for error message
#include <tchar.h>

namespace clbolt {

	// Default control structure; this can be accessed by the clbolt::control::getDefault()
	control control::_defaultControl(true);


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



	cl::Kernel compileFunctor(const std::string &kernelCodeString, const std::string kernelName, const std::string compileOptions)
	{
		cl::Program mainProgram(kernelCodeString, false);
		try
		{
			mainProgram.build("-x clc++");

		} catch(cl::Error e) {
			std::cout << "Code         :\n" << kernelCodeString << std::endl;
			std::cout << "Build Status: " << mainProgram.getBuildInfo<CL_PROGRAM_BUILD_STATUS>(cl::Device::getDefault()) << std::endl;
			std::cout << "Build Options:\t" << mainProgram.getBuildInfo<CL_PROGRAM_BUILD_OPTIONS>(cl::Device::getDefault()) << std::endl;
			std::cout << "Build Log:\t " << mainProgram.getBuildInfo<CL_PROGRAM_BUILD_LOG>(cl::Device::getDefault()) << std::endl;
			throw e;
		}

		return cl::Kernel(mainProgram, kernelName.c_str());
	}



	void constructAndCompile(cl::Kernel *masterKernel, const std::string &apiName, const std::string instantiationString, std::string userCode, std::string valueTypeName,  std::string functorTypeName, unsigned debugMode) {

		//FIXME, when this becomes more stable move the kernel code to a string in bolt.cpp
		// Note unfortunate dependency here on relative file path of run directory and location of clbolt dir.
		std::string templateFunctionString = clbolt::fileToString( apiName + "_kernels.cl"); 

		std::string codeStr = userCode + "\n\n" + templateFunctionString +   instantiationString;

		if (debugMode & control::debug::Compile) {
			std::cout << "debug: compiling: '" << apiName << "'" << std::endl;
			std::cout << "debug: valueType  ='" << valueTypeName << "'" << "functorType='" << functorTypeName << "'" << std::endl;
			std::cout << "debug: code=" << std::endl << codeStr;
		}

		std::string compileOptions = "";
		if (debugMode & control::debug::SaveCompilerTemps) {
			compileOptions += "-save-temps=BOLT";
		}

		*masterKernel = clbolt::compileFunctor(codeStr, apiName + "Instantiated");
	};






};

