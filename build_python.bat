@echo off
echo %%0 = %0
echo %%1 = %1

dir out/Python-%1/Include

cd out/Python-%1/PCbuild
call get_externals.bat
call build.bat -p x64

dir amd64


cd ../../..

mkdir nodejs\out
mkdir nodejs\out\Release
mkdir nodejs\out\Release\lib

copy out\Python-%1\PCbuild\amd64\python38.lib nodejs\

dir nodejs\out\Release\




cd nodejs
call vcbuild.bat










