############################################################################                                                                                     
#   Copyright 2012 Advanced Micro Devices, Inc.                                     
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
set( ampBolt.Test.Scan.Source stdafx.cpp Scan.cpp )
set( ampBolt.Test.Scan.Headers stdafx.h targetver.h ${BOLT_INCLUDE_DIR}/bolt/amp/Scan.h )

set( ampBolt.Test.Scan.Files ${ampBolt.Test.Scan.Source} ${ampBolt.Test.Scan.Headers} )

add_executable( ampBolt.Test.Scan ${ampBolt.Test.Scan.Files} )
target_link_libraries( ampBolt.Test.Scan ${GTEST_LIBRARIES} ampBolt.Runtime )

set_target_properties( ampBolt.Test.Scan PROPERTIES VERSION ${Bolt_VERSION} )
set_target_properties( ampBolt.Test.Scan PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/staging" )

set_property( TARGET ampBolt.Test.Scan PROPERTY FOLDER "Test/AMP")

# CPack configuration; include the executable into the package
install( TARGETS ampBolt.Test.Scan
	RUNTIME DESTINATION ${BIN_DIR}
	LIBRARY DESTINATION ${LIB_DIR}
	ARCHIVE DESTINATION ${LIB_DIR}/import
	)
