tangram-es
==========

OpenGL ES version of Tangram for mobile devices - EARLY work-in-progress!

tangram-es is a library for rendering 2D and 3D maps using OpenGL ES 2 with custom styling and interactions. We also maintain sample client applications that use the library to render on Android, iOS, and Mac OS X. 

build
=====
This project uses _CMake_ (minimum version **3.0**), you can download it [here](http://www.cmake.org/download/) or use your favorite installation package tool like [homebrew](http://brew.sh/).

```bash
brew install cmake
```

Currently we are targetting three platforms (OS X, iOS and Android). Once CMake installed, you can build the project for the platform of your choice. 

## platforms ##

### OS X ###
To build for OS X, you will need to install [GLFW](http://www.glfw.org/): 

```bash
brew tap homebrew/versions
brew install glfw3
```

Then build using GNU Make:

```bash
make osx
```

### iOS ###
For iOS, an XCode project would be generated and target would be built by running the following:

```bash
make ios && open build/ios/tangram.xcodeproject
```

Then just use Xcode as usual. Note that any Xcode configuration change you make to the project won't be preserved when Cmake runs again. Build configuration is defined only in the CMakeLists file(s).

### Android ###
To build for Android, ensure you have your `$ANDROID_NDK` environment variable set and pointing to your [NDK](https://developer.android.com/tools/sdk/ndk/index.html) toolset. 

```bash
make android [ANDROID_ARCH=[x86|armeabi-v7a|armeabi]]
```

### Raspberry Pi ###

* Install C++11 compatible compiler:

```
sudo apt-get install g++-4.7
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-4.6 60 --slave /usr/bin/g++ g++ /usr/bin/g++-4.6 
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-4.7 40 --slave /usr/bin/g++ g++ /usr/bin/g++-4.7 
sudo update-alternatives --config gcc
```

Then choose 4.7 version.

* Resolve dependence width CMake-3.X: right now there are two ways of resolving this: Downloading, compiling and installing[cmake-3.1.X](http://www.cmake.org/download/) or changing the ```CMakeLists.txt``` file to look for an older version: 

```
vim CMakeLists.txt
```

Then change the first line from ```VERSION 3.0``` to ```VERSION 2.8```. Should look like this:

```
cmake_minimum_required(VERSION 2.8)
…
…
```

* Install curl:

```
sudo apt-get install libcurl4-openssl-dev
```

 