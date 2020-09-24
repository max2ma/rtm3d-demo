#!/bin/bash
export MALLOC_CHECK_=2

PWRPID=0
startPowerMonitor(){
    if [ "$#" -lt 1 ]; then
        echo ">> !! Missing power report file! Abort !! <<"
        exit 1
    fi

    reportfile=$1
    # queries only for gpu 0
    ./script/fpgapwr.sh > $reportfile &
    local pid=$!
    PWRPID=$pid
}

sdaflow="hw"
source $XILINX_XRT/setup.sh > /dev/null 2>&1
# sdaflow="sw_emu"

if [ "$#" -lt 2 ]; then
    echo "Usage: "
    echo "> ./runfpga.sh input.json build_flag [kernel.xclbin]"
    echo ""
    exit 1
fi

# input file
inputJson=$1
build=$2
kernel="kernel/$sdaflow/rtmforward_maxY128_maxZ512_b16_nPEZ4_nPEX2_nFSM2.xclbin"
if [ "$#" -eq 3 ]; then
    kernel=$3
fi

binfile=bin/RTM3D_FPGA.bin
#build host
if [ "$build" = "y" ]; then
    rm $binfile > /dev/null 2>&1
    ./script/buildhost.sh "fpga" "$build"
    if [ ! -f "$binfile" ]; then
        exit 1
    fi
fi

# run
XPLATFORM="xilinx_u280_xdma_201920_3"
if [ "$sdaflow" != "hw" ]; then
    export XCL_EMULATION_MODE=$sdaflow
fi

./$binfile $inputJson $kernel

# reset fpga device
# echo "> Reset FPGA board:"
# xbutil reset