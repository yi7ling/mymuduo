#!/bin/bash

set -e

if [ ! -d `pwd`/build ]; then
    mkdir `pwd`/build
fi

rm -rf `pwd`/build/*

# 编译
cd `pwd`/build &&
    cmake .. &&
    make

# 回到项目根目录
cd ..

# 把头文件拷贝到 /usr/include/mymuduo   so库拷贝到 /usr/lib , 即 安装(install)
if [ ! -d /usr/include/mymuduo ]; then
    mkdir /usr/include/mymuduo
fi

# 拷贝头文件
for header in `ls *.h`
do
    cp $header /usr/include/mymuduo
done

# 拷贝动态库
cp `pwd`/lib/libmymuduo.so /usr/lib

ldconfig

