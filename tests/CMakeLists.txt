# Checks that need a built WebGPU implementation; see each program's header.

if (WEBGPU_BACKEND_U STREQUAL "WGPU")
	add_executable(wgpu_rows_per_image_3d wgpu-native/rows_per_image_3d.cpp)
	target_link_libraries(wgpu_rows_per_image_3d PRIVATE webgpu)
	target_compile_features(wgpu_rows_per_image_3d PRIVATE cxx_std_17)
	target_copy_webgpu_binaries(wgpu_rows_per_image_3d)
endif()
