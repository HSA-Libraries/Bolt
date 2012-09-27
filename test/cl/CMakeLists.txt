#############################################################################
##  Copyright (C) 2010 Advanced Micro Devices, Inc. All Rights Reserved.
#############################################################################
# List the names of common files to compile across all platforms
message( STATUS "Setting up OpenCL specific test projects..." )

set( Boost_USE_MULTITHREADED ON )
set( Boost_USE_STATIC_LIBS   ON )
set( Boost_DETAILED_FAILURE_MSG   ON )

# Set the test folder directory. This is common to cl and amp. 
set( BOLT_CL_TEST_DIR ${BOLT_TEST_DIR}/cl )

# set( Boost_DEBUG ON )

if( BOOST_ROOT )
    # The root tree of BOOST was specified on the command line; use it to to find the specific Boost the user points too
    find_package( Boost ${Boost.VERSION} COMPONENTS thread date_time chrono program_options REQUIRED )
    # This will define Boost_FOUND
else( )
    message( "Configure Bolt in <BOLT_ROOT>/bin to build the SuperBuild which will download and build Boost automatically" )    
    message( SEND_ERROR "The OpenCL Bolt library requires BOOST_ROOT to be defined" )
endif( )

# Include standard Boost headers
include_directories( ${Boost_INCLUDE_DIRS} )

# Include standard OpenCL headers
include_directories( ${OPENCL_INCLUDE_DIRS} ${BOLT_CL_TEST_DIR} ) 

add_subdirectory( CountTest )
add_subdirectory( DeviceVectorTest )
add_subdirectory( ReduceTest )
add_subdirectory( ScanTest )
add_subdirectory( SortTest )
add_subdirectory( TransformTest )   
add_subdirectory( TransformReduceTest )
