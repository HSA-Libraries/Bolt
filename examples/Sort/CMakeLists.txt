#############################################################################
##	Copyright (C) 2010 Advanced Micro Devices, Inc. All Rights Reserved.
#############################################################################
# List the names of common files to compile across all platforms
set( clBolt.Example.Sort.Source sort.cpp myocl.cpp )
set( clBolt.Example.Sort.Headers ${BOLT_INCLUDE_DIRS}/bolt/cl/sort.h ${BOLT_INCLUDE_DIRS}/bolt/cl/detail/sort.inl )

set( clBolt.Example.Sort.Files ${clBolt.Example.Sort.Source} ${clBolt.Example.Sort.Headers} )

# Include headers files
include_directories( ${BOLT_INCLUDE_DIRS} ${OPENCL_INCLUDE_DIRS} )

add_executable( clBolt.Example.Sort ${clBolt.Example.Sort.Files} )
target_link_libraries( clBolt.Example.Sort ${BOLT_LIBRARIES} ${OPENCL_LIBRARIES} ${Boost_LIBRARIES} )

set_target_properties( clBolt.Example.Sort PROPERTIES VERSION ${Examples_VERSION} )
set_target_properties( clBolt.Example.Sort PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/staging" )

# set_property( TARGET clBolt.Example.Sort PROPERTY FOLDER "Test/OpenCL")
install( FILES
            ${BOLT_INCLUDE_DIRS}/../${BIN_DIR}/sort_kernels.cl
		DESTINATION
			${BIN_DIR} )

install( FILES
            ${BOLT_INCLUDE_DIRS}/../${BIN_DIR}/sort_kernels.cl
		DESTINATION
			${PROJECT_BINARY_DIR}/staging/\${CMAKE_INSTALL_CONFIG_NAME} )

# CPack configuration; include the executable into the package
install( TARGETS clBolt.Example.Sort
	RUNTIME DESTINATION ${BIN_DIR}
	LIBRARY DESTINATION ${LIB_DIR}
	ARCHIVE DESTINATION ${LIB_DIR}/import
	)
