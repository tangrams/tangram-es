#!/bin/sh
cd `dirname $0`

CMAKE_OPTIONS="-DCMAKE_INSTALL_PREFIX=build" RELEASE=1 make android-native-lib ANDROID_ARCH=x86 && \
CMAKE_OPTIONS="-DCMAKE_INSTALL_PREFIX=build" RELEASE=1 make android-native-lib ANDROID_ARCH=armeabi && \
CMAKE_OPTIONS="-DCMAKE_INSTALL_PREFIX=build" RELEASE=1 make android-native-lib ANDROID_ARCH=arm64-v8a && \
RELEASE=1 make android-tangram-apk && \
echo "Output is `dirname $0`/android/tangram/build/outputs/aar/tangram-release.aar"

#### Currently Not building the following architectures
#make android-native-lib ANDROID_ARCH=x86_64
#make android-native-lib ANDROID_ARCH=mips
#make android-native-lib ANDROID_ARCH=mips64

