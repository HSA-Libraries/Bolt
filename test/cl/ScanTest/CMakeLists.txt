#############################################################################
##	Copyright (C) 2010 Advanced Micro Devices, Inc. All Rights Reserved.
#############################################################################
# List the names of common files to compile across all platforms
set( clBolt.Test.Scan.Source stdafx.cpp Scan.cpp )
set( clBolt.Test.Scan.Headers 	stdafx.h 
								targetver.h 
								${BOLT_INCLUDE_DIR}/bolt/cl/scan.h 
								${BOLT_INCLUDE_DIR}/bolt/cl/detail/scan.inl )

set( clBolt.Test.Scan.Files ${clBolt.Test.Scan.Source} ${clBolt.Test.Scan.Headers} )

add_executable( clBolt.Test.Scan ${clBolt.Test.Scan.Files} )
target_link_libraries( clBolt.Test.Scan clBolt.Runtime ${GTEST_LIBRARIES} )

set_target_properties( clBolt.Test.Scan PROPERTIES VERSION ${Bolt_VERSION} )
set_target_properties( clBolt.Test.Scan PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/staging" )

set_property( TARGET clBolt.Test.Scan PROPERTY FOLDER "Test/OpenCL")

# CPack configuration; include the executable into the package
install( TARGETS clBolt.Test.Scan
	RUNTIME DESTINATION ${BIN_DIR}
	LIBRARY DESTINATION ${LIB_DIR}
	ARCHIVE DESTINATION ${LIB_DIR}/import
	)
