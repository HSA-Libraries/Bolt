#############################################################################
##	Copyright (C) 2012 Advanced Micro Devices, Inc. All Rights Reserved.
#############################################################################
# List the names of common files to compile across all platforms
message( STATUS "Setting up documentation project..." )

set( Doxygen.Dependencies )

if( DOXYGEN_ROOT )
	# Kind of sad that the findDoxygen module has not concept of DOXYGEN_ROOT
	find_program( DOXYGEN_EXECUTABLE
		NAMES doxygen
		HINTS
	  		${DOXYGEN_ROOT}
		PATHS
			"[HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\doxygen_is1;Inno Setup: App Path]/bin"
		PATH_SUFFIXES
			bin
	)
else( )
	include( ExternalDoxygen )
	
	list( APPEND Doxygen.Dependencies Doxygen )
	
endif( )

add_custom_target( 	Bolt.Documentation
					COMMAND ${DOXYGEN_EXECUTABLE} bolt.doxy.cfg
					WORKING_DIRECTORY ${CMAKE_BINARY_SOURCE_DIR}
					DEPENDS ${Doxygen.Dependencies}
					COMMENT "Generating Bolt API documentation with Doxygen" VERBATIM
				)