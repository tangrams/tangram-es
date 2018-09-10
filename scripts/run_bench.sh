#!/bin/bash -eo pipefail
# USAGE
#   run_bench.sh [bench_build_dir]
#
build_dir="build"
if [[ $1 ]]; then
    build_dir=$1
fi
echo "Running benchmarks from: ${build_dir}"
cp bench/test_tile_10_301_384.mvt ${build_dir}/res/tile.mvt
pushd ${build_dir}
for file in bench/*.out
    do
        echo "Running ${file}"
        $file
    done
popd
