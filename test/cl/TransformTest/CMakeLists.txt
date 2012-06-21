#############################################################################
##	Copyright (C) 2010 Advanced Micro Devices, Inc. All Rights Reserved.
#############################################################################
# List the names of common files to compile across all platforms
set( clBolt.Test.Transform.Source stdafx.cpp TransformTest.cpp )
set( clBolt.Test.Transform.Headers stdafx.h targetver.h ${BOLT_INCLUDE_DIR}/bolt/cl/transform.h )

set( clBolt.Test.Transform.Files ${clBolt.Test.Transform.Source} ${clBolt.Test.Transform.Headers} )

# Include standard OpenCL headers
include_directories( ${OPENCL_INCLUDE_DIRS} )

add_executable( clBolt.Test.Transform ${clBolt.Test.Transform.Files} )
target_link_libraries( clBolt.Test.Transform ${OPENCL_LIBRARIES} clBolt.Runtime )

set_target_properties( clBolt.Test.Transform PROPERTIES VERSION ${Bolt_VERSION} )
set_target_properties( clBolt.Test.Transform PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/staging" )

set_property( TARGET clBolt.Test.Transform PROPERTY FOLDER "Test/OpenCL")
		
# CPack configuration; include the executable into the package
install( TARGETS clBolt.Test.Transform
	RUNTIME DESTINATION ${BIN_DIR}
	LIBRARY DESTINATION ${LIB_DIR}
	ARCHIVE DESTINATION ${LIB_DIR}/import
	)
