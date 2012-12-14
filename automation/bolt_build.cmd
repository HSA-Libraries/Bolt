@echo off

set buildStartTime=%time%
set OLD_PATH=%PATH%

set HR=###############################################################################

rem ################################################################################################
rem  # Master Bolt Build Script
rem ################################################################################################

set CMAKE="C:\Program Files (x86)\CMake 2.8\bin\cmake.exe"
set BOLT_HEAD=C:\GitRoot\Bolt

set BUILD_RELATIVE_PATH=..\BoltBuilds
set BUILD_PREFIX=Bolt
set BUILD_OS=Win
set BUILD_PLATFORM=x64
set BUILD_COMPILER=VS11
set BUILD_OPTIONS=""
set BUILD_SUFFIX=""


rem ################################################################################################
rem Read command line parameters
:Loop
  IF [%1]==[] GOTO Continue

  if /i "%1"=="-h" (
    goto :print_help
  )
  if /i "%1"=="--platform" (
    set BUILD_PLATFORM=%2
    SHIFT
  )
  if /i "%1"=="--compiler" (
    set BUILD_COMPILER=%2
    SHIFT
  )
  if /i "%1"=="--options" (
    set BUILD_OPTIONS=%2
    SHIFT
  )
  if /i "%1"=="--suffix" (
    set BUILD_SUFFIX=%2
    SHIFT
  )
SHIFT
GOTO Loop
:Continue


rem ################################################################################################
rem Construct Build Name
set BUILD_NAME=%BUILD_PREFIX%
set BUILD_NAME=%BUILD_NAME%.%BUILD_OS%
set BUILD_NAME=%BUILD_NAME%.%BUILD_PLATFORM%
set BUILD_NAME=%BUILD_NAME%.%BUILD_COMPILER%
if %BUILD_OPTIONS% NEQ "" (
  set BUILD_NAME=%BUILD_NAME%.%BUILD_OPTIONS%
)
if %BUILD_SUFFIX% NEQ "" (
  set BUILD_NAME=%BUILD_NAME%.%BUILD_SUFFIX%
)
set BUILD_PATH=%BOLT_HEAD%\%BUILD_RELATIVE_PATH%\%BUILD_NAME%

rem ################################################################################################
rem Print Build Info
echo.
echo %HR%
echo Build Parameters
echo Info: BOLT_HEAD=%BOLT_HEAD%
echo Info: BUILD_NAME=%BUILD_NAME%
echo Info: BUILD_PATH=%BUILD_PATH%
echo Info: Computer Name: %COMPUTER_NAME%


rem ################################################################################################
rem Load compiler environment; we prefer VS2012 and then VS2010
if "%BUILD_COMPILER%" == "VS10" ( 
  if not "%VS100COMNTOOLS%" == "" (
    set VCVARSALL="%VS100COMNTOOLS%..\..\VC\vcvarsall.bat"
  ) else (
    goto :error_no_VSCOMNTOOLS
  )
) else (
  if "%BUILD_COMPILER%" == "VS11" ( 
    if not "%VS110COMNTOOLS%" == "" ( 
      set VCVARSALL="%VS110COMNTOOLS%..\..\VC\vcvarsall.bat"
    ) else (
      goto :error_no_VSCOMNTOOLS
    )
  )
)
rem ### maybe move this call to a different script which the user can call once
rem ### this call permanently lengthens PATH, which eventually causes an error
if "%BUILD_PLATFORM%" == "x64" ( 
  echo Info: vcvarsall.bat = %VCVARSALL% x86_amd64
  call %VCVARSALL% x86_amd64
)
if "%BUILD_PLATFORM%" == "x86" (
  echo Info: vcvarsall.bat = %VCVARSALL% x86
  call %VCVARSALL% x86
)
echo Info: Done setting up Visual Studio environment variables.

rem Echo a blank line into a file called success; the existence of success determines whether we built successfully
echo. > %BUILD_PATH%\success

rem ################################################################################################
rem # Start of build logic here
rem ################################################################################################

rem Generate the build environment for VS11
if "%BUILD_COMPILER%" == "VS11" (


rem ################################################################################################
rem [Make and] Move Into Directory
echo.
echo %HR%
echo Making build directory %BUILD_PATH%
mkdir %BUILD_PATH%
pushd %BUILD_PATH%
  

rem ################################################################################################
rem Cmake
if not exist Bolt.SuperBuild.sln (
  echo.
  echo %HR%
  echo Running cmake to generate build files.
  "C:\Program Files (x86)\CMake 2.8\bin\cmake.exe" -G "Visual Studio 11 Win64" -D CMAKE_BUILD_TYPE=Release %BOLT_HEAD%\superbuild

  if errorlevel 1 (
    echo Cmake failed.
    del /Q /F %BUILD_PATH%\success
    popd
    goto :Done
  )
) else (
  echo "SuperBuild files already exist; not running cmake."
)


rem ################################################################################################
rem SuperBuild
if not exist Bolt-build\Bolt.sln (
  echo.
  echo %HR%
  echo Running MSBuild for superbuild.
  MSBuild.exe ALL_BUILD.vcxproj /m /fl /flp1:logfile=errors.log;errorsonly /flp2:logfile=warnings.log;warningsonly /flp3:logfile=build.log /p:Configuration=Release /p:PlatformToolset=v110 /p:Platform=%BUILD_PLATFORM% /t:build /v:d
  if errorlevel 1 (
    echo SuperBuild failed.
    del /Q /F %BUILD_PATH%\success
    popd
    goto :Done
  )
) else (
  echo "Bolt-build files already exist; not running MSBuild."
)


rem ################################################################################################
rem BoltBuild
echo.
echo %HR%
echo Running MSBuild for Bolt-build.
pushd Bolt-build
  MSBuild.exe ALL_BUILD.vcxproj /m /fl /flp1:logfile=errors.log;errorsonly /flp2:logfile=warnings.log;warningsonly /flp3:logfile=build.log /p:Configuration=Release /p:PlatformToolset=v110 /p:Platform=%BUILD_PLATFORM% /t:build /v:d
  if errorlevel 1 (
    echo Bolt-build failed.
    del /Q /F %BUILD_PATH%\success
  popd
    goto :Done
  )

  popd
  popd
)


rem ################################################################################################
rem # End
rem ################################################################################################
goto :Done


rem ################################################################################################
rem Cannot find compiler
:error_no_VSCOMNTOOLS
echo ERROR: Cannot determine the location of the VS Common Tools folder. (%VS100COMNTOOLS% or %VS110COMNTOOLS%)
if exist %BUILD_PATH%\success del /Q /F %BUILD_PATH%\success
goto :Done


rem ################################################################################################
rem Print Help
:print_help
echo Build script for Bolt
echo Command line options: 
echo -h     ) Print help
echo -debug ) Create and package a debug build of Amd.clFFT with debug files
echo -Win32 ) Build a 32bit (default: 64bit)
echo -VS10  ) Build with VS10 (default: VS11)
if exist %BUILD_PATH%\success del /Q /F %BUILD_PATH%\success
goto :Done


rem ################################################################################################
rem Done, Clean up
:Done
set PATH=%OLD_PATH%
echo.
echo %HR%
echo "Done. StartTime={%buildStartTime%} StopTime={%time%}"
echo %HR%
goto :eof
