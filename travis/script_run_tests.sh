#!/usr/bin/env bash

set -e
set -o pipefail

if [[ ${PLATFORM} == "osx" || ${PLATFORM} == "linux" ]]; then
    # Run unit tests
    echo "Running Unit Tests"
    pushd ./build/tests/unit/bin

    for file in *.out
        do
            echo "Running ${file}"
            ./$file
        done
    popd
fi
