#############################################################################
##	Copyright (C) 2010 Advanced Micro Devices, Inc. All Rights Reserved.
#############################################################################
# List the names of common files to compile across all platforms
set( clBolt.Test.Device.Vector.Source stdafx.cpp device.vector.test.cpp )
set( clBolt.Test.Device.Vector.Headers stdafx.h targetver.h ${BOLT_INCLUDE_DIR}/bolt/cl/device_vector.h )

set( clBolt.Test.Device.Vector.Files ${clBolt.Test.Device.Vector.Source} ${clBolt.Test.Device.Vector.Headers} )

add_executable( clBolt.Test.Device.Vector ${clBolt.Test.Device.Vector.Files} )
target_link_libraries( clBolt.Test.Device.Vector clBolt.Runtime ${GTEST_LIBRARIES} )

set_target_properties( clBolt.Test.Device.Vector PROPERTIES VERSION ${Bolt_VERSION} )
set_target_properties( clBolt.Test.Device.Vector PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/staging" )

set_property( TARGET clBolt.Test.Device.Vector PROPERTY FOLDER "Test/OpenCL")

# CPack configuration; include the executable into the package
install( TARGETS clBolt.Test.Device.Vector
	RUNTIME DESTINATION ${BIN_DIR}
	LIBRARY DESTINATION ${LIB_DIR}
	ARCHIVE DESTINATION ${LIB_DIR}/import
	)
