#!/bin/bash
NODE_DIR="nodejs"
VERSION="1.0.0"
NODE_VERSION=$1
PATCH_FILE=diff_$1.patch

if [[ ! -f $PATCH_FILE ]] ; then
    echo "File $PATCH_FILE is not there, aborting. [v15, v14]"
    exit
fi

if [[ ! -f $NODE_DIR/README.md ]] ; then
    git submodule update  -i
fi

cd $NODE_DIR;
git reset --hard 
git clean -f -d

git fetch
git reset --hard origin/$NODE_VERSION.x

cd ..

cp python.js $NODE_DIR/lib/
cp python_node.cc $NODE_DIR/src/
cp python_node.h $NODE_DIR/src/

cd $NODE_DIR;
cat ../$PATCH_FILE
git apply ../$PATCH_FILE
cd ..


python_url="https://www.python.org/ftp/python/3.8.8/Python-3.8.8.tgz"
curl -O $python_url
ls -al
tar zxvf Python-3.8.8.tgz
cd Python-3.8.8
ls -al
cd PCbuild
#start get_externals.bat