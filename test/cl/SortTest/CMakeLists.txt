#############################################################################
##	Copyright (C) 2010 Advanced Micro Devices, Inc. All Rights Reserved.
#############################################################################
# List the names of common files to compile across all platforms

set( clBolt.Test.Sort.Source SortTest.cpp myocl.cpp)
set( clBolt.Test.Sort.Headers  ${BOLT_INCLUDE_DIR}/bolt/cl/sort.h myocl.h)

set( clBolt.Test.Sort.Files ${clBolt.Test.Sort.Source} ${clBolt.Test.Sort.Headers} )

# Include standard OpenCL headers
include_directories( ${OPENCL_INCLUDE_DIRS} )

add_executable( clBolt.Test.Sort ${clBolt.Test.Sort.Files} )
target_link_libraries( clBolt.Test.Sort ${OPENCL_LIBRARIES} clBolt.Runtime )

set_target_properties( clBolt.Test.Sort PROPERTIES VERSION ${Bolt_VERSION} )
set_target_properties( clBolt.Test.Sort PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/staging" )

set_property( TARGET clBolt.Test.Sort PROPERTY FOLDER "Test/OpenCL")
		
# CPack configuration; include the executable into the package
install( TARGETS clBolt.Test.Sort
	RUNTIME DESTINATION ${BIN_DIR}
	LIBRARY DESTINATION ${LIB_DIR}
	ARCHIVE DESTINATION ${LIB_DIR}/import
	)
