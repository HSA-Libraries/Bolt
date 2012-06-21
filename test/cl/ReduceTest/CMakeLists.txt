#############################################################################
##	Copyright (C) 2010 Advanced Micro Devices, Inc. All Rights Reserved.
#############################################################################
# List the names of common files to compile across all platforms

set( clBolt.Test.Reduce.Source stdafx.cpp ReduceTest.cpp myocl.cpp)
set( clBolt.Test.Reduce.Headers stdafx.h myocl.h targetver.h ${CLBOLT_DIR}/reduce.h )

set( clBolt.Test.Reduce.Files ${clBolt.Test.Reduce.Source} ${clBolt.Test.Reduce.Headers} )

# Include standard OpenCL headers
include_directories( ${PROJECT_SOURCE_DIR}/BoltForOpenCL ${OPENCL_INCLUDE_DIRS} )

add_executable( clBolt.Test.Reduce ${clBolt.Test.Reduce.Files} )
target_link_libraries( clBolt.Test.Reduce ${OPENCL_LIBRARIES} clBolt.Runtime )

set_target_properties( clBolt.Test.Reduce PROPERTIES VERSION ${Bolt_VERSION} )
set_target_properties( clBolt.Test.Reduce PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/staging" )

set_property( TARGET clBolt.Test.Reduce PROPERTY FOLDER "Test/OpenCL")
		
# CPack configuration; include the executable into the package
install( TARGETS clBolt.Test.Reduce
	RUNTIME DESTINATION ${BIN_DIR}
	LIBRARY DESTINATION ${LIB_DIR}
	ARCHIVE DESTINATION ${LIB_DIR}/import
	)
