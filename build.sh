#!/bin/sh
NODE_DIR="nodejs"
VERSION="1.0.0"
echo $VERSION

NODE_VERSION=$1

PATCH_FILE=diff_$1.patch
if [[ ! -f $PATCH_FILE ]] ; then
    echo "File $PATCH_FILE is not there, aborting. [v15, v14]"
    exit
fi


#git submodule update  -i

cd $NODE_DIR;
git reset --hard 
git clean -f -d

git fetch
git reset --hard $NODE_VERSION.x
#git reset --hard FETCH_HEAD

cd ..

cp python.js $NODE_DIR/lib/
cp python_node.cc $NODE_DIR/src/
cp python_node.h $NODE_DIR/src/

cd $NODE_DIR;
echo $PATCH_FILE
cat ../$PATCH_FILE
git apply ../$PATCH_FILE
