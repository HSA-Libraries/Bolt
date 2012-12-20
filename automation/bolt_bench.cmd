@echo off

REM =============================================
REM	= Master Build script
REM = A good explanation of all DOS .cmd commands is available at http://ss64.com/nt/
REM =============================================

rem Localize batch environment variables; set statements are not visible outside of the .cmd file
rem Set the root directory if not already set; it's already set on the build server, and not set on a users machine
if "%DEVROOT%"=="" set DEVROOT=%CD%
echo Build.cmd: Devroot Path %DEVROOT%

rem Read command line parameters
set BUILD_PLATFORM=x64
set BUILD_TOOLS=vs10
set BUILD_ACML=OFF

for %%A in (%*) do (
	if /i "%%A"=="-vs9" (
		set BUILD_TOOLS=vs9
	)
	if /i "%%A"=="-acml" (
		set BUILD_ACML=ON
	)
	if /i "%%A"=="-h" (
		goto :print_help
	)
	if /i "%%A"=="-help" (
		goto :print_help
	)
	if /i "%%A"=="--help" (
		goto :print_help
	)
)

rem Set the build environment to point at a compiler; we prefer vs2010 and then vs2008
rem Example 64-bit environment: %comspec% /k ""C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\vcvarsall.bat"" amd64
rem Example 32-bit environment: %comspec% /k ""C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\vcvarsall.bat"" x86
if "%BUILD_TOOLS%" == "vs10" ( 
	if not "%VS100COMNTOOLS%" == "" (
		set VCVARSALL="%VS100COMNTOOLS%..\..\VC\vcvarsall.bat"
	) else (
		goto :error_no_VSCOMNTOOLS
	)
) else (
	if "%BUILD_TOOLS%" == "vs9" ( 
		if not "%VS90COMNTOOLS%" == "" ( 
			set VCVARSALL="%VS90COMNTOOLS%..\..\VC\vcvarsall.bat"
		) else (
			goto :error_no_VSCOMNTOOLS
		)
	)
)

if "%BUILD_PLATFORM%" == "x64" ( 
	echo Path to vcvarsall.bat: %VCVARSALL% x86_amd64
	call %VCVARSALL% x86_amd64
) else (
	echo Path to vcvarsall.bat: %VCVARSALL% x86
	call %VCVARSALL% x86
)

rem Name of the build server for RMS is CTRL1
rem Protect calls to batch files so that they don't execute on client machine, only on build server
rem Client machines should already have environment set up outside of this batch file

echo Computer Name: %COMPUTERNAME%

echo Setting up Cmake environment...
if exist d:\batch\cmake_287.cmd ( call d:\batch\cmake_287.cmd )

echo Setting up ATI Stream environment...
if exist binDependencies (
	set AMDAPPSDKROOT=%DevRoot%/binDependencies/windows/AMD_APP/
)

rem It is necessary to modify the env variables that come out of the .cmd files to substitute DOS style '\' 
rem directory terminators with '/', because cmake will interpret the '\' as escape characters
set AMDAPPSDKROOT=%AMDAPPSDKROOT:\=/%
echo AMDAPPSDKROOT = %AMDAPPSDKROOT%

echo Setting up Boost environment...
if exist binDependencies (
	set BOOST_ROOT=%DevRoot%/binDependencies/windows/Boost/
)
set BOOST_ROOT=%BOOST_ROOT:\=/%
echo BOOST_ROOT = %BOOST_ROOT%

rem Sometimes, the .cmd files are set up where the env. variables have leading spaces in them.
rem We use a for /f loop to eliminate any leading whitespace, because CMake apparently does 
rem not like leading spaces in -D parameters
echo Setting up FFTW environment...
if exist binDependencies (
	set FFTW_ROOT=%DevRoot%/binDependencies/windows/FFTW/
)
set FFTW_ROOT=%FFTW_ROOT:\=/%
echo FFTW_ROOT = %FFTW_ROOT%

echo Setting up GTEST environment...
if exist binDependencies (
	set GTEST_ROOT=%DevRoot%/binDependencies/windows/Gtest/
)
set GTEST_ROOT=%GTEST_ROOT:\=/%
echo GTEST_ROOT = %GTEST_ROOT%
)

if "%BUILD_ACML%" == "OFF" (
	set NETLIB_ROOT=%DevRoot%/binDependencies/windows/BLAS
) else (
	echo Setting up ACML environment...
	if exist d:\batch\ACML_440_64Bit.cmd ( call d:\batch\ACML_440_64Bit.cmd )
	echo ACML_ROOT_64 = %ACML_ROOT_64%
	set ACML_ROOT_64=%ACML_ROOT_64:\=/%
	for /f "tokens=* delims= " %%a in ("%ACML_ROOT_64%") do set ACML_ROOT_64=%%a
	echo ACML_ROOT_64 = %ACML_ROOT_64%

	if exist d:\batch\ACML_440_32Bit.cmd ( call d:\batch\ACML_440_32Bit.cmd )
	echo ACML_ROOT_32 = %ACML_ROOT_32%
	set ACML_ROOT_32=%ACML_ROOT_32:\=/%
	for /f "tokens=* delims= " %%a in ("%ACML_ROOT_32%") do set ACML_ROOT_32=%%a
	echo ACML_ROOT_32 = %ACML_ROOT_32%
)

rem Echo a blank line into a file called success; the existence of success determines whether we built successfully
echo. > %DevRoot%\success

rem #########################################
rem #########################################
rem # Start of build logic here

rem Generate the build environment for vs10
if "%BUILD_TOOLS%" == "vs10" (

rem #########################################
rem # Generate a 64bit release build of clFFT
	echo " ############################################"
	echo "######## clFFT: 64-bit release build ########"
	echo "############################################ "
	mkdir clFFT\bin\release\vs10_64
	pushd clFFT\bin\release\vs10_64
	
	echo cmake -G "Visual Studio 10 Win64" -D BOOST_ROOT="%BOOST_ROOT%" -D BOOST_LIBRARYDIR="%BOOST_ROOT%/lib64/lib" -D CMAKE_INSTALL_PREFIX="../package" -D CMAKE_BUILD_TYPE=Release -D FFTW_ROOT="%FFTW_ROOT%" -D GTEST_ROOT="%GTEST_ROOT%" ../../../code
	cmake -G "Visual Studio 10 Win64" -D BOOST_ROOT="%BOOST_ROOT%" -D BOOST_LIBRARYDIR="%BOOST_ROOT%/lib64/lib" -D CMAKE_INSTALL_PREFIX="../package" -D CMAKE_BUILD_TYPE=Release -D FFTW_ROOT="%FFTW_ROOT%" -D GTEST_ROOT="%GTEST_ROOT%" ../../../code
	if errorlevel 1 (
		echo Cmake failed to generate release build files for "Visual Studio 10 Win64"
		del /Q /F %DevRoot%\success
		goto :eof
	)

rem This command build the packaging project, which depends on the library and client to be built also, results in a tidy .zip file in current directory
 	echo Build Command: MSBuild.exe install.vcxproj /m /fl /flp1:logfile=errors.log;errorsonly /flp2:logfile=warnings.log;warningsonly /p:Configuration=Release /p:PlatformToolset=v100 /p:Platform="x64" /t:rebuild
	MSBuild.exe install.vcxproj /m /fl /flp1:logfile=errors.log;errorsonly /flp2:logfile=warnings.log;warningsonly /p:Configuration=Release /p:PlatformToolset=v100 /p:Platform="x64" /t:rebuild	
	if errorlevel 1 (
		echo Release build of vs10 "Release|x64" failed
		del /Q /F %DevRoot%\success
		goto :eof
	)

	REM rem For whatever reason, no source target is defined in the visual studio solutions; call cpack directly to package our source
	REM cpack -C Release --config ./CPackSourceConfig.cmake
	REM if errorlevel 1 (
		REM echo Cmake failed to generate source package for clAmdFft
		REM del /Q /F %DevRoot%\success
		REM goto :eof
	REM )
	
	popd

rem #########################################
rem # Generate a 64bit debug build of clFFT
	echo " ##########################################"
	echo "######## clFFT: 64-bit debug build ########"
	echo "########################################## "
	mkdir clFFT\bin\debug\vs10_64
	pushd clFFT\bin\debug\vs10_64
	
	echo cmake -G "Visual Studio 10 Win64" -D BOOST_ROOT="%BOOST_ROOT%" -D BOOST_LIBRARYDIR="%BOOST_ROOT%/lib64/lib" -D CMAKE_INSTALL_PREFIX="../package" -D CMAKE_BUILD_TYPE=Debug -D FFTW_ROOT="%FFTW_ROOT%" -D GTEST_ROOT="%GTEST_ROOT%" ../../../code
	cmake -G "Visual Studio 10 Win64" -D BOOST_ROOT="%BOOST_ROOT%" -D BOOST_LIBRARYDIR="%BOOST_ROOT%/lib64/lib" -D CMAKE_INSTALL_PREFIX="../package" -D CMAKE_BUILD_TYPE=Debug -D FFTW_ROOT="%FFTW_ROOT%" -D GTEST_ROOT="%GTEST_ROOT%" ../../../code
	if errorlevel 1 (
		echo Cmake failed to generate debug build files for "Visual Studio 10 Win64"
		del /Q /F %DevRoot%\success
		goto :eof
	)

rem This command build the packaging project, which depends on the library and client to be built also, results in a tidy .zip file in current directory
 	echo Build Command: MSBuild.exe install.vcxproj /m /fl /flp1:logfile=errors.log;errorsonly /flp2:logfile=warnings.log;warningsonly /p:Configuration=Debug /p:PlatformToolset=v100 /p:Platform="x64" /t:rebuild
	MSBuild.exe install.vcxproj /m /fl /flp1:logfile=errors.log;errorsonly /flp2:logfile=warnings.log;warningsonly /p:Configuration=Debug /p:PlatformToolset=v100 /p:Platform="x64" /t:rebuild	
	if errorlevel 1 (
		echo Debug build of vs10 "Debug|x64" failed
		del /Q /F %DevRoot%\success
		goto :eof
	)

	REM rem For whatever reason, no source target is defined in the visual studio solutions; call cpack directly to package our source
	REM cpack -C Debug --config ./CPackSourceConfig.cmake
	REM if errorlevel 1 (
		REM echo Cmake failed to generate source package for clAmdFft
		REM del /Q /F %DevRoot%\success
		REM goto :eof
	REM )
	
	popd
	
rem #########################################
rem # Generate a 32bit release build of clFFT
	echo " ############################################"
	echo "######## clFFT: 32-bit release build ########"
	echo "############################################ "
	mkdir clFFT\bin\release\vs10_32
	pushd clFFT\bin\release\vs10_32
	
	echo cmake -G "Visual Studio 10" -D BOOST_ROOT="%BOOST_ROOT%" -D BOOST_LIBRARYDIR="%BOOST_ROOT%/lib" -D CMAKE_INSTALL_PREFIX="../package" -D CMAKE_BUILD_TYPE=Release -D FFTW_ROOT="%FFTW_ROOT%" -D GTEST_ROOT="%GTEST_ROOT%" ../../../code
	cmake -G "Visual Studio 10" -D BOOST_ROOT="%BOOST_ROOT%" -D BOOST_LIBRARYDIR="%BOOST_ROOT%/lib" -D CMAKE_INSTALL_PREFIX="../package" -D CMAKE_BUILD_TYPE=Release -D FFTW_ROOT="%FFTW_ROOT%" -D GTEST_ROOT="%GTEST_ROOT%" ../../../code
	if errorlevel 1 (
		echo Cmake failed to generate release build files for "Visual Studio 10"
		del /Q /F %DevRoot%\success
		goto :eof
	)

	rem This command build the packaging project, which depends on the library and client to be built also, results in a tidy .zip file in current directory
 	echo Build Command: MSBuild.exe install.vcxproj /m /fl /flp1:logfile=errors.log;errorsonly /flp2:logfile=warnings.log;warningsonly /p:Configuration=Release /p:PlatformToolset=v100 /p:Platform="win32" /t:rebuild
	MSBuild.exe install.vcxproj /m /fl /flp1:logfile=errors.log;errorsonly /flp2:logfile=warnings.log;warningsonly /p:Configuration=Release /p:PlatformToolset=v100 /p:Platform="win32" /t:rebuild	
	if errorlevel 1 (
		echo Release build of vs10 "Release|win32" failed
		del /Q /F %DevRoot%\success
		goto :eof
	)
	
	popd
	
rem #########################################
rem # Generate a 32bit debug build of clFFT
	echo " ##########################################"
	echo "######## clFFT: 32-bit debug build ########"
	echo "########################################## "
	mkdir clFFT\bin\debug\vs10_32
	pushd clFFT\bin\debug\vs10_32
	
	echo cmake -G "Visual Studio 10" -D BOOST_ROOT="%BOOST_ROOT%" -D BOOST_LIBRARYDIR="%BOOST_ROOT%/lib" -D CMAKE_INSTALL_PREFIX="../package" -D CMAKE_BUILD_TYPE=Debug -D FFTW_ROOT="%FFTW_ROOT%" -D GTEST_ROOT="%GTEST_ROOT%" ../../../code
	cmake -G "Visual Studio 10" -D BOOST_ROOT="%BOOST_ROOT%" -D BOOST_LIBRARYDIR="%BOOST_ROOT%/lib" -D CMAKE_INSTALL_PREFIX="../package" -D CMAKE_BUILD_TYPE=Debug -D FFTW_ROOT="%FFTW_ROOT%" -D GTEST_ROOT="%GTEST_ROOT%" ../../../code
	if errorlevel 1 (
		echo Cmake failed to generate build files for "Visual Studio 10"
		del /Q /F %DevRoot%\success
		goto :eof
	)

	rem This command build the packaging project, which depends on the library and client to be built also, results in a tidy .zip file in current directory
 	echo Build Command: MSBuild.exe install.vcxproj /m /fl /flp1:logfile=errors.log;errorsonly /flp2:logfile=warnings.log;warningsonly /p:Configuration=Debug /p:PlatformToolset=v100 /p:Platform="win32" /t:rebuild
	MSBuild.exe install.vcxproj /m /fl /flp1:logfile=errors.log;errorsonly /flp2:logfile=warnings.log;warningsonly /p:Configuration=Debug /p:PlatformToolset=v100 /p:Platform="win32" /t:rebuild	
	if errorlevel 1 (
		echo Build of vs10 "Debug|win32" failed
		del /Q /F %DevRoot%\success
		goto :eof
	)
	
	popd
	
rem #########################################
rem # Generate a 64bit release build of clBLAS
	echo " #############################################"
	echo "######## clBLAS: 64-bit release build ########"
	echo "############################################# "
	mkdir clBLAS\bin\release\vs10_64
	pushd clBLAS\bin\release\vs10_64
	
	echo cmake -G "Visual Studio 10 Win64" -D BOOST_ROOT="%BOOST_ROOT%" -D BOOST_LIBRARYDIR="%BOOST_ROOT%/lib64/lib" -D ACMLROOT="%ACML_ROOT_64%/ifort64" -D NETLIB_ROOT="%NETLIB_ROOT%" -D GTEST_ROOT="%GTEST_ROOT%" -D CMAKE_INSTALL_PREFIX="../package" -D CMAKE_BUILD_TYPE=Release -DBUILD_RUNTIME=ON -DBUILD_TEST=ON -DBUILD_CLIENT=ON -DBUILD_SAMPLE=ON -DBUILD_PERFORMANCE=ON -DBUILD_KTEST=OFF -DBUILD_CUDA=OFF -D CORR_TEST_WITH_ACML=%BUILD_ACML% ../../..
	cmake -G "Visual Studio 10 Win64" -D BOOST_ROOT="%BOOST_ROOT%" -D BOOST_LIBRARYDIR="%BOOST_ROOT%/lib64/lib" -D ACMLROOT="%ACML_ROOT_64%/ifort64" -D NETLIB_ROOT="%NETLIB_ROOT%" -D GTEST_ROOT="%GTEST_ROOT%" -D CMAKE_INSTALL_PREFIX="../package" -D CMAKE_BUILD_TYPE=Release -DBUILD_RUNTIME=ON -DBUILD_TEST=ON -DBUILD_CLIENT=ON -DBUILD_SAMPLE=ON -DBUILD_PERFORMANCE=ON -DBUILD_KTEST=OFF -DBUILD_CUDA=OFF -D CORR_TEST_WITH_ACML=%BUILD_ACML% ../../..
	if errorlevel 1 (
		echo Cmake failed to generate release build files for "Visual Studio 10 Win64"
		del /Q /F %DevRoot%\success
		goto :eof
	)
	
rem This command build the packaging project, which depends on the library and client to be built also, results in a tidy .zip file in current directory
 	echo Build Command: MSBuild.exe install.vcxproj /m /fl /flp1:logfile=errors.log;errorsonly /flp2:logfile=warnings.log;warningsonly /p:Configuration=Release /p:PlatformToolset=v100 /p:Platform="x64" /t:rebuild
	MSBuild.exe install.vcxproj /m /fl /flp1:logfile=errors.log;errorsonly /flp2:logfile=warnings.log;warningsonly /p:Configuration=Release /p:PlatformToolset=v100 /p:Platform="x64" /t:rebuild	
	if errorlevel 1 (
		echo Release build of vs10 "Release|x64" failed
		del /Q /F %DevRoot%\success
		goto :eof
	)

	REM rem For whatever reason, no source target is defined in the visual studio solutions; call cpack directly to package our source
	REM cpack -C Release --config ./CPackSourceConfig.cmake
	REM if errorlevel 1 (
		REM echo Cmake failed to generate source package for clAmdFft
		REM del /Q /F %DevRoot%\success
		REM goto :eof
	REM )
	
	popd

rem #########################################
rem # Generate a 64bit debug build of clBLAS
	echo " ###########################################"
	echo "######## clBLAS: 64-bit debug build ########"
	echo "########################################### "
	mkdir clBLAS\bin\debug\vs10_64
	pushd clBLAS\bin\debug\vs10_64
	
	echo cmake -G "Visual Studio 10 Win64" -D BOOST_ROOT="%BOOST_ROOT%" -D BOOST_LIBRARYDIR="%BOOST_ROOT%/lib64/lib" -D ACMLROOT="%ACML_ROOT_64%/ifort64" -D GTEST_ROOT="%GTEST_ROOT%" -D CMAKE_INSTALL_PREFIX="../package" -D CMAKE_BUILD_TYPE=Debug -DBUILD_RUNTIME=ON -DBUILD_TEST=ON -DBUILD_CLIENT=ON -DBUILD_SAMPLE=ON -DBUILD_PERFORMANCE=ON -DBUILD_KTEST=OFF -DBUILD_CUDA=OFF -D CORR_TEST_WITH_ACML=%BUILD_ACML% ../../..
	cmake -G "Visual Studio 10 Win64" -D BOOST_ROOT="%BOOST_ROOT%" -D BOOST_LIBRARYDIR="%BOOST_ROOT%/lib64/lib" -D ACMLROOT="%ACML_ROOT_64%/ifort64" -D GTEST_ROOT="%GTEST_ROOT%" -D CMAKE_INSTALL_PREFIX="../package" -D CMAKE_BUILD_TYPE=Debug -DBUILD_RUNTIME=ON -DBUILD_TEST=ON -DBUILD_CLIENT=ON -DBUILD_SAMPLE=ON -DBUILD_PERFORMANCE=ON -DBUILD_KTEST=OFF -DBUILD_CUDA=OFF -D CORR_TEST_WITH_ACML=%BUILD_ACML% ../../..
	if errorlevel 1 (
		echo Cmake failed to generate debug build files for "Visual Studio 10 Win64"
		del /Q /F %DevRoot%\success
		goto :eof
	)
	
rem This command build the packaging project, which depends on the library and client to be built also, results in a tidy .zip file in current directory
 	echo Build Command: MSBuild.exe install.vcxproj /m /fl /flp1:logfile=errors.log;errorsonly /flp2:logfile=warnings.log;warningsonly /p:Configuration=Debug /p:PlatformToolset=v100 /p:Platform="x64" /t:rebuild
	MSBuild.exe install.vcxproj /m /fl /flp1:logfile=errors.log;errorsonly /flp2:logfile=warnings.log;warningsonly /p:Configuration=Debug /p:PlatformToolset=v100 /p:Platform="x64" /t:rebuild	
	if errorlevel 1 (
		echo Debug build of vs10 "Debug|x64" failed
		del /Q /F %DevRoot%\success
		goto :eof
	)

	REM rem For whatever reason, no source target is defined in the visual studio solutions; call cpack directly to package our source
	REM cpack -C Debug --config ./CPackSourceConfig.cmake
	REM if errorlevel 1 (
		REM echo Cmake failed to generate source package for clAmdFft
		REM del /Q /F %DevRoot%\success
		REM goto :eof
	REM )
	
	popd

rem #########################################
rem # Generate a 32bit release build of clBLAS
	echo " #############################################"
	echo "######## clBLAS: 32-bit release build ########"
	echo "############################################# "
	mkdir clBLAS\bin\release\vs10_32
	pushd clBLAS\bin\release\vs10_32
	
	echo cmake -G "Visual Studio 10" -D BOOST_ROOT="%BOOST_ROOT%" -D BOOST_LIBRARYDIR="%BOOST_ROOT%/lib" -D ACMLROOT="%ACML_ROOT_32%/ifort32" -D GTEST_ROOT="%GTEST_ROOT%" -D CMAKE_INSTALL_PREFIX="../package" -D CMAKE_BUILD_TYPE=Release -DBUILD_RUNTIME=ON -DBUILD_TEST=ON -DBUILD_CLIENT=ON -DBUILD_SAMPLE=ON -DBUILD_PERFORMANCE=ON -DBUILD_KTEST=OFF -DBUILD_CUDA=OFF -D CORR_TEST_WITH_ACML=%BUILD_ACML% ../../..
	cmake -G "Visual Studio 10" -D BOOST_ROOT="%BOOST_ROOT%" -D BOOST_LIBRARYDIR="%BOOST_ROOT%/lib" -D ACMLROOT="%ACML_ROOT_32%/ifort32" -D GTEST_ROOT="%GTEST_ROOT%" -D CMAKE_INSTALL_PREFIX="../package" -D CMAKE_BUILD_TYPE=Release -DBUILD_RUNTIME=ON -DBUILD_TEST=ON -DBUILD_CLIENT=ON -DBUILD_SAMPLE=ON -DBUILD_PERFORMANCE=ON -DBUILD_KTEST=OFF -DBUILD_CUDA=OFF -D CORR_TEST_WITH_ACML=%BUILD_ACML% ../../..
	if errorlevel 1 (
		echo Cmake failed to generate release build files for "Visual Studio 10"
		del /Q /F %DevRoot%\success
		goto :eof
	)
	
rem This command build the packaging project, which depends on the library and client to be built also, results in a tidy .zip file in current directory
 	echo Build Command: MSBuild.exe install.vcxproj /m /fl /flp1:logfile=errors.log;errorsonly /flp2:logfile=warnings.log;warningsonly /p:Configuration=Release /p:PlatformToolset=v100 /p:Platform="win32" /t:rebuild
	MSBuild.exe install.vcxproj /m /fl /flp1:logfile=errors.log;errorsonly /flp2:logfile=warnings.log;warningsonly /p:Configuration=Release /p:PlatformToolset=v100 /p:Platform="win32" /t:rebuild	
	if errorlevel 1 (
		echo Release build of vs10 "Release|win32" failed
		del /Q /F %DevRoot%\success
		goto :eof
	)
	
	popd
	
rem #########################################
rem # Generate a 32bit debug build of clBLAS
	echo " ###########################################"
	echo "######## clBLAS: 32-bit debug build ########"
	echo "########################################### "
	mkdir clBLAS\bin\debug\vs10_32
	pushd clBLAS\bin\debug\vs10_32
	
	echo cmake -G "Visual Studio 10" -D BOOST_ROOT="%BOOST_ROOT%" -D BOOST_LIBRARYDIR="%BOOST_ROOT%/lib" -D ACMLROOT="%ACML_ROOT_32%/ifort32" -D GTEST_ROOT="%GTEST_ROOT%" -D CMAKE_INSTALL_PREFIX="../package" -D CMAKE_BUILD_TYPE=Debug -DBUILD_RUNTIME=ON -DBUILD_TEST=ON -DBUILD_CLIENT=ON -DBUILD_SAMPLE=ON -DBUILD_PERFORMANCE=ON -DBUILD_KTEST=OFF -DBUILD_CUDA=OFF -D CORR_TEST_WITH_ACML=%BUILD_ACML% ../../..
	cmake -G "Visual Studio 10" -D BOOST_ROOT="%BOOST_ROOT%" -D BOOST_LIBRARYDIR="%BOOST_ROOT%/lib" -D ACMLROOT="%ACML_ROOT_32%/ifort32" -D GTEST_ROOT="%GTEST_ROOT%" -D CMAKE_INSTALL_PREFIX="../package" -D CMAKE_BUILD_TYPE=Debug -DBUILD_RUNTIME=ON -DBUILD_TEST=ON -DBUILD_CLIENT=ON -DBUILD_SAMPLE=ON -DBUILD_PERFORMANCE=ON -DBUILD_KTEST=OFF -DBUILD_CUDA=OFF -D CORR_TEST_WITH_ACML=%BUILD_ACML% ../../..
	if errorlevel 1 (
		echo Cmake failed to generate build files for "Visual Studio 10"
		del /Q /F %DevRoot%\success
		goto :eof
	)
	
rem This command build the packaging project, which depends on the library and client to be built also, results in a tidy .zip file in current directory
 	echo Build Command: MSBuild.exe install.vcxproj /m /fl /flp1:logfile=errors.log;errorsonly /flp2:logfile=warnings.log;warningsonly /p:Configuration=Debug /p:PlatformToolset=v100 /p:Platform="win32" /t:rebuild
	MSBuild.exe install.vcxproj /m /fl /flp1:logfile=errors.log;errorsonly /flp2:logfile=warnings.log;warningsonly /p:Configuration=Debug /p:PlatformToolset=v100 /p:Platform="win32" /t:rebuild	
	if errorlevel 1 (
		echo Build of vs10 "Debug|win32" failed
		del /Q /F %DevRoot%\success
		goto :eof
	)
	
	popd
)

goto :eof

:error_no_VSCOMNTOOLS
echo ERROR: Cannot determine the location of the VS Common Tools folder. (%VS100COMNTOOLS% or %VS90COMNTOOLS%)
if exist %DevRoot%\success del /Q /F %DevRoot%\success
goto :eof

:print_help
echo Build script for Amd.clFFT project
echo Command line options: 
echo -debug ) Create and package a debug build of Amd.clFFT with debug files
echo -win32 ) Build a 32bit version of Amd.clFFT (default: 64bit)
echo -vs9 ) Build Amd.clFFT with vs9 even if vs10 is available
echo -help,h ) Print out this documentation
if exist %DevRoot%\success del /Q /F %DevRoot%\success
goto :eof
