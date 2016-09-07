#!/usr/bin/env bash

set -e
set -o pipefail

if [[ ${PLATFORM} == "android" ]]; then

    # Install android ndk
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

fi

if [[ ${TRAVIS_OS_NAME} == "osx" ]]; then

    # https://github.com/travis-ci/travis-ci/issues/6307
    rvm get head

fi
