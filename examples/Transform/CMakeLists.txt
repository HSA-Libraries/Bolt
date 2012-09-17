#############################################################################
##  Copyright (C) 2010 Advanced Micro Devices, Inc. All Rights Reserved.
#############################################################################
# List the names of common files to compile across all platforms
set( clBolt.Example.Transform.Source Transform.cpp )
set( clBolt.Example.Transform.Headers ${BOLT_INCLUDE_DIRS}/bolt/cl/transform.h ${BOLT_INCLUDE_DIRS}/bolt/cl/detail/transform.inl )

set( clBolt.Example.Transform.Files ${clBolt.Example.Transform.Source} ${clBolt.Example.Transform.Headers} )

# Include headers files
include_directories( ${BOLT_INCLUDE_DIRS} ${OPENCL_INCLUDE_DIRS} )

add_executable( clBolt.Example.Transform ${clBolt.Example.Transform.Files} )
target_link_libraries( clBolt.Example.Transform ${BOLT_LIBRARIES} ${OPENCL_LIBRARIES} )

set_target_properties( clBolt.Example.Transform PROPERTIES VERSION ${Examples_VERSION} )
set_target_properties( clBolt.Example.Transform PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/staging" )

# set_property( TARGET clBolt.Example.Transform PROPERTY FOLDER "Test/OpenCL")
install( FILES
            ${BOLT_INCLUDE_DIRS}/../${BIN_DIR}/transform_kernels.cl
        DESTINATION
            ${BIN_DIR} )

install( FILES
            ${BOLT_INCLUDE_DIRS}/../${BIN_DIR}/transform_kernels.cl
        DESTINATION
            ${PROJECT_BINARY_DIR}/staging/\${CMAKE_INSTALL_CONFIG_NAME} )

# CPack configuration; include the executable into the package
install( TARGETS clBolt.Example.Transform
    RUNTIME DESTINATION ${BIN_DIR}
    LIBRARY DESTINATION ${LIB_DIR}
    ARCHIVE DESTINATION ${LIB_DIR}/import
    )
