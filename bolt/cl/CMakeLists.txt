#############################################################################
##	Copyright (C) 2010 Advanced Micro Devices, Inc. All Rights Reserved.
#############################################################################
# List the names of common files to compile across all platforms

set( Boost_USE_MULTITHREADED ON )
set( Boost_USE_STATIC_LIBS   ON )
set( Boost_DETAILED_FAILURE_MSG   ON )
set( Boost_DEBUG ON )

if( BOOST_ROOT )
	# The root tree of BOOST was specified on the command line; use it to to find the specific Boost the user points too
	find_package( Boost ${Boost.VERSION} REQUIRED )
	# This will define Boost_FOUND
else( )
	message( "Configure Bolt in <BOLT_ROOT>/bin to build the SuperBuild which will download and build Boost automatically" )	
	message( SEND_ERROR "The OpenCL Bolt library requires BOOST_ROOT to be defined" )
endif( )

# Include standard OpenCL headers
include_directories( ${Boost_INCLUDE_DIRS} )

set( clBolt.Include.Dir ${BOLT_INCLUDE_DIR}/bolt/cl )

set( clBolt.Runtime.Source 	
		bolt.cpp 
		${BOLT_LIBRARY_DIR}/statisticalTimer.cpp )

set( clBolt.Runtime.Headers 
		${clBolt.Include.Dir}/bolt.h 
		${clBolt.Include.Dir}/clcode.h 
		${clBolt.Include.Dir}/control.h 
		${clBolt.Include.Dir}/count.h 
		${clBolt.Include.Dir}/device_vector.h 
		${clBolt.Include.Dir}/functional.h 
		${clBolt.Include.Dir}/iterator_traits.h 
		${clBolt.Include.Dir}/reduce.h 
		${clBolt.Include.Dir}/scan.h 
		${clBolt.Include.Dir}/sort.h 
		${clBolt.Include.Dir}/transform.h 
		${clBolt.Include.Dir}/transform_reduce.h
		${clBolt.Include.Dir}/../countof.h
		${clBolt.Include.Dir}/../unicode.h
		${clBolt.Include.Dir}/../statisticalTimer.h )
		
set( clBolt.Runtime.Headers.Detail 
		${clBolt.Include.Dir}/detail/reduce.inl
		${clBolt.Include.Dir}/detail/scan.inl
        ${clBolt.Include.Dir}/detail/sort.inl )
		
set( clBolt.Runtime.clFiles 
		${clBolt.Include.Dir}/reduce_kernels.cl 
		${clBolt.Include.Dir}/transform_kernels.cl 
		${clBolt.Include.Dir}/transform_reduce_kernels.cl
        ${clBolt.Include.Dir}/scan_kernels.cl
        ${clBolt.Include.Dir}/sort_kernels.cl )

# The minidump tech is windows specific; I don't have a solution yet for linux, but google-breakpad looks promising
if( WIN32 )
	list( APPEND clBolt.Runtime.Source ${BOLT_LIBRARY_DIR}/miniDump.cpp )
	list( APPEND clBolt.Runtime.Headers ${clBolt.Include.Dir}/../miniDump.h )
endif( )
		
set( clBolt.Runtime.Files ${clBolt.Runtime.Source} ${clBolt.Runtime.Headers} ${clBolt.Runtime.Headers.Detail} ${clBolt.Runtime.clFiles} )

# Include standard OpenCL headers
include_directories( ${BOLT_INCLUDE_DIR} ${OPENCL_INCLUDE_DIRS} )

add_library( clBolt.Runtime STATIC ${clBolt.Runtime.Files} )
target_link_libraries( clBolt.Runtime ${OPENCL_LIBRARIES} )

set_target_properties( clBolt.Runtime PROPERTIES VERSION ${Bolt_VERSION} )
set_target_properties( clBolt.Runtime PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/staging" )
		
install( FILES	
			${clBolt.Runtime.clFiles}
		DESTINATION
			${BIN_DIR} )

install( FILES	
			${clBolt.Runtime.clFiles}
		DESTINATION
			${PROJECT_BINARY_DIR}/staging/\${CMAKE_INSTALL_CONFIG_NAME} )

# CPack configuration; include the executable into the package
install( TARGETS clBolt.Runtime
	RUNTIME DESTINATION ${BIN_DIR}
	LIBRARY DESTINATION ${LIB_DIR}
	ARCHIVE DESTINATION ${LIB_DIR}/import
	)
