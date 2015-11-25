#!/usr/bin/env bash

set -e
set -o pipefail

if [[ ${PLATFORM} == "osx" || ${PLATFORM} == "linux" ]]; then
    # Run unit tests
    echo "Running Benchmarks"

    pushd ./build/${PLATFORM}/bin
    # a tile for testing
    curl -L  https://vector.mapzen.com/osm/all/10/301/384.mvt | gunzip > tile.mvt

    for file in bench/*.out
        do
            echo "Running ${file}"
            $file
        done
    popd
fi
