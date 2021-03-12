#!/bin/bash
NODE_DIR="nodejs"
VERSION="1.0.0"
NODE_VERSION=$1
PYTHON_VERSION=$2
PATCH_FILE=diff_$1.patch

unameOut="$(uname -s)"
case "${unameOut}" in
    Linux*)     machine=Linux;;
    Darwin*)    machine=Mac;;
    CYGWIN*)    machine=Cygwin;;
    MINGW*)     machine=MinGw;;
    *)          machine="UNKNOWN:${unameOut}"
esac





if [ -z "$PYTHON_VERSION" ]
then
    echo "Error: No Python version specified, ex:3.8.8"
    exit
fi


if [[ ! -f $PATCH_FILE ]] ; then
    echo "Error: File $PATCH_FILE is not there, aborting. [v15, v14]"
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
python3 ./script_patches.py $PYTHON_VERSION




# ----------------------------------------------------
#                   Build Python
# ----------------------------------------------------
mkdir out
cd out

PYTHON_VER=$PYTHON_VERSION #$2 #"3.8.8"
python_url="https://www.python.org/ftp/python/$PYTHON_VER/Python-$PYTHON_VER.tgz"
curl -O $python_url
ls -al
tar zxvf Python-$PYTHON_VER.tgz
cd Python-$PYTHON_VER


ls -al

if [ "$machine" = "MinGw" ] ; then
    mv ../../nodejs/src/node.h ../../nodejs/src/node3.h

    cp -R Include/* ../../nodejs/src/
    cp -R PC/pyconfig.h ../../nodejs/src/
    mv ../../nodejs/src/node.h ../../nodejs/src/node2.h
    mv ../../nodejs/src/node3.h ../../nodejs/src/node.h

    if  [ "$3" != "--using_github_actions" ] ; then
        cd PCbuild
        cd ../../..
        start build_python.bat $PYTHON_VER
    fi 
    exit
fi


./configure --enable-shared --prefix=/tmp/python
#./configure --enable-optimizations --enable-shared
make -j2
make install
cd ../..


# ----------------------------------------------------
#                   Build Node.js
# ----------------------------------------------------
PYVER_SHORT=`echo "$PYTHON_VER" | cut -c 1-3`
ls -l /tmp/python/include/python${PYVER_SHORT}m
mv /tmp/python/include/python${PYVER_SHORT}m/node.h  /tmp/python/include/python${PYVER_SHORT}m/node2.h 
cp -R /tmp/python/include/python${PYVER_SHORT}m/* $NODE_DIR/src/


cd $NODE_DIR

git config user.email "you@example.com"
git config user.name "Your Name"

#git add .
#git commit -a -m 'temp' 

./configure && make