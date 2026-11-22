# This file is part of the "Learn WebGPU for C++" book.
#   https://eliemichel.github.io/LearnWebGPU
# 
# MIT License
# Copyright (c) 2022-2025 Elie Michel and the wgpu-native authors
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.


# Builds wgpu-native WGPU_VERSION from source with the patches in patch/ applied to the wgpu crates it depends on,
# then imports it as the target 'webgpu'. The build (cmake/build_wgpu_native.cmake) needs git and a Rust toolchain
# (the version in rust-toolchain.txt, installed with rustup) and runs at configure time. It is skipped when
# WGPU_NATIVE_DIST_DIR already holds a build of the same wgpu-native tag, wgpu tag, toolchain and patches, so a
# library built elsewhere (for example in the Linux release container) is reused.

set(WGPU_NATIVE_DIST_DIR "${CMAKE_BINARY_DIR}/wgpu-native-dist" CACHE PATH "\
	Where the wgpu-native source build is placed, laid out like a wgpu-native release.")

# Cargo builds for the host, so the source build serves only configures that target the host system and
# architecture.
detect_system_architecture()
set(_host_arch "${CMAKE_HOST_SYSTEM_PROCESSOR}")
if (_host_arch MATCHES "^(AMD64|amd64|x86_64)$")
	set(_host_arch "x86_64")
elseif (_host_arch MATCHES "^(arm64|ARM64|aarch64)$")
	set(_host_arch "aarch64")
endif()
set(_target_arch "${ARCH}")
if (CMAKE_OSX_ARCHITECTURES)
	string(REPLACE "arm64" "aarch64" _target_arch "${CMAKE_OSX_ARCHITECTURES}")
endif()
if (CMAKE_CROSSCOMPILING OR NOT _target_arch STREQUAL _host_arch)
	message(FATAL_ERROR "wgpu-native: the source build compiles for the host (${CMAKE_HOST_SYSTEM_NAME} ${_host_arch}), "
		"but this configure targets ${CMAKE_SYSTEM_NAME} ${_target_arch}; build on a host of the target system and "
		"architecture, or use the precompiled release (WEBGPU_BUILD_FROM_SOURCE=OFF)")
endif()

file(STRINGS "${CMAKE_CURRENT_LIST_DIR}/wgpu-git-tag.txt" _wgpu_tag LIMIT_COUNT 1)
file(STRINGS "${CMAKE_CURRENT_LIST_DIR}/rust-toolchain.txt" _rust_toolchain LIMIT_COUNT 1)
file(GLOB _patches "${CMAKE_CURRENT_LIST_DIR}/patch/*.patch")
list(SORT _patches)

set(_stamp "wgpu-native ${WGPU_VERSION}\nwgpu ${_wgpu_tag}\nrust ${_rust_toolchain}\n")
foreach(_patch IN LISTS _patches)
	get_filename_component(_name "${_patch}" NAME)
	file(SHA256 "${_patch}" _hash)
	string(APPEND _stamp "patch ${_name} ${_hash}\n")
endforeach()

set(_built_stamp)
if (EXISTS "${WGPU_NATIVE_DIST_DIR}/stamp.txt")
	file(READ "${WGPU_NATIVE_DIST_DIR}/stamp.txt" _built_stamp)
endif()

if (_built_stamp STREQUAL _stamp)
	message(STATUS "wgpu-native: reusing the source build in '${WGPU_NATIVE_DIST_DIR}'")
else()
	message(STATUS "wgpu-native: building ${WGPU_VERSION} from source into '${WGPU_NATIVE_DIST_DIR}'")
	file(WRITE "${CMAKE_BINARY_DIR}/wgpu-native-stamp.txt" "${_stamp}")
	execute_process(
		COMMAND "${CMAKE_COMMAND}"
			"-DWGPU_NATIVE_TAG=${WGPU_VERSION}"
			"-DWGPU_TAG=${_wgpu_tag}"
			"-DRUST_TOOLCHAIN=${_rust_toolchain}"
			"-DPATCH_DIR=${CMAKE_CURRENT_LIST_DIR}/patch"
			"-DWORK_DIR=${CMAKE_BINARY_DIR}/wgpu-native-src"
			"-DOUT_DIR=${WGPU_NATIVE_DIST_DIR}"
			"-DSTAMP_FILE=${CMAKE_BINARY_DIR}/wgpu-native-stamp.txt"
			-P "${PROJECT_SOURCE_DIR}/cmake/build_wgpu_native.cmake"
		RESULT_VARIABLE _result
	)
	if (NOT _result EQUAL 0)
		message(FATAL_ERROR "wgpu-native: the source build failed")
	endif()
endif()

set(ZIP_DIR "${WGPU_NATIVE_DIST_DIR}")
include("${CMAKE_CURRENT_LIST_DIR}/ImportWgpuNative.cmake")
