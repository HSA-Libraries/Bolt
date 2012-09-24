#############################################################################
##	Copyright (C) 2010 Advanced Micro Devices, Inc. All Rights Reserved.
#############################################################################
# List the names of common files to compile across all platforms
set( clBolt.Bench.Scan.Source stdafx.cpp Scan.cpp )
set( clBolt.Bench.Scan.Headers stdafx.h targetver.h ${BOLT_INCLUDE_DIR}/bolt/cl/Scan.h )

set( clBolt.Bench.Scan.Files ${clBolt.Bench.Scan.Source} ${clBolt.Bench.Scan.Headers} )

add_executable( clBolt.Bench.Scan ${clBolt.Bench.Scan.Files} )

target_link_libraries( clBolt.Bench.Scan ${Boost_LIBRARIES} clBolt.Runtime )

set_target_properties( clBolt.Bench.Scan PROPERTIES VERSION ${Bolt_VERSION} )
set_target_properties( clBolt.Bench.Scan PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/staging" )

set_property( TARGET clBolt.Bench.Scan PROPERTY FOLDER "Benchmark/OpenCL")

# CPack configuration; include the executable into the package
install( TARGETS clBolt.Bench.Scan
	RUNTIME DESTINATION ${BIN_DIR}
	LIBRARY DESTINATION ${LIB_DIR}
	ARCHIVE DESTINATION ${LIB_DIR}
	)
