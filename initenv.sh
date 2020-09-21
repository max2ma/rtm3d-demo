#!/bin/bash

daturl=""
datfile="rtm3d-demo-data.tar.xz"
datdir="data"
echo "> Initializing DEMO environment..."

echo "> Downloading demonstration data package..."
#wget -f $daturl $datfile


echo "> Extracting demonstration data..."
tar -xvf $datfile 

echo "> Build CWP library for nplot command. Please, answer 'y' when prompted."
curdir=`pwd`
cd data/cwp 
./buildcwp -y

cd $curdir

echo "> All set for demonstration."