@echo off
echo %%0 = %0
echo %%1 = %1

dir out/Python-%1/Include

cd out/Python-%1/PCbuild
call get_externals.bat
call build.bat -p x64

dir amd64
mkdir ../../../nodejs/out
mkdir ../../../nodejs/out/Release
mkdir ../../../nodejs/out/Release/lib
copy amd64/python38.lib ../../../nodejs/out/Release/
copy amd64/python38.lib ../../../nodejs/out/Release/lib/

dir ../../../nodejs/out/Release/


cd ../../..
cd nodejs
call vcbuild.bat










