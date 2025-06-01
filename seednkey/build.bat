@echo off
setlocal enabledelayedexpansion

set SEEDNKEY_DIR=%~dp0
SET PRJ_DIR=!SEEDNKEY_DIR!..\

cd !SEEDNKEY_DIR!

if exist build32 (
    rd /s /q build32
)

cmake -G "Visual Studio 17 2022" -A Win32 -S . -B "build32"
cmake --build build32
echo 32 bit build seednkey success
if !errorlevel! neq 0 goto seednkey_exit

if exist !PRJ_DIR!\output\seednkey.dll (
    rd /s /q !PRJ_DIR!\output\seednkey.dll
)

copy !SEEDNKEY_DIR!build32\Debug\seednkey.dll !PRJ_DIR!\output\seednkey.dll

cd !SEEDNKEY_DIR!
echo seednkey success
exit /b 0

:seednkey_exit
cd !SEEDNKEY_DIR!
echo seednkey failure
exit /b 1
