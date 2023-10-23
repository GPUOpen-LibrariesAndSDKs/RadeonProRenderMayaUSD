cmake_minimum_required(VERSION 3.16.0 FATAL_ERROR)

set(CMAKE_SYSTEM_VERSION 10.0.19041.0 CACHE STRING "" FORCE)

project(RprUsd CXX)

################################################################################
# Set target arch type if empty. Visual studio solution generator provides it.
################################################################################
if(NOT CMAKE_VS_PLATFORM_NAME)
    set(CMAKE_VS_PLATFORM_NAME "x64")
endif()
message("${CMAKE_VS_PLATFORM_NAME} architecture in use")

if(NOT ("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x64"))
    message(FATAL_ERROR "${CMAKE_VS_PLATFORM_NAME} arch is not supported!")
endif()

################################################################################
# Global configuration types
################################################################################
set(CMAKE_CONFIGURATION_TYPES
    "Release2024"
    CACHE STRING "" FORCE
)

################################################################################
# Global compiler options
################################################################################
if(MSVC)
    # remove default flags provided with CMake for MSVC
    set(CMAKE_CXX_FLAGS "")
    set(CMAKE_CXX_FLAGS_RELEASE2024 "")
endif()

################################################################################
# Global linker options
################################################################################
if(MSVC)
    # remove default flags provided with CMake for MSVC
    set(CMAKE_EXE_LINKER_FLAGS "")
    set(CMAKE_MODULE_LINKER_FLAGS "")
    set(CMAKE_SHARED_LINKER_FLAGS "")
    set(CMAKE_STATIC_LINKER_FLAGS "")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE2024 "${CMAKE_EXE_LINKER_FLAGS}")
    set(CMAKE_MODULE_LINKER_FLAGS_RELEASE2024 "${CMAKE_MODULE_LINKER_FLAGS}")
    set(CMAKE_SHARED_LINKER_FLAGS_RELEASE2024 "${CMAKE_SHARED_LINKER_FLAGS}")
    set(CMAKE_STATIC_LINKER_FLAGS_RELEASE2024 "${CMAKE_STATIC_LINKER_FLAGS}")
endif()

################################################################################
# Nuget packages function stub.
################################################################################
function(use_package TARGET PACKAGE VERSION)
    message(WARNING "No implementation of use_package. Create yours. "
                    "Package \"${PACKAGE}\" with version \"${VERSION}\" "
                    "for target \"${TARGET}\" is ignored!")
endfunction()

################################################################################
# Common utils
################################################################################
include(CMake/Utils.cmake)

################################################################################
# Additional Global Settings(add specific info there)
################################################################################
include(CMake/GlobalSettingsInclude.cmake OPTIONAL)

################################################################################
# Use solution folders feature
################################################################################
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

################################################################################
# Sub-projects
################################################################################




set(PROJECT_NAME RprUsd)

################################################################################
# Source groups
################################################################################
set(Source_Files
    "src/pluginMain.cpp"
    "src/defaultLightDelegate.cpp"
    "src/defaultLightDelegate.h"
    "src/version.h"
)
source_group("Source Files" FILES ${Source_Files})

set(Source_Files__ResolverHelper
    "src/RenderStudioResolverHelper.cpp"
    "src/RenderStudioResolverHelper.h"
)
source_group("Source Files\\ResolverHelper" FILES ${Source_Files__ResolverHelper})

set(Source_Files__BindMtlxCommand
    "src/BindMtlxCommand/RprUsdBindMtlxCmd.cpp"
    "src/BindMtlxCommand/RprUsdBindMtlxCmd.h"
)
source_group("Source Files\\BindMtlxCommand" FILES ${Source_Files__BindMtlxCommand})


set(Source_Files__SetIBLCommand
    "src/SetIBLCommand/RprUsdSetIBLCmd.cpp"
    "src/SetIBLCommand/RprUsdSetIBLCmd.h"
)
source_group("Source Files\\SetIBLCommand" FILES ${Source_Files__SetIBLCommand})


set(Source_Files__OpenUsdStudioStageCommand
    "src/OpenStudioUsdStageCommand/RprUsdOpenStudioStageCmd.cpp"
    "src/OpenStudioUsdStageCommand/RprUsdOpenStudioStageCmd.h"
)
source_group("Source Files\\OpenUsdStudioStageCommand" FILES ${Source_Files__OpenUsdStudioStageCommand})

set(Source_Files__ProductioonRender
    "src/ProductionRender/common.cpp"
    "src/ProductionRender/common.h"
    "src/ProductionRender/ProductionSettings.cpp"
    "src/ProductionRender/ProductionSettings.h"
    "src/ProductionRender/RenderProgressBars.cpp"
    "src/ProductionRender/RenderProgressBars.h"
    "src/ProductionRender/RprUsdProductionRender.cpp"
    "src/ProductionRender/RprUsdProductionRender.h"
    "src/ProductionRender/RprUsdProductionRenderCmd.cpp"
    "src/ProductionRender/RprUsdProductionRenderCmd.h"
)
source_group("Source Files\\ProductioonRender" FILES ${Source_Files__ProductioonRender})

set(Source_Files__ViewportRender
    "src/ViewportRender/pluginDebugCodes.cpp"
    "src/ViewportRender/pluginDebugCodes.h"
    "src/ViewportRender/renderGlobals.cpp"
    "src/ViewportRender/renderGlobals.h"
    "src/ViewportRender/renderOverride.cpp"
    "src/ViewportRender/renderOverride.h"
    "src/ViewportRender/renderOverrideUtils.h"
    "src/ViewportRender/tokens.cpp"
    "src/ViewportRender/tokens.h"
    "src/ViewportRender/utils.cpp"
    "src/ViewportRender/utils.h"
    "src/ViewportRender/viewCommand.cpp"
    "src/ViewportRender/viewCommand.h"
    "src/ViewportRender/shadingModeExporter.cpp"
)
source_group("Source Files\\ViewportRender" FILES ${Source_Files__ViewportRender})


set(ALL_FILES
    ${Source_Files}
    ${Source_Files__BindMtlxCommand}
    ${Source_Files__ProductioonRender}

    "$<$<CONFIG:Release2024>:"
     ${Source_Files__ViewportRender}
     ${Source_Files__OpenUsdStudioStageCommand}
     ${Source_Files__ResolverHelper}
     ${Source_Files__SetIBLCommand}
    ">"
)

# Turn off warnings for all MToH files
set_source_files_properties("src/defaultLightDelegate.cpp" PROPERTIES COMPILE_FLAGS /W0)

set_source_files_properties("src/ViewportRender/pluginDebugCodes.cpp" PROPERTIES COMPILE_FLAGS /W0)
set_source_files_properties("src/ViewportRender/renderGlobals.cpp" PROPERTIES COMPILE_FLAGS /W0)
set_source_files_properties("src/ViewportRender/renderOverride.cpp" PROPERTIES COMPILE_FLAGS /W0)
set_source_files_properties("src/ViewportRender/tokens.cpp" PROPERTIES COMPILE_FLAGS /W0)
set_source_files_properties("src/ViewportRender/utils.cpp" PROPERTIES COMPILE_FLAGS /W0)
set_source_files_properties("src/ViewportRender/viewCommand.cpp" PROPERTIES COMPILE_FLAGS /W0)
set_source_files_properties("src/ViewportRender/shadingModeExporter.cpp" PROPERTIES COMPILE_FLAGS /W0)

################################################################################
# Target
################################################################################
add_library(${PROJECT_NAME} SHARED ${ALL_FILES})

use_props(${PROJECT_NAME} "${CMAKE_CONFIGURATION_TYPES}" "${DEFAULT_CXX_PROPS}")
set(ROOT_NAMESPACE RprUsd)


set_target_properties(${PROJECT_NAME} PROPERTIES
    VS_GLOBAL_KEYWORD "Win32Proj"
    SUFFIX ".mll"

    VS_DEBUGGER_COMMAND "$(MAYA_x64_2024)/bin/maya.exe"
    CXX_STANDARD 17
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    INTERPROCEDURAL_OPTIMIZATION_RELEASE2024 "TRUE"
)
################################################################################
# Include directories
################################################################################
target_include_directories(${PROJECT_NAME} PUBLIC
    "$<$<CONFIG:Release2024>:"
        "$ENV{MAYA_SDK_2024}/include;"
        "$ENV{MAYA_x64_2024}/../MayaUSD/Maya2024/0.23.1/mayausd/USD/include/boost-1_76;"
        "$ENV{MAYA_x64_2024}/../MayaUSD/Maya2024/0.23.1/mayausd/USD/include;"
        "$ENV{MAYA_x64_2024}/include/Python310/Python;"
        "$ENV{MAYA_SDK_2024}/devkit/ufe/include;"
        "$ENV{MAYA_x64_2024}/../MayaUSD/Maya2024/0.23.1/mayausd/MayaUSD/include/hdMaya;"
        "$ENV{MAYA_x64_2024}/../MayaUSD/Maya2024/0.23.1/mayausd/MayaUSD/include;"
        "$ENV{MAYA_x64_2024}/../MayaUSD/Maya2024/0.23.1/mayausd/MayaUSD/include/hdMaya/adapters;"
        "${CMAKE_CURRENT_SOURCE_DIR}/.."
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Maya2024_hdMaya"
    ">"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ProductionRender"
    "${CMAKE_CURRENT_SOURCE_DIR}/../Build_RPRUsdInstall/usdResolver/include/RenderStudio"
)

################################################################################
# Compile definitions
################################################################################
target_compile_definitions(${PROJECT_NAME} PRIVATE
    "$<$<CONFIG:Release2024>:"
        "NDEBUG"
        "MATERIALX_BUILD_SHARED_LIBS"
        "MAYA_VERSION=24"
    ">"
    "RPRUSD_EXPORTS"
    "_WINDOWS"
    "_USRDLL"
    "WIN32"
    "NOMINMAX"
    "_CRT_SECURE_NO_WARNINGS"
    "_SCL_SECURE_NO_WARNINGS"
    "BOOST_CONFIG_SUPPRESS_OUTDATED_MESSAGE"
    "HAVE_SNPRINTF"
    "TBB_SUPPRESS_DEPRECATED_MESSAGES"
    "PXR_PLUGINPATH_NAME=PXR_PLUGINPATH_NAME"
    "WANT_UFE_BUILD"
    "MAYA_ENABLE_NEW_PRIM_DELETE"
    "PXR_PYTHON_ENABLED=1"
    "REQUIRE_IOSTREAM"
    "BOOST_ALL_DYN_LINK"
    "_BOOL"
    "_AFXDLL"
    "_MBCS"
    "NT_PLUGIN"
    "UNICODE"
    "_UNICODE"
    "_SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING"
    "BOOST_JSON_NO_LIB"
    "BOOST_CONTAINER_NO_LIB"
)

list (APPEND CMAKE_PREFIX_PATH "${CMAKE_SOURCE_DIR}/../Build_RPRUsdInstall/usdResolver/cmake")
list (APPEND CMAKE_PREFIX_PATH "${CMAKE_SOURCE_DIR}/Boost_1_76_0/bin")

find_package(Boost REQUIRED COMPONENTS python log)
find_package(OpenSSL REQUIRED)

find_package(RenderStudio REQUIRED COMPONENTS Kit)

################################################################################
# Compile and link options
################################################################################
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Release2024>:
            /O2;
            /Oi;
            /Gy
        >
        /permissive-;
        /sdl;
        /W3;
        ${DEFAULT_CXX_DEBUG_INFORMATION_FORMAT};
        ${DEFAULT_CXX_EXCEPTION_HANDLING};
        /Y-
    )
    target_link_options(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Release2024>:
            /OPT:REF;
            /OPT:ICF;
            /INCREMENTAL:NO
        >
        /DEBUG;
        /SUBSYSTEM:WINDOWS
    )
endif()

################################################################################
# Post build events
################################################################################
add_custom_command_if(
    TARGET ${PROJECT_NAME}
    POST_BUILD
    COMMANDS
    COMMAND $<CONFIG:Release2024> echo creating dist folders
    COMMAND $<CONFIG:Release2024> if not exist $<SHELL_PATH:${CMAKE_SOURCE_DIR}/>dist md $<SHELL_PATH:${CMAKE_SOURCE_DIR}/>dist
    COMMAND $<CONFIG:Release2024> if not exist $<SHELL_PATH:${CMAKE_SOURCE_DIR}/>dist\\bin md $<SHELL_PATH:${CMAKE_SOURCE_DIR}/>dist\\bin
    COMMAND $<CONFIG:Release2024> if not exist $<SHELL_PATH:${CMAKE_SOURCE_DIR}/>dist\\scripts md $<SHELL_PATH:${CMAKE_SOURCE_DIR}/>dist\\scripts
    COMMAND $<CONFIG:Release2024> if not exist $<SHELL_PATH:${CMAKE_SOURCE_DIR}/>dist\\plug-ins md $<SHELL_PATH:${CMAKE_SOURCE_DIR}/>dist\\plug-ins
    COMMAND $<CONFIG:Release2024> if not exist $<SHELL_PATH:${CMAKE_SOURCE_DIR}/>dist\\plug-ins\\2024 md $<SHELL_PATH:${CMAKE_SOURCE_DIR}/>dist\\plug-ins\\2024
    COMMAND $<CONFIG:Release2024> if not exist $<SHELL_PATH:${CMAKE_SOURCE_DIR}/>dist\\icons md $<SHELL_PATH:${CMAKE_SOURCE_DIR}/>dist\\icons
    COMMAND $<CONFIG:Release2024> XCOPY /E /Y "$<SHELL_PATH:${CMAKE_SOURCE_DIR}/>src\\python" "$<SHELL_PATH:${CMAKE_SOURCE_DIR}/>dist\\scripts\\"
    COMMAND $<CONFIG:Release2024> COPY /Y "$<SHELL_PATH:${CMAKE_SOURCE_DIR}/>lib\\2024\\hdMaya.dll" "$<SHELL_PATH:${CMAKE_SOURCE_DIR}/>dist\\bin\\hdMaya.dll"
    COMMAND $<CONFIG:Release2024> XCOPY /E /Y /I "$<SHELL_PATH:${CMAKE_SOURCE_DIR}/>\\..\\Build_RPRUsdInstall\\hdRPR\\lib\\python\\rpr"  "$<SHELL_PATH:${CMAKE_SOURCE_DIR}/>dist\\scripts\\rpr"
    COMMAND $<CONFIG:Release2024> COPY $<SHELL_PATH:$<TARGET_FILE:${PROJECT_NAME}>> $<SHELL_PATH:${CMAKE_SOURCE_DIR}/>dist\\plug-ins\\2024\\* /Y
    COMMAND $<CONFIG:Release2024> XCOPY /E /Y $<SHELL_PATH:${CMAKE_CURRENT_SOURCE_DIR}/>\\icons\\* $<SHELL_PATH:${CMAKE_SOURCE_DIR}/>dist\\icons\\
)

################################################################################
# Dependencies
################################################################################
set(ADDITIONAL_LIBRARY_DEPENDENCIES
    "$<$<CONFIG:Release2024>:"
        "usd_hgiInterop"
        "usd_hgi"
        "usd_usdImaging"
        "usd_usdImagingGL"
        "usd_hd"
        "usd_usdVol"
        "usd_pxOsd"
        "usd_glf"
        "usd_garch"
        "usd_hio"
        "usd_hf"
        "usd_usdHydra"
        "usd_usdShade"
        "usd_usdLux"
        "usd_usdRender"
        "usd_usdRi"
        "usd_sdr"
        "usd_ndr"
        "usd_usdUtils"
        "usd_usdGeom"
        "usd_usd"
        "usd_kind"
        "usd_pcp"
        "usd_sdf"
        "usd_ar"
        "usd_plug"
        "usd_vt"
        "usd_gf"
        "usd_work"
        "usd_trace"
        "usd_js"
        "usd_tf"
        "usd_cameraUtil"
        "usd_hdx"
        "usd_hdSt"
        "usd_hgiGL"
        "usd_hdMtlx"
        "usd_arch"
        "opengl32"
        "Shlwapi"
        "ufe_4"
        "OpenMayaFX"
        "OpenMayaRender"
        "OpenMayaUI"
        "OpenMayaAnim"
        "Image"
        "cg"
        "cgGL"
        "clew"
        "tbb"
        "Ws2_32"
        "hdMaya"
        "mayaUsd"
        "MaterialXFormatMayaUSD"
        "MaterialXCoreMayaUSD"
        "RenderStudio::Kit"
    ">"
    "OpenMaya;"
    "Foundation"
)
target_link_libraries(${PROJECT_NAME} PUBLIC "${ADDITIONAL_LIBRARY_DEPENDENCIES}")

target_link_directories(${PROJECT_NAME} PUBLIC
    "$<$<CONFIG:Release2024>:"
        "$ENV{MAYA_SDK_2024}/lib;"
        "$ENV{MAYA_x64_2024}/lib;"
        "$ENV{MAYA_x64_2024}/../MayaUSD/Maya2024/0.23.1/mayausd/USD/lib;"
        "$ENV{MAYA_x64_2024}/../MayaUSD/Maya2024/0.23.1/mayausd/MayaUSD/lib;"
        "${CMAKE_CURRENT_SOURCE_DIR}/lib/2024;"
        "$ENV{MAYA_SDK_2024}/devkit/ufe/lib"
        "${CMAKE_CURRENT_SOURCE_DIR}/../Build_RPRUsdInstall/usdResolver/lib"
    ">"
)
