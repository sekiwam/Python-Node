#!/bin/sh

git submodule update --remote -i

NODE_DIR="nodejs"
VERSION="1.0.0"
echo $VERSION
cp python.js $NODE_DIR/lib/
cp python_node.cc $NODE_DIR/src/
cp python_node.h $NODE_DIR/src/

cd $NODE_DIR;
git reset --hard 
git clean -f -d
git apply ../diff.patch
