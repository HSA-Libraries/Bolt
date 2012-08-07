#############################################################################
##	Copyright (C) 2010 Advanced Micro Devices, Inc. All Rights Reserved.
#############################################################################
# List the names of common files to compile across all platforms
set( clBolt.Test.Device.Vector.Source stdafx.cpp device.vector.test.cpp )
set( clBolt.Test.Device.Vector.Headers stdafx.h targetver.h ${BOLT_INCLUDE_DIR}/bolt/cl/device_vector.h )

set( clBolt.Test.Device.Vector.Files ${clBolt.Test.Device.Vector.Source} ${clBolt.Test.Device.Vector.Headers} )

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

add_executable( clBolt.Test.Device.Vector ${clBolt.Test.Device.Vector.Files} )
target_link_libraries( clBolt.Test.Device.Vector clBolt.Runtime ${GTEST_LIBRARIES} )

set_target_properties( clBolt.Test.Device.Vector PROPERTIES VERSION ${Bolt_VERSION} )
set_target_properties( clBolt.Test.Device.Vector PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/staging" )

set_property( TARGET clBolt.Test.Device.Vector PROPERTY FOLDER "Test/OpenCL")

# CPack configuration; include the executable into the package
install( TARGETS clBolt.Test.Device.Vector
	RUNTIME DESTINATION ${BIN_DIR}
	LIBRARY DESTINATION ${LIB_DIR}
	ARCHIVE DESTINATION ${LIB_DIR}/import
	)
