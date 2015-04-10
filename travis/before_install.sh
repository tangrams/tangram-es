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
    
    GLFW_VERSION="3.1.1"
    
    #Add PPA for gcc-4.8
    sudo add-apt-repository -y ppa:ubuntu-toolchain-r/test > /dev/null
    #Add PPA for CMake 2.8.11
    sudo add-apt-repository -y ppa:kalakris/cmake > /dev/null
    sudo apt-get update -qq
    
    #Use a c++11 compatible compiler
    export CXX=clang++-3.4
    export CC=clang-3.4
    
    #Install X11, OpenGL, Doxygen, and CMake for GLFW
    sudo apt-get install -y -qq xorg-dev libglu1-mesa-dev doxygen cmake

    # Download and install GLFW from source
    wget https://github.com/glfw/glfw/releases/download/${GLFW_VERSION}/glfw-${GLFW_VERSION}.zip
    unzip -qq glfw-${GLFW_VERSION}.zip
    cd glfw-${GLFW_VERSION}
    cmake .
    sudo make install
    cd ../
    
fi

if [[ ${PLATFORM} == "android" ]]; then

    ANDROID_SDK_VERSION="r24.0.2"
    ANDROID_BUILD_TOOL_VERSION="21.1.2"
    ANDROID_PLATFORM_VERSION="19"

    # Install ant
    brew install ant

    # Install android sdk
    wget https://dl-ssl.google.com/android/android-sdk_${ANDROID_SDK_VERSION}-macosx.zip
    tar -zxf android-sdk_${ANDROID_SDK_VERSION}-macosx.zip
    export ANDROID_HOME=$PWD/android-sdk-macosx

    # Install android ndk
    echo "Cloning mindk..."
    git clone --quiet --depth 1 https://github.com/tangrams/mindk.git
    export ANDROID_NDK=$PWD/mindk/android-ndk-r10d
    echo "Done."

    # Update PATH
    echo "Updating PATH..."
    export PATH=${PATH}:${ANDROID_HOME}/tools:${ANDROID_NDK}
    echo $PATH
    echo "Done."

    # Install required Android components; automatically accept the license prompt
    echo "Updating Android SDK..."
    echo "y" | android update sdk --all --filter platform-tools,build-tools-${ANDROID_BUILD_TOOL_VERSION},android-${ANDROID_PLATFORM_VERSION},extra-android-support --no-ui --force >/dev/null
    echo "Done."
fi
