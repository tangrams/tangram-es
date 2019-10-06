#!/bin/bash -eo pipefail
# USAGE
#   run_tests.sh [test_build_dir] [test_results_dir]
#
build_dir="build/tests"
if [[ $1 ]]; then
    build_dir=$1
fi
echo "Running unit tests from: ${build_dir}"
pushd ${build_dir}
if [[ $2 ]]; then
    results_dir=$2
    mkdir -p ${results_dir}
fi
for file in tests/*.out
    do
        echo "Running ${file}"
        if [[ ${results_dir} ]]; then
            $file -r junit -o ${results_dir}/$(basename ${file}).xml
        else
            $file
        fi
    done
popd
