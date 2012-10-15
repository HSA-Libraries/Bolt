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

set( Bolt.Include.Dir ${BOLT_INCLUDE_DIR}/bolt/amp )

set( Bolt.Runtime.Source ${BOLT_LIBRARY_DIR}/statisticalTimer.cpp )

set( Bolt.Runtime.Headers 
		${Bolt.Include.Dir}/functional.h 
		${Bolt.Include.Dir}/parallel_iteration.h 
		${Bolt.Include.Dir}/pool_alloc.h 
		${Bolt.Include.Dir}/reduce.h 
		${Bolt.Include.Dir}/scan.h 
		${Bolt.Include.Dir}/synchronized_view.h 
		${Bolt.Include.Dir}/transform.h 
		${Bolt.Include.Dir}/transform_reduce.h 
		${Bolt.Include.Dir}/transform_reduce_range.h
		${Bolt.Include.Dir}/../countof.h
		${Bolt.Include.Dir}/../statisticalTimer.h
		${Bolt.Include.Dir}/../unicode.h )

# The minidump tech is windows specific; I don't have a solution yet for linux, but google-breakpad looks promising
if( WIN32 )
	list( APPEND Bolt.Runtime.Source ${BOLT_LIBRARY_DIR}/miniDump.cpp )
	list( APPEND Bolt.Runtime.Headers ${Bolt.Include.Dir}/../miniDump.h )
endif( )

set( Bolt.Runtime.Files ${Bolt.Runtime.Source} ${Bolt.Runtime.Headers} )

# Include standard OpenCL headers
include_directories( ${PROJECT_SOURCE_DIR}/include )

add_library( ampBolt.Runtime STATIC ${Bolt.Runtime.Files} )

set_target_properties( ampBolt.Runtime PROPERTIES VERSION ${Bolt_VERSION} )
set_target_properties( ampBolt.Runtime PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/staging" )

if( MSVC_VERSION VERSION_LESS 1600 )
    set_target_properties( ampBolt.Runtime PROPERTIES ARCHIVE_OUTPUT_NAME "ampBolt.Runtime.vc90" )
elseif( MSVC_VERSION VERSION_LESS 1700 )
    set_target_properties( ampBolt.Runtime PROPERTIES ARCHIVE_OUTPUT_NAME "ampBolt.Runtime.vc100" )
elseif( MSVC_VERSION VERSION_LESS 1800 )
    set_target_properties( ampBolt.Runtime PROPERTIES ARCHIVE_OUTPUT_NAME "ampBolt.Runtime.vc110" )
endif( )

# CPack configuration; include the executable into the package
install( TARGETS ampBolt.Runtime
	RUNTIME DESTINATION ${BIN_DIR}
	LIBRARY DESTINATION ${LIB_DIR}
	ARCHIVE DESTINATION ${LIB_DIR}
	)
