#!/bin/sh
cd `dirname $0`

make android-sdk
echo "Output is `dirname $0`/platforms/android/tangram/build/outputs/aar/tangram-full-release.aar"


