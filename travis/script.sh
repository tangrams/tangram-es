#!/usr/bin/env bash

set -e

if [[ ${TRAVIS_OS_NAME} == "osx" ]]; then
    make tests
fi
