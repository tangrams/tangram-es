#!/usr/bin/env bash

set -e
set -o pipefail

if [[ ${TRAVIS_OS_NAME} == "osx" ]]; then
    # Run unit tests
    echo "Running Unit Tests"
    for file in ./build/tests/unit/bin/*
        do
            if [ ${file: -4} == ".out" ]
                then
                    echo "Running ${file}"
                    $file
            fi
        done
fi

