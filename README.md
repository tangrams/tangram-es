tangram-es
==========

[![Travis CI Build Status](https://travis-ci.org/tangrams/tangram-es.svg?branch=master)](https://travis-ci.org/tangrams/tangram-es/builds)

OpenGL ES version of Tangram for mobile devices - EARLY work-in-progress!

tangram-es is a library for rendering 2D and 3D maps using OpenGL ES 2 with custom styling and interactions. We maintain sample client applications that use the library to render on Android, iOS, Mac OS X, Ubuntu, and Rasberry Pi. 

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
To build for OS X, you will need to install [GLFW](http://www.glfw.org/) and [pkg-config](http://www.freedesktop.org/wiki/Software/pkg-config/): 

```bash
brew tap homebrew/versions
brew install glfw3 pkg-config
```

Then build using GNU Make:

```bash
make osx
```

### Ubuntu Linux ###
To build on Ubuntu you will again need [GLFW](http://www.glfw.org/) and on linux platforms it's best to compile from source. GLFW provides [instructions for compiling the library](http://www.glfw.org/docs/latest/compile.html). Once you've installed GLFW, build Tangram from the project root using GNU Make:

```bash
make linux
```

Then run the binary from the output folder:

```bash
cd build/linux/bin/ && ./tangram
```

### iOS Simulator ###
For running on the iOS simulator, generate and compile an XCode project:

```bash
make ios-sim
```

Then just open the Xcode project and run/debug from there: 

```bash
open build/ios/tangram.xcodeproject
```

Note that any Xcode configuration change you make to the project won't be preserved when Cmake runs again. Build configuration is defined only in the CMakeLists file(s).

### iOS Devices ###
For running on iOS devices you will need an iOS developer account, a valid code signing certificate, and a valid provisioning profile. Help on these topics can be found at [Apple's developer website](http://developer.apple.com). 

First generate an XCode project without compiling:

```bash
make cmake-ios
```

Then open the Xcode project and set up your developer account information to run on a device:

```bash
open build/ios/tangram.xcodeproj
```

If you run into problems deploying to an iOS device, see [this note](https://github.com/tangrams/tangram-es/wiki/iOS-Notes).

### Android ###
To build for Android you'll need to have installed both the [Android SDK](http://developer.android.com/sdk/installing/index.html?pkg=tools) and the [Android NDK](https://developer.android.com/tools/sdk/ndk/index.html). Set an `ANDROID_HOME` evironment variable with the root directory of your SDK and an `ANDROID_NDK` environment variable with the root directory of your NDK. 

Build an APK of the demo application and optionally specify an architecture (default is armeabi-v7a):

```bash
make android [ANDROID_ARCH=[x86|armeabi-v7a|armeabi]]
```

Then install to a connected device or emulator. You can (re)install and run the APK with a small script:

```bash
./android/run.sh
```

### Raspberry Pi ###

First, install cmake and libcurl:

```
sudo apt-get install cmake libcurl4-openssl-dev
```

To build the project, you will need to have C++11 compatible compiler installed, for example GNU g++-4.8 or greater:
```
sudo apt-get install g++-4.8
```

Before compiling, choose which compiler to use by running the following:
```
export CXX=/usr/bin/g++-4.8
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

