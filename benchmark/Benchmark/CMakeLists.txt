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
set( clBolt.Benchmark.Source Benchmark.cpp statisticalTimer.cpp )
set( clBolt.Benchmark.Headers data_type.hpp statisticalTimer.h )

set( clBolt.Benchmark.Files ${clBolt.Benchmark.Source} ${clBolt.Benchmark.Headers} )

# Include headers files
include_directories( ${BOLT_INCLUDE_DIRS} ${OPENCL_INCLUDE_DIRS} )

# Set project specific compile and link options
if( MSVC )
set( CMAKE_CXX_FLAGS "-bigobj ${CMAKE_CXX_FLAGS}" )
                set( CMAKE_C_FLAGS "-bigobj ${CMAKE_C_FLAGS}" )
endif()


add_executable( clBolt.Benchmark ${clBolt.Benchmark.Files} )
if(BUILD_TBB)
    target_link_libraries( clBolt.Benchmark ${BOLT_LIBRARIES} ${OPENCL_LIBRARIES} ${GTEST_LIBRARIES} ${Boost_LIBRARIES} ${TBB_LIBRARIES} )
else (BUILD_TBB)
    target_link_libraries( clBolt.Benchmark ${BOLT_LIBRARIES} ${OPENCL_LIBRARIES} ${GTEST_LIBRARIES} ${Boost_LIBRARIES} )
endif()


