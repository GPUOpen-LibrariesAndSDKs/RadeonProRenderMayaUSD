# RPRMayaUSD
This is project which allows you to work with hdRPR render delegate inside Maya.

### How to build:
##### Install all prerequsities for PixarUSD and MtoH ( except Python, we use Maya's python ):
 - Install Maya 2022 and Maya 2023
 - Install SDKs for Maya 2022 and 2023
 - Define environment variables:
   - MAYA_x64_2022 - Path to Maya 2022 installation folder, i.e C:\Program Files\Autodesk\Maya2022
   - MAYA_x64_2023 - Path to Maya 2023 installation folder, i.e C:\Program Files\Autodesk\Maya2023
   - MAYA_SDK_2022 - Path to Maya 2022 SDK, i.e C:\Autodesk\MAYA2022_SDK\devkitBase
   - MAYA_SDK_2023 - Path to Maya 2023 SDK, i.e C:\Autodesk\MAYA2023_SDK\devkitBase
 - Visual Studio 2017
 - Cmake 3.17.5
 - NASM
 - Install python packages for both Mayas 2022 & 2023 (Better to do it from an administator's mode.)
   - mayapy -m pip install PyYAML
   - mayapy -m pip install jinja2
   - mayapy -m pip install PyOpengl    
   - mayapy -m pip install Pyside2 
   - For Maya 2023 there is a problem with installing Pyside2. To make successful install you need to remove directory
  <Maya_2023_ROOT>\Python\Lib\site-packages\PySide2-5.15.2.dist-info.
  After this make force reinstall for PSide2: mayapy -m pip install --force-reinstall --no-deps PySide2
   - Its better to remove visibility of all Pythons in the system because for building everything we use Maya's python (mayapy),
 System's python sometimes takes into account by the build scripts and might fail.
 
 ##### Build modes: 
  - Run build.bat from x64 VS 2017 Build Tools to build full bundle USD + MToH + hdRPR. In order to make it work you need to move built-in mayausd.mod file outside of Maya Modules directory.
  - Or run build_with_devkit.bat (currently only for Maya 2023) to build only hdRPR which allows to work together with built-in USD + MtoH. To make it works you need to extract <Maya_2023_ROOT>\..\MayaUSD\Maya2023\0.18.0\mayausd\USD\devkit.zip in place.
