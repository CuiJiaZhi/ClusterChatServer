#!/bin/bash

set -x

rm -rf `pwd`/build/*

cd `pwd`/build

cmake ..

# cmake --build . 
# cmake --build . --config Release
make