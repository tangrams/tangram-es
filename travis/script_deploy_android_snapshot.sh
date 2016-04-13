#!/usr/bin/env bash

set -e
set -o pipefail

if [ "${PLATFORM}" = "android" ] && [ "${TRAVIS_PULL_REQUEST}" = "false" ] && [ "${TRAVIS_BRANCH}" = "master" ]; then

    # Configure private repository credentials (used to sign release artifacts)
    echo -e "machine github.com\n  login $GITHUB_USERNAME\n  password $GITHUB_PASSWORD" >> ~/.netrc

    # Build all android architectures (armeabi-v7a already build)
    make android-native-lib ANDROID_ARCH=x86
    make android-native-lib ANDROID_ARCH=armeabi
    make android-native-lib ANDROID_ARCH=arm64-v8a
    #### Currently Not building the following architectures
    #make android-native-lib ANDROID_ARCH=x86_64
    #make android-native-lib ANDROID_ARCH=mips
    #make android-native-lib ANDROID_ARCH=mips64

    make android-tangram-apk
    cd "$TRAVIS_BUILD_DIR"/android
    git clone https://github.com/mapzen/android-config.git
    ./gradlew uploadArchives -PsonatypeUsername="$SONATYPE_USERNAME" -PsonatypePassword="$SONATYPE_PASSWORD" -Psigning.keyId="$SIGNING_KEY_ID" -Psigning.password="$SIGNING_PASSWORD" -Psigning.secretKeyRingFile="$SIGNING_SECRET_KEY_RING_FILE"
    cd "$TRAVIS_BUILD_DIR"
fi
