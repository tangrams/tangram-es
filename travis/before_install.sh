#!/usr/bin/env bash

set -e
set -o pipefail

if [[ ${PLATFORM} == "android" ]]; then

    # Install android ndk
    echo "Cloning mindk..."
    git clone --quiet --depth 1 --branch linux-x86_64-api15-armv7 https://github.com/tangrams/mindk.git
    export ANDROID_NDK=$PWD/mindk/android-ndk-r10d
    echo "Done."

    # Update PATH
    echo "Updating PATH..."
    export PATH=${PATH}:${ANDROID_HOME}/tools:${ANDROID_NDK}
    echo $PATH
    echo "Done."

fi
