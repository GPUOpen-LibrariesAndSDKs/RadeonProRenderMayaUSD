rem Support for Maya 2023 only for now, It's possible to add 2022 support if needed. However USD is pretty old within MAYA 2022

set Maya_x64=%MAYA_x64_2023%
set Maya_sdk=%MAYA_SDK_2023%
set Python_ver=3.9
set Python_Include_Dir=%MAYA_x64_2023%/include/Python39/Python
set Python_Library=%MAYA_x64_2023%/lib/python39.lib

echo Python_Ver=%Python_ver%
echo Maya_x64=%Maya_x64%
echo Maya_sdk=%Maya_sdk%
echo Python_Include_Dir=%Python_Include_Dir%
echo Python_Library=%Python_Library%

set usd_build_fullpath=%MAYA_x64_2023%\..\MayaUSD\Maya2023\0.19.0\mayausd\USD

echo Building RadeonProRenderUSD (hdRPR) within builtin Maya's USD package...

rmdir Build_RPRUsdInstall /Q /S

cd RadeonProRenderUSD
rmdir build /Q /S
mkdir build
cd build
cmake -Dpxr_DIR="%usd_build_fullpath%" -DMAYAUSD_OPENEXR_STATIC=ON -DPXR_USD_LOCATION="%usd_build_fullpath%" -DCMAKE_INSTALL_PREFIX="..\..\Build_RPRUsdInstall\hdRPR" -DCMAKE_GENERATOR="Visual Studio 15 2017 Win64" -DPYTHON_INCLUDE_DIR="%Python_Include_Dir%" -DPYTHON_EXECUTABLE="%Maya_x64%/bin/mayapy.exe" -DPYTHON_LIBRARIES="%Python_Library%" ..
cmake -DOPENEXR_LOCATION=%usd_build_fullpath% ..
cmake --build . --config RelWithDebInfo --target install
cd ../..

echo Building RPR USD...
devenv RprUsd\RprUsd.sln /Build Release2023
xcopy /S /Y /I RprUsd\dist Build_RPRUsdInstall\RprUsd
copy /Y RprUsd\mod\rprUsd.mod Build_RPRUsdInstall\RprUsd\rprUsd.mod


echo Building Modifier...
devenv installation\ModModifier\ModModifier.sln /Build Release
copy /Y installation\ModModifier\x64\Release\MayaEnvModifier.exe Build_RPRUsdInstall\MayaEnvModifier.exe
copy /Y installation\ModModifier\x64\Release\RprUsdModModifier.exe Build_RPRUsdInstall\RprUsdModModifier.exe

echo Building Installer...
cd installation
iscc installation_hdrpr_only.iss
cd ..

