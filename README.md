tangram-es
==========

[![Travis CI Build Status](https://travis-ci.org/tangrams/tangram-es.svg?branch=master)](https://travis-ci.org/tangrams/tangram-es/builds)

![screenshot](images/screenshot.png)

tangram-es is a C++ library for rendering 2D and 3D maps from vector data using OpenGL ES, it is a counterpart to [tangram](https://github.com/tangrams/tangram) focused on mobile and embedded devices.

This repository contains both the core rendering library and sample applications that use the library on Android, iOS, Mac OS X, Ubuntu, and Raspberry Pi. 

*tangram-es is in active development and is not yet feature-complete*

build
=====
This project uses _CMake_ (minimum version **3.0**), you can download it [here](http://www.cmake.org/download/) or use your favorite installation package tool like [homebrew](http://brew.sh/).

```bash
brew install cmake
```

Make sure to update git submodules before you build:

```bash
git submodule init && git submodule update
```

Currently we are targeting five platforms: OS X, Ubuntu Linux, iOS, Android, and Raspberry Pi. 

## platforms ##

### OS X ###
To build a runnable OS X application bundle, run:

```bash
make osx
```
And open the application with:

```bash
open build/osx/bin/tangram.app
```

### Ubuntu Linux ###
To build on Ubuntu you will need to install a libcurl development package:

```bash
sudo apt-get install libcurl4-openssl-dev
```

Then build an executable:

```bash
make linux
```

And run it from the output folder:

```bash
cd build/linux/bin/ && ./tangram
```

### iOS Simulator ###
For running on the iOS simulator, generate and compile an Xcode project:

```bash
make ios-sim
```

Then just open the Xcode project and run/debug from there: 

```bash
open build/ios-sim/tangram.xcodeproj
```

Note that any Xcode configuration change you make to the project won't be preserved when CMake runs again. Build configuration is defined only in the CMakeLists file(s).

### iOS Devices ###
For running on iOS devices you will need an iOS developer account, a valid code signing certificate, and a valid provisioning profile. Help on these topics can be found at [Apple's developer website](http://developer.apple.com). 

First generate an Xcode project without compiling:

```bash
make cmake-ios
```

Then open the Xcode project and set up your developer account information to run on a device:

```bash
open build/ios/tangram.xcodeproj
```

If you run into problems deploying to an iOS device, see [this note](https://github.com/tangrams/tangram-es/wiki/iOS-Notes).

### Android ###
To build for Android you'll need to have installed both the [Android SDK](http://developer.android.com/sdk/installing/index.html?pkg=tools) and the [Android NDK](https://developer.android.com/tools/sdk/ndk/index.html). Set an `ANDROID_HOME` environment variable with the root directory of your SDK and an `ANDROID_NDK` environment variable with the root directory of your NDK. 

Build an APK of the demo application and optionally specify an architecture (default is armeabi-v7a):

```bash
make android [ANDROID_ARCH=[x86|armeabi-v7a|armeabi]]
```

Then install to a connected device or emulator. You can (re)install and run the APK with a small script:

```bash
./android/run.sh
```

### Raspberry Pi ###

First, install CMake and libcurl:

```
sudo apt-get install cmake libcurl4-openssl-dev
```

To build the project, you will need to have C++11 compatible compiler installed, for example GNU g++-4.9 or greater (refer [here](https://solarianprogrammer.com/2015/01/13/raspberry-pi-raspbian-install-gcc-compile-cpp-14-programs/) for instructions on getting g++-4.9)

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

code-styling
=====
Tangram is using clang-format to style the code. 
When submitting a PR, make sure the code conforms to the styling rules defined in the clang style file.

Install clang-format (available through brew or apt-get)
```
brew install clang-format
```  
or
```
sudo apt-get install clang-format
```

Running clang-format with specified style (use -i to modify the contents of the specified file):
```
clang-format -i -style=file [file] 
```

