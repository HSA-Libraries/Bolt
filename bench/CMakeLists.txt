#############################################################################
##	Copyright (C) 2010 Advanced Micro Devices, Inc. All Rights Reserved.
#############################################################################
# List the names of common files to compile across all platforms
message( STATUS "Setting up benchmarking projects..." )
include( ExternalBoost )

if( BUILD_ampBolt )
	# Include standard OpenCL headers
	include_directories( ${PROJECT_SOURCE_DIR}/BoltForC++AMP )

	add_subdirectory( AMP )
endif( )

if( BUILD_clBolt )
#	add_subdirectory( OpenCL )
endif( )
