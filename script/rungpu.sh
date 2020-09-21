#!/bin/bash
export MALLOC_CHECK_=2

rootdir=`pwd`
if [ ! -d "$rootdir/script" ]; then
    echo ">> Missing 'script/' dir! Must run perf test from root folder. "
    exit 1
fi
if [ "$#" -lt 2 ]; then
    echo "Usage: "
    echo "> ./rungpu.sh input.json build_flag"
    echo ""
    exit 1
fi

# input file
inputJson=$1
build=$2
binfile=bin/RTM3D_GPU.bin
#build host
$rootdir/script/buildhost.sh "gpu" "$build"
if [ ! -f "$binfile" ]; then
    exit 1
fi
 
./$binfile $inputJson
