#############################################################################
##	Copyright (C) 2010 Advanced Micro Devices, Inc. All Rights Reserved.
#############################################################################
# List the names of common files to compile across all platforms
set( Bolt.Test.Transform.Source stdafx.cpp simple_transform.cpp )
set( Bolt.Test.Transform.Headers stdafx.h targetver.h ${BOLT_INCLUDE_DIR}/bolt/amp/transform.h )

set( Bolt.Test.Transform.Files ${Bolt.Test.Transform.Source} ${Bolt.Test.Transform.Headers} )

add_executable( Bolt.Test.Transform ${Bolt.Test.Transform.Files} )

set_target_properties( Bolt.Test.Transform PROPERTIES VERSION ${Bolt_VERSION} )
set_target_properties( Bolt.Test.Transform PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/staging" )

set_property( TARGET Bolt.Test.Transform PROPERTY FOLDER "Test/AMP")

# CPack configuration; include the executable into the package
install( TARGETS Bolt.Test.Transform
	RUNTIME DESTINATION ${BIN_DIR}
	LIBRARY DESTINATION ${LIB_DIR}
	ARCHIVE DESTINATION ${LIB_DIR}/import
	)
