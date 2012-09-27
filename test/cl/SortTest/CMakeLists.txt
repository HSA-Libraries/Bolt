#############################################################################
##  Copyright (C) 2010 Advanced Micro Devices, Inc. All Rights Reserved.
#############################################################################
# List the names of common files to compile across all platforms

set( clBolt.Test.Sort.Source SortTest.cpp 
                             ${BOLT_CL_TEST_DIR}/common/myocl.cpp)
set( clBolt.Test.Sort.Headers   ${BOLT_CL_TEST_DIR}/common/myocl.h
                                ${BOLT_INCLUDE_DIR}/bolt/cl/sort.h
                                ${BOLT_INCLUDE_DIR}/bolt/cl/detail/sort.inl )

set( clBolt.Test.Sort.Files ${clBolt.Test.Sort.Source} ${clBolt.Test.Sort.Headers} )

# Include standard OpenCL headers
include_directories( ${OPENCL_INCLUDE_DIRS} )

# Set project specific compile and link options
if( MSVC )
set( CMAKE_CXX_FLAGS "-bigobj ${CMAKE_CXX_FLAGS}" )
                set( CMAKE_C_FLAGS "-bigobj ${CMAKE_C_FLAGS}" )
endif()

add_executable( clBolt.Test.Sort ${clBolt.Test.Sort.Files} )
target_link_libraries( clBolt.Test.Sort ${OPENCL_LIBRARIES} clBolt.Runtime ${GTEST_LIBRARIES}  )

set_target_properties( clBolt.Test.Sort PROPERTIES VERSION ${Bolt_VERSION} )
set_target_properties( clBolt.Test.Sort PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/staging" )

set_property( TARGET clBolt.Test.Sort PROPERTY FOLDER "Test/OpenCL")
        
# CPack configuration; include the executable into the package
install( TARGETS clBolt.Test.Sort
    RUNTIME DESTINATION ${BIN_DIR}
    LIBRARY DESTINATION ${LIB_DIR}
    ARCHIVE DESTINATION ${LIB_DIR}/import
    )
