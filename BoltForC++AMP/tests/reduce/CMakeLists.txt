#############################################################################
##	Copyright (C) 2010 Advanced Micro Devices, Inc. All Rights Reserved.
#############################################################################
# List the names of common files to compile across all platforms
set( Bolt.Test.Reduce.Source stdafx.cpp simple_reduce.cpp )
set( Bolt.Test.Reduce.Headers stdafx.h targetver.h ${BOLT_DIR}/reduce.h )

set( Bolt.Test.Reduce.Files ${Bolt.Test.Reduce.Source} ${Bolt.Test.Reduce.Headers} )

# Include standard OpenCL headers
include_directories( ${PROJECT_SOURCE_DIR}/BoltForC++AMP )

add_executable( Bolt.Test.Reduce ${Bolt.Test.Reduce.Files} )

set_target_properties( Bolt.Test.Reduce PROPERTIES VERSION ${Bolt_VERSION} )
set_target_properties( Bolt.Test.Reduce PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/staging" )

set_property( TARGET Bolt.Test.Reduce PROPERTY FOLDER "Test/AMP")

# CPack configuration; include the executable into the package
install( TARGETS Bolt.Test.Reduce
	RUNTIME DESTINATION ${BIN_DIR}
	LIBRARY DESTINATION ${LIB_DIR}
	ARCHIVE DESTINATION ${LIB_DIR}/import
	)
