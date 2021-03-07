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
    echo "\$No Python version specified, ex:3.8.8"
fi


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



mkdir out
cd out


# ----------------------------------------------------
#                   Build Python
# ----------------------------------------------------
PYTHON_VER=$PYTHON_VERSION #$2 #"3.8.8"
python_url="https://www.python.org/ftp/python/$PYTHON_VER/Python-$PYTHON_VER.tgz"
curl -O $python_url
ls -al
tar zxvf Python-$PYTHON_VER.tgz
cd Python-$PYTHON_VER
ls -al
cd PCbuild


if  [ "$3" != "--using_github_actions" ] ; then
    if [ "$machine" = "MinGw" ] ; then
        cd ../../..
        start build_python.bat $PYTHON_VER
        exit
    fi 
fi

echo "3000"