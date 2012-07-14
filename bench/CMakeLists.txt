#############################################################################
##	Copyright (C) 2010 Advanced Micro Devices, Inc. All Rights Reserved.
#############################################################################
# List the names of common files to compile across all platforms
message( STATUS "Setting up benchmarking projects..." )

set( Boost_USE_MULTITHREADED ON )
set( Boost_USE_STATIC_LIBS   ON )
set( Boost_DETAILED_FAILURE_MSG   ON )
set( Boost_DEBUG ON )

if( BOOST_ROOT )
	# The root tree of BOOST was specified on the command line; use it to to find the specific Boost the user points too
	find_package( Boost ${Boost.VERSION} COMPONENTS program_options REQUIRED )
	# This will define Boost_FOUND
	message( STATUS "Boost_PROGRAM_OPTIONS_LIBRARY: ${Boost_PROGRAM_OPTIONS_LIBRARY}" )
else( )
	message( "Configure Bolt in <BOLT_ROOT>/bin to build the SuperBuild which will download and build Boost automatically" )	
	message( SEND_ERROR "The Benchmarking projects for Bolt requires BOOST_ROOT to be defined" )
endif( )

if( BUILD_ampBolt AND Boost_FOUND )
	add_subdirectory( AMP )
endif( )

if( BUILD_clBolt AND Boost_FOUND )
#	add_subdirectory( OpenCL )
endif( )

install( FILES	
			errorHandler.py
			measurePerformance.py
			performanceTesting.py
			plotPerformance.py
		DESTINATION
			${BIN_DIR} )

install( FILES	
			errorHandler.py
			measurePerformance.py
			performanceTesting.py
			plotPerformance.py
		DESTINATION
			${PROJECT_BINARY_DIR}/staging/\${CMAKE_INSTALL_CONFIG_NAME} )
