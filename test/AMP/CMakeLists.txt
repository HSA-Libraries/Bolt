#############################################################################
##	Copyright (C) 2010 Advanced Micro Devices, Inc. All Rights Reserved.
#############################################################################
# List the names of common files to compile across all platforms
message( STATUS "Setting up AMP specific test projects..." )

add_subdirectory( reduce )
add_subdirectory( scan )
add_subdirectory( transform )	
add_subdirectory( transform_reduce )
