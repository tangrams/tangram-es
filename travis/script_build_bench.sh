#!/usr/bin/env bash

set -e
set -o pipefail

if [[ ${PLATFORM} == "osx" || ${PLATFORM} == "linux" ]]; then
    # Build unit tests
    make -j4 benchmark
fi

