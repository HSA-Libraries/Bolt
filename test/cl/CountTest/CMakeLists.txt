#############################################################################
##	Copyright (C) 2010 Advanced Micro Devices, Inc. All Rights Reserved.
#############################################################################
# List the names of common files to compile across all platforms

set( clBolt.Test.Count.Source stdafx.cpp CountTest.cpp )
set( clBolt.Test.Count.Headers stdafx.h targetver.h ${BOLT_INCLUDE_DIR}/bolt/cl/count.h )

set( clBolt.Test.Count.Files ${clBolt.Test.Count.Source} ${clBolt.Test.Count.Headers} )

add_executable( clBolt.Test.Count ${clBolt.Test.Count.Files} )
target_link_libraries( clBolt.Test.Count clBolt.Runtime ${OPENCL_LIBRARIES} ${GTEST_LIBRARIES} )

set_target_properties( clBolt.Test.Count PROPERTIES VERSION ${Bolt_VERSION} )
set_target_properties( clBolt.Test.Count PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/staging" )

set_property( TARGET clBolt.Test.Count PROPERTY FOLDER "Test/OpenCL")
		
# CPack configuration; include the executable into the package
install( TARGETS clBolt.Test.Count
	RUNTIME DESTINATION ${BIN_DIR}
	LIBRARY DESTINATION ${LIB_DIR}
	ARCHIVE DESTINATION ${LIB_DIR}/import
	)
