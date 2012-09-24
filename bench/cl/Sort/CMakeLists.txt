#############################################################################
##	Copyright (C) 2010 Advanced Micro Devices, Inc. All Rights Reserved.
#############################################################################
# List the names of common files to compile across all platforms
set( clBolt.Bench.Sort.Source Sort.cpp )
set( clBolt.Bench.Sort.Headers stdafx.h ${BOLT_INCLUDE_DIR}/bolt/cl/Sort.h 
                                        ${BOLT_INCLUDE_DIR}/bolt/cl/detail/Sort.inl)

set( clBolt.Bench.Sort.Files ${clBolt.Bench.Sort.Source} ${clBolt.Bench.Sort.Headers} )

add_executable( clBolt.Bench.Sort ${clBolt.Bench.Sort.Files} )

target_link_libraries( clBolt.Bench.Sort ${Boost_LIBRARIES} clBolt.Runtime )

set_target_properties( clBolt.Bench.Sort PROPERTIES VERSION ${Bolt_VERSION} )
set_target_properties( clBolt.Bench.Sort PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/staging" )

set_property( TARGET clBolt.Bench.Sort PROPERTY FOLDER "Benchmark/OpenCL")

# CPack configuration; include the executable into the package
install( TARGETS clBolt.Bench.Sort
	RUNTIME DESTINATION ${BIN_DIR}
	LIBRARY DESTINATION ${LIB_DIR}
	ARCHIVE DESTINATION ${LIB_DIR}
	)
