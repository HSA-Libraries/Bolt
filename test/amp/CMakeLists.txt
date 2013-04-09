############################################################################                                                                                     
#   Copyright 2012 - 2013 Advanced Micro Devices, Inc.                                     
#                                                                                    
#   Licensed under the Apache License, Version 2.0 (the "License");   
#   you may not use this file except in compliance with the License.                 
#   You may obtain a copy of the License at                                          
#                                                                                    
#       http://www.apache.org/licenses/LICENSE-2.0                      
#                                                                                    
#   Unless required by applicable law or agreed to in writing, software              
#   distributed under the License is distributed on an "AS IS" BASIS,              
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.         
#   See the License for the specific language governing permissions and              
#   limitations under the License.                                                   

############################################################################                                                                                     

# List the names of common files to compile across all platforms
message( STATUS "Setting up AMP specific test projects..." )

set( Boost_USE_MULTITHREADED ON )
set( Boost_USE_STATIC_LIBS   ON )
set( Boost_DETAILED_FAILURE_MSG   ON )

# Set the test folder directory. This is common to cl and amp. 
set( BOLT_AMP_TEST_DIR ${BOLT_TEST_DIR}/amp )
set( BOLT_CL_TEST_DIR ${BOLT_TEST_DIR}/cl )

# set( Boost_DEBUG ON )

if( BOOST_ROOT )
    # The root tree of BOOST was specified on the command line; use it to to find the specific Boost the user points too
    find_package( Boost ${Boost.VERSION} COMPONENTS thread date_time chrono program_options REQUIRED )
    # This will define Boost_FOUND
else( )
    message( "Configure Bolt in <BOLT_ROOT>/bin to build the SuperBuild which will download and build Boost automatically" )    
    message( SEND_ERROR "The AMP Bolt library requires BOOST_ROOT to be defined" )
endif( )

# Include standard Boost headers
include_directories( ${Boost_INCLUDE_DIRS} )

# Include standard OpenCL headers
include_directories(
    ${BOLT_AMP_TEST_DIR}
    ${BOLT_AMP_TEST_DIR}/common
    ${OPENCL_INCLUDE_DIRS}) 
# include_directories( ${BOLT_CL_TEST_DIR} )

add_subdirectory( CountTest )
add_subdirectory( ReduceTest )
add_subdirectory( ScanTest )
add_subdirectory( SortTest )
add_subdirectory( TransformTest )	
add_subdirectory( TransformReduceTest )
add_subdirectory( device_vector )
