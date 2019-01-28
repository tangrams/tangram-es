Tangram iOS SDK
===============

The recommended way to use Tangram in an iOS project is to add it as a CocoaPods dependency. The library is hosted in CocoaPods under the pod name _Tangram-es_. To find the latest version, check CocoaPods: https://cocoapods.org/pods/Tangram-es. Then follow the instructions from CocoaPods on adding a pod to your Xcode project: https://guides.cocoapods.org/using/using-cocoapods.html.

That's it! If you want to build Tangram for iOS from source instead, continue reading.

## Setup ##

This project uses CMake (minimum version 3.2), you can download it [here](http://www.cmake.org/download/) or use your favorite installation package tool like [homebrew](http://brew.sh/).

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

You will need a Nextzen API key to use the vector-tile and terrain service that is being used from the stylesheet of the demo app.
To get an API key visit: [developers.nextzen.org](https://developers.nextzen.org/).

Building the iOS demo application requires Xcode 9.0 or newer. From the root directory of the project, run:

```bash
make ios-xcode NEXTZEN_API_KEY=yourApiKeyHere
```

You can optionally append a `BUILD_TYPE` variable to choose the build type, for example `BUILD_TYPE=Debug` or `BUILD_TYPE=Release`.

This will generate an Xcode project for the Tangram iOS framework and demo application and open the project in an Xcode
workspace. 

Note on Code Signing and Provisioning Profiles:
* For Simulator: Code signing is not required, so you can ignore any signing errors in target _TangramDemo_ > _General_ > _Signing_.
* For Device: You will have to modify the _Bundle Identifier_ in target _TangramDemo_ > _General_ > _Identity_ > _Bundle Identifier_ to something other than `com.mapzen.ios.TangramDemo`, since App IDs are associated with a specific signing identity.

### iOS Binary Framework ###

To build an iOS binary framework bundle compiled for iOS devices, run:

```bash
make ios-framework
```

The framework will be output in '/build/ios/CONFIG-iphoneos/', where CONFIG is 'Release' or 'Debug' according to the build type.

To build a universal binary framework compiled for both iOS devices and the iOS simulator, run:

```bash
make ios-framework-universal
```

The framework will be output in '/build/ios/CONFIG-universal/', where CONFIG is 'Release' or 'Debug' according to the build type.
