@echo off
rmdir build /Q /S
mkdir build

cd build

cmake -G "Visual Studio 16 2019" -A "x64" -T v141 ..
#rem cmake --build . --config Release2023