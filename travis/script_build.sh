#!/usr/bin/env bash

set -e
set -o pipefail

if [[ ${PLATFORM} == "osx" ]]; then
    # Build osx project
    echo "Building osx project"
    make osx
fi

if [[ ${PLATFORM} == "ios" ]]; then
    # Build ios project
    echo "Building ios project (simulator)"
    make ios
fi

if [[ ${PLATFORM} == "android" ]]; then
    # Build android project
    echo "Building android project"
    make android
fi

