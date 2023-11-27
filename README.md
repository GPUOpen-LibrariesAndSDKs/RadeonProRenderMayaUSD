# RPRMayaUSD
This project allows you to work with hdRPR hydra render delegate inside Maya. Also it contains some additional features like AMD MaterialX Library which is available in RprUsd plugin

## How to build:
We support Maya 2024.2. In order to make an installer, please, do the following:
### Prerequisities:
 - Visual Studio 2019 (toolset 142 for Maya 2024).
 - CMake (tested with 3.20.3).
 - Python (tested with 3.10.8)
 - OpenSSL installed on your machine and visible for cmake

#### For Maya 2024.1
 - Install Maya 2024.2
 - Download Maya 2024.2 SDK
 - Define environment variables:
     MAYA_SDK_2024 - Path to Maya 2024 SDK, i.e C:\Autodesk\MAYA2024_SDK\devkitBase
     MAYA_x64_2024 - Path to Maya 2024 installation folder, i.e C:\Program Files\Autodesk\Maya2024
 - extract <MAYA_x64_2024>\..\MayaUSD\Maya2024\0.25.0\mayausd\USD\devkit.zip in place.
 - run build_with_devkit_2024.bat from x64 VS2019 command build tools to build hdRPR and RprUsd maya plugin.
 
 ### Developer mode:
 For developers: 
  - Run steps Maya 2024.2.
  - Define environment variable RPR_USD_PLUGIN_DEV_PATH - <REPO_ROOT>/RprUsd/dist
  - Copy mod file for Maya 2024 (rprUsd_dev.mod) from <REPO_ROOT>/RprUsd/mod to Maya Modules Directory (i.e C:\Program Files\Common Files\Autodesk Shared\Modules\Maya\2024.
