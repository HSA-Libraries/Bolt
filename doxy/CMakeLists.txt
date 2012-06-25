#############################################################################
##	Copyright (C) 2012 Advanced Micro Devices, Inc. All Rights Reserved.
#############################################################################
# List the names of common files to compile across all platforms
message( STATUS "Setting up documentation project..." )
include( ExternalDoxygen )

add_custom_target( 	Bolt.Documentation
					COMMAND ${DOXYGEN_EXECUTABLE} bolt.doxy.cfg
					WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
					DEPENDS Doxygen
					COMMENT "Generating Bolt API documentation with Doxygen" VERBATIM
				)