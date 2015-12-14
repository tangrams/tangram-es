#!/usr/bin/env bash

set -e
set -o pipefail

if [ "${PLATFORM}" = "android" ] && [ "${TRAVIS_PULL_REQUEST}" = "false" ] && [ "${TRAVIS_BRANCH}" = "mult-android-arch" ]; then
    
    # Build all android architectures (armeabi-v7a already build)
    make android-native-lib ANDROID_ARCH=x86
    make android-native-lib ANDROID_ARCH=x86_64
    make android-native-lib ANDROID_ARCH=armeabi
    make android-native-lib ANDROID_ARCH=arm64-v8a
    make android-native-lib ANDROID_ARCH=mips
    make android-native-lib ANDROID_ARCH=mips64
    make android-tangram-apk
    cd "$TRAVIS_BUILD_DIR"/android; ./gradlew uploadArchives -PsonatypeUsername="$SONATYPE_USERNAME" -PsonatypePassword="$SONATYPE_PASSWORD"
    cd "$TRAVIS_BUILD_DIR"
fi
