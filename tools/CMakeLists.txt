#############################################################################
##	Copyright (C) 2012 Advanced Micro Devices, Inc. All Rights Reserved.
#############################################################################
# List the names of common files to compile across all platforms
message( STATUS "Setting up tools projects..." )

# Include Bolt headers for all testing projects
include_directories( ${BOLT_INCLUDE_DIR} ${PROJECT_BINARY_DIR}/include )

if( BUILD_clBolt )
	add_subdirectory( StringifyKernels )
endif( )
