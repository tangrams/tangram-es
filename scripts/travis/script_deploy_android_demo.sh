#!/usr/bin/env bash

set -e
set -o pipefail

if [ "${PLATFORM}" = "android" ]; then
    printf "[default]\naccess_key = $S3_ACCESS_KEY\n secret_key = $S3_SECRET_KEY" > ~/.s3cfg

    if [ "${TRAVIS_PULL_REQUEST}" = "false" ] && [ "${TRAVIS_BRANCH}" = "master" ]; then
        url_latest="s3://android.mapzen.com/tangram-latest.apk"
        echo "Uploading latest build to $url_latest"
        s3cmd put platforms/android/demo/build/outputs/apk/demo-debug.apk $url_latest

        url_snapshot="s3://android.mapzen.com/tangram-snapshots/master-$TRAVIS_BUILD_NUMBER.apk"
        echo "Uploading snapshot build to $url_snapshot"
        s3cmd put platforms/android/demo/build/outputs/apk/demo-debug.apk $url_snapshot
    elif ! [ "${TRAVIS_BRANCH}" = "master" ]; then
        url_dev="s3://android.mapzen.com/tangram-development/$TRAVIS_BRANCH-$TRAVIS_BUILD_NUMBER.apk"
        echo "Uploading development build to $url_dev"
        s3cmd put platforms/android/demo/build/outputs/apk/demo-debug.apk $url_dev
    fi
fi
