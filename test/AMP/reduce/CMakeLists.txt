#############################################################################
##	Copyright (C) 2010 Advanced Micro Devices, Inc. All Rights Reserved.
#############################################################################
# List the names of common files to compile across all platforms
set( ampBolt.Test.Reduce.Source stdafx.cpp simple_reduce.cpp )
set( ampBolt.Test.Reduce.Headers stdafx.h targetver.h ${BOLT_INCLUDE_DIR}/bolt/amp/reduce.h )

set( ampBolt.Test.Reduce.Files ${ampBolt.Test.Reduce.Source} ${ampBolt.Test.Reduce.Headers} )

add_executable( ampBolt.Test.Reduce ${ampBolt.Test.Reduce.Files} )
target_link_libraries( ampBolt.Test.Reduce ${GTEST_BOTH_LIBRARIES} ampBolt.Runtime )

set_target_properties( ampBolt.Test.Reduce PROPERTIES VERSION ${Bolt_VERSION} )
set_target_properties( ampBolt.Test.Reduce PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/staging" )

set_property( TARGET ampBolt.Test.Reduce PROPERTY FOLDER "Test/AMP")

# CPack configuration; include the executable into the package
install( TARGETS ampBolt.Test.Reduce
	RUNTIME DESTINATION ${BIN_DIR}
	LIBRARY DESTINATION ${LIB_DIR}
	ARCHIVE DESTINATION ${LIB_DIR}/import
	)
