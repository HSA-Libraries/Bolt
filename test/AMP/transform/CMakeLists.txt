#############################################################################
##	Copyright (C) 2010 Advanced Micro Devices, Inc. All Rights Reserved.
#############################################################################
# List the names of common files to compile across all platforms
set( ampBolt.Test.Transform.Source stdafx.cpp simple_transform.cpp )
set( ampBolt.Test.Transform.Headers stdafx.h targetver.h ${BOLT_INCLUDE_DIR}/bolt/amp/transform.h )

set( ampBolt.Test.Transform.Files ${ampBolt.Test.Transform.Source} ${ampBolt.Test.Transform.Headers} )

add_executable( ampBolt.Test.Transform ${ampBolt.Test.Transform.Files} )

set_target_properties( ampBolt.Test.Transform PROPERTIES VERSION ${Bolt_VERSION} )
set_target_properties( ampBolt.Test.Transform PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/staging" )

set_property( TARGET ampBolt.Test.Transform PROPERTY FOLDER "Test/AMP")

# CPack configuration; include the executable into the package
install( TARGETS ampBolt.Test.Transform
	RUNTIME DESTINATION ${BIN_DIR}
	LIBRARY DESTINATION ${LIB_DIR}
	ARCHIVE DESTINATION ${LIB_DIR}/import
	)
