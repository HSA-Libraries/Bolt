@echo off

rem ################################################################################################
REM	# Master Bolt Build Script
rem ################################################################################################

set BUILD_NAME="Build.Win64.VS11.123456789"

rem ################################################################################################
rem Set the ROOT directory if not already set
if "%DEVROOT%"=="" set DEVROOT=%CD%
echo Info: DEVROOT=%DEVROOT%
echo Info: BUILD_NAME=%BUILD_NAME%
echo Info: Computer Name: %COMPUTERNAME%

rem ################################################################################################
rem Read command line parameters
set BUILD_PLATFORM=x64
set BUILD_TOOLS=vs11
set BUILD_ACML=OFF

for %%A in (%*) do (
	if /i "%%A"=="-vs10" (
		set BUILD_TOOLS=vs10
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

rem ################################################################################################
rem Load compiler environment; we prefer VS2012 and then VS2010
if "%BUILD_TOOLS%" == "vs10" ( 
	if not "%VS100COMNTOOLS%" == "" (
		set VCVARSALL="%VS100COMNTOOLS%..\..\VC\vcvarsall.bat"
	) else (
		goto :error_no_VSCOMNTOOLS
	)
) else (
	if "%BUILD_TOOLS%" == "vs11" ( 
		if not "%VS110COMNTOOLS%" == "" ( 
			set VCVARSALL="%VS110COMNTOOLS%..\..\VC\vcvarsall.bat"
		) else (
			goto :error_no_VSCOMNTOOLS
		)
	)
)

rem ### this call permanently lengthens PATH, which eventually causes an error
if "%BUILD_PLATFORM%" == "x64" ( 
	echo Info: vcvarsall.bat = %VCVARSALL% x86_amd64
	call %VCVARSALL% x86_amd64
) else (
	echo Info: vcvarsall.bat = %VCVARSALL% x86
	call %VCVARSALL% x86
)
echo Info: Done setting up Visual Studio environment variables.

rem Echo a blank line into a file called success; the existence of success determines whether we built successfully
echo. > %DevRoot%\success

rem ################################################################################################
rem # Start of build logic here
rem ################################################################################################

rem Generate the build environment for vs11
if "%BUILD_TOOLS%" == "vs11" (

rem ################################################################################################
rem Make Directory
mkdir %BUILD_NAME%
pushd %BUILD_NAME%
	
rem ################################################################################################
rem Cmake
"C:\Program Files (x86)\CMake 2.8\bin\cmake.exe" -G "Visual Studio 11 Win64" -D CMAKE_BUILD_TYPE=Release ../Bolt/superbuild
	if errorlevel 1 (
		echo Cmake failed to generate release build files for "Visual Studio 11 Win64"
		del /Q /F %DevRoot%\success
	  popd
		goto :eof
	)

rem ################################################################################################
rem SuperBuild
	MSBuild.exe ALL_BUILD.vcxproj /m /fl /flp1:logfile=errors.log;errorsonly /flp2:logfile=warnings.log;warningsonly /flp3:logfile=build.log /p:Configuration=Release /p:PlatformToolset=v110 /p:Platform="x64" /t:rebuild	
	if errorlevel 1 (
		echo Release SuperBuild of vs11 "Release|x64" failed
		del /Q /F %DevRoot%\success
	  popd
		goto :eof
	)

rem ################################################################################################
rem BoltBuild
pushd Bolt-build
	MSBuild.exe ALL_BUILD.vcxproj /m /fl /flp1:logfile=errors.log;errorsonly /flp2:logfile=warnings.log;warningsonly /flp3:logfile=build.log /p:Configuration=Release /p:PlatformToolset=v110 /p:Platform="x64" /t:rebuild	
	if errorlevel 1 (
		echo Release BoltBuild of vs11 "Release|x64" failed
		del /Q /F %DevRoot%\success
	popd
		goto :eof
	)

	popd
	popd
)

goto :eof
rem ################################################################################################
rem # End
rem ################################################################################################

rem ################################################################################################
:error_no_VSCOMNTOOLS
echo ERROR: Cannot determine the location of the VS Common Tools folder. (%VS100COMNTOOLS% or %VS110COMNTOOLS%)
if exist %DevRoot%\success del /Q /F %DevRoot%\success
goto :eof

rem ################################################################################################
:print_help
echo Build script for Amd.clFFT project
echo Command line options: 
echo -debug ) Create and package a debug build of Amd.clFFT with debug files
echo -win32 ) Build a 32bit version of Amd.clFFT (default: 64bit)
echo -vs9 ) Build Amd.clFFT with vs9 even if vs10 is available
echo -help,h ) Print out this documentation
if exist %DevRoot%\success del /Q /F %DevRoot%\success
goto :eof
