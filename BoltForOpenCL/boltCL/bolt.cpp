
// #include "stdafx.h"  // not present in the BoltCL dir, but don't really need it 

#include <boltCL/bolt.h>

#include <iostream>
#include <fstream>
#include <streambuf>
#include <direct.h>  //windows CWD for error message


namespace boltcl {

	std::string fileToString(const std::string &fileName)
	{


		std::ifstream infile (fileName);
		if (infile.fail() ) {
			char cCurrentPath[FILENAME_MAX];

			if (_getcwd(cCurrentPath, sizeof(cCurrentPath) / sizeof(TCHAR))) {
				std::cout <<  "CWD=" << cCurrentPath << std::endl;
			};
			std::cout << "error: failed to open file '" << fileName << std::endl;
			throw;
		} 

		std::string str((std::istreambuf_iterator<char>(infile)),
			std::istreambuf_iterator<char>());
		return str;
	};



	cl::Kernel compileFunctor(const std::string &kernelCodeString, const std::string kernelName)
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


};

