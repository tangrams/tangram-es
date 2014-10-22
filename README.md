tangram-es
==========

OpenGL ES version of Tangram for mobile devices - EARLY work-in-progress!

tangram-es is a library for rendering 2D and 3D maps using OpenGL ES 2 with custom styling and interactions. We also maintain sample client applications that use the library to render on Android, iOS, and Mac OS X. 

build
=====
This project uses _CMake_ (minimum version **2.8**), you can download it [here](http://www.cmake.org/download/) or use your favorite installation package tool like [homebrew](http://brew.sh/).

```bash
brew install cmake
```

Currently we are targetting three platforms (OS X, iOS and Android). Once CMake installed, you can build the project for the platform of your choice. If you are planning to test more than one platform we advise you to create subfolders inside the **build/** folder and run `cmake ../.. [OPTIONS]` from each of them.

## platforms ##

### OS X ###
To build for OS X, you will need to install [GLFW](http://www.glfw.org/): 

```bash
brew tap homebrew/versions
brew install glfw3
```

Then build using GNU Make by calling these commands from `build` folder:

```bash
cmake .. -DPLATFORM_TARGET=darwin
make
bin/tangram.out
```

### iOS ###
For iOS, you can generate an XCode project by running the following from the `build` folder:

```bash
cmake .. -DPLATFORM_TARGET=ios -DIOS_PLATFORM=SIMULATOR -DCMAKE_TOOLCHAIN_FILE=toolchains/iOS.toolchain.cmake -G Xcode
open tangram.xcodeproject
```

Then just use Xcode as usual. Note that any Xcode configuration change you make to the project won't be preserved when Cmake runs again. Build configuration is defined only in the CMakeLists file(s).

### Android ###
To build for Android, ensure you have your `$NDK_ROOT` environment variable set and pointing to your [NDK](https://developer.android.com/tools/sdk/ndk/index.html) toolset. 

```bash
cmake .. -DPLATFORM_TARGET=android -DCMAKE_TOOLCHAIN_FILE=toolchains/android.toolchain.cmake -DMAKE_BUILD_TOOL=$NDK_ROOT/prebuilt/[YOUR_OS]/bin/make [-DANDROID_ABI=[x86|armeabi-v7a|armeabi]]
make
make install
cd .. 
ant -f android/build.xml debug
adb install [GENERATED_APK]
```
