#############################################################################
##	Copyright (C) 2010 Advanced Micro Devices, Inc. All Rights Reserved.
#############################################################################
# List the names of common files to compile across all platforms
set( Bolt.Transform.Source stdafx.cpp simple_transform.cpp )
set( Bolt.Transform.Headers stdafx.h targetver.h )

set( Bolt.Transform.Files ${Bolt.Transform.Source} ${Bolt.Transform.Headers} )

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

add_executable( Bolt.Transform ${Bolt.Transform.Files} )

set_target_properties( Bolt.Transform PROPERTIES VERSION ${Bolt_VERSION} )
set_target_properties( Bolt.Transform PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/staging" )

# CPack configuration; include the executable into the package
install( TARGETS Bolt.Transform
	RUNTIME DESTINATION ${BIN_DIR}
	LIBRARY DESTINATION ${LIB_DIR}
	ARCHIVE DESTINATION ${LIB_DIR}/import
	)
