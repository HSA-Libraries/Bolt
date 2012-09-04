#############################################################################
##	Copyright (C) 2010 Advanced Micro Devices, Inc. All Rights Reserved.
#############################################################################
# List the names of common files to compile across all platforms
set( clBolt.Example.Device.Vector.Source stdafx.cpp deviceVector.cpp )
set( clBolt.Example.Device.Vector.Headers 	stdafx.h 
								targetver.h 
								${BOLT_INCLUDE_DIRS}/bolt/cl/device_Vector.h )

set( clBolt.Example.Device.Vector.Files ${clBolt.Example.Device.Vector.Source} ${clBolt.Example.Device.Vector.Headers} )

# Include headers files
include_directories( ${BOLT_INCLUDE_DIRS} ${OPENCL_INCLUDE_DIRS} )

add_executable( clBolt.Example.Device.Vector ${clBolt.Example.Device.Vector.Files} )
target_link_libraries( clBolt.Example.Device.Vector ${BOLT_LIBRARIES} ${OPENCL_LIBRARIES} )

set_target_properties( clBolt.Example.Device.Vector PROPERTIES VERSION ${Examples_VERSION} )
set_target_properties( clBolt.Example.Device.Vector PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/staging" )

# CPack configuration; include the executable into the package
install( TARGETS clBolt.Example.Device.Vector
	RUNTIME DESTINATION ${BIN_DIR}
	LIBRARY DESTINATION ${LIB_DIR}
	ARCHIVE DESTINATION ${LIB_DIR}/import
	)
