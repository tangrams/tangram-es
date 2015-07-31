#!/usr/bin/env bash

set -e
set -o pipefail

if [ "${PLATFORM}" = "android" ]; then
    apt-get update
    apt-get install s3cmd
    printf "[default]\naccess_key = $S3_ACCESS_KEY\n secret_key = $S3_SECRET_KEY" > ~/.s3cfg

    if [ "${TRAVIS_PULL_REQUEST}" = "false" ] && [ "${TRAVIS_BRANCH}" = "master" ]; then
        s3cmd put android/demo/build/outputs/apk/*.apk s3://android.mapzen.com/tangram/latest.apk
        s3cmd put android/demo/build/outputs/apk/*.apk s3://android.mapzen.com/tangram-snapshots/master-$TRAVIS_BUILD_NUMBER.apk
    else
        s3cmd put android/demo/build/outputs/apk/*.apk s3://android.mapzen.com/tangram-development/$TRAVIS_BRANCH-$TRAVIS_BUILD_NUMBER.apk
    fi
fi
