@echo off

setlocal enabledelayedexpansion

if "%1"=="skip_build" goto :skip_build

rmdir Build /Q /S

call build_internal.bat 2022
call build_internal.bat 2023

:skip_build

echo Building ModModifier...
devenv installation\ModModifier\ModModifier.sln /Build Release
copy /Y installation\ModModifier\x64\Release\ModModifier.exe Build\ModModifier.exe

echo Building Installer...
cd installation
iscc installation.iss
cd ..

