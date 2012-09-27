#############################################################################
##  Copyright (C) 2010 Advanced Micro Devices, Inc. All Rights Reserved.
#############################################################################
# List the names of common files to compile across all platforms

# set( testName clBolt.Test.TransformReduce )
set( clBolt.Test.TransformReduce.Source ${BOLT_CL_TEST_DIR}/common/stdafx.cpp 
                                        ${BOLT_CL_TEST_DIR}/common/myocl.cpp 
                                        TransformReduceTest.cpp )
set( clBolt.Test.TransformReduce.Headers ${BOLT_CL_TEST_DIR}/common/stdafx.h
                                         ${BOLT_CL_TEST_DIR}/common/targetver.h 
                                         ${BOLT_CL_TEST_DIR}/common/myocl.h 
                                         ${BOLT_INCLUDE_DIR}/bolt/cl/transform_reduce.h 
                                         ${BOLT_INCLUDE_DIR}/bolt/cl/detail/transform_reduce.inl)

set( clBolt.Test.TransformReduce.Files ${clBolt.Test.TransformReduce.Source} ${clBolt.Test.TransformReduce.Headers} )

# Set project specific compile and link options
if( MSVC )
    set( CMAKE_CXX_FLAGS "-bigobj ${CMAKE_CXX_FLAGS}" )
    set( CMAKE_C_FLAGS "-bigobj ${CMAKE_C_FLAGS}" )
endif()

add_executable( clBolt.Test.TransformReduce ${clBolt.Test.TransformReduce.Files} )
target_link_libraries( clBolt.Test.TransformReduce clBolt.Runtime ${OPENCL_LIBRARIES} ${GTEST_LIBRARIES} )

set_target_properties( clBolt.Test.TransformReduce PROPERTIES VERSION ${Bolt_VERSION} )
set_target_properties( clBolt.Test.TransformReduce PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/staging" )

set_property( TARGET clBolt.Test.TransformReduce PROPERTY FOLDER "Test/OpenCL")

# CPack configuration; include the executable into the package
install( TARGETS clBolt.Test.TransformReduce
    RUNTIME DESTINATION ${BIN_DIR}
    LIBRARY DESTINATION ${LIB_DIR}
    ARCHIVE DESTINATION ${LIB_DIR}/import
    )
