#!/bin/bash
export MALLOC_CHECK_=2
rootdir=`pwd`
if [ ! -d "$rootdir/script" ]; then
    echo ">> Missing 'script/' dir! Must run perf test from root folder. "
    exit 1
fi
if [ "$#" -lt 2 ]; then
    echo "Usage: "
    echo "> ./runcpu.sh input.json build_flag"
    echo ""
    exit 1
fi

# input file
inputJson=$1
build=$2
binfile=bin/RTM3D.bin
#build host
if [ "$build" = "y" ]; then
    rm $binfile > /dev/null 2>&1
	$rootdir/script/buildhost.sh "off" "$build"
	if [ ! -f "$binfile" ]; then
	    exit 1
	fi
fi

# run
export OMP_NUM_THREADS=72
./$binfile $inputJson
