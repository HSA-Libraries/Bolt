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

message( STATUS "Setting up Doxygen SuperBuild... Shiny!" )
include( ExternalProject )

set( Doxygen_Version 1.8.1.1 )

message( STATUS "Doxygen_Version: " ${Doxygen_Version} )

# Purely for debugging the file downloading URLs
# file( DOWNLOAD "http://ftp.stack.nl/pub/users/dimitri/doxygen-${Doxygen_Version}.windows.bin.zip" 
		# "${CMAKE_CURRENT_BINARY_DIR}/download/Doxygen-${Doxygen_Version}/Doxygen_1_49_0.7z" SHOW_PROGRESS STATUS fileStatus LOG fileLog )
# message( STATUS "status: " ${fileStatus} )
# message( STATUS "log: " ${fileLog} )

# Below is a fancy CMake command to download, build and install Doxygen on the users computer
ExternalProject_Add(
    Doxygen
	PREFIX ${CMAKE_CURRENT_BINARY_DIR}/external/Doxygen
#    URL http://ftp.stack.nl/pub/users/dimitri/doxygen-${Doxygen_Version}.windows.bin.zip
    URL http://see-srv/share/code/externals/doxygen/doxygen-${Doxygen_Version}.windows.bin.zip
	URL_MD5 20c22209c85d3e8cc1d7d59f7c0bf351
    UPDATE_COMMAND ""
#    PATCH_COMMAND ""
	CONFIGURE_COMMAND ""
	BUILD_COMMAND ""
	BUILD_IN_SOURCE 1
    INSTALL_COMMAND ""
)

set_property( TARGET Doxygen PROPERTY FOLDER "Externals")

ExternalProject_Get_Property( Doxygen source_dir )
ExternalProject_Get_Property( Doxygen binary_dir )

set( DOXYGEN_EXECUTABLE ${binary_dir}/doxygen.exe )
# set( Doxygen_INCLUDE_DIRS ${source_dir} )
# set( Doxygen_LIBRARIES debug;${binary_dir}/stage/lib/libDoxygen_program_options-vc110-mt-gd-1_49.lib;optimized;${binary_dir}/stage/lib/libDoxygen_program_options-vc110-mt-1_49.lib )
