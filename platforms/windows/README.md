Windows
======================

## Setup ##

This project uses CMake (minimum version 3.10), you can download it [here](http://www.cmake.org/download/)

Make sure to update git submodules before you build:

```bash
git submodule update --init
```

To build on Windows you will need a Visual Studio 2019 and vcpkg(https://github.com/microsoft/vcpkg) installed.

The demo application uses the Nextzen vector tile service, so you will need a Nextzen API key to build and run the demo. 

 1. Visit https://developers.nextzen.org/ to get an API key.

 2. Setup an environment variable (`NEXTZEN_API_KEY`) to point to your API key.

## Build ##

There are two ways to build the tangram-es library and demo application on Windows:

 1. Open reporitory folder with Visual Studio in cmake settings specify path to vcpkg toolchain <path to vcpkg>/scripts/buildsystems/vcpkg.cmake
 
 2. Generate solution via command line CMake
     ```bash
     mkdir build and cd build
     cmake .. -DCMAKE_TOOLCHAIN_FILE="D:/dev/vcpkg/scripts/buildsystems/vcpkg.cmake"
     ```

