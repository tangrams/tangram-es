#!/usr/bin/env bash

set -e
set -o pipefail

if [[ ${PLATFORM} == "osx" ]]; then
    # Build unit tests
    make tests
fi

