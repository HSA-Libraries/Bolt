#############################################################################
##	Copyright (C) 2010 Advanced Micro Devices, Inc. All Rights Reserved.
#############################################################################
# List the names of common files to compile across all platforms

set( clBolt.TransformReduce.Source stdafx.cpp TransformReduceTest.cpp ${BOLTCL_DIR}/bolt.cpp )
set( clBolt.TransformReduce.Headers stdafx.h targetver.h ${BOLTCL_DIR}/transform_reduce.h ${BOLTCL_DIR}/transform_reduce_kernels.cl)

set( clBolt.TransformReduce.Files ${clBolt.TransformReduce.Source} ${clBolt.TransformReduce.Headers} )

# For a rainy day, add pre-compiled header support
#if( MSVC )
#	if (USE_MSVC_PCH)
	
#		set_source_files_properties(LungAnalysisPCH.cxx
#			PROPERTIES
#			COMPILE_FLAGS "/YcLungAnalysisPCH.h"
#			)
#		foreach( src_file ${UPMC_LA_SRCS} )
#			set_source_files_properties(
#				${src_file}
#				PROPERTIES
#				COMPILE_FLAGS "/YuLungAnalysisPCH.h"
#				)
#		endforeach( src_file ${UPMC_LA_SRCS} )
		
#		list(APPEND UPMC_LA_SRCS LungAnalysisPCH.cxx)
#		list(APPEND UPMC_LA_HDRS LungAnalysisPCH.h)

#	endif(USE_MSVC_PCH)
#endif (MSVC)
  
#	add_definitions( ${Boost_LIB_DIAGNOSTIC_DEFINITIONS} )
# add_definitions( "/DReduce_EXPORTS" )

# Include standard OpenCL headers
include_directories( ${PROJECT_SOURCE_DIR}/BoltForOpenCL ${OPENCL_INCLUDE_DIRS} )

add_executable( clBolt.TransformReduce ${clBolt.TransformReduce.Files} )
target_link_libraries( clBolt.TransformReduce ${OPENCL_LIBRARIES} )

set_target_properties( clBolt.TransformReduce PROPERTIES VERSION ${Bolt_VERSION} )
set_target_properties( clBolt.TransformReduce PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/staging" )

# CPack configuration; include the executable into the package
install( TARGETS clBolt.TransformReduce
	RUNTIME DESTINATION ${BIN_DIR}
	LIBRARY DESTINATION ${LIB_DIR}
	ARCHIVE DESTINATION ${LIB_DIR}/import
	)
