#############################################################################
##	Copyright (C) 2010 Advanced Micro Devices, Inc. All Rights Reserved.
#############################################################################
# List the names of common files to compile across all platforms
message( STATUS "Setting up CL specific benchmarking projects" )

# Include standard OpenCL headers
include_directories( ${OPENCL_INCLUDE_DIRS} )

if( BUILD_clBolt )
	# Include standard OpenCL headers
	add_subdirectory( Scan )
	add_subdirectory( Sort )
endif( )
