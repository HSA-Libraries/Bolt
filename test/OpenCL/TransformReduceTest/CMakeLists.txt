#############################################################################
##	Copyright (C) 2010 Advanced Micro Devices, Inc. All Rights Reserved.
#############################################################################
# List the names of common files to compile across all platforms

# set( testName clBolt.Test.TransformReduce )
set( clBolt.Test.TransformReduce.Source stdafx.cpp TransformReduceTest.cpp )
set( clBolt.Test.TransformReduce.Headers stdafx.h targetver.h ${CLBOLT_DIR}/transform_reduce.h )

set( clBolt.Test.TransformReduce.Files ${clBolt.Test.TransformReduce.Source} ${clBolt.Test.TransformReduce.Headers} )

# Include standard OpenCL headers
include_directories( ${PROJECT_SOURCE_DIR}/BoltForOpenCL ${OPENCL_INCLUDE_DIRS} )

add_executable( clBolt.Test.TransformReduce ${clBolt.Test.TransformReduce.Files} )
target_link_libraries( clBolt.Test.TransformReduce ${OPENCL_LIBRARIES} clBolt.Runtime )

set_target_properties( clBolt.Test.TransformReduce PROPERTIES VERSION ${Bolt_VERSION} )
set_target_properties( clBolt.Test.TransformReduce PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/staging" )

set_property( TARGET clBolt.Test.TransformReduce PROPERTY FOLDER "Test/OpenCL")

# CPack configuration; include the executable into the package
install( TARGETS clBolt.Test.TransformReduce
	RUNTIME DESTINATION ${BIN_DIR}
	LIBRARY DESTINATION ${LIB_DIR}
	ARCHIVE DESTINATION ${LIB_DIR}/import
	)
