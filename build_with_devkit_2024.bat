@echo off

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

set usd_build_fullpath=%MAYA_x64_2024%\..\MayaUSD\Maya2024\0.23.1\mayausd\USD

echo Building RadeonProRenderUSD (hdRPR) against builtin Maya's USD package...

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

echo Building USD Resolver...
cd RprUsd

cd RenderStudioResolver

rmdir build /Q /S
mkdir build
cd build
cmake -DMAYA_SUPPORT=1 -DWITH_SHARED_WORKSPACE_SUPPORT=OFF -Dpxr_DIR="%usd_build_fullpath%" -DCMAKE_INSTALL_PREFIX="..\..\..\Build_RPRUsdInstall\usdResolver" -DPXR_USD_LOCATION="%usd_build_fullpath%" -DCMAKE_GENERATOR="Visual Studio 16 2019" -DCMAKE_GENERATOR_PLATFORM="x64" ..
IF %ERRORLEVEL% NEQ 0 (Echo An error occured while building Usd Resolver! &Exit /b 1)

cmake --build . --config Release --target install
IF %ERRORLEVEL% NEQ 0 (Echo An error occured while building Usd Resolver! &Exit /b 1)

cd ../..

rmdir build /Q /S
mkdir build

echo Building RPR USD...
rmdir RprUsd\dist /Q /S

cd build

cmake -G "Visual Studio 16 2019" -A "x64" -T v142 ..
IF %ERRORLEVEL% NEQ 0 (Echo An error occured while building RPR USD! (Maya RprUsd project failed) &Exit /b 1)

cmake --build . --config Release2024
IF %ERRORLEVEL% NEQ 0 (Echo An error occured while building RPR USD! (Maya RprUsd project failed) &Exit /b 1)

cd ../..

xcopy /S /Y /I RprUsd\dist Build_RPRUsdInstall\RprUsd
copy /Y RprUsd\mod\rprUsd.mod Build_RPRUsdInstall\RprUsd\rprUsd.mod


echo Building Mod Modifier...
cd installation\ModModifier
rmdir build /Q /S
mkdir build

cd build

cmake -G "Visual Studio 16 2019" -A "x64" ..
IF %ERRORLEVEL% NEQ 0 (Echo An error occured while building Mod Modifier! &Exit /b 1)

cmake --build . --config Release
IF %ERRORLEVEL% NEQ 0 (Echo An error occured while building Mod Modifier! &Exit /b 1)

cd ..\..\..

copy /Y installation\ModModifier\x64\Release\MayaEnvModifier.exe Build_RPRUsdInstall\MayaEnvModifier.exe
copy /Y installation\ModModifier\x64\Release\RprUsdModModifier.exe Build_RPRUsdInstall\RprUsdModModifier.exe


echo Building Installer...
cd installation
iscc installation_hdrpr_only.iss "/DMayaVersionString=2024"

IF %ERRORLEVEL% NEQ 0 (Echo An error occured while building Installer! &Exit /b 1)
cd ..
