#############################################################################
##	Copyright (C) 2010 Advanced Micro Devices, Inc. All Rights Reserved.
#############################################################################
# List the names of common files to compile across all platforms
set( Bolt.Bench.Scan.Source stdafx.cpp Scan.cpp )
set( Bolt.Bench.Scan.Headers stdafx.h targetver.h ${BOLT_INCLUDE_DIR}/bolt/amp/Scan.h )

set( Bolt.Bench.Scan.Files ${Bolt.Bench.Scan.Source} ${Bolt.Bench.Scan.Headers} )

# Include standard OpenCL headers
include_directories( ${Boost_INCLUDE_DIRS} )

add_executable( Bolt.Bench.Scan ${Bolt.Bench.Scan.Files} )

target_link_libraries( Bolt.Bench.Scan ${Boost_LIBRARIES} )

add_dependencies( Bolt.Bench.Scan Boost )

set_target_properties( Bolt.Bench.Scan PROPERTIES VERSION ${Bolt_VERSION} )
set_target_properties( Bolt.Bench.Scan PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/staging" )

set_property( TARGET Bolt.Bench.Scan PROPERTY FOLDER "Benchmark/AMP")

# CPack configuration; include the executable into the package
install( TARGETS Bolt.Bench.Scan
	RUNTIME DESTINATION ${BIN_DIR}
	LIBRARY DESTINATION ${LIB_DIR}
	ARCHIVE DESTINATION ${LIB_DIR}/import
	)
