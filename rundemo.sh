#!/bin/bash

help(){
    echo "> Usage: rundemo.sh < target platform > "
    echo "> rundemo.sh --target=<fpga|gpu|cpu> --json=<input.json> --build=<Y|n>"
    exit 1
}

create_modelfolders(){
    # check if output dir exists. If not, create it
    json=$1
    if [ -f "$json" ]; then
        echo "> Input JSON: $json"
        mname=`cat $json | grep mname | cut -d: -f2 | cut -d\" -f2`
        echo "> Model Name: $mname"
    else
        echo "> Input JSON: $json (File not found!)"
        exit;
    fi
    outdir=`cat $json | grep outdir | cut -d: -f2 | cut -d\" -f2`
    if [ -d "$outdir" ]; then
        echo "> Outdir: $outdir "
    else
        echo "> Outdir: $outdir (Not found!)"
        mkdir -p $outdir
    fi
    # create output tmp files
    inputpath=$json
    outdir_log="$outdir/log"
    outdir_tmp="$outdir/tmp"
    outdir_img="$outdir/img"
    mkdir -p $outdir_log
    mkdir -p $outdir_tmp
    mkdir -p $outdir_img

    # check if datdir exists. If not, create it
    datdir=`cat $json | grep datdir | cut -d: -f2 | cut -d\" -f2`
    if [ -d "$datdir" ]; then
        echo "> Datdir: $datdir"
    else
        echo "> Datdir: $datdir (Not found!)"
        mkdir -p $datdir
    fi
}
## default vars
target="gpu"
acc="off"
build="y"
jsonPath=""
runscript="script/runcpu.sh"
reportdir="data"
fpgakernel="data/kernel/hw/rtmforward_maxY128_maxZ512_b16_nPEZ4_nPEX2_nFSM2.xclbin"
# fpgakernel="data/kernel/sw_emu/rtmforward_maxY128_maxZ256_b16_nPEZ4_nPEX2_nFSM2.xclbin"
while [ $# -gt 0 ]; do
  case "$1" in
    --target=*)
      target="${1#*=}"
      ;;
    --json=*)
      jsonPath="${1#*=}"
      ;;
    --build=*)
      build="${1#*=}"
      ;;
    --xclbin=*)
      fpgakernel="${1#*=}"
      ;;
    *)
      help
  esac
  shift
done

create_modelfolders $jsonPath

if [ "$target" = "gpu" ];then
    runscript="script/rungpu.sh"
    ./$runscript $jsonPath $build
elif [ "$target" = "fpga" ];then
    runscript="script/runfpga.sh"
    if [ "$fpgakernel" != "" ]; then
       ./$runscript $jsonPath $build $fpgakernel
    else
        ./$runscript $jsonPath $build
    fi
else
    runscript="script/runcpu.sh"
    ./$runscript $jsonPath $build
fi

