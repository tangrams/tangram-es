#!/usr/bin/env bash

set -e
set -o pipefail

if [ "${PLATFORM}" = "android" ] && [ "${TRAVIS_PULL_REQUEST}" = "false" ] && [ "${TRAVIS_BRANCH}" = "master" ]; then

    # Configure private repository credentials (used to sign release artifacts)
    echo -e "machine github.com\n  login $GITHUB_USERNAME\n  password $GITHUB_PASSWORD" >> ~/.netrc

    make android-sdk

    cd "$TRAVIS_BUILD_DIR"/platforms/android
    ./gradlew bintrayUpload -bintray.user="$BINTRAY_USERNAME" -bintray.apikey="$BINTRAY_APIKEY" -bintray.gpg.password="$BINTRAY_PASSWORD"

    cd "$TRAVIS_BUILD_DIR"
fi
