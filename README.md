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
  -DWEBGPU_BUILD_GLFW3WEBGPU=ON \
  -DWEBGPU_BUILD_EXAMPLES=ON

cmake --build build --config Release
```

## Workflows

Per-platform CI build workflows:

- `.github/workflows/build_windows.yaml`
- `.github/workflows/build_linux.yaml`
- `.github/workflows/build_macos.yaml`
- `.github/workflows/build_wasm.yaml`

Prebuilt release workflow:

- `.github/workflows/release_prebuilt.yaml`

## Acknowledgements

- [WebGPU-distribution](https://github.com/eliemichel/WebGPU-distribution)

## License

MIT
