#!/usr/bin/env bash

set -e
set -o pipefail

if [[ ${PLATFORM} == "osx" || ${PLATFORM} == "linux" ]]; then
    # Build unit tests
    make tests CMAKE_OPTIONS="-GNinja"
fi
