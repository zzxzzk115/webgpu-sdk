# Builds wgpu-native from source, with webgpu-sdk's patches applied to the wgpu crates it depends on, into OUT_DIR
# laid out like a wgpu-native release (include/webgpu, lib). Called in script mode by
# external/wgpu-native/FetchWgpuNativeSource.cmake:
#
#   cmake -DWGPU_NATIVE_TAG=<tag> -DWGPU_TAG=<tag> -DRUST_TOOLCHAIN=<version> -DPATCH_DIR=<dir>
#         -DWORK_DIR=<dir> -DOUT_DIR=<dir> -DSTAMP_FILE=<file> -DWINDOWS_ABI=<msvc|gnu|> -P build_wgpu_native.cmake
#
# WINDOWS_ABI is the C/C++ compiler's ABI on a Windows host (empty elsewhere); the Rust toolchain must match it.
# Every step must succeed: a patch that does not apply, a wgpu-native tag that pins another wgpu, or a build that
# does not use the patched crates stops it. STAMP_FILE is copied to OUT_DIR/stamp.txt last, once the output is complete.

cmake_minimum_required(VERSION 3.21)

foreach(_var WGPU_NATIVE_TAG WGPU_TAG RUST_TOOLCHAIN PATCH_DIR WORK_DIR OUT_DIR STAMP_FILE WINDOWS_ABI)
	if (NOT DEFINED ${_var})
		message(FATAL_ERROR "build_wgpu_native.cmake: ${_var} is not set")
	endif()
endforeach()

find_program(GIT_EXECUTABLE git REQUIRED)
find_program(CARGO_EXECUTABLE cargo REQUIRED)
find_program(RUSTC_EXECUTABLE rustc REQUIRED)

# Runs a command and stops with its output if it fails.
function(run_checked DESCRIPTION WORKING_DIR)
	execute_process(
		COMMAND ${ARGN}
		WORKING_DIRECTORY "${WORKING_DIR}"
		RESULT_VARIABLE _result
	)
	if (NOT _result EQUAL 0)
		message(FATAL_ERROR "build_wgpu_native.cmake: ${DESCRIPTION} failed (${_result})")
	endif()
endfunction()

# The clones keep LF line endings (core.autocrlf=false) so the patches, written with LF, apply on Windows too.
set(_native "${WORK_DIR}/wgpu-native")
set(_wgpu "${WORK_DIR}/wgpu")

file(REMOVE_RECURSE "${WORK_DIR}" "${OUT_DIR}")
file(MAKE_DIRECTORY "${WORK_DIR}")

message(STATUS "wgpu-native: fetching ${WGPU_NATIVE_TAG} and wgpu ${WGPU_TAG}")
run_checked("cloning wgpu-native ${WGPU_NATIVE_TAG}" "${WORK_DIR}"
	"${GIT_EXECUTABLE}" -c advice.detachedHead=false clone --quiet -c core.autocrlf=false --depth 1 --branch "${WGPU_NATIVE_TAG}"
	--recurse-submodules --shallow-submodules https://github.com/gfx-rs/wgpu-native.git "${_native}")
run_checked("cloning wgpu ${WGPU_TAG}" "${WORK_DIR}"
	"${GIT_EXECUTABLE}" -c advice.detachedHead=false clone --quiet -c core.autocrlf=false --depth 1 --branch "${WGPU_TAG}"
	https://github.com/gfx-rs/wgpu.git "${_wgpu}")

# The patches are written against the wgpu that this wgpu-native tag pins; anything else is a different build.
file(READ "${_native}/Cargo.toml" _manifest)
string(REPLACE "\r" "" _manifest "${_manifest}")
string(FIND "${_manifest}" "git = \"https://github.com/gfx-rs/wgpu\"\ntag = \"${WGPU_TAG}\"" _pinned)
if (_pinned EQUAL -1)
	message(FATAL_ERROR "build_wgpu_native.cmake: wgpu-native ${WGPU_NATIVE_TAG} does not pin wgpu ${WGPU_TAG} "
		"through git; update external/wgpu-native/wgpu-git-tag.txt and this script together")
endif()

file(GLOB _patches "${PATCH_DIR}/*.patch")
list(SORT _patches)
if (NOT _patches)
	message(FATAL_ERROR "build_wgpu_native.cmake: no patches in ${PATCH_DIR}")
endif()
foreach(_patch IN LISTS _patches)
	get_filename_component(_name "${_patch}" NAME)
	message(STATUS "wgpu-native: applying ${_name}")
	run_checked("checking ${_name} against wgpu ${WGPU_TAG}" "${_wgpu}" "${GIT_EXECUTABLE}" apply --check "${_patch}")
	run_checked("applying ${_name}" "${_wgpu}" "${GIT_EXECUTABLE}" apply "${_patch}")
endforeach()

# Build wgpu-native against the patched checkout instead of the tag it fetches, and with the pinned toolchain
# (rustup reads rust-toolchain.toml from the working directory).
file(APPEND "${_native}/Cargo.toml"
	"\n[patch.\"https://github.com/gfx-rs/wgpu\"]\n"
	"wgpu-core = { path = \"${_wgpu}/wgpu-core\" }\n"
	"wgpu-hal = { path = \"${_wgpu}/wgpu-hal\" }\n"
	"wgpu-types = { path = \"${_wgpu}/wgpu-types\" }\n"
	"naga = { path = \"${_wgpu}/naga\" }\n")
file(WRITE "${_native}/rust-toolchain.toml" "[toolchain]\nchannel = \"${RUST_TOOLCHAIN}\"\n")

execute_process(
	COMMAND "${CARGO_EXECUTABLE}" tree --invert wgpu-hal --edges normal --depth 0
	WORKING_DIRECTORY "${_native}"
	RESULT_VARIABLE _result
	OUTPUT_VARIABLE _tree
)
if (NOT _result EQUAL 0 OR _tree MATCHES "github\\.com/gfx-rs/wgpu" OR NOT _tree MATCHES "wgpu-hal v")
	message(FATAL_ERROR "build_wgpu_native.cmake: wgpu-native does not resolve wgpu-hal to the patched checkout:\n${_tree}")
endif()
message(STATUS "wgpu-native: ${_tree}")

# Build for the host triple into a directory of our own, so neither CARGO_TARGET_DIR nor CARGO_BUILD_TARGET (or
# their config equivalents) moves the output. FetchWgpuNativeSource.cmake refuses configures for another target.
execute_process(
	COMMAND "${RUSTC_EXECUTABLE}" -vV
	WORKING_DIRECTORY "${_native}"
	RESULT_VARIABLE _result
	OUTPUT_VARIABLE _rustc
)
if (NOT _result EQUAL 0 OR NOT _rustc MATCHES "host: ([^\r\n]+)")
	message(FATAL_ERROR "build_wgpu_native.cmake: rustc -vV did not report the host triple:\n${_rustc}")
endif()
set(_triple "${CMAKE_MATCH_1}")
if (CMAKE_HOST_WIN32 AND NOT _triple MATCHES "-${WINDOWS_ABI}$")
	message(FATAL_ERROR "build_wgpu_native.cmake: the Rust toolchain builds for ${_triple}, but the C/C++ compiler uses "
		"the ${WINDOWS_ABI} ABI; install the toolchain for ${RUST_TOOLCHAIN} with that ABI as rustup's default host")
endif()

message(STATUS "wgpu-native: cargo build --release --target ${_triple} (toolchain ${RUST_TOOLCHAIN})")
# wgpuGetVersion() reports this, as in the upstream release build.
set(ENV{WGPU_NATIVE_VERSION} "${WGPU_NATIVE_TAG}")
run_checked("cargo build" "${_native}" "${CARGO_EXECUTABLE}" build --release --target "${_triple}"
	--target-dir "${WORK_DIR}/target")

# Lay the output out like a wgpu-native release.
set(_target "${WORK_DIR}/target/${_triple}/release")
if (CMAKE_HOST_WIN32 AND WINDOWS_ABI STREQUAL "msvc")
	set(_static wgpu_native.lib)
	set(_shared wgpu_native.dll wgpu_native.dll.lib)
elseif (CMAKE_HOST_WIN32)
	set(_static libwgpu_native.a)
	set(_shared wgpu_native.dll libwgpu_native.dll.a)
elseif (CMAKE_HOST_APPLE)
	set(_static libwgpu_native.a)
	set(_shared libwgpu_native.dylib)
else()
	set(_static libwgpu_native.a)
	set(_shared libwgpu_native.so)
endif()
file(MAKE_DIRECTORY "${OUT_DIR}/lib" "${OUT_DIR}/include/webgpu")
foreach(_file IN LISTS _static _shared)
	if (NOT EXISTS "${_target}/${_file}")
		message(FATAL_ERROR "build_wgpu_native.cmake: cargo did not produce ${_file}")
	endif()
	file(COPY "${_target}/${_file}" DESTINATION "${OUT_DIR}/lib")
endforeach()
file(COPY "${_native}/ffi/webgpu-headers/webgpu.h" "${_native}/ffi/wgpu.h" DESTINATION "${OUT_DIR}/include/webgpu")
file(COPY_FILE "${STAMP_FILE}" "${OUT_DIR}/stamp.txt")
message(STATUS "wgpu-native: built into ${OUT_DIR}")
