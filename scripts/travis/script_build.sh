#!/usr/bin/env bash

set -e
set -o pipefail

if [[ ${PLATFORM} == "osx" ]]; then
    # Build osx project
    echo "Building osx project"
    CMAKE_OPTIONS="-DUNIT_TESTS=1 -DBENCHMARK=1" make -j osx
fi

if [[ ${PLATFORM} == "linux" ]]; then
    # Build linux project
    echo "Building linux project"
    CMAKE_OPTIONS="-DUNIT_TESTS=1 -DBENCHMARK=1" make -j 4 linux
fi

if [[ ${PLATFORM} == "ios" ]]; then
    # Build ios project
    echo "Building ios framework for simulator"
    make ios-framework-sim
fi

if [[ ${PLATFORM} == "android" ]]; then
    # Build android project
    echo "Building android project"
    export TERM=dumb
    make android
fi

