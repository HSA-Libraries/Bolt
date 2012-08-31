#############################################################################
##	Copyright (C) 2010 Advanced Micro Devices, Inc. All Rights Reserved.
#############################################################################
# List the names of common files to compile across all platforms
set( clBolt.Test.Transform.Source stdafx.cpp TransformTest.cpp ReadFromFile.cpp)
set( clBolt.Test.Transform.Headers stdafx.h targetver.h utils.h saxpy_functor.h ${BOLT_INCLUDE_DIR}/bolt/cl/transform.h ${BOLT_INCLUDE_DIR}/bolt/cl/detail/transform.inl )

set( clBolt.Test.Transform.Files ${clBolt.Test.Transform.Source} ${clBolt.Test.Transform.Headers} )

add_executable( clBolt.Test.Transform ${clBolt.Test.Transform.Files} )
target_link_libraries( clBolt.Test.Transform clBolt.Runtime ${OPENCL_LIBRARIES} ${GTEST_LIBRARIES} )

set_target_properties( clBolt.Test.Transform PROPERTIES VERSION ${Bolt_VERSION} )
set_target_properties( clBolt.Test.Transform PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/staging" )

set_property( TARGET clBolt.Test.Transform PROPERTY FOLDER "Test/OpenCL")
		
# CPack configuration; include the executable into the package
install( TARGETS clBolt.Test.Transform
	RUNTIME DESTINATION ${BIN_DIR}
	LIBRARY DESTINATION ${LIB_DIR}
	ARCHIVE DESTINATION ${LIB_DIR}/import
	)

install( FILES       
         saxpy_functor.h   DESTINATION ${BIN_DIR} 
		 )

install( FILES       
         saxpy_functor.h DESTINATION ${PROJECT_BINARY_DIR}/staging/\${CMAKE_INSTALL_CONFIG_NAME} 
		 )


