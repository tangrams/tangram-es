#!/usr/bin/env bash

set -e
set -o pipefail

if [[ ${TRAVIS_OS_NAME} == "osx" ]]; then

    # https://github.com/travis-ci/travis-ci/issues/6307
    rvm get head

fi
