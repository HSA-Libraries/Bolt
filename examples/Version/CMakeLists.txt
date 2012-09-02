#############################################################################
##	Copyright (C) 2010 Advanced Micro Devices, Inc. All Rights Reserved.
#############################################################################
# List the names of common files to compile across all platforms
set( clBolt.Example.Version.Source stdafx.cpp version.cpp )
set( clBolt.Example.Version.Headers stdafx.h 
								targetver.h 
								${BOLT_INCLUDE_DIRS}/bolt/cl/bolt.h 
                                )

set( clBolt.Example.Version.Files ${clBolt.Example.Version.Source} ${clBolt.Example.Version.Headers} )

# Include headers files
include_directories( ${BOLT_INCLUDE_DIRS} ${OPENCL_INCLUDE_DIRS} )

add_executable( clBolt.Example.Version ${clBolt.Example.Version.Files} )
target_link_libraries( clBolt.Example.Version ${BOLT_LIBRARIES} ${OPENCL_LIBRARIES} )

set_target_properties( clBolt.Example.Version PROPERTIES VERSION ${Examples_VERSION} )
set_target_properties( clBolt.Example.Version PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/staging" )

# CPack configuration; include the executable into the package
install( TARGETS clBolt.Example.Version
	RUNTIME DESTINATION ${BIN_DIR}
	LIBRARY DESTINATION ${LIB_DIR}
	ARCHIVE DESTINATION ${LIB_DIR}/import
	)
