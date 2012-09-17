#############################################################################
##    Copyright (C) 2010 Advanced Micro Devices, Inc. All Rights Reserved.
#############################################################################
# List the names of common files to compile across all platforms

set( Boost_USE_MULTITHREADED ON )
set( Boost_USE_STATIC_LIBS   ON )
set( Boost_DETAILED_FAILURE_MSG   ON )
# set( Boost_DEBUG ON )

if( BOOST_ROOT )
    # The root tree of BOOST was specified on the command line; use it to to find the specific Boost the user points too
    find_package( Boost ${Boost.VERSION} COMPONENTS thread system REQUIRED )
    # This will define Boost_FOUND
else( )
    message( "Configure Bolt in <BOLT_ROOT>/bin to build the SuperBuild which will download and build Boost automatically" )    
    message( SEND_ERROR "The OpenCL Bolt library requires BOOST_ROOT to be defined" )
endif( )

# A function to help debug cmake files; this will print each element of a list line by line
function( printList myList )
    foreach( element ${${myList}} )
        message( "${myList} element: [" ${element} "]" )
    endforeach( )
endfunction( )

# A macro to create a new list ${resultList} from a given list of filenames and a path to prepend on each element
macro( prependPath resultList filesList path )
    foreach( kernelFile ${${filesList}} )
        list( APPEND ${resultList} "${path}/${kernelFile}" )
        # message( "prependPath element: [" ${${resultList}} "]" )
    endforeach( )
endmacro( )

# Macro that iterates through a source list, and appends into a result list the results of a substitution
macro( substList match_string replace_string resultList sourceList )
    foreach( sourceString ${${sourceList}} )
        string( REPLACE ${match_string} ${replace_string} sourceString ${sourceString} )
        list( APPEND ${resultList} ${sourceString} )
    endforeach( )
endmacro( )

set( clBolt.Include.Dir ${BOLT_INCLUDE_DIR}/bolt/cl )

set( clBolt.Runtime.Source     
        bolt.cpp 
        ${BOLT_LIBRARY_DIR}/statisticalTimer.cpp
    )

set( clBolt.Runtime.Headers 
        ${clBolt.Include.Dir}/bolt.h 
        ${clBolt.Include.Dir}/clcode.h 
        ${clBolt.Include.Dir}/control.h 
        ${clBolt.Include.Dir}/count.h 
        ${clBolt.Include.Dir}/device_vector.h 
        ${clBolt.Include.Dir}/functional.h 
        ${clBolt.Include.Dir}/iterator_traits.h 
        ${clBolt.Include.Dir}/reduce.h 
        ${clBolt.Include.Dir}/scan.h 
        ${clBolt.Include.Dir}/sort.h 
        ${clBolt.Include.Dir}/transform.h 
        ${clBolt.Include.Dir}/transform_reduce.h
    )

set( clBolt.Runtime.Headers.Misc
        ${clBolt.Include.Dir}/../countof.h
        ${clBolt.Include.Dir}/../unicode.h
        ${clBolt.Include.Dir}/../statisticalTimer.h
    )
        
set( clBolt.Runtime.Headers.Detail 
        ${clBolt.Include.Dir}/detail/reduce.inl
        ${clBolt.Include.Dir}/detail/scan.inl
        ${clBolt.Include.Dir}/detail/sort.inl
        ${clBolt.Include.Dir}/detail/transform.inl
        ${clBolt.Include.Dir}/detail/transform_reduce.inl
    )

set( clBolt.Runtime.clFiles 
        reduce_kernels.cl 
        transform_kernels.cl 
        transform_reduce_kernels.cl
        scan_kernels.cl
        sort_kernels.cl
    )

# Create a list of .cl files that we would like to be a part of the IDE
prependPath( clBolt.Runtime.clFiles.FullPath clBolt.Runtime.clFiles ${clBolt.Include.Dir} )
# printList( clBolt.Runtime.clFiles.FullPath )

# The minidump tech is windows specific; I don't have a solution yet for linux, but google-breakpad looks promising
if( WIN32 )
    list( APPEND clBolt.Runtime.Source ${BOLT_LIBRARY_DIR}/miniDump.cpp )
    list( APPEND clBolt.Runtime.Headers.Misc ${clBolt.Include.Dir}/../miniDump.h )
endif( )

# These create convenient file folders for our inline and kernel files
source_group( "Inline Files" REGULAR_EXPRESSION .*inl$ )
source_group( "Kernel Files" REGULAR_EXPRESSION .*cl$ )

set( clBolt.Runtime.Files ${clBolt.Runtime.Source} ${clBolt.Runtime.Headers} ${clBolt.Runtime.Headers.Detail} ${clBolt.Runtime.Headers.Misc} ${clBolt.Runtime.clFiles.FullPath} )

# Include headers files
include_directories( ${BOLT_INCLUDE_DIR} ${PROJECT_BINARY_DIR}/include ${OPENCL_INCLUDE_DIRS} ${Boost_INCLUDE_DIRS} )

# Create a list of files that we expect to generate from the kernel files
substList( ".cl" ".hpp" clBolt.Runtime.hppFiles clBolt.Runtime.clFiles )
# printList( clBolt.Runtime.hppFiles )

prependPath( clBolt.Runtime.hppFiles.FullPath clBolt.Runtime.hppFiles ${PROJECT_BINARY_DIR}/include/bolt )
# printList( clBolt.Runtime.hppFiles.FullPath )

add_custom_command(
  OUTPUT ${clBolt.Runtime.hppFiles.FullPath}
  COMMAND clBolt.StringifyKernels -d "${PROJECT_BINARY_DIR}/include/bolt/" ${clBolt.Runtime.clFiles.FullPath}
  DEPENDS clBolt.StringifyKernels ${clBolt.Runtime.clFiles.FullPath}
  COMMENT "Creating kernel header files from .cl files"
  VERBATIM
)

add_library( clBolt.Runtime STATIC ${clBolt.Runtime.Files} ${clBolt.Runtime.hppFiles.FullPath} )
target_link_libraries( clBolt.Runtime ${OPENCL_LIBRARIES} ${Boost_LIBRARIES} )

set_target_properties( clBolt.Runtime PROPERTIES VERSION ${Bolt_VERSION} )
set_target_properties( clBolt.Runtime PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/staging" )

if( MSVC_VERSION VERSION_LESS 1600 )
    set_target_properties( clBolt.Runtime PROPERTIES ARCHIVE_OUTPUT_NAME "clBolt.Runtime.vc90" )
elseif( MSVC_VERSION VERSION_LESS 1700 )
    set_target_properties( clBolt.Runtime PROPERTIES ARCHIVE_OUTPUT_NAME "clBolt.Runtime.vc100" )
elseif( MSVC_VERSION VERSION_LESS 1800 )
    set_target_properties( clBolt.Runtime PROPERTIES ARCHIVE_OUTPUT_NAME "clBolt.Runtime.vc110" )
endif( )

# install( FILES
            # ${clBolt.Runtime.clFiles.FullPath}
        # DESTINATION
            # ${BIN_DIR} )

# install( FILES
            # ${clBolt.Runtime.clFiles.FullPath}
        # DESTINATION
            # ${PROJECT_BINARY_DIR}/staging/\${CMAKE_INSTALL_CONFIG_NAME} )

install( FILES
            ${clBolt.Runtime.Headers}
        DESTINATION
            ${INCLUDE_DIR}/bolt/cl )

install( FILES
            ${clBolt.Runtime.Headers.Detail}
        DESTINATION
            ${INCLUDE_DIR}/bolt/cl/detail )

install( FILES
            ${clBolt.Runtime.Headers.Misc}
        DESTINATION
            ${INCLUDE_DIR}/bolt )

# Install dependent Boost header files
set( ROOT_EXTERNAL_BOOST ${PROJECT_BINARY_DIR}/../external/boost/src/Boost/boost )

# smart_ptr library
install( DIRECTORY 
            ${ROOT_EXTERNAL_BOOST}/smart_ptr
            ${ROOT_EXTERNAL_BOOST}/detail
        DESTINATION
            ${INCLUDE_DIR}/boost )

install( FILES
            ${ROOT_EXTERNAL_BOOST}/enable_shared_from_this.hpp
            ${ROOT_EXTERNAL_BOOST}/get_pointer.hpp
            ${ROOT_EXTERNAL_BOOST}/intrusive_ptr.hpp
            ${ROOT_EXTERNAL_BOOST}/make_shared.hpp
            ${ROOT_EXTERNAL_BOOST}/memory_order.hpp
            ${ROOT_EXTERNAL_BOOST}/pointer_cast.hpp
            ${ROOT_EXTERNAL_BOOST}/pointer_to_other.hpp
            ${ROOT_EXTERNAL_BOOST}/scoped_array.hpp
            ${ROOT_EXTERNAL_BOOST}/scoped_ptr.hpp
            ${ROOT_EXTERNAL_BOOST}/shared_array.hpp
            ${ROOT_EXTERNAL_BOOST}/shared_ptr.hpp
            ${ROOT_EXTERNAL_BOOST}/smart_ptr.hpp
            ${ROOT_EXTERNAL_BOOST}/weak_ptr.hpp
        DESTINATION
            ${INCLUDE_DIR}/boost )

# iterator library
install( DIRECTORY 
            ${ROOT_EXTERNAL_BOOST}/iterator
            ${ROOT_EXTERNAL_BOOST}/pending
            ${ROOT_EXTERNAL_BOOST}/config
            ${ROOT_EXTERNAL_BOOST}/mpl
            ${ROOT_EXTERNAL_BOOST}/type_traits
            ${ROOT_EXTERNAL_BOOST}/preprocessor
            ${ROOT_EXTERNAL_BOOST}/utility
            ${ROOT_EXTERNAL_BOOST}/exception
        DESTINATION
            ${INCLUDE_DIR}/boost )

install( FILES
            ${ROOT_EXTERNAL_BOOST}/config.hpp
            ${ROOT_EXTERNAL_BOOST}/function_output_iterator.hpp
            ${ROOT_EXTERNAL_BOOST}/iterator.hpp
            ${ROOT_EXTERNAL_BOOST}/iterator_adaptors.hpp
            ${ROOT_EXTERNAL_BOOST}/pointee.hpp
            ${ROOT_EXTERNAL_BOOST}/shared_container_iterator.hpp
            ${ROOT_EXTERNAL_BOOST}/static_assert.hpp
            ${ROOT_EXTERNAL_BOOST}/utility.hpp
            ${ROOT_EXTERNAL_BOOST}/checked_delete.hpp
            ${ROOT_EXTERNAL_BOOST}/next_prior.hpp
            ${ROOT_EXTERNAL_BOOST}/noncopyable.hpp
            ${ROOT_EXTERNAL_BOOST}/assert.hpp
            ${ROOT_EXTERNAL_BOOST}/current_function.hpp
            ${ROOT_EXTERNAL_BOOST}/throw_exception.hpp
        DESTINATION
            ${INCLUDE_DIR}/boost )

# CPack configuration; include the executable into the package
install( TARGETS clBolt.Runtime
    EXPORT exportBOLT
    RUNTIME DESTINATION ${BIN_DIR}
    LIBRARY DESTINATION ${LIB_DIR}
    ARCHIVE DESTINATION ${LIB_DIR}
    )

# install( EXPORT exportBOLT
        # DESTINATION exportCmake
        # )
