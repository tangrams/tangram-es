#!/usr/bin/env bash

set -e
set -o pipefail

if [ "${PLATFORM}" = "android" ] && [ "${TRAVIS_PULL_REQUEST}" = "false" ] && [ "${TRAVIS_BRANCH}" = "master" ]; then
    cd "$TRAVIS_BUILD_DIR"/android; ./gradlew uploadArchives -PsonatypeUsername="$SONATYPE_USERNAME" -PsonatypePassword="$SONATYPE_PASSWORD"
fi
