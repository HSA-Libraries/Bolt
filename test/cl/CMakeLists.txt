#############################################################################
##	Copyright (C) 2010 Advanced Micro Devices, Inc. All Rights Reserved.
#############################################################################
# List the names of common files to compile across all platforms
message( STATUS "Setting up OpenCL specific test projects..." )

# Include standard OpenCL headers
include_directories( ${OPENCL_INCLUDE_DIRS} )

add_subdirectory( CountTest )
add_subdirectory( DeviceVectorTest )
add_subdirectory( ReduceTest )
add_subdirectory( SortTest )
add_subdirectory( TransformTest )	
add_subdirectory( TransformReduceTest )
