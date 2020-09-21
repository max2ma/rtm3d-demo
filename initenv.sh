#!/bin/bash

#daturl="https://drive.google.com/file/d/1Ka-mo9sGddU6n7yQBLeTYppt9TtzJ-BA/view?usp=sharing"
datfile="rtm3d-demo-data.tar.xz"
datdir="data"
echo "> Initializing DEMO environment..."
echo "> Downloading demonstration data package..."
#wget --no-check-certificate $daturl -O $datfile
git lfs install
git lfs pull


echo "> Extracting demonstration data..."
tar -xvf $datfile 

echo "> Build CWP library for nplot command. Please, answer 'y' when prompted."
curdir=`pwd`
cd data/cwp 
./buildcwp -y

cd $curdir

echo "> All set for demonstration."