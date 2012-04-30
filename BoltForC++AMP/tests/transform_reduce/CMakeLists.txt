#############################################################################
##	Copyright (C) 2010 Advanced Micro Devices, Inc. All Rights Reserved.
#############################################################################
# List the names of common files to compile across all platforms
set( Bolt.TransformReduce.Source stdafx.cpp simple_transform_reduce.cpp )
set( Bolt.TransformReduce.Headers stdafx.h targetver.h ${BOLT_DIR}/transform_reduce.h)

set( Bolt.TransformReduce.Files ${Bolt.TransformReduce.Source} ${Bolt.TransformReduce.Headers} )

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
include_directories( ${PROJECT_SOURCE_DIR}/BoltForC++AMP )

add_executable( Bolt.TransformReduce ${Bolt.TransformReduce.Files} )

set_target_properties( Bolt.TransformReduce PROPERTIES VERSION ${Bolt_VERSION} )
set_target_properties( Bolt.TransformReduce PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/staging" )

# CPack configuration; include the executable into the package
install( TARGETS Bolt.TransformReduce
	RUNTIME DESTINATION ${BIN_DIR}
	LIBRARY DESTINATION ${LIB_DIR}
	ARCHIVE DESTINATION ${LIB_DIR}/import
	)
