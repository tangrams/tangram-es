#!/bin/bash -eo pipefail
# USAGE
#   run_bench.sh [bench_build_dir]
#
bench_dir="build/bench/bin"
if [[ $1 ]]; then
    bench_dir=$1
fi
echo "Running benchmarks from: ${bench_dir}"
cp bench/test_tile_10_301_384.mvt ${bench_dir}/tile.mvt
pushd ${bench_dir}
for file in bench/*.out
    do
        echo "Running ${file}"
        $file
    done
popd
