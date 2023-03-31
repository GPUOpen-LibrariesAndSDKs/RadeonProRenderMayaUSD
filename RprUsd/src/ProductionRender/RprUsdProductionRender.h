#ifndef __RPRUSDPRODUCTIONRENDER__
#define __RPRUSDPRODUCTIONRENDER__

#include <pxr/base/tf/singleton.h>
#include <pxr/imaging/hd/driver.h>
#include <pxr/imaging/hd/engine.h>
#include <pxr/imaging/hd/renderIndex.h>
#include <pxr/imaging/hd/rendererPlugin.h>
#include <pxr/imaging/hd/rprimCollection.h>
#include <pxr/imaging/hdSt/renderDelegate.h>
#include <pxr/imaging/hdx/taskController.h>

#include <maya/MMessage.h>

#include "../defaultLightDelegate.h"

#include "RenderProgressBars.h"


PXR_NAMESPACE_OPEN_SCOPE

using HgiUniquePtr = std::unique_ptr<class Hgi>;

typedef std::chrono::time_point<std::chrono::steady_clock> TimePoint;

class RprUsdProductionRender
{
public:
	RprUsdProductionRender();
	~RprUsdProductionRender();

	MStatus StartRender(unsigned int width, unsigned int height, MString newLayerName, MDagPath cameraPath, bool synchronousRender);
	void StopRender();

	static void Initialize();
	static void Uninitialize();

private:
	bool InitHydraResources();
	void ClearHydraResources();

	void ApplySettings();
	MStatus Render();

	void SaveToFile();

	HdRenderDelegate* _GetRenderDelegate();

	void RefreshRenderView();

	bool RefreshAndCheck();

	static void RPRMainThreadTimerEventCallback(float, float, void * pClientData);
	bool ProcessTimerMessage();

	void ProcessSyncRender(float refreshRate);

	static void RegisterRenderer(const std::map<std::string, std::string>& controlCreationCmds);
	static void UnregisterRenderer();

	void OutputHardwareSetupAndSyncTime();

private:
	bool _renderIsStarted;
	bool _initialized;

	bool _hasDefaultLighting;

	bool _isConverged;

	GfVec4d _viewport;
	MDagPath _camPath;

	MString _oldLayerName;
	MString _newLayerName;

	MCallbackId _callbackTimerId;

	std::unique_ptr<RenderProgressBars> _renderProgressBars;

	HdRprimCollection _renderCollection
	{
		HdTokens->geometry,
			HdReprSelector(
#if MAYA_APP_VERSION >= 2019
				HdReprTokens->refined
#else
				HdReprTokens->smoothHull
#endif
				),
			SdfPath::AbsoluteRootPath()
	};


	static TfToken _rendererName;
	SdfPath _ID;

	std::vector<HdMayaDelegatePtr> _delegates;

	HgiUniquePtr                              _hgi;
	HdDriver                                  _hgiDriver;
	HdEngine                                  _engine;
	HdRendererPlugin*                         _rendererPlugin = nullptr;
	HdxTaskController*                        _taskController = nullptr;
	HdRenderIndex*                            _renderIndex = nullptr;
	std::unique_ptr<MtohDefaultLightDelegate> _defaultLightDelegate = nullptr;

	TimePoint _startRenderTime;
	bool _additionalStatsWasOutput;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif //__RPRUSDPRODUCTIONRENDER__
