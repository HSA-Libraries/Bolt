#############################################################################
##	Copyright (C) 2010 Advanced Micro Devices, Inc. All Rights Reserved.
#############################################################################
# List the names of common files to compile across all platforms

set( clBolt.TransformReduce.Source stdafx.cpp TransformReduceTest.cpp )
set( clBolt.TransformReduce.Headers stdafx.h targetver.h ${BOLTCL_DIR}/transform_reduce.h )

set( clBolt.TransformReduce.Files ${clBolt.TransformReduce.Source} ${clBolt.TransformReduce.Headers} )

# Include standard OpenCL headers
include_directories( ${PROJECT_SOURCE_DIR}/BoltForOpenCL ${OPENCL_INCLUDE_DIRS} )

add_executable( clBolt.TransformReduce ${clBolt.TransformReduce.Files} )
target_link_libraries( clBolt.TransformReduce ${OPENCL_LIBRARIES} clBolt.Runtime )

set_target_properties( clBolt.TransformReduce PROPERTIES VERSION ${Bolt_VERSION} )
set_target_properties( clBolt.TransformReduce PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/staging" )

# CPack configuration; include the executable into the package
install( TARGETS clBolt.TransformReduce
	RUNTIME DESTINATION ${BIN_DIR}
	LIBRARY DESTINATION ${LIB_DIR}
	ARCHIVE DESTINATION ${LIB_DIR}/import
	)
