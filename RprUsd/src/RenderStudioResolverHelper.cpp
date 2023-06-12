#include "RenderStudioResolverHelper.h"
#include <maya/MGlobal.h>

MCallbackId RenderStudioResolverHelper::g_LiveModeTimerCallbackId = 0;
bool RenderStudioResolverHelper::m_IsLiveModeStarted = false;


void RenderStudioResolverHelper::StartLiveMode(const LiveModeInfo& liveModeParams)
{
    StopLiveMode();

    try {
        RenderStudioResolver::StartLiveMode(liveModeParams);

        MStatus status;
        // Run Usd Resolver Live updates   
        float REFRESH_RATE = 0.02f;
        g_LiveModeTimerCallbackId = MTimerMessage::addTimerCallback(REFRESH_RATE, LiveModeTimerCallbackId, nullptr, &status);

        m_IsLiveModeStarted = true;
    }
    catch (const std::runtime_error& e) {
        MGlobal::displayError((std::string("RprUsd StartLiveMode failed: ") + e.what()).c_str());
    }
}

void RenderStudioResolverHelper::StopLiveMode()
{
    if (!m_IsLiveModeStarted) {
        return;
    }

    if (g_LiveModeTimerCallbackId) {
        MTimerMessage::removeCallback(g_LiveModeTimerCallbackId);
        g_LiveModeTimerCallbackId = 0;
    }

    StopLiveMode();
    m_IsLiveModeStarted = false;
}

bool RenderStudioResolverHelper::IsUnresovableToRenderStudioPath(const std::string& path)
{
    return RenderStudioResolver::IsUnresovableToRenderStudioPath(path);
}


std::string RenderStudioResolverHelper::Unresolve(const std::string& path)
{
    return RenderStudioResolver::Unresolve(path);
}

void RenderStudioResolverHelper::LiveModeTimerCallbackId(float, float, void* pClientData)
{
    RenderStudioResolver::ProcessLiveUpdates();
}
