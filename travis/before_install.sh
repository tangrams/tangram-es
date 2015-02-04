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

if [[ ${PLATFORM} == "linux" ]]; then
    
    GLFW_VERSION="3.0.4"

    sudo apt-get update -qq
    
    #Install X11 and OpenGL for GLFW
    sudo apt-get install -y -qq xorg-dev libglu1-mesa-dev

    # Download and install GLFW from source
    wget http://downloads.sourceforge.net/project/glfw/glfw/${GLFW_VERSION}/glfw-${GLFW_VERSION}.zip
    unzip glfw-${GLFW_VERSION}.zip
    cd glfw-${GLFW_VERSION}
    cmake .
    sudo make install
    cd ../
fi

if [[ ${PLATFORM} == "android" ]]; then

    ANDROID_SDK_VERSION="r24.0.2"
    ANDROID_NDK_VERSION="r10d"
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
    # only binary link avaiable for r10d, no download links available for r10b
    ANDROID_NDK_NAME="android-ndk-${ANDROID_NDK_VERSION}-darwin-x86_64.bin"
    wget http://dl.google.com/android/ndk/${ANDROID_NDK_NAME}
    chmod a+x ${ANDROID_NDK_NAME}
    ./android-ndk-r10d-darwin-x86_64.bin | egrep -v ^Extracting
    export ANDROID_NDK=$PWD/android-ndk-${ANDROID_NDK_VERSION}

    # Update PATH
    export PATH=${PATH}:${ANDROID_HOME}/tools:${ANDROID_NDK}

    # Install required Android components.
    # automatically accept the license prompt
    echo "y" | android update sdk --filter platform-tools,build-tools-${ANDROID_BUILD_TOOL_VERSION},android-${ANDROID_PLATFORM_VERSION},extra-android-support --no-ui --force

    echo $PATH
fi
