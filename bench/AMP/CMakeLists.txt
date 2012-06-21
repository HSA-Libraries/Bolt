#############################################################################
##	Copyright (C) 2010 Advanced Micro Devices, Inc. All Rights Reserved.
#############################################################################
# List the names of common files to compile across all platforms
message( STATUS "Setting up AMP specific benchmarking projects" )

include_directories( ${BOLT_INCLUDE_DIR} )

if( BUILD_ampBolt )
	# Include standard OpenCL headers
	add_subdirectory( Scan )
endif( )
