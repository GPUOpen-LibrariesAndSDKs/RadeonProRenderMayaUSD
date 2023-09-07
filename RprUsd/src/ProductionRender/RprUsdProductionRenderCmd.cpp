//
// Copyright 2023 Advanced Micro Devices, Inc
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//    http://www.apache.org/licenses/LICENSE-2.0
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#include "RprUsdProductionRenderCmd.h"

#include "ProductionSettings.h"
#include "RprUsdProductionRender.h"
#include "maya/MDagPath.h"
#include "maya/MFnCamera.h"
#include "maya/MFnRenderLayer.h"
#include "maya/MFnTransform.h"
#include "maya/MGlobal.h"
#include "maya/MRenderView.h"

#pragma warning(push, 0)

#include <hdMaya/delegates/delegateRegistry.h>
#include <hdMaya/delegates/sceneDelegate.h>
#include <hdMaya/utils.h>

#include <pxr/base/gf/matrix4d.h>
#include <pxr/base/tf/debug.h>
#include <pxr/base/tf/instantiateSingleton.h>
#include <pxr/base/vt/value.h>
#include <pxr/imaging/glf/contextCaps.h>
#include <pxr/imaging/hd/camera.h>
#include <pxr/imaging/hd/renderBuffer.h>
#include <pxr/imaging/hd/rendererPluginRegistry.h>
#include <pxr/imaging/hd/rprim.h>
#include <pxr/imaging/hdx/colorizeSelectionTask.h>
#include <pxr/imaging/hdx/pickTask.h>
#include <pxr/imaging/hdx/renderTask.h>
#include <pxr/imaging/hdx/tokens.h>
#include <pxr/imaging/hgi/hgi.h>
#include <pxr/imaging/hgi/tokens.h>

#pragma warning(pop)

#include <memory>

#include <ShlObj.h>

PXR_NAMESPACE_OPEN_SCOPE

MString                                 RprUsdProductionRenderCmd::s_commandName = "rprUsdRender";
std::unique_ptr<RprUsdProductionRender> RprUsdProductionRenderCmd::s_productionRender;
bool                                    RprUsdProductionRenderCmd::s_waitForIt = false;

RprUsdProductionRenderCmd::RprUsdProductionRenderCmd() { }

// MPxCommand Implementation
// -----------------------------------------------------------------------------
void* RprUsdProductionRenderCmd::creator() { return new RprUsdProductionRenderCmd; }

// -----------------------------------------------------------------------------
MSyntax RprUsdProductionRenderCmd::newSyntax()
{
    MSyntax syntax;

    CHECK_MSTATUS(syntax.addFlag(kCameraFlag, kCameraFlagLong, MSyntax::kString));
    CHECK_MSTATUS(syntax.addFlag(kRenderLayerFlag, kRenderLayerFlagLong, MSyntax::kString));
    CHECK_MSTATUS(syntax.addFlag(kWidthFlag, kWidthFlagLong, MSyntax::kLong));
    CHECK_MSTATUS(syntax.addFlag(kHeightFlag, kHeightFlagLong, MSyntax::kLong));

    CHECK_MSTATUS(syntax.addFlag(kWaitForItTwoStep, kWaitForItTwoStepLong, MSyntax::kNoArg));

    CHECK_MSTATUS(syntax.addFlag(kWaitForIt, kWaitForItLong, MSyntax::kNoArg));

    CHECK_MSTATUS(
        syntax.addFlag(kUSDCameraListRefreshFlag, kUSDCameraListRefreshFlagLong, MSyntax::kNoArg));

    return syntax;
}

MStatus getDimensions(const MArgDatabase& argData, unsigned int& width, unsigned int& height)
{
    // Get dimension arguments.
    if (argData.isFlagSet(kWidthFlag))
        argData.getFlagArgument(kWidthFlag, 0, width);

    if (argData.isFlagSet(kHeightFlag))
        argData.getFlagArgument(kHeightFlag, 0, height);

    // Check that the dimensions are valid.
    if (width <= 0 || height <= 0) {
        MGlobal::displayError("Invalid dimensions");
        return MS::kFailure;
    }

    // Success.
    return MS::kSuccess;
}

MStatus getCameraPath(const MArgDatabase& argData, MDagPath& cameraPath)
{
    // Get camera name argument.
    MString cameraName;
    if (argData.isFlagSet(kCameraFlag))
        argData.getFlagArgument(kCameraFlag, 0, cameraName);

    // Get the camera scene DAG path.
    MSelectionList sList;
    sList.add(cameraName);
    MStatus status = sList.getDagPath(0, cameraPath);
    if (status != MS::kSuccess) {
        MGlobal::displayError("Invalid camera");
        return MS::kFailure;
    }

    // Extend to include the camera shape.
    cameraPath.extendToShape();
    if (cameraPath.apiType() != MFn::kCamera) {
        MGlobal::displayError("Invalid camera");
        return MS::kFailure;
    }

    // Success.
    return MS::kSuccess;
}

// -----------------------------------------------------------------------------
MStatus RprUsdProductionRenderCmd::doIt(const MArgList& args)
{
    // Parse arguments.
    MArgDatabase argData(syntax(), args);

    if (argData.isFlagSet(kUSDCameraListRefreshFlag)
        || argData.isFlagSet(kUSDCameraListRefreshFlagLong)) {
        ProductionSettings::UsdCameraListRefresh();
        return MStatus::kSuccess;
    }

    if (argData.isFlagSet(kWaitForItTwoStep) || argData.isFlagSet(kWaitForItTwoStepLong)) {
        s_waitForIt = true;
        return MS::kSuccess;
    }

    unsigned int width;
    unsigned int height;

    MStatus status = getDimensions(argData, width, height);

    if (status != MStatus::kSuccess) {
        return MStatus::kFailure;
    }

    MDagPath camPath;
    status = getCameraPath(argData, camPath);
    if (status != MS::kSuccess)
        return status;

    MString newLayerName;

    if (argData.isFlagSet(kRenderLayerFlag)) {
        argData.getFlagArgument(kRenderLayerFlag, 0, newLayerName);
    }

    if (!s_productionRender) {
        s_productionRender = std::make_unique<RprUsdProductionRender>();
    }

    s_waitForIt = s_waitForIt || argData.isFlagSet(kWaitForIt);

    status = s_productionRender->StartRender(width, height, newLayerName, camPath, s_waitForIt);
    s_waitForIt = false;

    return !s_productionRender->IsCancelled() ? MS::kSuccess : MS::kFailure;
}

// Static Methods
// -----------------------------------------------------------------------------
void RprUsdProductionRenderCmd::cleanUp() { s_productionRender.reset(); }

void RprUsdProductionRenderCmd::RegisterEnvVariables()
{
    // setup shader cache folder for hdRPR to avoid "access denied" problem if try to write to the
    // default folder

    PWSTR sz = nullptr;
    if (S_OK == ::SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &sz)) {
        std::wstring cacheFolder(sz);

        cacheFolder += L"\\RadeonProRender\\Maya\\USD";

        _wputenv(((L"HDRPR_CACHE_PATH_OVERRIDE=") + cacheFolder).c_str());
    }
}

void RprUsdProductionRenderCmd::Initialize() { RprUsdProductionRender::Initialize(); }

void RprUsdProductionRenderCmd::Uninitialize() { RprUsdProductionRender::Uninitialize(); }

PXR_NAMESPACE_CLOSE_SCOPE
