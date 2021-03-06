############################################################################                                                                                     
#   � 2012,2014 Advanced Micro Devices, Inc. All rights reserved.                                     
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

set( clBolt.Test.BinarySearch.Source BinarySearchTest.cpp 
                             ${BOLT_CL_TEST_DIR}/common/myocl.cpp)
set( clBolt.Test.BinarySearch.Headers   ${BOLT_CL_TEST_DIR}/common/myocl.h
                                ${BOLT_CL_TEST_DIR}/common/test_common.h
                                ${BOLT_INCLUDE_DIR}/bolt/cl/binary_search.h
                                ${BOLT_INCLUDE_DIR}/bolt/cl/detail/binary_search.inl )

set( clBolt.Test.BinarySearch.Files ${clBolt.Test.BinarySearch.Source} ${clBolt.Test.BinarySearch.Headers} )

# Include standard OpenCL headers
include_directories( ${OPENCL_INCLUDE_DIRS} )

# Set project specific compile and link options
if( MSVC )
set( CMAKE_CXX_FLAGS "-bigobj ${CMAKE_CXX_FLAGS}" )
                set( CMAKE_C_FLAGS "-bigobj ${CMAKE_C_FLAGS}" )
endif()

add_executable( clBolt.Test.BinarySearch ${clBolt.Test.BinarySearch.Files} )

if(BUILD_TBB)
    target_link_libraries( clBolt.Test.BinarySearch ${OPENCL_LIBRARIES} ${GTEST_LIBRARIES} ${Boost_LIBRARIES} clBolt.Runtime  ${TBB_LIBRARIES} )
else (BUILD_TBB)
    target_link_libraries( clBolt.Test.BinarySearch ${OPENCL_LIBRARIES} ${GTEST_LIBRARIES} ${Boost_LIBRARIES} clBolt.Runtime  )
endif()

set_target_properties( clBolt.Test.BinarySearch PROPERTIES VERSION ${Bolt_VERSION} )
set_target_properties( clBolt.Test.BinarySearch PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/staging" )

set_property( TARGET clBolt.Test.BinarySearch PROPERTY FOLDER "Test/OpenCL")
        
# CPack configuration; include the executable into the package
install( TARGETS clBolt.Test.BinarySearch
    RUNTIME DESTINATION ${BIN_DIR}
    LIBRARY DESTINATION ${LIB_DIR}
    ARCHIVE DESTINATION ${LIB_DIR}/import
    )
