#!/bin/bash
NODE_DIR="nodejs"

cp python.js $NODE_DIR/lib/
cp ./*.cc $NODE_DIR/src/
cp ./*.h $NODE_DIR/src/

python3 ./script_patches.py 3.8.8

cd nodejs
memsize=`grep MemTotal /proc/meminfo | awk '{print $2}'`
if [ $memsize -gt 3000000 ] ; then
    make -j$(nproc)
else
    make
fi