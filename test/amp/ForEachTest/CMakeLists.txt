############################################################################                                                                                     
#   © 2012,2014 Advanced Micro Devices, Inc. All rights reserved.                                     
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


set( ampBolt.Test.ForEach.Source ForEachTest.cpp ) 
                                  
set( ampBolt.Test.ForEach.Headers ${BOLT_INCLUDE_DIR}/bolt/amp/for_each.h
                              ${BOLT_INCLUDE_DIR}/bolt/amp/detail/for_each.inl)

set( ampBolt.Test.ForEach.Files ${ampBolt.Test.ForEach.Source} ${ampBolt.Test.ForEach.Headers} )

add_executable( ampBolt.Test.ForEach ${ampBolt.Test.ForEach.Files} )


if( MSVC )
    set( CMAKE_CXX_FLAGS "-bigobj ${CMAKE_CXX_FLAGS}" )
    set( CMAKE_C_FLAGS "-bigobj ${CMAKE_C_FLAGS}" )
endif()

target_link_libraries( ampBolt.Test.ForEach ampBolt.Runtime ${GTEST_LIBRARIES} ${Boost_LIBRARIES} ${TBB_LIBRARIES}  )

set_target_properties( ampBolt.Test.ForEach PROPERTIES VERSION ${Bolt_VERSION} )
set_target_properties( ampBolt.Test.ForEach PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/staging" )

set_property( TARGET ampBolt.Test.ForEach PROPERTY FOLDER "Test/AMP")

# CPack configuration; include the executable into the package
install( TARGETS ampBolt.Test.ForEach
	RUNTIME DESTINATION ${BIN_DIR}
	LIBRARY DESTINATION ${LIB_DIR}
	ARCHIVE DESTINATION ${LIB_DIR}/import
	)