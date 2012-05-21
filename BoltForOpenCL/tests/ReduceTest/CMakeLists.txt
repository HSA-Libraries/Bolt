#############################################################################
##	Copyright (C) 2010 Advanced Micro Devices, Inc. All Rights Reserved.
#############################################################################
# List the names of common files to compile across all platforms

set( clBolt.Reduce.Source stdafx.cpp ReduceTest.cpp )
set( clBolt.Reduce.Headers stdafx.h targetver.h ${BOLTCL_DIR}/reduce.h )

set( clBolt.Reduce.Files ${clBolt.Reduce.Source} ${clBolt.Reduce.Headers} )

# Include standard OpenCL headers
include_directories( ${PROJECT_SOURCE_DIR}/BoltForOpenCL ${OPENCL_INCLUDE_DIRS} )

add_executable( clBolt.Reduce ${clBolt.Reduce.Files} )
target_link_libraries( clBolt.Reduce ${OPENCL_LIBRARIES} clBolt.Runtime )

set_target_properties( clBolt.Reduce PROPERTIES VERSION ${Bolt_VERSION} )
set_target_properties( clBolt.Reduce PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/staging" )
		
# CPack configuration; include the executable into the package
install( TARGETS clBolt.Reduce
	RUNTIME DESTINATION ${BIN_DIR}
	LIBRARY DESTINATION ${LIB_DIR}
	ARCHIVE DESTINATION ${LIB_DIR}/import
	)
