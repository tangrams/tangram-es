iOS
===

The recommended way to use tangram-es in an iOS project is to add it as a CocoaPods dependency. The library is hosted in CocoaPods under the pod name 'Tangram-es'. To find the latest version, check CocoaPods: https://cocoapods.org/pods/Tangram-es.

## Setup ##

This project uses CMake (minimum version 3.0), you can download it [here](http://www.cmake.org/download/) or use your favorite installation package tool like [homebrew](http://brew.sh/).

```bash
brew install cmake
```

Make sure to update git submodules before you build:

```bash
git submodule update --init
```

You can optionally install xcpretty to produce much prettier and more legible output during the build process:

```bash
gem install xcpretty
```

## Build ##

Building the iOS demo application requires Xcode 8.0 or newer. From the root directory of the project, run:

```bash
make ios
```

You can optionally append `DEBUG=1` or `RELEASE=1` to choose the build type.

This will generate an Xcode project that you can use to deploy on device or simulator:

```bash
open build/ios/tangram.xcodeproj
```

Make sure to set up the code signing identity and code sign the framework on copy (select target _tangram_ > _Build Phases_ > _Copy Files_ > _TangramMap.framework_ > _Code Sign On Copy_).

Note on Code Signing and Provisioning Profiles:
* For Simulator: Does not need any code signing identity, so you can ignore any provionining profile failures on target _tangram_ > _General_ > _Signing_.
* For Device: You will have to modify the _Bundle Identifier_ under target _tangram_ > _General_ > _Identity_ > _Bundle Identifier_, to something other than `com.mapzen.tangram`, since this needs to be unique.

For development, you can use the Makefile option `TANGRAM_IOS_FRAMEWORK_SLIM` to build for simulator only and faster your build times.

### iOS Binary Framework ###

An iOS binary framework bundle targeted for ARM architectures can be produced by running the following:

```bash
make ios-framework
```

The framework will be output in '/build/ios-framework/lib/', in a folder named 'release' or 'debug' according to the build type.

To build a universal binary working on both device and simulator architectures run the following:

```bash
make ios-framework-universal
```

The universal framework will be output in '/build/ios-framework-universal/', in a folder named 'release' or 'debug' according to the build type.
