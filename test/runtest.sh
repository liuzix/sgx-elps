#!/bin/sh

#Build out of source
rm -rf ./build
mkdir build
cd build
cmake ..
make

#Run test
./testall
