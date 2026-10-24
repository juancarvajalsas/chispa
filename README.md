# gamedev

A small 2D game engine written in C++20 on top of Vulkan, developed by
**JuanCarvajal SAS**. It includes three games built with the engine: Pong,
Tetris, and Asteroids (in progress).

## Features

- Vulkan 1.3 renderer with memory managed by [VMA](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator)
- Textured, rotatable sprites with alpha blending
- Texture atlases with per-texture filtering (nearest / linear)
- PNG loading through [stb_image](https://github.com/nothings/stb)
- Keyboard input (held and edge-triggered key presses)
- Shaders compiled to SPIR-V automatically during the build

## Layout

```
engine/      The engine (static library: engine.h / engine.cpp)
  shaders/   GLSL shaders, compiled to SPIR-V by the build
  third_party/  stb_image.h, vk_mem_alloc.h
pong/        Pong
tetris/      Tetris
asteroids/   Asteroids (work in progress)
```

## Requirements

- A C++20 compiler
- CMake 3.20 or newer
- The Vulkan SDK, or the Vulkan headers, loader, and `glslc`
- GLFW 3.4
- A GPU and driver with Vulkan 1.3 support

Debug builds turn on `VK_LAYER_KHRONOS_validation`, so you need the validation
layers installed too. Release builds (`-DCMAKE_BUILD_TYPE=Release`) don't use
them.

### Linux

Arch Linux:

```sh
sudo pacman -S cmake ninja vulkan-headers vulkan-icd-loader vulkan-validation-layers shaderc glfw
```

Build:

```sh
cmake -S . -B build -G Ninja
cmake --build build
```

Run the games **from the repository root**, because they load assets from
paths relative to it:

```sh
./build/pong/pong
./build/tetris/tetris
./build/asteroids/asteroids
```

### Windows

Windows support is in progress.

## Third-party code

| Library | Location | License |
|---------|----------|---------|
| [stb_image](https://github.com/nothings/stb) v2.30 | `engine/third_party/stb_image.h` | Public domain or MIT (you choose) |
| [Vulkan Memory Allocator](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator) 3.4.0 | `engine/third_party/vk_mem_alloc.h` | MIT, Copyright (c) Advanced Micro Devices, Inc. |
| [GLFW](https://www.glfw.org/) | system dependency | zlib/libpng |

Each bundled header keeps its original license text.

## License

[MIT](LICENSE). Copyright (c) 2026 JuanCarvajal SAS.
