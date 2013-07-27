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
set( clBolt.Example.RadixSort.Source RadixSort.cpp )
set( clBolt.Example.RadixSort.Headers   ${BOLT_INCLUDE_DIRS}/bolt/cl/count.h ${BOLT_INCLUDE_DIRS}/bolt/cl/detail/count.inl
                                        ${BOLT_INCLUDE_DIRS}/bolt/cl/device_vector.h
                                        ${BOLT_INCLUDE_DIRS}/bolt/cl/scan.h ${BOLT_INCLUDE_DIRS}/bolt/cl/detail/scan.inl
                                        ${BOLT_INCLUDE_DIRS}/bolt/cl/scatter.h ${BOLT_INCLUDE_DIRS}/bolt/cl/detail/scatter.inl
                                        ${BOLT_INCLUDE_DIRS}/bolt/cl/transform.h ${BOLT_INCLUDE_DIRS}/bolt/cl/detail/transform.inl )

set( clBolt.Example.RadixSort.Files ${clBolt.Example.RadixSort.Source} ${clBolt.Example.RadixSort.Headers} )

# Include headers files
include_directories( ${BOLT_INCLUDE_DIRS} ${OPENCL_INCLUDE_DIRS} )

add_executable( clBolt.Example.RadixSort ${clBolt.Example.RadixSort.Files} )
if(BUILD_TBB)
    target_link_libraries( clBolt.Example.RadixSort ${BOLT_LIBRARIES} ${OPENCL_LIBRARIES} ${GTEST_LIBRARIES} ${Boost_LIBRARIES}  ${TBB_LIBRARIES} )
else (BUILD_TBB)
    target_link_libraries( clBolt.Example.RadixSort ${BOLT_LIBRARIES} ${OPENCL_LIBRARIES} ${GTEST_LIBRARIES} ${Boost_LIBRARIES}  )
endif()

set_target_properties( clBolt.Example.RadixSort PROPERTIES VERSION ${Examples_VERSION} )
set_target_properties( clBolt.Example.RadixSort PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/staging" )

# CPack configuration; include the executable into the package
install( TARGETS clBolt.Example.RadixSort
	RUNTIME DESTINATION ${BIN_DIR}
	LIBRARY DESTINATION ${LIB_DIR}
	ARCHIVE DESTINATION ${LIB_DIR}
	)
