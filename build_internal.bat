@echo off
setlocal enabledelayedexpansion

if "%1"=="" exit /B 1

set MayaVer=%1

echo Building Maya %MayaVer%


if %MayaVer%==2022 (
	set Maya_x64=%MAYA_x64_2022%
	set Maya_sdk=%MAYA_SDK_2022%
	set Python_Include_Dir=%MAYA_x64_2022%/include/Python37/Python
	set Python_Library=%MAYA_x64_2022%/lib/python37.lib

	set Python_ver=3.7
)

if %MayaVer%==2023 (
	set Maya_x64=%MAYA_x64_2023%
	set Maya_sdk=%MAYA_SDK_2023%
	set Python_ver=3.9
	set Python_Include_Dir=%MAYA_x64_2023%/include/Python39/Python
	set Python_Library=%MAYA_x64_2023%/lib/python39.lib
)

echo Python_Ver=%Python_ver%
echo Maya_x64=%Maya_x64%
echo Maya_sdk=%Maya_sdk%
echo Python_Include_Dir=%Python_Include_Dir%
echo Python_Library=%Python_Library%

set usd_build_dir=USDBuild
set usd_build_fullpath="%cd%\USD\%usd_build_dir%"

echo Building USD Pixar...
cd USD

echo Building USD Pixar to %usd_build_fullpath%


rmdir %usd_build_dir% /Q /S

python build_scripts\build_usd.py %usd_build_fullpath% --openimageio --materialx --build-python-info "%Maya_x64%/bin/mayapy.exe" "%Python_Include_Dir%" "%Python_Library%" %Python_ver%

cd ..


:skip_usd_build

echo Building Autodesk's MtoH...
cd maya-usd
python build.py --qt-location=%QT_LOCATION% --maya-location "%Maya_x64%" --pxrusd-location %usd_build_fullpath% --devkit-location "%Maya_sdk%" build --build-args="-G \"Visual Studio 15 2017 Win64\" -DBUILD_ADSK_PLUGIN=ON,-DBUILD_PXR_PLUGIN=OFF,-DBUILD_AL_PLUGIN=OFF,-DBUILD_TESTS=OFF,-DBUILD_WITH_PYTHON_3=ON,-DPYTHON_INCLUDE_DIR=\"%Python_Include_Dir%\",-DPython_EXECUTABLE=\"%Maya_x64%/bin/mayapy.exe\",-DPYTHON_LIBRARIES=\"%Python_Library%\",-DBUILD_WITH_PYTHON_3_VERSION=%Python_ver%"
cd ..


echo Building RadeonProRenderUSD (hdRPR) ...
cd RadeonProRenderUSD
rmdir build /Q /S
mkdir build
cd build
cmake -Dpxr_DIR=%usd_build_fullpath% -DCMAKE_INSTALL_PREFIX=%usd_build_fullpath% -DCMAKE_GENERATOR="Visual Studio 15 2017 Win64" ..
cmake --build . --config RelWithDebInfo --target install
cd ../..

mkdir Build\%MayaVer%

xcopy /S /Y /I USD\%usd_build_dir%\bin Build\%MayaVer%\USD\%usd_build_dir%\bin
xcopy /S /Y /I USD\%usd_build_dir%\lib Build\%MayaVer%\USD\%usd_build_dir%\lib
xcopy /S /Y /I USD\%usd_build_dir%\libraries Build\%MayaVer%\USD\%usd_build_dir%\libraries
xcopy /S /Y /I USD\%usd_build_dir%\mdl Build\%MayaVer%\USD\%usd_build_dir%\mdl
xcopy /S /Y /I USD\%usd_build_dir%\plugin Build\%MayaVer%\USD\%usd_build_dir%\plugin
xcopy /S /Y /I USD\%usd_build_dir%\resources Build\%MayaVer%\USD\%usd_build_dir%\resources

xcopy /S /Y /I maya-usd\build\install\RelWithDebInfo  Build\%MayaVer%\maya-usd\build\install\RelWithDebInfo
rename Build\%MayaVer%\maya-usd\build\install\RelWithDebInfo\mayaUSD.mod RPRMayaUSD.mod
 