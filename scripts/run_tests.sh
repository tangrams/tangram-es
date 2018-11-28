#!/bin/bash -eo pipefail
# USAGE
#   run_tests.sh [test_build_dir]
#
build_dir="build/tests"
if [[ $1 ]]; then
    build_dir=$1
fi
echo "Running unit tests from: ${build_dir}"
pushd ${build_dir}
for file in tests/*.out
    do
        echo "Running ${file}"
        $file
    done
popd
