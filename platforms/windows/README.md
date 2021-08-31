Windows
======================

## Setup ##

This project uses CMake (minimum version 3.10), you can download it [here](http://www.cmake.org/download/)

Make sure to update git submodules before you build:

```bash
git submodule update --init
```

To build on Windows you will need Visual Studio 2019 and [vcpkg](https://github.com/microsoft/vcpkg) installed.

The demo application uses the Nextzen vector tile service, so you will need a Nextzen API key to build and run the demo. 

 1. Visit https://developers.nextzen.org/ to get an API key.

 2. Setup an environment variable (`NEXTZEN_API_KEY`) to point to your API key.

## Build ##

There are two ways to build the tangram-es library and demo application on Windows:

### Visual Studio IDE ###

Open the repository folder with Visual Studio. Visual Studio should recognize the CMakeLists.txt in the repository and begin importing the CMake project.

In the CMake settings (**Project** > **CMake Settings**) set the **CMake Toolchain File** to the vcpkg toolchain, located in your vcpkg installation: <path to vcpkg>/scripts/buildsystems/vcpkg.cmake
 
Now you can build and run the project within Visual Studio.

### Command Line ###

First, generate a Visual Studio solution using CMake on the command line:

```
mkdir build
cmake -S . -B .\build -G "Visual Studio 16 2019" -DCMAKE_TOOLCHAIN_FILE="<path to vcpkg>\scripts\buildsystems\vcpkg.cmake"
```

Then run the build using the CMake build option:

```
cmake --build .\build
```

## Run ##

After building the demo application, run it from the output folder:

```
cd .\build
.\Debug\tangram.exe
```

You can open a different YAML scene file by dragging and dropping it into the window, or passing it as an argument:

```
.\tangram.exe -f /path/to/your/scene.yaml
```
