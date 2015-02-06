#!/usr/bin/env bash

set -e
set -o pipefail

if [[ ${TRAVIS_OS_NAME} == "osx" ]]; then
    brew update >/dev/null
fi

if [[ ${PLATFORM} == "osx" ]]; then
    brew tap homebrew/versions
    brew install glfw3
fi

if [[ ${PLATFORM} == "android" ]]; then

    ANDROID_SDK_VERSION="r24.0.2"
    ANDROID_BUILD_TOOL_VERSION="21.1.2"
    ANDROID_PLATFORM_VERSION="19"

    # gcc-4.8 should be default in travis ci now (https://github.com/travis-ci/travis-cookbooks/pull/215), but for safety
    #sudo add-apt-repository ppa:ubuntu-toolchain-r/test -y
    #sudo apt-get update -qq
    # No linux specific build right now, for android builds, gcc bundled with ndk is used
    # if [ "$CXX" = "g++" ]; then sudo apt-get install -qq g++-4.8; fi
    # if [ "$CXX" = "g++" ]; then export CXX="g++-4.8" CC="gcc-4.8"; fi

    # install other required packages
    #sudo apt-get -y install git build-essential automake gdb libtool make cmake pkg-config 

    # not installing mesa and opengl right now, will need with linux builds though

    # install jdk, ant and 32bit dependencies for android sdk
    #sudo apt-get -qq -y install openjdk-7-jdk ant lib32z1-dev lib32stdc++6
    brew install ant

    # install android sdk
    wget https://dl-ssl.google.com/android/android-sdk_${ANDROID_SDK_VERSION}-macosx.zip
    tar -zxf android-sdk_${ANDROID_SDK_VERSION}-macosx.zip
    export ANDROID_HOME=$PWD/android-sdk-macosx
    
    # install android ndk
    git clone --quiet --depth 1 https://github.com/tangrams/mindk.git
    export ANDROID_NDK=$PWD/mindk/android-ndk-r10d

    # Update PATH
    export PATH=${PATH}:${ANDROID_HOME}/tools:${ANDROID_NDK}

    # Install required Android components.
    # automatically accept the license prompt
    echo "y" | android update sdk --filter platform-tools,build-tools-${ANDROID_BUILD_TOOL_VERSION},android-${ANDROID_PLATFORM_VERSION},extra-android-support --no-ui --force >/dev/null

    echo $PATH
fi
