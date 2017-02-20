tangram-es
==========

tangram-es is a C++ library for rendering 2D and 3D maps from vector data using OpenGL ES, it is a counterpart to [tangram](https://github.com/tangrams/tangram) focused on mobile and embedded devices.

This repository contains both the core rendering library and sample applications that use the library on Android, iOS, Mac OS X, Ubuntu, and Raspberry Pi.

[![Gitter](https://badges.gitter.im/tangrams/tangram-chat.svg)](https://gitter.im/tangrams/tangram-chat?utm_source=share-link&utm_medium=link&utm_campaign=share-link)

| Platform                                | Build status                       |
| --------------------------------------- | ---------------------------------- |
| Linux/Android                           | [![Travis CI BuildStatus](https://travis-ci.org/tangrams/tangram-es.svg?branch=master)](https://travis-ci.org/tangrams/tangram-es/builds) |
| iOS | [![CircleCI](https://circleci.com/gh/tangrams/tangram-es.svg?style=shield&circle-token=741ff7f06a008b6eb491680c2d47968a7c4eaa3a)](https://circleci.com/gh/tangrams/tangram-es) |

![screenshot](images/screenshot.png)

*tangram-es is in active development and is not yet feature-complete*

build
=====
This project uses _CMake_ (minimum version **3.0**), you can download it [here](http://www.cmake.org/download/) or use your favorite installation package tool like [homebrew](http://brew.sh/).

```bash
brew install cmake
```

Make sure to update git submodules before you build:

```bash
git submodule update --init
```

Currently we are targeting five platforms: OS X, Ubuntu Linux, iOS, Android, and Raspberry Pi.

## platforms ##

### OS X (command line) ###
To build a runnable OS X application bundle, run:

```bash
make osx
```
And open the application with:

```bash
open build/osx/bin/tangram.app
```

### OS X (Xcode) ###
For running on OS X from Xcode you will need Xcode version **6.0** or higher. Generate and compile an Xcode project:

```bash
make xcode
```

Then just open the Xcode project and run/debug from there:

```bash
open build/xcode/tangram.xcodeproj
```

Note that any Xcode configuration change you make to the project won't be preserved when CMake runs again. Build configuration is defined only in the CMakeLists file(s).

### CLion (OS X & Ubuntu Linux) ###
You can easily run and debug from CLion if you prefer.

After cloning and updating your git submodules, open CLion and __Import Project from Sources__. Select the root of this repo. Choose __Open Project__. Do not overwrite CMakeLists.txt.

CLion will automatically set everything up, all you have to do is wait a minute for the project to get initialized. Then, select the __tangram__ target and hit run / debug.

![CLion Tangram Target](images/clion-tangram-target.png)

### Ubuntu or Debian Linux ###
To build on Ubuntu or Debian you will need a C++ toolchain with support for C++14. GCC 5 (or higher) and Clang 3.4 (or higher) are known to work.

You will also need to install development packages for libcurl, x11, and opengl. On Ubuntu 16.04 or Debian Stretch all the required packages can be installed with

```bash
sudo apt-get install make g++ pkg-config libcurl4-openssl-dev \
  libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libgl1-mesa-dev
```

Then build an executable:

```bash
make linux
```

And run it from the output folder:

```bash
cd build/linux/bin/ && ./tangram
```

Note that any Xcode configuration change you make to the project won't be preserved when CMake runs again. Build configuration is defined only in the CMakeLists file(s).

Tangram ES can also be build with system font and GLFW libraries. This can be done with

```bash
CMAKE_OPTIONS=" -DUSE_SYSTEM_GLFW_LIBS=1 -DUSE_SYSTEM_FONT_LIBS=1" make linux
```

On Ubuntu 16.04 or Debian Stretch the additional packages required can be installed with

```bash
sudo apt-get install libglfw3-dev libicu-dev libfreetype6-dev libharfbuzz-dev
```

### iOS (demo application) ###

Building the iOS demo application requires Xcode 8.0 or newer. First, run:

```bash
make ios
```

This will generate an Xcode project that you can use to deploy on device or simulator:

```bash
open build/ios/tangram.xcodeproj
```

Make sure to set up the code signing identity and code sign the framework on copy (select target _tangram_ > _Build Phases_ > _Copy Files_ > _TangramMap.framework_ > _Code Sign On Copy_).

Note on Code Signing and Provisioning Profiles:
* For Simulator: Does not need any code signing identity, so you can ignore any provionining profile failures on target _tangram_ > _General_ > _Signing_.
* For Device: You will have to modify the _Bundle Identifier_ under target _tangram_ > _General_ > _Identity_ > _Bundle Identifier_, to something other than `com.mapzen.tangram`, since this needs to be unique.


### iOS Binary Framework ###

An iOS binary framework bundle targeted for ARM architectures can be produced by running the following:

```bash
make ios-framework [RELEASE=1|DEBUG=1]
```

The framework will be available in the configuration build type in `/build/ios-framework/lib/`.

To build a universal binary working on both device and simulator architectures run the following:

```bash
make ios-framework-universal [RELEASE=1|DEBUG=1]
```

The universal framework will be available in the configuration build type in `/build/ios-framework-universal/`.

### Android ###
To build for Android you'll need to use [Android Studio](https://developer.android.com/studio/index.html) version **2.2** or newer on Mac OS X, Ubuntu, or Windows 10. Using the Android Studio SDK Manager, install or update the 'CMake', 'LLDB', and 'NDK' packages from the 'SDK Tools' tab. Once dependencies are installed, you can execute Android builds from either the command line or the Android Studio interface.

To build the demo application for the ARMv7 architecture (covers most Android devices), run:

```bash
make android
```

Or open the project in Android Studio and press the 'Run' button (^R). More options are provided through Gradle.

The Gradle project in the `android/` directory contains two modules: a library module called `tangram` containing the Tangram Android SDK and an application module called `demo`, containing a demo application that uses the `tangram` module. The `tangram` module has two `buildTypes`, `debug` and `release`, and two `productFlavors`, `slim` and `full`. The `slim` flavor includes native libraries for just the ARMv7 architecture, the `full` flavor includes all supported architectures (ARMv6, ARMv7, ARM64, and x86).

To build the library or demo application from the `android/` folder using Gradle, use the conventional syntax, e.g.:

```bash
./gradlew tangram:assembleFullRelease
```

Android Studio supports debugging both the Java and C++ parts of tangram-es on a connected device or emulator. Choose one of the 'debug' build variants, set your desired breakpoints, and press the 'Debug' button (^D).

### Raspberry Pi ###
To build on Rasberry Pi you will need a C++ toolchain with support for C++14. GCC 4.9 (or higher) is known to work (refer [here](https://community.thinger.io/t/starting-with-the-raspberry-pi/36) for instructions on getting GCC 4.9).

First, install CMake and libcurl:

```
sudo apt-get install cmake libcurl4-openssl-dev
```

Before compiling, choose which compiler to use:
```
export CXX=/usr/bin/g++-4.9
```

Then compile and run:

```
make rpi
cd build/rpi/bin
./tangram
```

Tangram will be rendered directly to the screen without a window manager, if you want see the mouse cursor run the application with the ```-m``` argument like this:

```
cd build/rpi/bin
./tangram -m
```

You can also move the map with `w`, `a`, `s`, and `z`, zoom in and out with `-` and `=`, and quit with `q`.

## debug ##

To build in `RELEASE` or `DEBUG` run the following:

```sh
make [platform] DEBUG=1
```
or
```sh
make [platform] RELEASE=1
```

Code Style
==========
In general, code changes should follow the style of the surrounding code.

When in doubt, you can use the provided clang-format style file for automatic styling.

Install clang-format (available through brew or apt-get):
```
brew install clang-format
```
or
```
sudo apt-get install clang-format
```

Run clang-format with specified style (use -i to modify the contents of the specified file):
```
clang-format -i -style=file [file]
```
