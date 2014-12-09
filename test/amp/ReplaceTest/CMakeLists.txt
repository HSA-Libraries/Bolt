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
set( ampBolt.Test.Replace.Source  ReplaceTest.cpp )
set( ampBolt.Test.Replace.Headers ${BOLT_INCLUDE_DIR}/bolt/amp/replace.h ${BOLT_INCLUDE_DIR}/bolt/amp/detail/replace.inl)

set( ampBolt.Test.Replace.Files ${ampBolt.Test.Replace.Source} ${ampBolt.Test.Replace.Headers} )

add_executable( ampBolt.Test.Replace ${ampBolt.Test.Replace.Files} )


if( MSVC )
    set( CMAKE_CXX_FLAGS "-bigobj ${CMAKE_CXX_FLAGS}" )
    set( CMAKE_C_FLAGS "-bigobj ${CMAKE_C_FLAGS}" )
endif()


if(BUILD_TBB)
    target_link_libraries( ampBolt.Test.Replace ampBolt.Runtime ${GTEST_LIBRARIES} ${Boost_LIBRARIES}  ${TBB_LIBRARIES} )
else (BUILD_TBB)
    target_link_libraries( ampBolt.Test.Replace ampBolt.Runtime ${GTEST_LIBRARIES} ${Boost_LIBRARIES}  )
endif()



set_target_properties( ampBolt.Test.Replace PROPERTIES VERSION ${Bolt_VERSION} )
set_target_properties( ampBolt.Test.Replace PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/staging" )

set_property( TARGET ampBolt.Test.Replace PROPERTY FOLDER "Test/AMP")

# CPack configuration; include the executable into the package
install( TARGETS ampBolt.Test.Replace
	RUNTIME DESTINATION ${BIN_DIR}
	LIBRARY DESTINATION ${LIB_DIR}
	ARCHIVE DESTINATION ${LIB_DIR}/import
	)
