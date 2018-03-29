#!/usr/bin/env bash

set -e
set -o pipefail

# if [ "${PLATFORM}" = "android" ] && [ "${TRAVIS_PULL_REQUEST}" = "false" ] && [ "${TRAVIS_BRANCH}" = "master" ]; then
if [ "${PLATFORM}" = "android" ]; then
    cd "$TRAVIS_BUILD_DIR"/platforms/android

    ./gradlew artifactoryUpload -PbuildNumber=${TRAVIS_BUILD_NUMBER}

    cd "$TRAVIS_BUILD_DIR"
fi
