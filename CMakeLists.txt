#############################################################################
##	Copyright (C) 2010 Advanced Micro Devices, Inc. All Rights Reserved. 
#############################################################################
if( WIN32 )
	# We require 2.8.3 for windows because of a bug in cmake that prevented vs2010 from generating
	# executables properly with multiple periods
	cmake_minimum_required( VERSION 2.8.3 )
else( )
	cmake_minimum_required( VERSION 2.6 )
endif( )

# uncomment these to debug nmake and borland makefiles
#SET(CMAKE_START_TEMP_FILE "")
#SET(CMAKE_END_TEMP_FILE "")
#SET(CMAKE_VERBOSE_MAKEFILE 1)

# This becomes the name of the solution file
project( Bolt )

# Define a version for the code
set( Bolt_VERSION_MAJOR 0 )
set( Bolt_VERSION_MINOR 0 )
set( Bolt_VERSION_PATCH 1 )
set( Bolt_VERSION "${Bolt_VERSION_MAJOR}.${Bolt_VERSION_MINOR}.${Bolt_VERSION_PATCH}")
	
set( CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR} )

# On windows, it's convenient to change the default install prefix such that it does NOT point to 'program files'
# Need to check out CMAKE_RUNTIME_OUTPUT_DIRECTORY variable, and see if that eliminates the need to modify install path
if( CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT )
	set( CMAKE_INSTALL_PREFIX "${PROJECT_BINARY_DIR}/package" CACHE PATH "Install path prefix, prepended onto install directories" FORCE )
endif( )

# Set the default of CMAKE_BUILD_TYPE to be release, unless user specifies with -D.  MSVC_IDE does not use CMAKE_BUILD_TYPE
if( NOT MSVC_IDE AND NOT CMAKE_BUILD_TYPE )
  set( CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel." FORCE )
endif()

# Currently, linux has a problem outputing both narrow and wide characters,
# which happens in our client because openCL only supports narrow characters
if( WIN32 )
	option( UNICODE "Build with Unicode Support" ON )
	if( UNICODE )
		message( STATUS "UNICODE build" )
	endif( )
else()
	set( UNICODE OFF )
	message( STATUS "UNICODE feature disabled on linux" )
endif()

if( MSVC_IDE )
	set( Bolt_BUILD64 ${CMAKE_CL_64} )
else()
	option( Bolt_BUILD64 "Build a 64-bit product" ON )
endif()

# Modify the global find property to help us find libraries like Boost in the correct paths for 64-bit
# Essentially, find_library calls will look for /lib64 instead of /lib; works for windows and linux
if( Bolt_BUILD64 )
	set_property( GLOBAL PROPERTY FIND_LIBRARY_USE_LIB64_PATHS TRUE )
	message( STATUS "64bit build - FIND_LIBRARY_USE_LIB64_PATHS TRUE" )
else()
	set_property( GLOBAL PROPERTY FIND_LIBRARY_USE_LIB64_PATHS FALSE )
	message( STATUS "32bit build - FIND_LIBRARY_USE_LIB64_PATHS FALSE" )
endif()

# FFLAGS depend on the compiler, grab the compiler name from the path
get_filename_component( C_COMPILER_NAME ${CMAKE_C_COMPILER} NAME_WE )
# message( "C_COMPILER_NAME: " ${C_COMPILER_NAME} )
# message( "CMAKE_C_COMPILER: " ${CMAKE_C_COMPILER} )

# Set common compile and link options
if( C_COMPILER_NAME STREQUAL "cl" )
	# Following options for nMake
	message( STATUS "Detected MSVS Ver: " ${MSVC_VERSION} )
	if( MSVC_VERSION VERSION_LESS 1700 )
		message( FATAL_ERROR "Visual Studio 11 or higher is required because bolt uses AMP C++ features" )
	endif( )
elseif( C_COMPILER_NAME STREQUAL "gcc" )
	message( STATUS "Detected GNU fortran compiler." )
	message( FATAL_ERROR "GCC compiler not currently supported, because of AMP C++ syntax" )
	
	if( Bolt_BUILD64 )
		set( CMAKE_CXX_FLAGS "-m64 ${CMAKE_CXX_FLAGS}" )
		set( CMAKE_C_FLAGS "-m64 ${CMAKE_C_FLAGS}" )
	else( )
		set( CMAKE_CXX_FLAGS "-m32 ${CMAKE_CXX_FLAGS}" )
		set( CMAKE_C_FLAGS "-m32 ${CMAKE_C_FLAGS}" )
	endif( )
else( )
	message( FATAL_ERROR "Compiler name not detected" )
endif( )

# If UNICODE is defined, pass extra definitions into 
if( UNICODE )
	add_definitions( "/DUNICODE /D_UNICODE" )
endif( )

# Print out compiler flags for viewing/debug
message( STATUS "CMAKE_CXX_COMPILER flags: " ${CMAKE_CXX_FLAGS} )
message( STATUS "CMAKE_CXX_COMPILER debug flags: " ${CMAKE_CXX_FLAGS_DEBUG} )
message( STATUS "CMAKE_CXX_COMPILER release flags: " ${CMAKE_CXX_FLAGS_RELEASE} )
message( STATUS "CMAKE_CXX_COMPILER relwithdebinfo flags: " ${CMAKE_CXX_FLAGS_RELWITHDEBINFO} )
message( STATUS "CMAKE_EXE_LINKER link flags: " ${CMAKE_EXE_LINKER_FLAGS} )

# Depending on whether we are building for 64 or 32 bit, construct common paths and names that subdirectories can reference for their use
if( Bolt_BUILD64 )
	set( CPACK_PACKAGE_FILE_NAME "${CMAKE_PROJECT_NAME}-${Bolt_VERSION}-${CMAKE_HOST_SYSTEM_NAME}-x64")
    set( BIN_DIR bin64 )
    set( LIB_DIR lib64 )
else( )
	set( CPACK_PACKAGE_FILE_NAME "${CMAKE_PROJECT_NAME}-${Bolt_VERSION}-${CMAKE_HOST_SYSTEM_NAME}-x32")
    set( BIN_DIR bin32 )
    set( LIB_DIR lib32 )
endif( )
			
if( IS_DIRECTORY "${PROJECT_SOURCE_DIR}/BoltForC++AMP" AND WIN32 )
	add_subdirectory( BoltForC++AMP/tests/reduce )
	add_subdirectory( BoltForC++AMP/tests/transform )	
endif( )
	
if( IS_DIRECTORY "${PROJECT_SOURCE_DIR}/BoltForOpenCL" )
	# This will define OPENCL_FOUND
	find_package( OpenCL )

	if( OPENCL_FOUND )
		add_subdirectory( BoltForOpenCL/tests/TransformTest )
	endif( )
endif( )

# The following code is setting variables to control the behavior of CPack to generate our 
if( WIN32 )
	set( CPACK_SOURCE_GENERATOR "ZIP" )
	set( CPACK_GENERATOR "ZIP" )
else( )
	set( CPACK_SOURCE_GENERATOR "TGZ" )
	set( CPACK_GENERATOR "TGZ" )
endif( )

set( CPACK_SOURCE_PACKAGE_FILE_NAME "${CMAKE_PROJECT_NAME}-${Bolt_VERSION}-${CMAKE_HOST_SYSTEM_NAME}-Source")

set( CPACK_PACKAGE_VERSION_MAJOR ${Bolt_VERSION_MAJOR} )
set( CPACK_PACKAGE_VERSION_MINOR ${Bolt_VERSION_MINOR} )
set( CPACK_PACKAGE_VERSION_PATCH ${Bolt_VERSION_PATCH} )
set( CPACK_PACKAGE_DESCRIPTION_SUMMARY "Bolt reference implementation")
set( CPACK_PACKAGE_VENDOR "AMD")
set( CPACK_SOURCE_IGNORE_FILES "/\\\\.hg/;/\\\\.svn/;" )

# Define all variables that influence CPack before including CPack, such as install targets
include( CPack )
