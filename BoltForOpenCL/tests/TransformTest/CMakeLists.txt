#############################################################################
##	Copyright (C) 2010 Advanced Micro Devices, Inc. All Rights Reserved.
#############################################################################
# List the names of common files to compile across all platforms
set( clBolt.Transform.Source stdafx.cpp TransformTest.cpp )
set( clBolt.Transform.Headers stdafx.h targetver.h ${BOLTCL_DIR}/transform.h )

set( clBolt.Transform.Files ${clBolt.Transform.Source} ${clBolt.Transform.Headers} )

# Include standard OpenCL headers
include_directories( ${PROJECT_SOURCE_DIR}/BoltForOpenCL ${OPENCL_INCLUDE_DIRS} )

add_executable( clBolt.Transform ${clBolt.Transform.Files} )
target_link_libraries( clBolt.Transform ${OPENCL_LIBRARIES} clBolt.Runtime )

set_target_properties( clBolt.Transform PROPERTIES VERSION ${Bolt_VERSION} )
set_target_properties( clBolt.Transform PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/staging" )
		
# CPack configuration; include the executable into the package
install( TARGETS clBolt.Transform
	RUNTIME DESTINATION ${BIN_DIR}
	LIBRARY DESTINATION ${LIB_DIR}
	ARCHIVE DESTINATION ${LIB_DIR}/import
	)
