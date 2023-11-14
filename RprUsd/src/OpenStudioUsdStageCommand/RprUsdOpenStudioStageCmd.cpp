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

#include "RprUsdOpenStudioStageCmd.h"

#include "../RenderStudioResolverHelper.h"
#include "common.h"
#include "pxr/base/arch/env.h"

#include <maya/MGlobal.h>

#include <filesystem>

#include <combaseapi.h>

PXR_NAMESPACE_OPEN_SCOPE

MString RprUsdOpenStudioStageCmd::s_commandName = "rprUsdOpenStudioStage";
MString RprUsdOpenStudioStageCmd::s_LastRencetUsedFilePath = "";

RprUsdOpenStudioStageCmd::RprUsdOpenStudioStageCmd() { }

// MPxCommand Implementation
// -----------------------------------------------------------------------------
void* RprUsdOpenStudioStageCmd::creator() { return new RprUsdOpenStudioStageCmd; }

// -----------------------------------------------------------------------------
MSyntax RprUsdOpenStudioStageCmd::newSyntax()
{
    MSyntax syntax;

    CHECK_MSTATUS(syntax.addFlag(kFilePathFlag, kFilePathFlagLong, MSyntax::kString));
    CHECK_MSTATUS(syntax.addFlag(
        kGetRecentFilePathUsedFlag, kGetRecentFilePathUsedFlagLong, MSyntax::kString));

    return syntax;
}

std::string GenerateGUID()
{
    GUID    guid;
    HRESULT hCreateGuid = CoCreateGuid(&guid);

    char szGuid[64] = { 0 };

    sprintf(
        szGuid,
        "%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X",
        guid.Data1,
        guid.Data2,
        guid.Data3,
        guid.Data4[0],
        guid.Data4[1],
        guid.Data4[2],
        guid.Data4[3],
        guid.Data4[4],
        guid.Data4[5],
        guid.Data4[6],
        guid.Data4[7]);

    return szGuid;
}

// -----------------------------------------------------------------------------
MStatus RprUsdOpenStudioStageCmd::doIt(const MArgList& args)
{
    // Parse arguments.
    MArgDatabase argData(syntax(), args);

    if (argData.isFlagSet(kGetRecentFilePathUsedFlag)) {
        setResult(s_LastRencetUsedFilePath);
        return MStatus::kSuccess;
    }

    std::string filePath;
    if (argData.isFlagSet(kFilePathFlag)) {
        MString path;
        argData.getFlagArgument(kFilePathFlag, 0, path);
        filePath = path.asChar();
    }

    if (filePath.empty()) {
        MGlobal::displayError("RprUsd: RprUsdOpenStudioStageCmd: filePath is not defined");
        return MS::kFailure;
    }

    if (RenderStudioResolverHelper::IsUnresovableToRenderStudioPath(filePath)) {
        std::string studioFilePath = RenderStudioResolverHelper::Unresolve(filePath);

        MString cmd = MString("RprUsd_DoCreateStage(\"") + studioFilePath.c_str() + "\")";

        MGlobal::executeCommand(cmd);

        // Base url from env variable
        std::string envUrl = ArchGetEnv("RENDER_STUDIO_WORKSPACE_URL");

        // Base url from maya's ui
        MFnDependencyNode node(GetSettingsNode());
        std::string baseUrlAttr;
        _GetAttribute(node, "HdRprPlugin_LiveModeBaseUrl", baseUrlAttr, true);

        std::string prioritizeUIUrl = ArchGetEnv("MAYAUSD_WORKSPACE_PRIORITIZE_UI_OVER_ENV");
        bool prioritizeUI = prioritizeUIUrl == "1";

        std::string baseUrl = "http://localhost";

        if (!envUrl.empty() && !prioritizeUI) {
            baseUrl = envUrl;
        }
        else if (!baseUrlAttr.empty()) {
            baseUrl = baseUrlAttr;
        }
        else if (!envUrl.empty()) {
            baseUrl = envUrl;
        }

        LiveModeInfo liveModeInfo;
        liveModeInfo.liveUrl = baseUrl; 
        liveModeInfo.storageUrl = baseUrl;

        liveModeInfo.liveUrl += "/workspace/live";

        liveModeInfo.storageUrl = "";
        liveModeInfo.channelId = "Maya";

        MPlug channelNamePlug = MFnDependencyNode(GetSettingsNode())
                                    .findPlug("HdRprPlugin_LiveModeChannelName", false);
        if (!channelNamePlug.isNull()) {
            MString channelName = channelNamePlug.asString();
            if (channelName != "") {
                liveModeInfo.channelId = channelName.asChar();
            }
        }

        liveModeInfo.userId = "MayaUser_" + GenerateGUID();

        MGlobal::displayInfo(
            MString("Rpr USD Live Mode Server Url = ") + liveModeInfo.liveUrl.c_str());
        RenderStudioResolverHelper::StartLiveMode(liveModeInfo);

        s_LastRencetUsedFilePath = filePath.c_str();
    } else {
        MGlobal::displayError("RprUsd: RprUsdOpenStudioStageCmd: wrong filePath is specified");
        return MS::kFailure;
    }

    return MStatus::kSuccess;
}

// Static Methods
// -----------------------------------------------------------------------------
void RprUsdOpenStudioStageCmd::cleanUp() { }

void RprUsdOpenStudioStageCmd::Initialize() { }

void RprUsdOpenStudioStageCmd::Uninitialize() { }

PXR_NAMESPACE_CLOSE_SCOPE
