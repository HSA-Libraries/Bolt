#############################################################################
##	Copyright (C) 2010 Advanced Micro Devices, Inc. All Rights Reserved.
#############################################################################
# List the names of common files to compile across all platforms
set( clBolt.StringifyKernels.Source StringifyKernels.cpp )
set( clBolt.StringifyKernels.Headers "" )

set( clBolt.StringifyKernels.Files ${clBolt.StringifyKernels.Source} ${clBolt.StringifyKernels.Headers} )

add_executable( clBolt.StringifyKernels ${clBolt.StringifyKernels.Files} )
target_link_libraries( clBolt.StringifyKernels ${Boost_LIBRARIES} )

set_target_properties( clBolt.StringifyKernels PROPERTIES VERSION ${Bolt_VERSION} )
set_target_properties( clBolt.StringifyKernels PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/staging" )

set_property( TARGET clBolt.StringifyKernels PROPERTY FOLDER "Tools")
		
# CPack configuration; include the executable into the package
install( TARGETS clBolt.StringifyKernels
	RUNTIME DESTINATION ${BIN_DIR}
	LIBRARY DESTINATION ${LIB_DIR}
	ARCHIVE DESTINATION ${LIB_DIR}/import
	)
