#############################################################################
##	Copyright (C) 2010 Advanced Micro Devices, Inc. All Rights Reserved.
#############################################################################
# List the names of common files to compile across all platforms
message( STATUS "Setting up OpenCL specific test projects..." )

add_subdirectory( CountTest )
add_subdirectory( ReduceTest )
add_subdirectory( TransformTest )	
add_subdirectory( TransformReduceTest )
