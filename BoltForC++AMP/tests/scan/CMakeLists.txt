#############################################################################
##	Copyright (C) 2010 Advanced Micro Devices, Inc. All Rights Reserved.
#############################################################################
# List the names of common files to compile across all platforms
set( Bolt.Scan.Source stdafx.cpp Scan.cpp )
set( Bolt.Scan.Headers stdafx.h targetver.h )

set( Bolt.Scan.Files ${Bolt.Scan.Source} ${Bolt.Scan.Headers} )

# Include standard OpenCL headers
include_directories( ${PROJECT_SOURCE_DIR}/BoltForC++AMP )

add_executable( Bolt.Scan ${Bolt.Scan.Files} )

set_target_properties( Bolt.Scan PROPERTIES VERSION ${Bolt_VERSION} )
set_target_properties( Bolt.Scan PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/staging" )

# CPack configuration; include the executable into the package
install( TARGETS Bolt.Scan
	RUNTIME DESTINATION ${BIN_DIR}
	LIBRARY DESTINATION ${LIB_DIR}
	ARCHIVE DESTINATION ${LIB_DIR}/import
	)
