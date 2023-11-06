/**********************************************************************
Copyright 2023 Advanced Micro Devices, Inc
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at
    http://www.apache.org/licenses/LICENSE-2.0
Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
********************************************************************/


#include "RenderStudioResolverHelper.h"
#include <maya/MGlobal.h>

MCallbackId RenderStudioResolverHelper::g_LiveModeTimerCallbackId = 0;
bool RenderStudioResolverHelper::m_IsLiveModeStarted = false;

void RenderStudioResolverHelper::StartLiveMode(const LiveModeInfo& liveModeParams)
{
    StopLiveMode();

    try {
        RenderStudio::Kit::LiveSessionConnect(liveModeParams);

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

    RenderStudio::Kit::LiveSessionDisconnect();
    m_IsLiveModeStarted = false;
}

bool RenderStudioResolverHelper::IsUnresovableToRenderStudioPath(const std::string& path)
{
    return RenderStudio::Kit::IsUnresolvable(path);
}


std::string RenderStudioResolverHelper::Unresolve(const std::string& path)
{
    return RenderStudio::Kit::Unresolve(path);
}

void RenderStudioResolverHelper::SetWorkspacePath(const std::string& path)
{
    RenderStudio::Kit::SetWorkspacePath(path);
}

void RenderStudioResolverHelper::LiveModeTimerCallbackId(float, float, void* pClientData)
{
    RenderStudio::Kit::LiveSessionUpdate();
}
