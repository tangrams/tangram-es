Raspberry Pi
============

## Setup ##

This project uses CMake (minimum version 3.0), you can download it [here](http://www.cmake.org/download/) or install it with apt-get.

```bash
sudo apt-get install cmake
```

Make sure to update git submodules before you build:

```bash
git submodule update --init
```

To build on Rasberry Pi you will need a C++ toolchain with support for C++14. GCC 4.9.2 (or higher) and Clang 3.4 (or higher) are known to work (refer [here](https://community.thinger.io/t/starting-with-the-raspberry-pi/36) for instructions on getting GCC 4.9).

You will also need to install development packages for libcurl:

```
sudo apt-get install libcurl4-openssl-dev
```

## Build ##

Before compiling, choose which compiler to use:
```
export CXX=/usr/bin/g++-4.9
```

Then compile:

```
make rpi
```

You can optionally use `make -j` to parallelize the build and append `DEBUG=1` or `RELEASE=1` to choose the build type.

Run the demo program from the output folder:
```
cd build/rpi/bin
./tangram
```

Tangram will be rendered directly to the screen without a window manager. To show a mouse cursor, run with `-m`:

```
cd build/rpi/bin
./tangram -m
```

You can also move the map with `w`, `a`, `s`, and `z`, zoom in and out with `-` and `=`, and quit with `q`.
