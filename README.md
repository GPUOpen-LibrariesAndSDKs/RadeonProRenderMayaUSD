# RPRMayaUSD
This project allows you to work with hdRPR hydra render delegate inside Maya. Also it contains some additional features like AMD MaterialX Library which is available in RprUsd plugin

## How to build:
We support Maya 2023.3 and Maya 2024. In order to make an installer, please, do the following:
### Prerequisities:
 - Visual Studio 2019 (with toolset 141 for Maya 2023 and toolset 142 for Maya 2024).
 - CMake (tested with 3.17.5).

#### For Maya 2023.3
 - Install Maya 2023.3
 - Download Maya 2023 SDK
 - Define environment variables:
     MAYA_SDK_2023 - Path to Maya 2023 SDK, i.e C:\Autodesk\MAYA2023_SDK\devkitBase
     MAYA_x64_2023 - Path to Maya 2023 installation folder, i.e C:\Program Files\Autodesk\Maya2023
 - extract <MAYA_x64_2023>\..\MayaUSD\Maya2023\0.20.0\mayausd\USD\devkit.zip in place.
 - run build_with_devkit_2023.bat from x64 VS2019 command build tools to build hdRPR and RprUsd maya plugin.
 
#### For Maya 2024
 - Install Maya 2024
 - Download Maya 2024 SDK
 - Define environment variables:
     MAYA_SDK_2024 - Path to Maya 2024 SDK, i.e C:\Autodesk\MAYA2024_SDK\devkitBase
     MAYA_x64_2024 - Path to Maya 2024 installation folder, i.e C:\Program Files\Autodesk\Maya2024
 - extract <MAYA_x64_2024>\..\MayaUSD\Maya2024\0.22.0\mayausd\USD\devkit.zip in place.
 - run build_with_devkit_2024.bat from x64 VS2019 command build tools to build hdRPR and RprUsd maya plugin.
 
 ### Developer mode:
 For developers: 
  - Run steps for Maya 2023.3 or Maya 2024.
  - Define environment variable RPR_USD_PLUGIN_DEV_PATH - <REPO_ROOT>/RprUsd/dist
  - Copy mod file for Maya 2023 (rprUsd_dev.mod) or Maya 2024 (rprUsd_dev_2024.mod) from <REPO_ROOT>/RprUsd/mod to Maya Modules Directory (i.e C:\Program Files\Common Files\Autodesk Shared\Modules\Maya\2023 (or \2024)).
