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

Currently we are targetting three platforms (OS X, iOS and Android). Once CMake installed, you can build the project for the platform of your choice. If you are planning to test each of these platform we advise you to create subfolders inside the **build/** and run `cmake ../.. [DEPENDENT-PLATFORM PARAMS]` from each of them.

## dependent-platform build ##

### OS X ###
To build for OS X, you will need to run these commands:
```bash
cmake .. -DPLATFORM_TARGET=darwin
make
bin/tangram.out
```
To build Xcode project for OS X, you will need to run the following:
```bash
cmake .. -DPLATFORM_TARGET=darwin -G Xcode
open tangram.xcodeproj
```
### iOS ###
For iOS, an Xcode project will be generated:
```bash
cmake .. -DPLATFORM_TARGET=ios -DIOS_PLATFORM=SIMULATOR -DCMAKE_TOOLCHAIN_FILE=toolchains/iOS.toolchain.cmake -G Xcode
open tangram.xcodeproject
```
Then just use Xcode as usual. Notice that every Xcode configuration modification you could make on the project won't be sustainable once you run the CMake command a second time.

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
