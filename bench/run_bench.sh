#!/usr/bin/env bash

set -e
set -o pipefail

# Run unit tests
echo "Running benchmarks"
pushd ./build/bench/bin

for file in *.out
    do
        echo "Running ${file}"
        ./$file
    done
popd
