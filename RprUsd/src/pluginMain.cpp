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

#include <maya/MFnPlugin.h>
#include <maya/MGlobal.h>

#if MAYA_VERSION >= 24

#pragma warning(push, 0)

#include <hdMaya/adapters/adapter.h>
#include <mayaUsd/utils/plugRegistryHelper.h>

// this headers were takn from MtoH rpo. Switch warnings off for such files
#include "ViewportRender/renderGlobals.h"
#include "ViewportRender/renderOverride.h"
#include "ViewportRender/viewCommand.h"

#pragma warning(pop)

#include "RenderStudioResolverHelper.h"

#endif

#include "BindMtlxCommand/RprUsdBindMtlxCmd.h"
#include "OpenStudioUsdStageCommand/RprUsdOpenStudioStageCmd.h"
#include "ProductionRender/RprUsdProductionRenderCmd.h"
#include "SetIBLCommand/RprUsdSetIBLCmd.h"
#include "version.h"

#if MAYA_VERSION >= 24
using MtohRenderOverridePtr = std::unique_ptr<MtohRenderOverride>;
static std::vector<MtohRenderOverridePtr> gsRenderOverrides;
#endif

PXR_NAMESPACE_USING_DIRECTIVE

PLUGIN_EXPORT MStatus initializePlugin(MObject obj)
{
    MStatus status = MS::kSuccess;

    MFnPlugin plugin(obj, "AMD", PLUGIN_VERSION, "Any");

    MGlobal::executePythonCommand("import menu\nmenu.createRprUsdMenu()");

    status = plugin.registerCommand(
        RprUsdProductionRenderCmd::s_commandName,
        RprUsdProductionRenderCmd::creator,
        RprUsdProductionRenderCmd::newSyntax);
    CHECK_MSTATUS(status);
    RprUsdProductionRenderCmd::Initialize();

    status = plugin.registerCommand(
        RprUsdBiodMtlxCmd::s_commandName, RprUsdBiodMtlxCmd::creator, RprUsdBiodMtlxCmd::newSyntax);
    CHECK_MSTATUS(status);

#if MAYA_VERSION >= 24

    status = plugin.registerCommand(
        RprUsdOpenStudioStageCmd::s_commandName,
        RprUsdOpenStudioStageCmd::creator,
        RprUsdOpenStudioStageCmd::newSyntax);
    CHECK_MSTATUS(status);

    status = plugin.registerCommand(
        RprUsdSetIBLCmd::s_commandName, RprUsdSetIBLCmd::creator, RprUsdSetIBLCmd::newSyntax);
    CHECK_MSTATUS(status);

    // Initialize Viewport

    MStatus ret = MS::kSuccess;

    // Call one time registration of plugins compiled for same USD version as MayaUSD plugin.
    MayaUsd::registerVersionedPlugins();

    ret = HdMayaAdapter::Initialize();
    if (!ret) {
        return ret;
    }

    if (!plugin.registerCommand(
            MtohViewCmd::name, MtohViewCmd::creator, MtohViewCmd::createSyntax)) {
        ret = MS::kFailure;
        ret.perror("Error registering mtoh command!");
        return ret;
    }

    if (auto* renderer = MHWRender::MRenderer::theRenderer()) {
        for (const auto& desc : MtohGetRendererDescriptions()) {
            MtohRenderOverridePtr mtohRenderer(new MtohRenderOverride(desc));
            MStatus               status = renderer->registerOverride(mtohRenderer.get());
            if (status == MS::kSuccess) {
                gsRenderOverrides.push_back(std::move(mtohRenderer));
            } else
                mtohRenderer = nullptr;
        }
    }
#endif

    return status;
}

PLUGIN_EXPORT MStatus uninitializePlugin(MObject obj)
{
    MFnPlugin plugin(obj, "AMD", PLUGIN_VERSION, "Any");

    MStatus status = MS::kSuccess;

    status = plugin.deregisterCommand(RprUsdProductionRenderCmd::s_commandName);
    CHECK_MSTATUS(status);

    RprUsdProductionRenderCmd::Uninitialize();

    status = plugin.deregisterCommand(RprUsdBiodMtlxCmd::s_commandName);
    CHECK_MSTATUS(status);

    MGlobal::executePythonCommand("import menu\nmenu.removeRprUsdMenu()");

#if MAYA_VERSION >= 24

    status = plugin.deregisterCommand(RprUsdOpenStudioStageCmd::s_commandName);
    CHECK_MSTATUS(status);

    status = plugin.deregisterCommand(RprUsdSetIBLCmd::s_commandName);
    CHECK_MSTATUS(status);

    // DeinitializeViewport

    if (auto* renderer = MHWRender::MRenderer::theRenderer()) {
        for (unsigned int i = 0; i < gsRenderOverrides.size(); i++) {
            renderer->deregisterOverride(gsRenderOverrides[i].get());
            gsRenderOverrides[i] = nullptr;
        }
    }

    // Note: when Maya is doing its default "quick exit" that does not uninitialize plugins,
    //       these overrides crash on destruction because Hydra ha already destroyed things
    //       these rely on. There is not much we can do about it...
    gsRenderOverrides.clear();

    // Clear any registered callbacks
    MGlobal::executeCommand("callbacks -cc rprUsd;");

    if (!plugin.deregisterCommand(MtohViewCmd::name)) {
        status = MS::kFailure;
        status.perror("Error deregistering mtoh command!");
    }

    RenderStudioResolverHelper::StopLiveMode();

#endif

    return status;
}
