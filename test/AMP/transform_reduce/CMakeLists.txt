#############################################################################
##	Copyright (C) 2010 Advanced Micro Devices, Inc. All Rights Reserved.
#############################################################################
# List the names of common files to compile across all platforms
set( Bolt.Test.TransformReduce.Source stdafx.cpp simple_transform_reduce.cpp )
set( Bolt.Test.TransformReduce.Headers stdafx.h targetver.h ${BOLT_INCLUDE_DIR}/bolt/amp/transform_reduce.h)

set( Bolt.Test.TransformReduce.Files ${Bolt.Test.TransformReduce.Source} ${Bolt.Test.TransformReduce.Headers} )

add_executable( Bolt.Test.TransformReduce ${Bolt.Test.TransformReduce.Files} )

set_target_properties( Bolt.Test.TransformReduce PROPERTIES VERSION ${Bolt_VERSION} )
set_target_properties( Bolt.Test.TransformReduce PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/staging" )

set_property( TARGET Bolt.Test.TransformReduce PROPERTY FOLDER "Test/AMP")

# CPack configuration; include the executable into the package
install( TARGETS Bolt.Test.TransformReduce
	RUNTIME DESTINATION ${BIN_DIR}
	LIBRARY DESTINATION ${LIB_DIR}
	ARCHIVE DESTINATION ${LIB_DIR}/import
	)
