#!/usr/bin/env bash

set -e
set -o pipefail

if [[ ${PLATFORM} == "android" ]]; then

    # Install android ndk
    echo "Cloning mindk..."
    git clone --quiet --depth 1 --branch deploy-all-arch https://github.com/tangrams/mindk.git
    export ANDROID_NDK=$PWD/mindk/android-ndk-r10e
    echo "Done."

    # Update PATH
    echo "Updating PATH..."
    export PATH=${PATH}:${ANDROID_HOME}/tools:${ANDROID_NDK}
    echo $PATH
    echo "Done."

fi

if [[ ${TRAVIS_OS_NAME} == "osx" ]]; then

    # https://github.com/travis-ci/travis-ci/issues/6307
    rvm get head

fi
