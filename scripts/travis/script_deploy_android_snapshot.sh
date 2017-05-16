#!/usr/bin/env bash

set -e
set -o pipefail

if [ "${PLATFORM}" = "android" ] && [ "${TRAVIS_PULL_REQUEST}" = "false" ] && [ "${TRAVIS_BRANCH}" = "master" ]; then

    # Configure private repository credentials (used to sign release artifacts)
    echo -e "machine github.com\n  login $GITHUB_USERNAME\n  password $GITHUB_PASSWORD" >> ~/.netrc

    make android-sdk

    cd "$TRAVIS_BUILD_DIR"/platforms/android
    git clone https://github.com/mapzen/android-config.git
    ./gradlew uploadArchives -PdoFullRelease -PsonatypeUsername="$SONATYPE_USERNAME" -PsonatypePassword="$SONATYPE_PASSWORD" -Psigning.keyId="$SIGNING_KEY_ID" -Psigning.password="$SIGNING_PASSWORD" -Psigning.secretKeyRingFile="$SIGNING_SECRET_KEY_RING_FILE"
    cd "$TRAVIS_BUILD_DIR"
fi
