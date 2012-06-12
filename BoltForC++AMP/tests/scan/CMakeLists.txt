#############################################################################
##	Copyright (C) 2010 Advanced Micro Devices, Inc. All Rights Reserved.
#############################################################################
# List the names of common files to compile across all platforms
set( Bolt.Test.Scan.Source stdafx.cpp Scan.cpp )
set( Bolt.Test.Scan.Headers stdafx.h targetver.h ${BOLT_DIR}/Scan.h )

set( Bolt.Test.Scan.Files ${Bolt.Test.Scan.Source} ${Bolt.Test.Scan.Headers} )

# Include standard OpenCL headers
include_directories( ${PROJECT_SOURCE_DIR}/BoltForC++AMP )

add_executable( Bolt.Test.Scan ${Bolt.Test.Scan.Files} )

set_target_properties( Bolt.Test.Scan PROPERTIES VERSION ${Bolt_VERSION} )
set_target_properties( Bolt.Test.Scan PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/staging" )

set_property( TARGET Bolt.Test.Scan PROPERTY FOLDER "Test/AMP")

# CPack configuration; include the executable into the package
install( TARGETS Bolt.Test.Scan
	RUNTIME DESTINATION ${BIN_DIR}
	LIBRARY DESTINATION ${LIB_DIR}
	ARCHIVE DESTINATION ${LIB_DIR}/import
	)
