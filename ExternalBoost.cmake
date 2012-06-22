message( STATUS "Setting up Boost SuperBuild... Shiny!" )
include( ExternalProject )

set( Boost_Version 1.49.0 )
string( REPLACE "." "_" Boost_Version_Underscore ${Boost_Version} )

message( STATUS "Boost_Version: " ${Boost_Version} )

# Purely for debugging the file downloading URLs
# file( DOWNLOAD "http://downloads.sourceforge.net/project/boost/boost/1.49.0/boost_1_49_0.7z" 
		# "${CMAKE_CURRENT_BINARY_DIR}/download/boost-${Boost_Version}/boost_1_49_0.7z" SHOW_PROGRESS STATUS fileStatus LOG fileLog )
# message( STATUS "status: " ${fileStatus} )
# message( STATUS "log: " ${fileLog} )

# Below is a fancy CMake command to download, build and install Boost on the users computer
ExternalProject_Add(
    Boost
	PREFIX ${CMAKE_CURRENT_BINARY_DIR}/external/boost
#    URL http://downloads.sourceforge.net/project/boost/boost/${Boost_Version}/boost_${Boost_Version_Underscore}.zip
    URL http://see-srv/share/code/externals/boost/boost_${Boost_Version_Underscore}.zip
	URL_MD5 854dcbbff31b896c85c38247060b7713
    UPDATE_COMMAND "bootstrap.bat"
#    PATCH_COMMAND ""
	CONFIGURE_COMMAND ""
	BUILD_COMMAND b2 --with-program_options address-model=64 toolset=msvc-11.0 link=static stage
	BUILD_IN_SOURCE 1
    INSTALL_COMMAND ""
)

set_property( TARGET Boost PROPERTY FOLDER "Externals")

ExternalProject_Get_Property( Boost source_dir )
ExternalProject_Get_Property( Boost binary_dir )
set( Boost_INCLUDE_DIRS ${source_dir} )
set( Boost_LIBRARIES debug;${binary_dir}/stage/lib/libboost_program_options-vc110-mt-gd-1_49.lib;optimized;${binary_dir}/stage/lib/libboost_program_options-vc110-mt-1_49.lib )

# Can't get packages to download from github because the cmake file( download ... ) does not understand https protocal
# Gitorious is problematic because it apparently only offers .tar.gz files to download, which windows doesn't support by default
# Also, both repo's do not like to append a filetype to the URL, instead they use some forwarding script.  ExternalProject_Add wants a filetype.
# set( BOOST_BUILD_PROJECTS program_options CACHE STRING "* seperated Boost modules to be built")
# ExternalProject_Add(
   # Boost
   # URL http://gitorious.org/boost/cmake/archive-tarball/cmake-${Boost_Version}.tar.gz
   # LIST_SEPARATOR *
   # CMAKE_ARGS 	-DENABLE_STATIC=ON 
				# -DENABLE_SHARED=OFF 
				# -DENABLE_DEBUG=OFF 
				# -DENABLE_RELEASE=ON 
				# -DENABLE_SINGLE_THREADED=OFF 
				# -DENABLE_MULTI_THREADED=ON 
				# -DENABLE_STATIC_RUNTIME:BOOL=OFF 
				# -DENABLE_DYNAMIC_RUNTIME=ON 
				# -DWITH_PYTHON:BOOL=OFF 
				# -DBUILD_PROJECTS=${BOOST_BUILD_PROJECTS} ${CMAKE_ARGS}
# )
