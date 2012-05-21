#############################################################################
##	Copyright (C) 2010 Advanced Micro Devices, Inc. All Rights Reserved.
#############################################################################
# List the names of common files to compile across all platforms

set( clBolt.Runtime.Source bolt.cpp )
set( clBolt.Runtime.Headers bolt.h functional.h reduce.h transform.h transform_reduce.h )
set( clBolt.Runtime.clFiles reduce_kernels.cl transform_kernels.cl transform_reduce_kernels.cl )

set( clBolt.Runtime.Files ${clBolt.Runtime.Source} ${clBolt.Runtime.Headers} ${clBolt.Runtime.clFiles} )

# Include standard OpenCL headers
include_directories( ${PROJECT_SOURCE_DIR}/BoltForOpenCL ${OPENCL_INCLUDE_DIRS} )

add_library( clBolt.Runtime STATIC ${clBolt.Runtime.Files} )
target_link_libraries( clBolt.Runtime ${OPENCL_LIBRARIES} )

set_target_properties( clBolt.Runtime PROPERTIES VERSION ${Bolt_VERSION} )
set_target_properties( clBolt.Runtime PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/staging" )
		
install( FILES	
			reduce_kernels.cl
			transform_kernels.cl
			transform_reduce_kernels.cl
		DESTINATION
			${BIN_DIR} )

install( FILES	
			reduce_kernels.cl
			transform_kernels.cl
			transform_reduce_kernels.cl
		DESTINATION
			${PROJECT_BINARY_DIR}/staging/\${CMAKE_INSTALL_CONFIG_NAME} )

# CPack configuration; include the executable into the package
install( TARGETS clBolt.Runtime
	RUNTIME DESTINATION ${BIN_DIR}
	LIBRARY DESTINATION ${LIB_DIR}
	ARCHIVE DESTINATION ${LIB_DIR}/import
	)
