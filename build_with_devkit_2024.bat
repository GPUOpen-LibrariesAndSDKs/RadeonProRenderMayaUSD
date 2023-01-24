@echo off

rem Support for Maya 2023 only for now, It's possible to add 2022 support if needed. However USD is pretty old within MAYA 2022

set Maya_x64=%MAYA_x64_2024%
set Maya_sdk=%MAYA_SDK_2024%
set Python_ver=3.10
set Python_Include_Dir=%MAYA_x64_2024%/include/Python310/Python
set Python_Library=%MAYA_x64_2024%/lib/python310.lib

echo Python_Ver=%Python_ver%
echo Maya_x64=%Maya_x64%
echo Maya_sdk=%Maya_sdk%
echo Python_Include_Dir=%Python_Include_Dir%
echo Python_Library=%Python_Library%

set usd_build_fullpath=%MAYA_x64_2024%\..\MayaUSD\Maya2024\0.22.0_202301152314-a488582\mayausd\USD

echo Building RadeonProRenderUSD (hdRPR) within builtin Maya's USD package...

rmdir Build_RPRUsdInstall /Q /S

cd RadeonProRenderUSD
rmdir build /Q /S
mkdir build
cd build
cmake -Dpxr_DIR="%usd_build_fullpath%" -DMAYAUSD_OPENEXR_STATIC=ON -DPXR_USD_LOCATION="%usd_build_fullpath%" -DCMAKE_INSTALL_PREFIX="..\..\Build_RPRUsdInstall\hdRPR" -DCMAKE_GENERATOR="Visual Studio 16 2019" -DCMAKE_GENERATOR_PLATFORM="x64" -DPYTHON_INCLUDE_DIR="%Python_Include_Dir%" -DPYTHON_EXECUTABLE="%Maya_x64%/bin/mayapy.exe" -DPYTHON_LIBRARIES="%Python_Library%" ..
IF %ERRORLEVEL% NEQ 0 (Echo An error occured while building hdRPR! &Exit /b 1)

cmake -DOPENEXR_LOCATION=%usd_build_fullpath% ..
IF %ERRORLEVEL% NEQ 0 (Echo An error occured while building hdRPR! &Exit /b 1)

cmake --build . --config RelWithDebInfo --target install
IF %ERRORLEVEL% NEQ 0 (Echo An error occured while building hdRPR! &Exit /b 1)
cd ../..


echo Building RPR USD...
rmdir RprUsd\dist /Q /S
devenv RprUsd\RprUsd.sln /Clean Release2024
devenv RprUsd\RprUsd.sln /Build Release2024

IF %ERRORLEVEL% NEQ 0 (Echo An error occured while building RPR USD! &Exit /b 1)

xcopy /S /Y /I RprUsd\dist Build_RPRUsdInstall\RprUsd
copy /Y RprUsd\mod\rprUsd.mod Build_RPRUsdInstall\RprUsd\rprUsd.mod


echo Building Mod Modifier...
devenv installation\ModModifier\ModModifier.sln /Clean Release
devenv installation\ModModifier\ModModifier.sln /Build Release

IF %ERRORLEVEL% NEQ 0 (Echo An error occured while building Mod Modifier! &Exit /b 1)

copy /Y installation\ModModifier\x64\Release\MayaEnvModifier.exe Build_RPRUsdInstall\MayaEnvModifier.exe
copy /Y installation\ModModifier\x64\Release\RprUsdModModifier.exe Build_RPRUsdInstall\RprUsdModModifier.exe

echo Building Installer...
cd installation
iscc installation_hdrpr_only.iss "/DMayaVersionString=2024"

IF %ERRORLEVEL% NEQ 0 (Echo An error occured while building Installer! &Exit /b 1)
cd ..