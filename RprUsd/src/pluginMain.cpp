#include <maya/MFnPlugin.h>
#include <maya/MGlobal.h>
#include <maya/MTimerMessage.h>


#if MAYA_VERSION >= 24
#include <hdMaya/adapters/adapter.h>
#include <mayaUsd/utils/plugRegistryHelper.h>
#include "ViewportRender/renderGlobals.h"
#include "ViewportRender/renderOverride.h"
#include "ViewportRender/viewCommand.h"
#endif

#include "Resolver.h"

#include "version.h"

#include "ProductionRender/RprUsdProductionRenderCmd.h"
#include "BindMtlxCommand/RprUsdBindMtlxCmd.h"


#if MAYA_VERSION >= 24
using MtohRenderOverridePtr = std::unique_ptr<MtohRenderOverride>;
static std::vector<MtohRenderOverridePtr> gsRenderOverrides;
#endif

MCallbackId g_LiveModeTimerCallbackId = 0;

void LiveModeTimerCallbackId(float, float, void* pClientData);


PXR_NAMESPACE_USING_DIRECTIVE

PLUGIN_EXPORT MStatus initializePlugin(MObject obj)
{
    MStatus status = MS::kSuccess;

    MFnPlugin plugin(obj, "AMD", PLUGIN_VERSION, "Any");

    MGlobal::executePythonCommand("import menu\nmenu.createRprUsdMenu()");

	status = plugin.registerCommand(RprUsdProductionRenderCmd::s_commandName, RprUsdProductionRenderCmd::creator, RprUsdProductionRenderCmd::newSyntax);
	CHECK_MSTATUS(status);
	RprUsdProductionRenderCmd::Initialize();

    status = plugin.registerCommand(RprUsdBiodMtlxCmd::s_commandName, RprUsdBiodMtlxCmd::creator, RprUsdBiodMtlxCmd::newSyntax);
	CHECK_MSTATUS(status);

#if MAYA_VERSION >= 24
    // Initialize Viewport

    MStatus ret = MS::kSuccess;

    // Call one time registration of plugins compiled for same USD version as MayaUSD plugin.
    MayaUsd::registerVersionedPlugins();

    ret = HdMayaAdapter::Initialize();
    if (!ret) {
        return ret;
    }

    // For now this is required for the HdSt backed to use lights.
    // putenv requires char* and I'm not willing to use const cast!
    /*constexpr const char* envVarSet = "USDIMAGING_ENABLE_SCENE_LIGHTS=1";
    const auto            envVarSize = strlen(envVarSet) + 1;
    std::vector<char>     envVarData;
    envVarData.resize(envVarSize);
    snprintf(envVarData.data(), envVarSize, "%s", envVarSet);
    putenv(envVarData.data());*/

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
            }
            else
                mtohRenderer = nullptr;
        }
    }
#endif

    RenderStudioResolver::SetRemoteServerAddress("wss://renderstudio.luxoft.com/live/", "");
    RenderStudioResolver::SetCurrentUserId("Maya");

    RenderStudioResolver::StartLiveMode();

// Run Usd Resolver Live updates   
    float REFRESH_RATE = 0.02;
    g_LiveModeTimerCallbackId = MTimerMessage::addTimerCallback(REFRESH_RATE, LiveModeTimerCallbackId, nullptr, &status);

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
#endif 

    RenderStudioResolver::StopLiveMode();

    if (g_LiveModeTimerCallbackId){ 
        MTimerMessage::removeCallback(g_LiveModeTimerCallbackId);
        g_LiveModeTimerCallbackId = 0;
    }

    return status;
}

void LiveModeTimerCallbackId(float, float, void* pClientData)
{
    RenderStudioResolver::ProcessLiveUpdates();
}
