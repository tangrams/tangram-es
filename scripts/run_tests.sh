#!/bin/bash -eo pipefail
# USAGE
#   run_tests.sh [test_build_dir]
#
test_dir="build/tests/bin"
if [[ $1 ]]; then
    test_dir=$1
fi
echo "Running unit tests from: ${test_dir}"
pushd ${test_dir}
for file in unit/*.out
    do
        echo "Running ${file}"
        $file
    done
popd
