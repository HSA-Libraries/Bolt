#############################################################################
##	Copyright (C) 2010 Advanced Micro Devices, Inc. All Rights Reserved.
#############################################################################
# List the names of common files to compile across all platforms
message( STATUS "Setting up benchmarking projects..." )
include( ExternalBoost )

include_directories( ${BOLT_INCLUDE_DIR} )

if( BUILD_ampBolt )
	add_subdirectory( AMP )
endif( )

if( BUILD_clBolt )
#	add_subdirectory( OpenCL )
endif( )
