#pragma once

#include <maya/MTimerMessage.h>

#include "Resolver.h"

PXR_NAMESPACE_USING_DIRECTIVE

using LiveModeInfo = RenderStudioResolver::LiveModeInfo;

class RenderStudioResolverHelper
{
public:
	static void StartLiveMode(const LiveModeInfo& liveModeParams);
	static void StopLiveMode();

	static bool IsUnresovableToRenderStudioPath(const std::string& path);
	static std::string Unresolve(const std::string& path);

private:
	static void LiveModeTimerCallbackId(float, float, void* pClientData);

private:
	static MCallbackId g_LiveModeTimerCallbackId;
	static bool m_IsLiveModeStarted;
};