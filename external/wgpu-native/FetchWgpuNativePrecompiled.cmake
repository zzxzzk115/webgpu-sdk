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

include(FetchContent)

# Not using emscripten, so we download binaries. There are many different
# combinations of OS, CPU architecture and compiler (the later is only
# relevant when using static linking), so here are a lot of boring "if".

detect_system_architecture()


# Build URL to fetch
set(URL_OS)
set(URL_COMPILER)
if (CMAKE_SYSTEM_NAME STREQUAL "Windows")

	set(URL_OS "windows")

	if (MSVC)
		set(URL_COMPILER "msvc")
	else()
		set(URL_COMPILER "gnu")
	endif()

elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")

	set(URL_OS "linux")

elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin")

	set(URL_OS "macos")

else()

	message(FATAL_ERROR "Platform system '${CMAKE_SYSTEM_NAME}' not supported by this release of WebGPU. You may consider building it yourself from its source (see https://github.com/gfx-rs/wgpu-native)")

endif()

set(URL_ARCH)
if (ARCH STREQUAL "x86_64")
	set(URL_ARCH "x86_64")
elseif (ARCH STREQUAL "aarch64")
	set(URL_ARCH "aarch64")
elseif (ARCH STREQUAL "i686" AND CMAKE_SYSTEM_NAME STREQUAL "Windows")
	set(URL_ARCH "i686")
else()
	message(FATAL_ERROR "Platform architecture '${ARCH}' not supported by this release of WebGPU. You may consider building it yourself from its source (see https://github.com/gfx-rs/wgpu-native)")
endif()

# We only fetch release builds (NB: this may cause issue when using static
# linking, but not with dynamic)
set(URL_CONFIG release)

# Finally build the URL. The URL_NAME is also used as FetchContent
# identifier (BTW it must be lowercase).
if (URL_COMPILER)
	set(URL_NAME "wgpu-${URL_OS}-${URL_ARCH}-${URL_COMPILER}-${URL_CONFIG}")
else()
	set(URL_NAME "wgpu-${URL_OS}-${URL_ARCH}-${URL_CONFIG}")
endif()
set(URL "${WGPU_BINARY_MIRROR}/releases/download/${WGPU_VERSION}/${URL_NAME}.zip")

string(TOLOWER "${URL_NAME}" FC_NAME)

# Declare FetchContent, then make available
FetchContent_Declare(${FC_NAME}
	URL ${URL}
)
# TODO: Display the "Fetching" message only when actually downloading
message(STATUS "Fetching WebGPU implementation from '${URL}'")
FetchContent_MakeAvailable(${FC_NAME})
set(ZIP_DIR "${${FC_NAME}_SOURCE_DIR}")

include("${CMAKE_CURRENT_LIST_DIR}/ImportWgpuNative.cmake")
