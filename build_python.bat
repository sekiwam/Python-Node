@echo off
echo %%0 = %0
echo %%1 = %1

dir out/Python-%1/Include

cd out/Python-%1/PCbuild
call get_externals.bat
errorLevel0=%errorlevel%
call build.bat -p x64
errorLevel1=%errorlevel%

dir amd64

cd ../../..
cd nodejs
call vcbuild.bat
errorLevel2=%errorlevel%

echo %errorLevel0%
echo %errorLevel1%
echo %errorLevel2%









