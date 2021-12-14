@echo off

set usd_build_dir=USDBuild
set usd_build_fullpath="%cd%\USD\%usd_build_dir%"

if "%1"=="build_installer" goto :build_installer
if "%1"=="skip_usd_build" goto :skip_usd_build

echo Building USD Pixar...
cd USD

echo Building USD Pixar to %usd_build_fullpath%

rmdir %usd_build_dir% /Q /S
 
python build_scripts\build_usd.py %usd_build_fullpath% --openimageio --materialx
cd ..


:skip_usd_build

echo Building Autodesk's MtoH...
cd maya-usd
python build.py --qt-location=%QT_LOCATION% --maya-location "%MAYA_x64_2022%" --pxrusd-location %usd_build_fullpath% --devkit-location "%MAYA_SDK_2022%" build --build-args="-G \"Visual Studio 15 2017 Win64\" -DBUILD_ADSK_PLUGIN=ON,-DBUILD_PXR_PLUGIN=OFF,-DBUILD_AL_PLUGIN=OFF,-DBUILD_TESTS=OFF,-DBUILD_WITH_PYTHON_3=ON"
cd ..


echo Building RadeonProRenderUSD (hdRPR) ...
cd RadeonProRenderUSD
rmdir build /Q /S
mkdir build
cd build
cmake -Dpxr_DIR=%usd_build_fullpath% -DCMAKE_INSTALL_PREFIX=%usd_build_fullpath% -DCMAKE_GENERATOR="Visual Studio 15 2017 Win64" ..
cmake --build . --config RelWithDebInfo --target install
cd ../..


:build_installer

echo Building ModModifier...
devenv installation\ModModifier\ModModifier.sln /Build Release

rmdir Build /Q /S
mkdir Build

xcopy /S /Y /I USD\%usd_build_dir%\bin Build\USD\%usd_build_dir%\bin
xcopy /S /Y /I USD\%usd_build_dir%\lib Build\USD\%usd_build_dir%\lib
xcopy /S /Y /I USD\%usd_build_dir%\libraries Build\USD\%usd_build_dir%\libraries
xcopy /S /Y /I USD\%usd_build_dir%\mdl Build\USD\%usd_build_dir%\mdl
xcopy /S /Y /I USD\%usd_build_dir%\plugin Build\USD\%usd_build_dir%\plugin
xcopy /S /Y /I USD\%usd_build_dir%\resources Build\USD\%usd_build_dir%\resources

xcopy /S /Y /I maya-usd\build\install\RelWithDebInfo  Build\maya-usd\build\install\RelWithDebInfo
rename Build\maya-usd\build\install\RelWithDebInfo\mayaUSD.mod RPRMayaUSD.mod

copy /Y build_patch\bin\*.dll Build\USD\%usd_build_dir%\lib\
copy /Y installation\ModModifier\x64\Release\ModModifier.exe Build\ModModifier.exe

echo Building Installer...
cd installation
iscc installation.iss
cd ..

