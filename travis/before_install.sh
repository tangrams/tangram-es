#!/usr/bin/env bash

set -e
set -o pipefail

if [[ ${TRAVIS_OS_NAME} == "linux" ]]; then
    ANDROID_SDK_VERSION="r24.0.2"
    ANDROID_NDK_VERSION="r10b"
fi

if [[ ${TRAVIS_OS_NAME} == "osx" ]]; then
    brew update
    brew tap homebrew/versions
    brew install glfw3
fi


if [[ ${TRAVIS_OS_NAME} == "linux" ]]; then
    sudo apt-get update -y
    # gcc-4.8 should be default in travis ci now (https://github.com/travis-ci/travis-cookbooks/pull/215), but for safety
    sudo add-apt-repository ppa:ubuntu-toolchain-r/test -y
    sudo apt-get update -qq
    if [ "$CXX" = "g++" ]; then sudo apt-get install -qq g++-4.8; fi
    if [ "$CXX" = "g++" ]; then export CXX="g++-4.8" CC="gcc-4.8"; fi
    # install other required packages
    sudo apt-get -y install git build-essential automake gdb libtool xutils-dev make cmake pkg-config libcurl4-openssl-dev
    # not installing mesa and opengl right now

    # install jdk
    sudo apt-get -qq -y install openjdk-8-jdk ant lib32z1-dev lib32stdc++6

    # install android sdk
    wget https://dl-ssl.google.com/android/android-sdk_${ANDROID_SDK_VERSION}-linux.tgz
    tar -zxf android-sdk_${ANDROID_SDK_VERSION}-linux.tgz
    export ANDROID_HOME=$PWD/android-sdk-linux
    # Install required Android components.
    android update sdk --filter platform-tools,android-19,extra-android-support --no-ui --force

    # install android ndk
    wget http://dl.google.com/android/ndk/android-ndk-${ANDROID_NDK_VERSION}-linux-x86_64.tar.bz2
    tar -zxf android-ndk-${ANDROID_NDK_VERSION}-linux-x86_64.tar.bz2
    export ANDROID_NDK=$PWD/android-ndk-${ANDROID_NDK_VERSION}

    export PATH=${PATH}:${ANDROID_HOME}/tools:${ANDROID_HOME}/platform-tools:${ANDROID_NDK}
    echo $PATH
fi
