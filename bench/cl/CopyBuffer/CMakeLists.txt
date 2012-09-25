#############################################################################
##	Copyright (C) 2010 Advanced Micro Devices, Inc. All Rights Reserved.
#############################################################################
# List the names of common files to compile across all platforms
set( clBolt.Bench.CopyBuffer.Source stdafx.cpp copyBuffer.cpp )
set( clBolt.Bench.CopyBuffer.Headers stdafx.h targetver.h )

set( clBolt.Bench.CopyBuffer.Files ${clBolt.Bench.CopyBuffer.Source} ${clBolt.Bench.CopyBuffer.Headers} )

add_executable( clBolt.Bench.CopyBuffer ${clBolt.Bench.CopyBuffer.Files} )

target_link_libraries( clBolt.Bench.CopyBuffer ${Boost_LIBRARIES} clBolt.Runtime )

set_target_properties( clBolt.Bench.CopyBuffer PROPERTIES VERSION ${Bolt_VERSION} )
set_target_properties( clBolt.Bench.CopyBuffer PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/staging" )

set_property( TARGET clBolt.Bench.CopyBuffer PROPERTY FOLDER "Benchmark/OpenCL")

# CPack configuration; include the executable into the package
install( TARGETS clBolt.Bench.CopyBuffer
	RUNTIME DESTINATION ${BIN_DIR}
	LIBRARY DESTINATION ${LIB_DIR}
	ARCHIVE DESTINATION ${LIB_DIR}
	)
