#!/usr/bin/env bash

set -e
set -o pipefail

# if [ "${PLATFORM}" = "android" ] && [ "${TRAVIS_PULL_REQUEST}" = "false" ] && [ "${TRAVIS_BRANCH}" = "master" ]; then
if [ "${PLATFORM}" = "android" ]; then
    cd "$TRAVIS_BUILD_DIR"/platforms/android

    ./gradlew artifactoryPublish -PbuildName=tangram-android -PbuildNumber=${TRAVIS_BUILD_NUMBER}

    # If the git tag for the build is not empty then promote the build to bintray.
    if [ -n "${TRAVIS_TAG}" ]; then
        # Bintray REST call: https://www.jfrog.com/confluence/display/RTF/Deploying+Snapshots+to+oss.jfrog.org#DeployingSnapshotstooss.jfrog.org-PromotingaReleaseBuild
        curl -X POST -u ${ARTIFACTORY_USERNAME}:${ARTIFACTORY_PASSWORD} https://oss.jfrog.org/api/plugins/build/promote/snapshotsToBintray/tangram-android/${TRAVIS_BUILD_NUMBER}
    fi

    cd "$TRAVIS_BUILD_DIR"
fi
