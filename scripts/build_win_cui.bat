@echo off
setlocal
set CURDIR=%~dp0
set BASEDIR=%CURDIR%..
if not defined SRCDIR set SRCDIR=%BASEDIR%\cui\
if not defined DSTDIR set DSTDIR=%BASEDIR%\out\
echo SRCDIR : %SRCDIR%
echo DSTDIR : %DSTDIR%

if "%VCPKG_PREFIX%" == "" (
    if exist %BASEDIR%\vcpkg (
        set VCPKG_PREFIX=%BASEDIR%\vcpkg
    )
)

pushd %SRCDIR%

if exist build rd /s /q build
mkdir build
pushd build 

cmake -DCMAKE_TOOLCHAIN_FILE="%VCPKG_PREFIX%\scripts\buildsystems\vcpkg.cmake" -DVCPKG_TARGET_TRIPLET=x64-windows-static -DCMAKE_PROJECT_NAME="PSDtoSS6" .. || exit /b 1
cmake --build . -- /t:clean;rebuild /p:configuration=release /clp:ErrorsOnly /m:4 || exit /b 1

xcopy /y Release\*.exe %DSTDIR%

rem pause
exit /b
