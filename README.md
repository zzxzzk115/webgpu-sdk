# webgpu-sdk

<h4 align="center">
  webgpu-sdk is a reusable WebGPU C/C++ SDK bundle for desktop and WASM.
</h4>

## Overview

`webgpu-sdk` packages:

- WebGPU backends under `external/`:
  `external/wgpu-native`, `external/dawn`, `external/emdawnwebgpu`, `external/emscripten`
- `glfw3webgpu` helper library as a first-class target
- Prebuilt release workflows for downstream engine/library consumption

## Features

- Cross-platform build workflows: Windows / Linux / macOS / WASM
- Release prebuilt bundles as GitHub Release assets
- `glfw3webgpu` shipped as independent library target (not inside examples)
- Vendored GLFW source in-repo (no default system GLFW assumption)
- Project layout rule: except `glfw3webgpu` and `examples`, dependencies live in `external/`

## GLFW Policy

Desktop build defaults to vendored GLFW source:

- `external/glfw/`

If you explicitly want system GLFW:

```bash
-DWEBGPU_USE_SYSTEM_GLFW=ON
```

## Build

```bash
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DWEBGPU_BACKEND=WGPU \
  -DWEBGPU_BUILD_FROM_SOURCE=OFF \
  -DWEBGPU_LINK_TYPE=STATIC \
  -DWEBGPU_BUILD_GLFW3WEBGPU=ON \
  -DWEBGPU_BUILD_EXAMPLES=ON

cmake --build build --config Release
```

### wgpu-native from source (patched)

With `-DWEBGPU_BACKEND=WGPU -DWEBGPU_BUILD_FROM_SOURCE=ON`, wgpu-native is built from source instead of downloaded.
The configure step does the following:

1. Fetches the wgpu-native tag `WGPU_VERSION` and the gfx-rs/wgpu tag it pins
   (`external/wgpu-native/wgpu-git-tag.txt`).
2. Applies the patches in `external/wgpu-native/patch/` to wgpu.
3. Builds with `cargo` using the Rust version in `external/wgpu-native/rust-toolchain.txt`.

Requirements: git, rustup with that toolchain installed, and libclang (for bindgen).

The build fails if any of these happens:
- a patch no longer applies;
- the wgpu-native tag pins another wgpu;
- cargo does not use the patched crates.

The release packages are built this way, so they carry the patches; the upstream wgpu-native release does not.

Each patch file starts with what it fixes and how it is checked. `tests/wgpu-native/` holds the regression checks,
built with `-DWEBGPU_BUILD_TESTS=ON` and run on WARP in CI.

When upgrading wgpu-native:
1. Update `WGPU_VERSION`, `wgpu-git-tag.txt` and, if needed, `rust-toolchain.txt` together.
2. Rebase every patch, or delete it once upstream has fixed the defect.
3. Keep its regression check passing.

## Workflows

Per-platform CI build workflows:

- `.github/workflows/build_windows.yaml`
- `.github/workflows/build_linux.yaml`
- `.github/workflows/build_macos.yaml`
- `.github/workflows/build_wasm.yaml`

Patched wgpu-native source build: builds it on Windows, Linux (in manylinux_2_28) and macOS. Checks that the
patched library passes the regression checks on WARP while the upstream release fails them, and that it exports the
same `wgpu*` symbols as the upstream release:

- `.github/workflows/build_wgpu_native_source.yaml`

Prebuilt release workflow:

- `.github/workflows/release_prebuilt.yaml`

## Acknowledgements

- [WebGPU-distribution](https://github.com/eliemichel/WebGPU-distribution)

## License

MIT
