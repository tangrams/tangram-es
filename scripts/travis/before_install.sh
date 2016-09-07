#!/usr/bin/env bash

set -e
set -o pipefail

if [[ ${PLATFORM} == "android" ]]; then

    # Note: the right way to download these packages is through the Android Studio SDK manager,
    # these steps should be removed when/if ndk-bundle and cmake become available from the
    # command-line SDK update tool.

    # Download android ndk
    echo "Downloading ndk..."
    curl -L https://dl.google.com/android/repository/android-ndk-r13b-linux-x86_64.zip -o ndk.zip
    echo "Done."

    # Extract android ndk
    echo "Extracting ndk..."
    unzip -qq ndk.zip
    echo "Done."

    # Update PATH
    echo "Updating PATH..."
    export ANDROID_NDK_HOME=${PWD}/android-ndk-r13b
    export PATH=${PATH}:${ANDROID_HOME}/tools:${ANDROID_NDK_HOME}
    echo $PATH
    echo "Done."

    # Download android cmake package
    echo "Downloading android cmake package..."
    curl -L https://dl.google.com/android/repository/cmake-3.6.3155560-linux-x86_64.zip -o cmake.zip
    echo "Done."

    # Extract android cmake package
    echo "Extracting android cmake package..."
    mkdir -p ${ANDROID_HOME}/cmake
    unzip -qq cmake.zip -d ${ANDROID_HOME}/cmake
    echo "Done."

fi

if [[ ${TRAVIS_OS_NAME} == "osx" ]]; then

    # https://github.com/travis-ci/travis-ci/issues/6307
    rvm get head

fi
