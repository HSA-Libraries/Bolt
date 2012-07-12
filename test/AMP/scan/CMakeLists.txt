#############################################################################
##	Copyright (C) 2010 Advanced Micro Devices, Inc. All Rights Reserved.
#############################################################################
# List the names of common files to compile across all platforms
set( ampBolt.Test.Scan.Source stdafx.cpp Scan.cpp )
set( ampBolt.Test.Scan.Headers stdafx.h targetver.h ${BOLT_INCLUDE_DIR}/bolt/amp/Scan.h )

set( ampBolt.Test.Scan.Files ${ampBolt.Test.Scan.Source} ${ampBolt.Test.Scan.Headers} )

add_executable( ampBolt.Test.Scan ${ampBolt.Test.Scan.Files} )
target_link_libraries( ampBolt.Test.Scan ${GTEST_BOTH_LIBRARIES} ampBolt.Runtime )

set_target_properties( ampBolt.Test.Scan PROPERTIES VERSION ${Bolt_VERSION} )
set_target_properties( ampBolt.Test.Scan PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/staging" )

set_property( TARGET ampBolt.Test.Scan PROPERTY FOLDER "Test/AMP")

# CPack configuration; include the executable into the package
install( TARGETS ampBolt.Test.Scan
	RUNTIME DESTINATION ${BIN_DIR}
	LIBRARY DESTINATION ${LIB_DIR}
	ARCHIVE DESTINATION ${LIB_DIR}/import
	)
