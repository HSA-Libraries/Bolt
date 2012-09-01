#############################################################################
##	Copyright (C) 2010 Advanced Micro Devices, Inc. All Rights Reserved.
#############################################################################
# List the names of common files to compile across all platforms
set( clBolt.Example.Scan.Source stdafx.cpp Scan.cpp )
set( clBolt.Example.Scan.Headers 	stdafx.h 
								targetver.h 
								${BOLT_INCLUDE_DIRS}/bolt/cl/scan.h 
								${BOLT_INCLUDE_DIRS}/bolt/cl/detail/scan.inl )

set( clBolt.Example.Scan.Files ${clBolt.Example.Scan.Source} ${clBolt.Example.Scan.Headers} )

# Include headers files
include_directories( ${BOLT_INCLUDE_DIRS} ${OPENCL_INCLUDE_DIRS} )

add_executable( clBolt.Example.Scan ${clBolt.Example.Scan.Files} )
target_link_libraries( clBolt.Example.Scan ${BOLT_LIBRARIES} ${OPENCL_LIBRARIES} )

set_target_properties( clBolt.Example.Scan PROPERTIES VERSION ${Examples_VERSION} )
set_target_properties( clBolt.Example.Scan PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/staging" )

# set_property( TARGET clBolt.Example.Scan PROPERTY FOLDER "Test/OpenCL")
install( FILES
            ${BOLT_INCLUDE_DIRS}/../${BIN_DIR}/scan_kernels.cl
		DESTINATION
			${BIN_DIR} )

install( FILES
            ${BOLT_INCLUDE_DIRS}/../${BIN_DIR}/scan_kernels.cl
		DESTINATION
			${PROJECT_BINARY_DIR}/staging/\${CMAKE_INSTALL_CONFIG_NAME} )

# CPack configuration; include the executable into the package
install( TARGETS clBolt.Example.Scan
	RUNTIME DESTINATION ${BIN_DIR}
	LIBRARY DESTINATION ${LIB_DIR}
	ARCHIVE DESTINATION ${LIB_DIR}/import
	)
