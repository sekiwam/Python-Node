@echo off
echo %%0 = %0
echo %%1 = %1

cd out/Python-%1/PCbuild
call get_externals.bat
call build.bat










