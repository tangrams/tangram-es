#!/bin/bash -eo pipefail
# USAGE
#   check_bitcode.sh input_file [archs]
#
if [[ ${2} ]]; then
	# Take all args after input_file as archs.
	archs=${@:2}
else
	# If archs not given in args, use lipo to list all archs in file.
	archs=$(lipo -archs ${1})
fi

echo "Checking bitcode in file=${1} archs=${archs}"

missing_bitcode=0

for arch in ${archs}; do
	# The otool command prints object file data for the given arch in the input file.
	# The awk command prints the 'size' value following a 'segname' value equal to '__LLVM' (the bitcode segment).
	bitcode_size=$(otool -l -arch ${arch} ${1} | awk '/ segname/ { SEGNAME = $2 }; / size/ { if(SEGNAME == "__LLVM") print $2 }')
	echo "arch=${arch} bitcode_size=${bitcode_size}"
	# Sometimes bitcode exists but is just a stub, so check for segments that are too small.
	# This check also covers bitcode_size being empty.
	if [[ ${bitcode_size} -lt 0x10 ]]; then
		((missing_bitcode++))
	fi
done

if [[ $missing_bitcode -gt 0 ]]; then
	echo "Some archs are missing bitcode."
	exit 1
fi
