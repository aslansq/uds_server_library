@echo off
setlocal enableDelayedExpansion

set THIS_DIR=%~dp0
set PRJ_DIR=!THIS_DIR!..\
set PRJ_PARENT_DIR=!THIS_DIR!..\..\
set BUILD_TYPE=

cd !THIS_DIR!

if not exist "!THIS_DIR!win_lib" (
        mklink /J "!THIS_DIR!win_lib" "!THIS_DIR!..\library"
)

if "%~1"=="" (
        echo Release build
) else (
        echo Debug build
        set BUILD_TYPE="-DCMAKE_BUILD_TYPE=Debug"
)

SET "PATH=!PRJ_PARENT_DIR!cmake-4.0.2-windows-x86_64\bin;!PRJ_PARENT_DIR!gcc-arm-none-eabi-10.3-2021.10\bin;!PRJ_PARENT_DIR!ninja-win;%PATH%"

rmdir /S /Q "!THIS_DIR!build"
mkdir "!THIS_DIR!build"
cd "!THIS_DIR!build"

cmake -GNinja .. !BUILD_TYPE! -DUSE_LED_BLUE=1

if !errorlevel! equ 0 (
        echo cmake blue success
) else (
        echo cmake blue fail
        goto ungracefulExit
)

cmake --build .

if !errorlevel! equ 0 (
        echo build blue success
) else (
        echo build blue fail
        goto ungracefulExit
)

if exist "!PRJ_DIR!output\uds_application_server_blue.hex" (
        del "!PRJ_DIR!output\uds_application_server_blue.hex"
)

copy !THIS_DIR!build\uds_application_server.hex !PRJ_DIR!output\uds_application_server_blue.hex

rmdir /S /Q "!THIS_DIR!build"
mkdir "!THIS_DIR!build"
cd "!THIS_DIR!build"

cmake -GNinja .. !BUILD_TYPE! -DUSE_LED_BLUE=1 -DUSE_LED_GREEN=1

if !errorlevel! equ 0 (
        echo cmake blue green success
) else (
        echo cmake blue green fail
        goto ungracefulExit
)

cmake --build .

if !errorlevel! equ 0 (
        echo build blue green success
) else (
        echo build blue green fail
        goto ungracefulExit
)

if exist "!PRJ_DIR!output\uds_application_server_green_blue.hex" (
        del "!PRJ_DIR!output\uds_application_server_green_blue.hex"
)

copy !THIS_DIR!build\uds_application_server.hex !PRJ_DIR!output\uds_application_server_green_blue.hex

cd !THIS_DIR!
exit /b 0

:ungracefulExit
cd !THIS_DIR!
exit /b 1