#!/bin/sh

NODE_DIR="nodejs"
VERSION="1.0.0"
echo $VERSION
cp python.js $NODE_DIR/lib/
cp python_node.cc $NODE_DIR/src/
cp python_node.h $NODE_DIR/src/
cp diff.patch $NODE_DIR

cd $NODE_DIR;
git apply diff.patch
