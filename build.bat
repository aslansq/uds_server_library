@echo off
setlocal enableDelayedExpansion
cls

set PRJ_DIR=%~dp0

if exist "!PRJ_DIR!output" (
        rmdir /s /q "!PRJ_DIR!output"
)

mkdir !PRJ_DIR!output

cd !PRJ_DIR!\bootloader
echo BUILDING BOOTLOADER
call build.bat

if !errorlevel! equ 0 (
        echo bootloader build success
) else (
        echo bootloader build fail
        goto ungracefulExit
)

cd !PRJ_DIR!\application
echo BUILDING APPLICATION
build.bat

if !errorlevel! equ 0 (
        echo application build success
) else (
        echo application build fail
        goto ungracefulExit
)

cd !PRJ_DIR!
exit /b 0

:ungracefulExit
cd !PRJ_DIR!
exit /b 1