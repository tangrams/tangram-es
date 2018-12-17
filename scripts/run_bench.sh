#!/bin/bash -eo pipefail
# USAGE
#   run_bench.sh [bench_build_dir]
#
build_dir="build/bench"
if [[ $1 ]]; then
    build_dir=$1
fi
echo "Running benchmarks from: ${build_dir}"
pushd ${build_dir}
for file in bench/*.out
    do
        echo "Running ${file}"
        $file
    done
popd
