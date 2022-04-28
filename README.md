# RPRMayaUSD
This is "bundle" which consists of three components: PixarUSD, Autodesk Mtoh (forked repo) and hdRPR Hydra render delegate

How to build:
Install all prerequsities for PixarUSD and MtoH ( except Python, we use Maya's python ):
1. Install Maya 2022 and Maya 2023
2. Install SDKs for Maya 2022 and 2023
3. Define environment variable:
   MAYA_x64_2022 - Path to Maya 2022 installation folder, i.e C:\Program Files\Autodesk\Maya2022
   MAYA_x64_2023 - Path to Maya 2023 installation folder, i.e C:\Program Files\Autodesk\Maya2023
   MAYA_SDK_2022 - Path to Maya 2022 SDK, i.e C:\Autodesk\MAYA2022_SDK\devkitBase
   MAYA_SDK_2023 - Path to Maya 2023 SDK, i.e C:\Autodesk\MAYA2023_SDK\devkitBase   
5. Visual Studio 2017
6. Cmake 3.17.5
7. Install python packages for For both Mayas 2022 & 2023
   mayapy -m pip install PyYAML
   mayapy -m pip install Pyside2
   mayapy -m pip install jinja2
   mayapy -m pip install PyOpengl
8. NASM

Better to do it from an administator mode.

 Run build.bat from x64 VS 2017 Build Tools. 
 Its better to remove visibility of all Pythons in the system because for building everything we use Maya's python (mayapy),
 System's python sometimes takes into account by the build scripts and might fail.
 
 
