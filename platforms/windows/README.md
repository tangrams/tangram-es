# Windows

## Setup

This project uses CMake (minimum version 3.0), you can download it [here](http://www.cmake.org/download/).

Make sure to update git submodules before you build:

```bash
git submodule update --init
```

Builds were tested under MinGW-w64 (required). You will need to install/build zlib and curl manually.

The demo application uses the Nextzen vector tile service, so you will need a Nextzen API key to build and run the demo.

 1. Visit https://developers.nextzen.org/ to get an API key.

 2. Setup an environment variable (`SET NEXTZEN_API_KEY=YOUR_API_KEY` or via system settings) to point to your API key.

## Build

  The **only** supported toolchain to build Tangram ES on Windows is currently MinGW (MinGW GCC, POSIX threads).

 1. Download zlib sources from https://zlib.net/

 2. Download curl sources fom https://curl.haxx.se/download.html

### Zlib and Curl under MinGW-w64

CMake typically installs everything under Program Files, which requires elevated privileges. It is encouraged you to make dedicated directory, e.g. `c:/cmake-install`. To make it work, set `CMAKE_PREFIX_PATH` env var to this directory. The build has been tested with curl 7.63 and zlib 1.2.

From the zlib source directory:
``` bat
cmake -H. -Bbuild -G "MinGW Makefiles" -DCMAKE_INSTALL_PREFIX=c:/cmake-install
cmake --build build
cmake --build build --target install
```
From the curl source directory:
``` bat
cmake -H. -Bbuild -G "MinGW Makefiles" -DCMAKE_INSTALL_PREFIX=c:/cmake-install -DBUILD_CURL_EXE=OFF -DBUILD_TESTING=OFF -DCMAKE_USE_WINSSL=ON
cmake --build build
cmake --build build --target install
```

### Build Tangram

In the root directory type `mingw32-make windows`. You can append `DEBUG=1` or `RELEASE=1` to choose the build type.

## Running app

You can open a different YAML scene file by using the argument `-f`:

```
tangram.exe -f "C:\Users\user\dev\tangram-es\build\windows\res\scene.yaml"
```
