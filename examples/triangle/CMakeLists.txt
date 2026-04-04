add_executable(webgpu_triangle main.cpp)
target_compile_features(webgpu_triangle PRIVATE cxx_std_17)
target_link_libraries(webgpu_triangle PRIVATE glfw3webgpu)
target_copy_webgpu_binaries(webgpu_triangle)

if (EMSCRIPTEN)
	set_target_properties(webgpu_triangle PROPERTIES SUFFIX ".html")
	target_link_options(webgpu_triangle PRIVATE -sASYNCIFY)
endif()
