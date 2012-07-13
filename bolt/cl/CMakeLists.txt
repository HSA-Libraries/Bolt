#############################################################################
##	Copyright (C) 2010 Advanced Micro Devices, Inc. All Rights Reserved.
#############################################################################
# List the names of common files to compile across all platforms

set( clBolt.clInclude.Dir ${BOLT_INCLUDE_DIR}/bolt/cl )

set( clBolt.Runtime.Source bolt.cpp ${BOLT_LIBRARY_DIR}/statisticalTimer.cpp )
set( clBolt.Runtime.Headers 
		${clBolt.clInclude.Dir}/bolt.h 
		${clBolt.clInclude.Dir}/control.h 
		${clBolt.clInclude.Dir}/count.h 
		${clBolt.clInclude.Dir}/functional.h 
		${clBolt.clInclude.Dir}/iterator_traits.h 
		${clBolt.clInclude.Dir}/reduce.h 
		${clBolt.clInclude.Dir}/transform.h 
		${clBolt.clInclude.Dir}/transform_reduce.h )
set( clBolt.Runtime.Headers.Detail 
		${clBolt.clInclude.Dir}/detail/reduce.inl )
set( clBolt.Runtime.clFiles 
		${clBolt.clInclude.Dir}/reduce_kernels.cl 
		${clBolt.clInclude.Dir}/transform_kernels.cl 
		${clBolt.clInclude.Dir}/transform_reduce_kernels.cl )

set( clBolt.Runtime.Files ${clBolt.Runtime.Source} ${clBolt.Runtime.Headers} ${clBolt.Runtime.Headers.Detail} ${clBolt.Runtime.clFiles} )

# Include standard OpenCL headers
include_directories( ${BOLT_INCLUDE_DIR} ${OPENCL_INCLUDE_DIRS} )

add_library( clBolt.Runtime STATIC ${clBolt.Runtime.Files} )
target_link_libraries( clBolt.Runtime ${OPENCL_LIBRARIES} )

set_target_properties( clBolt.Runtime PROPERTIES VERSION ${Bolt_VERSION} )
set_target_properties( clBolt.Runtime PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/staging" )
		
install( FILES	
			${clBolt.clInclude.Dir}/reduce_kernels.cl
			${clBolt.clInclude.Dir}/transform_kernels.cl
			${clBolt.clInclude.Dir}/transform_reduce_kernels.cl
		DESTINATION
			${BIN_DIR} )

install( FILES	
			${clBolt.clInclude.Dir}/reduce_kernels.cl
			${clBolt.clInclude.Dir}/transform_kernels.cl
			${clBolt.clInclude.Dir}/transform_reduce_kernels.cl
		DESTINATION
			${PROJECT_BINARY_DIR}/staging/\${CMAKE_INSTALL_CONFIG_NAME} )

# CPack configuration; include the executable into the package
install( TARGETS clBolt.Runtime
	RUNTIME DESTINATION ${BIN_DIR}
	LIBRARY DESTINATION ${LIB_DIR}
	ARCHIVE DESTINATION ${LIB_DIR}/import
	)
