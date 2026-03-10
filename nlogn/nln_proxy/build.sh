#!/bin/bash
set -e

export CC=/usr/bin/gcc

mkdir -p cmakebuild
cd cmakebuild
cmake "$@" .. 

START=$(date +%s)
make
#make test ARGS="-VV"
END=$(date +%s)
echo "Total build time (real) = $(( $END - $START )) seconds"