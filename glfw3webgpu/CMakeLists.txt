if (EMSCRIPTEN)
	add_library(glfw INTERFACE)
	set(EMSCRIPTEN_GLFW3_PORT "${CMAKE_SOURCE_DIR}/external/emscripten-glfw3/port/glfw3.py")
	target_compile_options(glfw INTERFACE "--use-port=${EMSCRIPTEN_GLFW3_PORT}")
	target_link_options(glfw INTERFACE "--use-port=${EMSCRIPTEN_GLFW3_PORT}")
else()
	option(WEBGPU_USE_SYSTEM_GLFW "Use system-installed glfw3 instead of bundled source." OFF)
	set(WEBGPU_GLFW_SOURCE_DIR "${CMAKE_SOURCE_DIR}/external/glfw" CACHE PATH
		"Path to bundled glfw source tree used when WEBGPU_USE_SYSTEM_GLFW=OFF.")

	if (WEBGPU_USE_SYSTEM_GLFW)
		find_package(glfw3 3.4 REQUIRED)
		if (NOT TARGET glfw)
			message(FATAL_ERROR "System glfw3 found, but CMake target 'glfw' is unavailable.")
		endif()
	else()
		if (NOT EXISTS "${WEBGPU_GLFW_SOURCE_DIR}/CMakeLists.txt")
			message(FATAL_ERROR
				"Bundled GLFW source not found: '${WEBGPU_GLFW_SOURCE_DIR}'. "
				"Either provide this directory or set WEBGPU_USE_SYSTEM_GLFW=ON.")
		endif()

		set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
		set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
		set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
		set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
		add_subdirectory("${WEBGPU_GLFW_SOURCE_DIR}" "${CMAKE_BINARY_DIR}/_deps/glfw-build" EXCLUDE_FROM_ALL)
		if (NOT TARGET glfw)
			message(FATAL_ERROR "Bundled GLFW source was added, but target 'glfw' was not created.")
		endif()
	endif()
endif()

add_library(glfw3webgpu STATIC
	src/glfw3webgpu.c
)

target_include_directories(glfw3webgpu
	PUBLIC
		"${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_link_libraries(glfw3webgpu
	PUBLIC
		glfw
		webgpu
)

if (APPLE)
	target_compile_options(glfw3webgpu PRIVATE -x objective-c)
	target_link_libraries(glfw3webgpu PRIVATE "-framework Cocoa" "-framework QuartzCore")
endif()

install(TARGETS glfw3webgpu
	ARCHIVE DESTINATION lib
	LIBRARY DESTINATION lib
	RUNTIME DESTINATION bin
)

install(FILES include/glfw3webgpu.h DESTINATION include)
