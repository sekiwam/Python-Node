@echo off
echo %%0 = %0
echo %%1 = %1

dir out/Python-%1/Include

cd out/Python-%1/PCbuild
call get_externals.bat
call build.bat -p x64

dir amd64










