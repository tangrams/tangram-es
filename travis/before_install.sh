#!/usr/bin/env bash

set -e
set -o pipefail

if [[ ${PLATFORM} == "android" ]]; then

    # Install android ndk
    echo "Cloning android-ndk"
    git clone --quiet --depth 1 https://github.com/urho3d/android-ndk.git $HOME/android-ndk-root
    export ANDROID_NDK=$HOME/android-ndk-root
    echo "Done."

    # Update PATH
    echo "Updating PATH..."
    export PATH=${PATH}:${ANDROID_HOME}/tools:${ANDROID_NDK}
    echo $PATH
    echo "Done."

fi
