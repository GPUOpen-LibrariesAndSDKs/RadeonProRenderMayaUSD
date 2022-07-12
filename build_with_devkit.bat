# Support for Maya 2023 only for now, It's possible to add 2022 support if needed. However USD is pretty old within MAYA 2022

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

set usd_build_fullpath=%MAYA_x64_2023%\..\MayaUSD\Maya2023\0.18.0\mayausd\USD

echo Building RadeonProRenderUSD (hdRPR) within builtin Maya's USD package...

cd RadeonProRenderUSD
rmdir build /Q /S
mkdir build
cd build
cmake -Dpxr_DIR="%usd_build_fullpath%" -DMAYAUSD_OPENEXR_STATIC=ON -DPXR_USD_LOCATION="%usd_build_fullpath%" -DCMAKE_INSTALL_PREFIX="..\..\Build_RPRUsdInstall" -DCMAKE_GENERATOR="Visual Studio 15 2017 Win64" -DPYTHON_INCLUDE_DIR="%Python_Include_Dir%" -DPYTHON_EXECUTABLE="%Maya_x64%/bin/mayapy.exe" -DPYTHON_LIBRARIES="%Python_Library%" ..
cmake --build . --config RelWithDebInfo --target install
cd ../..

rem echo Building Modifier...
rem devenv installation\ModModifier\ModModifier.sln /Build Release
rem copy /Y installation\ModModifier\x64\Release\MayaEnvModifier.exe Build_RPRUsdInstall\MayaEnvModifier.exe

rem echo Building Installer...
rem cd installation
rem iscc installation_hdrpr_only.iss
rem cd ..

