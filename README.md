# AudioVideoEngine-CPP

A standalone C++ audio/video engine extracted from the original `asset/cpp` project and rebuilt as a native, Linux-first module that remains easy to adapt for Android and iOS toolchains.

## Features

- Cross-platform platform detection
- Linux camera backend via V4L2
- LMS adaptive noise suppression engine
- CMake-based build system
- Standalone repo ready for independent development

## Directory layout

- `include/` public headers
- `src/` implementation files
- `CMakeLists.txt` build configuration

## Build on Linux

```bash
git clone https://github.com/PMR-Pubclications/AudioVideoEngine-CPP.git
cd AudioVideoEngine-CPP
cmake -S . -B build
cmake --build build
./build/audio_video_engine
```

## What this repo includes

- `LMSAdaptiveFilter` for adaptive noise cancellation
- `CameraInterface` and platform abstraction
- Linux camera discovery using `/dev/video*`
- A minimal executable to validate the engine at runtime

## Notes

This project is intentionally lightweight and builds as a standalone C++ engine. The Android/iOS-specific code paths are left as optional extensions rather than forcing a mobile toolchain into the host build.
