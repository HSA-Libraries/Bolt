#############################################################################
##	Copyright (C) 2010 Advanced Micro Devices, Inc. All Rights Reserved.
#############################################################################
# List the names of common files to compile across all platforms

set( Bolt.Include.Dir ${BOLT_INCLUDE_DIR}/bolt/amp )

set( Bolt.Runtime.Source ${BOLT_LIBRARY_DIR}/statisticalTimer.cpp )

set( Bolt.Runtime.Headers 
		${Bolt.Include.Dir}/functional.h 
		${Bolt.Include.Dir}/parallel_iteration.h 
		${Bolt.Include.Dir}/pool_alloc.h 
		${Bolt.Include.Dir}/reduce.h 
		${Bolt.Include.Dir}/scan.h 
		${Bolt.Include.Dir}/synchronized_view.h 
		${Bolt.Include.Dir}/transform.h 
		${Bolt.Include.Dir}/transform_reduce.h 
		${Bolt.Include.Dir}/transform_reduce_range.h
		${Bolt.Include.Dir}/../countof.h
		${Bolt.Include.Dir}/../statisticalTimer.h
		${Bolt.Include.Dir}/../unicode.h )

# The minidump tech is windows specific; I don't have a solution yet for linux, but google-breakpad looks promising
if( WIN32 )
	list( APPEND Bolt.Runtime.Source ${BOLT_LIBRARY_DIR}/miniDump.cpp )
	list( APPEND Bolt.Runtime.Headers ${Bolt.Include.Dir}/../miniDump.h )
endif( )

set( Bolt.Runtime.Files ${Bolt.Runtime.Source} ${Bolt.Runtime.Headers} )

# Include standard OpenCL headers
include_directories( ${PROJECT_SOURCE_DIR}/include )

add_library( ampBolt.Runtime STATIC ${Bolt.Runtime.Files} )

set_target_properties( ampBolt.Runtime PROPERTIES VERSION ${Bolt_VERSION} )
set_target_properties( ampBolt.Runtime PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/staging" )
		
# CPack configuration; include the executable into the package
install( TARGETS ampBolt.Runtime
	RUNTIME DESTINATION ${BIN_DIR}
	LIBRARY DESTINATION ${LIB_DIR}
	ARCHIVE DESTINATION ${LIB_DIR}/import
	)

	
	