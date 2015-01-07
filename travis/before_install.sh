#!/usr/bin/env bash

set -e
set -o pipefail

if [[ ${TRAVIS_OS_NAME} == "linux" ]]; then
    ANDROID_SDK_VERSION="r24.0.2"
    ANDROID_NDK_VERSION="r10d"
    ANDROID_BUILD_TOOL_VERSION="21.1.2"
    ANDROID_PLATFORM_VERSION="19"
fi

if [[ ${TRAVIS_OS_NAME} == "osx" ]]; then
    brew update
    brew tap homebrew/versions
    brew install glfw3
fi


if [[ ${TRAVIS_OS_NAME} == "linux" ]]; then
    # gcc-4.8 should be default in travis ci now (https://github.com/travis-ci/travis-cookbooks/pull/215), but for safety
    sudo add-apt-repository ppa:ubuntu-toolchain-r/test -y
    sudo apt-get update -qq
    # No linux specific build right now, for android builds, gcc bundled with ndk is used
    # if [ "$CXX" = "g++" ]; then sudo apt-get install -qq g++-4.8; fi
    # if [ "$CXX" = "g++" ]; then export CXX="g++-4.8" CC="gcc-4.8"; fi

    # install other required packages
    sudo apt-get -y install git build-essential automake gdb libtool make cmake pkg-config 

    # not installing mesa and opengl right now, will need with linux builds though

    # install jdk
    sudo apt-get -qq -y install openjdk-7-jdk ant lib32z1-dev

    # install android sdk
    wget https://dl-ssl.google.com/android/android-sdk_${ANDROID_SDK_VERSION}-linux.tgz
    tar -zxf android-sdk_${ANDROID_SDK_VERSION}-linux.tgz
    export ANDROID_HOME=$PWD/android-sdk-linux
    # install android ndk
    # only binary link avaiable for r10d, no download links available for r10b
    wget http://dl.google.com/android/ndk/android-ndk-r10d-linux-x86_64.bin
    chmod +x android-ndk-r10d-linux-x86_64.bin
    ./android-ndk-r10d-linux-x86_64.bin | egrep -v ^Extracting
    export ANDROID_NDK=$PWD/android-ndk-${ANDROID_NDK_VERSION}

    # Update PATH
    export PATH=${PATH}:${ANDROID_HOME}/tools:${ANDROID_HOME}/platform-tools:${ANDROID_NDK}

    # Install required Android components.
    # automatically accept the license prompt
    echo "y" | android update sdk --filter platform-tools,build-tools-${ANDROID_BUILD_TOOL_VERSION},android-${ANDROID_PLATFORM_VERSION},extra-android-support --no-ui --force

    echo $PATH
fi
