@echo off
REM ################################################################################################
REM # Master Bolt Build Script
REM ################################################################################################
set HR=###############################################################################
set buildStartTime=%time%
set OLD_SYSTEM_PATH=%PATH%
set CMAKE="C:\Program Files (x86)\CMake 2.8\bin\cmake.exe"

set BOLT_BUILD_SOURCE_PATH=C:\Jenkins_FS_Root\workspace\bolt_GitHub_repository_clone
set BOLT_BUILD_INSTALL_PATH=%CD%

set BOLT_BUILD_OS=Win
set BOLT_BUILD_OS_VER=7
set BOLT_BUILD_COMP=VS
set BOLT_BUILD_COMP_VER=11
set BOLT_BUILD_BIT=64


REM ################################################################################################
REM # Read command line parameters
:Loop
  IF [%1]==[] GOTO Continue

  if /i "%1"=="-h" (
    goto :print_help
  )
  if /i "%1"=="--source" (
    set BOLT_BUILD_SOURCE_PATH=%2
    SHIFT
  )
  if /i "%1"=="--install" (
    set BOLT_BUILD_INSTALL_PATH=%2
    SHIFT
  )
  if /i "%1"=="--os" (
    set BOLT_BUILD_OS=%2
    SHIFT
  )
  if /i "%1"=="--os-ver" (
    set BOLT_BUILD_OS_VER=%2
    SHIFT
  )
  if /i "%1"=="--comp" (
    set BOLT_BUILD_COMP=%2
    SHIFT
  )
  if /i "%1"=="--comp-ver" (
    set BOLT_BUILD_COMP_VER=%2
    SHIFT
  )
  if /i "%1"=="--bit" (
    set BOLT_BUILD_BIT=%2
    SHIFT
  )
SHIFT
GOTO Loop
:Continue


REM ################################################################################################
REM # Construct Build Parameters
if "%BOLT_BUILD_BIT%" == "64" (
  set BOLT_BUILD_MSBUILD_PLATFORM=x64
) else (
  set BOLT_BUILD_MSBUILD_PLATFORM=x86
)
set BOLT_BUILD_MSBUILD_PLATFORM_TOOLSET=v%BOLT_BUILD_COMP_VER%0
set BOLT_BUILD_CMAKE_GEN=Visual Studio %BOLT_BUILD_COMP_VER%
if "%BOLT_BUILD_BIT%" == "64" (
  set BOLT_BUILD_CMAKE_GEN="%BOLT_BUILD_CMAKE_GEN% %BOLT_BUILD_OS%%BOLT_BUILD_BIT%"
) else (
  set BOLT_BUILD_CMAKE_GEN="%BOLT_BUILD_CMAKE_GEN%"
)

REM ################################################################################################
REM # Print Build Info
echo.
echo %HR%
echo Info: Bolt Build Parameters
echo Info: Source:    %BOLT_BUILD_SOURCE_PATH%
echo Info: Install:   %BOLT_BUILD_INSTALL_PATH%
echo Info: OS:        %BOLT_BUILD_OS%%BOLT_BUILD_OS_VER%
echo Info: Compiler:  %BOLT_BUILD_COMP%%BOLT_BUILD_COMP_VER% %BOLT_BUILD_BIT%bit
echo Info: CMake Gen: %BOLT_BUILD_CMAKE_GEN%
echo Info: Platform:  %BOLT_BUILD_MSBUILD_PLATFORM%
echo Info: Toolset:   %BOLT_BUILD_MSBUILD_PLATFORM_TOOLSET%


REM ################################################################################################
REM # Load compiler environment
if "%BOLT_BUILD_COMP_VER%" == "10" ( 
  if not "%VS100COMNTOOLS%" == "" (
    set VCVARSALL="%VS100COMNTOOLS%..\..\VC\vcvarsall.bat"
  ) else (
    goto :error_no_VSCOMNTOOLS
  )
) else (
  if "%BOLT_BUILD_COMP_VER%" == "11" ( 
    if not "%VS110COMNTOOLS%" == "" ( 
      set VCVARSALL="%VS110COMNTOOLS%..\..\VC\vcvarsall.bat"
    ) else (
      goto :error_no_VSCOMNTOOLS
    )
  )
)
REM ### maybe move this call to a different script which the user can call once
REM ### this call permanently lengthens PATH, which eventually causes an error
if "%BOLT_BUILD_BIT%" == "64" ( 
  echo Info: vcvarsall.bat = %VCVARSALL% x86_amd64
  call %VCVARSALL% x86_amd64
)
if "%BOLT_BUILD_BIT%" == "32" (
  echo Info: vcvarsall.bat = %VCVARSALL% x86
  call %VCVARSALL% x86
)
echo Info: Done setting up compiler environment variables.

REM Echo a blank line into a file called success; the existence of success determines whether we built successfully
echo. > %BOLT_BUILD_INSTALL_PATH%\success


REM ################################################################################################
REM # Start of build logic here
REM ################################################################################################


REM ################################################################################################
REM # Cmake
if not exist Bolt.SuperBuild.sln (
echo.
echo %HR%
echo Info: Running CMake to generate build files.
%CMAKE% ^
  -G %BOLT_BUILD_CMAKE_GEN% ^
  -D CMAKE_BOLT_BUILD_TYPE=Release ^
  %BOLT_BUILD_SOURCE_PATH%\superbuild
if errorlevel 1 (
  echo Info: CMake failed.
  del /Q /F %BOLT_BUILD_INSTALL_PATH%\success
  popd
  goto :Done
)
) else (
  echo Info: Bolt.SuperBuild.sln already build.
)


REM ################################################################################################
REM # Super Build
echo.
echo %HR%
echo Info: Running MSBuild for SuperBuild.
MSBuild.exe ^
  ALL_BUILD.vcxproj ^
  /m ^
  /fl ^
  /flp1:logfile=errors.log;errorsonly ^
  /flp2:logfile=warnings.log;warningsonly ^
  /flp3:logfile=build.log ^
  /p:Configuration=Release ^
  /p:Platform=%BOLT_BUILD_MSBUILD_PLATFORM% ^
  /p:PlatformToolset=%BOLT_BUILD_MSBUILD_PLATFORM_TOOLSET% ^
  /t:build
if errorlevel 1 (
  echo Info: MSBuild failed for SuperBuild.
  del /Q /F %BOLT_BUILD_INSTALL_PATH%\success
  popd
  goto :Done
)


REM ################################################################################################
REM # Build Documentation
echo.
echo %HR%
echo Info: Running MSBuild for Bolt documentation.
pushd Bolt-build
pushd doxy
MSBuild.exe ^
  Bolt.Documentation.vcxproj ^
  /m ^
  /fl ^
  /flp1:logfile=errors.log;errorsonly ^
  /flp2:logfile=warnings.log;warningsonly ^
  /flp3:logfile=build.log ^
  /p:Configuration=Release ^
  /p:Platform=%BOLT_BUILD_MSBUILD_PLATFORM% ^
  /p:PlatformToolset=%BOLT_BUILD_MSBUILD_PLATFORM_TOOLSET% ^
  /t:build
if errorlevel 1 (
  echo Info: MSBuild failed for Bolt documentation.
  del /Q /F %BOLT_BUILD_INSTALL_PATH%\success
  popd
  goto :Done
)
popd

REM ################################################################################################
REM # Zip Package
echo.
echo %HR%
echo Info: Running MSBuild for Bolt-build.
MSBuild.exe ^
  PACKAGE.vcxproj ^
  /m ^
  /fl ^
  /flp1:logfile=errors.log;errorsonly ^
  /flp2:logfile=warnings.log;warningsonly ^
  /flp3:logfile=build.log ^
  /p:Configuration=Release ^
  /p:Platform=%BOLT_BUILD_MSBUILD_PLATFORM% ^
  /p:PlatformToolset=%BOLT_BUILD_MSBUILD_PLATFORM_TOOLSET% ^
  /t:build
if errorlevel 1 (
  echo Info: MSBuild failed for Bolt-build.
  del /Q /F %BOLT_BUILD_INSTALL_PATH%\success
  popd
  goto :Done
)
popd


REM ################################################################################################
REM # End
REM ################################################################################################
goto :Done


REM ################################################################################################
REM Cannot find compiler
:error_no_VSCOMNTOOLS
echo ERROR: Cannot determine the location of the VS Common Tools folder. (%VS100COMNTOOLS% or %VS110COMNTOOLS%)
if exist %BOLT_BUILD_PATH%\success del /Q /F %BOLT_BUILD_PATH%\success
goto :Done


REM ################################################################################################
REM Print Help
:print_help
echo Build script for Bolt
echo Command line options: 
echo -h     ) Print help
echo -debug ) Create and package a debug build of Amd.clFFT with debug files
echo -Win32 ) Build a 32bit (default: 64bit)
echo -VS10  ) Build with VS10 (default: VS11)
if exist %BOLT_BUILD_PATH%\success del /Q /F %BOLT_BUILD_PATH%\success
goto :Done


REM ################################################################################################
REM Done, Clean up
:Done
set PATH=%OLD_SYSTEM_PATH%
echo.
echo %HR%
echo "Done. StartTime={%buildStartTime%} StopTime={%time%}"
echo %HR%
goto :eof
