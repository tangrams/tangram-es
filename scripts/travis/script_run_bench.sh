#!/usr/bin/env bash

set -e
set -o pipefail

if [[ ${PLATFORM} == "osx" || ${PLATFORM} == "linux" ]]; then
    # Run unit tests
    echo "Running Benchmarks"

    pushd ./build/${PLATFORM}/bin
    # a tile for testing
    curl -L https://tile.mapzen.com/mapzen/vector/v1/all/10/301/384.mvt?api_key=vector-tiles-tyHL4AY& -o tile.mvt

    for file in bench/*.out
        do
            echo "Running ${file}"
            $file
        done
    popd
fi
