#!/bin/sh
NODE_DIR="nodejs"
VERSION="1.0.0"
echo $VERSION

cd $NODE_DIR;
git reset --hard 
git clean -f -d
cd ..

git submodule update --remote -i

cp python.js $NODE_DIR/lib/
cp python_node.cc $NODE_DIR/src/
cp python_node.h $NODE_DIR/src/

cd $NODE_DIR;
git apply ../diff.patch
