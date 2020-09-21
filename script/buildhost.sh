#!/bin/bash
rootdir=`pwd`
srcroot="$rootdir/source/rtm-3d"
#srcroot="$HOME/work/xilinx/rtm3d-fpga/trunk/sw/rtm-3d"
outdir="bin/"
acc="off"
buildclean="y"
if [ "$#" -gt 0 ]; then
acc=$1 # accelerator [gpu|fpga|off]
fi

if [ "$#" -gt 1 ]; then
buildclean=$2 # accelerator [gpu|fpga|off]
fi

echo "> Building Host Program..."
#check source root
if [ ! -f "$srcroot/build.sh" ]; then
    echo "> Invalid SOURCE ROOT folder ($srcroot). "
    exit 1
fi
cd $srcroot
rm *.bin > /dev/null 2>&1
./build.sh --clean=$buildclean --target=host --mpi=off --acc=$acc --define="$buildmacros"
binfile=`ls *.bin`
if [ -f "$binfile" ]; then
    mkdir -p $rootdir/bin
    mv $binfile $rootdir/bin
else
    cp build/build.log $rootdir
fi
cd $rootdir 
exit 0