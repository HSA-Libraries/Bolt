#############################################################################
##	Copyright (C) 2012 Advanced Micro Devices, Inc. All Rights Reserved.
#############################################################################
# List the names of common files to compile across all platforms
message( STATUS "Setting up testing projects..." )

if( GTEST_ROOT )
	# The root tree of BOOST was specified on the command line; use it to to find the specific gTest the user wants
    find_package( GTest REQUIRED )
	# This will define GTEST_FOUND
else( )
	message( "Configure Bolt in <BOLT_ROOT>/bin to build the SuperBuild which will download and build GoogleTest automatically" )	
	message( SEND_ERROR "The testing projects for Bolt requires GTEST_ROOT to be defined" )	
endif( )

# Include Bolt headers for all testing projects
include_directories( ${BOLT_INCLUDE_DIR} ${PROJECT_BINARY_DIR}/include ${GTEST_INCLUDE_DIRS})

# Googletest requires this to be defined to 10 in order to compile on windows, because windows does not support variable length templates yet
add_definitions( "-D_VARIADIC_MAX=10" )

if( BUILD_ampBolt AND GTEST_FOUND )
	add_subdirectory( AMP )
endif( )

if( BUILD_clBolt AND GTEST_FOUND )
	add_subdirectory( cl )
endif( )
