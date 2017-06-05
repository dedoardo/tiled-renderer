cmake_minimum_required(VERSION 3.2 FATAL_ERROR)
project(camy VERSION 0.9 LANGUAGES CXX)

# Headers
set(CAMY_HEADERS
		include/camy/config.hpp
		include/camy/core.hpp
		include/camy/core.inl
		include/camy/math.hpp
		include/camy/system.hpp
		include/camy/system.inl
		include/camy/graphics.hpp
		include/camy/render_context.hpp
		include/camy/render_context.inl
		include/camy/command_list.hpp
		include/camy/buckets.hpp
		include/camy/frame.hpp
		include/camy/program.hpp
		include/camy/camy.hpp

		include/camy/containers/auto_ptr.hpp
		include/camy/containers/blob.hpp
		include/camy/containers/dyn_array.hpp
		include/camy/containers/dyn_linear_buffer.hpp
		include/camy/containers/dyn_linear_array.hpp
		include/camy/containers/hash_map.hpp
		include/camy/containers/paged_dyn_linear_array.hpp
		include/camy/containers/paged_pool.hpp
		include/camy/containers/pages.hpp
		include/camy/containers/pool.hpp
		include/camy/containers/stack.hpp
		include/camy/containers/static_array.hpp
		include/camy/containers/static_string.hpp

		include/camy/d3d11/d3d11_base_post.hpp
		include/camy/d3d11/d3d11_base_pre.hpp
		
		include/camy/opengl4/opengl4_base_post.hpp
		include/camy/opengl4/opengl4_base_pre.hpp
		include/camy/opengl4/opengl4_command_list.hpp
)

set (CAMY_SOURCES
	src/graphics.cpp
	src/compute_bucket.cpp
	src/effect_bucket.cpp
	src/file.cpp
	src/frame.cpp
	src/log.cpp
	src/math.cpp
	src/memory.cpp
	src/program.cpp
	src/render_bucket.cpp
	src/thread.cpp
	src/time.cpp

	src/containers/blob.cpp
	src/containers/dyn_linear_buffer.cpp
	src/containers/int2int_map.cpp
	
	src/d3d11/d3d11_command_list.cpp
	src/d3d11/d3d11_platform.cpp
	src/d3d11/d3d11_program.cpp
	src/d3d11/d3d11_render_context.cpp

	src/opengl4/opengl4_command_list.cpp
	src/opengl4/opengl4_platform.cpp
	src/opengl4/opengl4_program.cpp
	src/opengl4/opengl4_render_context.cpp
)

# Adding target
add_library(camy STATIC ${CAMY_SOURCES} ${CAMY_HEADERS})

# Include directories
target_include_directories(camy
	PRIVATE
		"${CMAKE_CURRENT_SOURCE_DIR}/include"
	PUBLIC
		"${GLEW_INSTALL_DIR}/include"
)

# Making sure output is in ./lib
set(CAMY_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lib")
set_target_properties(camy 
	PROPERTIES
		CXX_STANDARD 14
		CXX_STANDARD_REQUIRED YES
		CXX_EXTENSIONS NO
		ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CAMY_LIB_DIR}
		ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CAMY_LIB_DIR}
		ARCHIVE_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CAMY_LIB_DIR}
)

# Finding glew.lib
IF(${PLATFORM} MATCHES x86)
	set(GLEW_LIB "${GLEW_INSTALL_DIR}/lib/Release/Win32/glew32s.lib")
ELSE(${PLATFORM} MATCHES x86)
	set(GLEW_LIB "${GLEW_INSTALL_DIR}/lib/Release/x64/glew32s.lib")
ENDIF(${PLATFORM} MATCHES x86)

# Interface linking
target_link_libraries(camy
	INTERFACE d3d11 d3dcompiler dxgi dxguid OpenGL32 ${GLEW_LIB})

# We are linking statically to glew
target_compile_definitions(camy
	PUBLIC GLEW_STATIC
)