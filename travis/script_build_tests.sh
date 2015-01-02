#!/usr/bin/env bash

set -e
set -o pipefail

if [[ ${TRAVIS_OS_NAME} == "osx" ]]; then
    # Build unit tests
    make tests
fi

