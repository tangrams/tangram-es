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
    # install gcc-4.8 for c++11 features
    sudo apt-get install python-software-properties
    sudo add-apt-repository ppa:ubuntu-toolchain-r/test
    sudo apt-get update
    sudo apt-get install gcc-4.8
    sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-4.8 50
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
