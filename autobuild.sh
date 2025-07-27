#!/bin/bash

set -e

# 若不存在build目录，则创建
if [ ! -d `pwd`/build ]; then
    mkdir `pwd`/build
fi

# 递归强制删除build目录下的内容
rm -rf `pwd`/build/*

cd `pwd`/build &&
cmake .. &&
make
# cmake --build . 
# cmake --build . --config Release
