#!/usr/bin/env bash

set -e
set -o pipefail

if [[ ${PLATFORM} == "osx" || ${PLATFORM} == "linux" ]]; then
    # Run unit tests
    echo "Running Unit Tests"
    pushd ./build/${PLATFORM}/bin

    for file in unit/*.out
        do
            echo "Running ${file}"
            $file
        done
    popd
fi
