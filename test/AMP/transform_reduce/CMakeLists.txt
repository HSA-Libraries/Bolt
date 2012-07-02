#############################################################################
##	Copyright (C) 2010 Advanced Micro Devices, Inc. All Rights Reserved.
#############################################################################
# List the names of common files to compile across all platforms
set( ampBolt.Test.TransformReduce.Source stdafx.cpp simple_transform_reduce.cpp )
set( ampBolt.Test.TransformReduce.Headers stdafx.h targetver.h ${BOLT_INCLUDE_DIR}/bolt/amp/transform_reduce.h)

set( ampBolt.Test.TransformReduce.Files ${ampBolt.Test.TransformReduce.Source} ${ampBolt.Test.TransformReduce.Headers} )

add_executable( ampBolt.Test.TransformReduce ${ampBolt.Test.TransformReduce.Files} )

set_target_properties( ampBolt.Test.TransformReduce PROPERTIES VERSION ${Bolt_VERSION} )
set_target_properties( ampBolt.Test.TransformReduce PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/staging" )

set_property( TARGET ampBolt.Test.TransformReduce PROPERTY FOLDER "Test/AMP")

# CPack configuration; include the executable into the package
install( TARGETS ampBolt.Test.TransformReduce
	RUNTIME DESTINATION ${BIN_DIR}
	LIBRARY DESTINATION ${LIB_DIR}
	ARCHIVE DESTINATION ${LIB_DIR}/import
	)
