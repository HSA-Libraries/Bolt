#############################################################################
##	Copyright (C) 2012 Advanced Micro Devices, Inc. All Rights Reserved.
#############################################################################
# List the names of common files to compile across all platforms
message( STATUS "Setting up testing projects..." )

# Include Bolt headers for all testing projects
include_directories( ${BOLT_INCLUDE_DIR} )

if( BUILD_ampBolt )
	add_subdirectory( AMP )
endif( )

if( BUILD_clBolt )
	add_subdirectory( cl )
endif( )
