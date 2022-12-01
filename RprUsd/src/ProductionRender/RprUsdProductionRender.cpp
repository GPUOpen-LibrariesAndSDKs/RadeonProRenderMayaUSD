#include "RprUsdProductionRender.h" 
#include "ProductionSettings.h"
#include "common.h"

#include <maya/MGlobal.h>
#include <maya/MDagPath.h>
#include <maya/MFnCamera.h>
#include <maya/MFnRenderLayer.h>
#include <maya/MFnTransform.h>
#include <maya/MRenderView.h>
#include <maya/MTimerMessage.h>
#include <maya/MCommonRenderSettingsData.h>
#include <maya/MDistance.h>

#include <pxr/base/gf/matrix4d.h>
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
#include <pxr/usd/usdGeom/camera.h>

#include <hdMaya/delegates/delegateRegistry.h>
#include <hdMaya/delegates/sceneDelegate.h>
#include <hdMaya/utils.h>


#include <pxr/base/tf/debug.h>
#include <thread>

PXR_NAMESPACE_OPEN_SCOPE

TfToken RprUsdProductionRender::_rendererName;

RprUsdProductionRender::RprUsdProductionRender() :
	 _renderIsStarted(false)
	, _initialized(false)
	, _hasDefaultLighting(false)
	, _isConverged(false)
	, _hgi(Hgi::CreatePlatformDefaultHgi())
	, _hgiDriver{ HgiTokens->renderDriver, VtValue(_hgi.get()) }
	, _additionalStatsWasOutput(false)
{

}

RprUsdProductionRender::~RprUsdProductionRender()
{
	ClearHydraResources();
}

// -----------------------------------------------------------------------------

TimePoint GetCurrentChronoTime()
{
	return std::chrono::high_resolution_clock::now();
}

template <typename T>
unsigned long TimeDiffChrono(TimePoint currTime, TimePoint startTime)
{
	return (long)std::chrono::duration_cast<T>(currTime - startTime).count();
}

MString GetTimeStringFromMiliseconds(unsigned long milisecondsTotal)
{
	unsigned int ms = milisecondsTotal % 1000;
	unsigned int secondsTotal = milisecondsTotal / 1000;
	unsigned int hours = secondsTotal / 3600;
	unsigned int minutes = (secondsTotal / 60) % 60;
	unsigned int seconds = secondsTotal % 60;

	return MString((std::to_string(hours) + "h " + std::to_string(minutes) + "m " + std::to_string(seconds) + "s " + std::to_string(ms) + "ms").c_str());
}

void OutputInfoToMayaConsoleCommon(MString text)
{
	MGlobal::displayInfo("hdRPR: " + text);
}

void OutputInfoToMayaConsole(MString text, unsigned long miliseconds)
{
	OutputInfoToMayaConsoleCommon(text + ": " + GetTimeStringFromMiliseconds(miliseconds));
}

MStatus switchRenderLayer(MString& oldLayerName, MString& newLayerName)
{
	// Find the current render layer.
	MObject existingRenderLayer = MFnRenderLayer::currentLayer();
	MFnDependencyNode layerNodeFn(existingRenderLayer);
	oldLayerName = layerNodeFn.name();

	if (newLayerName.length() == 0)
	{
		newLayerName = oldLayerName;
	}

	// Switch the render layer if required.
	if (newLayerName != oldLayerName)
	{
		return MGlobal::executeCommand("editRenderLayerGlobals -currentRenderLayer " + newLayerName, false, true);
	}

	// Success.
	return MS::kSuccess;
}

// -----------------------------------------------------------------------------
MStatus restoreRenderLayer(const MString& oldLayerName, const MString& newLayerName)
{
	// Switch back to the layer that was active
	// before rendering started if necessary.
	if (oldLayerName != newLayerName)
		return MGlobal::executeCommand("editRenderLayerGlobals -currentRenderLayer " + oldLayerName, false, true);

	// Success.
	return MS::kSuccess;
}

void RprUsdProductionRender::RPRMainThreadTimerEventCallback(float, float, void* pClientData)
{
	RprUsdProductionRender* pProductionRender = static_cast<RprUsdProductionRender*> (pClientData);

	assert(pProductionRender);
	pProductionRender->ProcessTimerMessage();
}

bool RprUsdProductionRender::ProcessTimerMessage()
{
	HdRenderDelegate* renderDelegate = _renderIndex->GetRenderDelegate();

	assert(renderDelegate);

	VtDictionary dict = renderDelegate->GetRenderStats();
	double percentDone = dict.find("percentDone")->second.Get<double>();

	_renderProgressBars->update((int)percentDone);

	if (!_additionalStatsWasOutput)
	{
		float firstIterationTime = dict.find("firstIterationRenderTime")->second.Get<float>();

		if (firstIterationTime > 0.0f)
		{
			OutputInfoToMayaConsole("First Iteration Time: ", (unsigned long)firstIterationTime);

			_additionalStatsWasOutput = true;
		}
	}

	return RefreshAndCheck();
}

bool RprUsdProductionRender::RefreshAndCheck()
{
	RefreshRenderView();

	HdRenderBuffer* bufferPtr = _taskController->GetRenderOutput(HdAovTokens->color);

	assert(bufferPtr);

	if (bufferPtr->IsConverged() || (_renderProgressBars && _renderProgressBars->isCancelled()))
	{
		StopRender();
		return false;
	}

	return true;
}

MStatus RprUsdProductionRender::StartRender(unsigned int width, unsigned int height, MString newLayerName, MDagPath cameraPath, bool synchronousRender)
{
	StopRender();

	_ID = SdfPath("/HdMayaViewportRenderer")
		.AppendChild(
			TfToken(TfStringPrintf("_HdMaya_%s_%p", _rendererName.GetText(), this)));

	_viewport = GfVec4d(0, 0, width, height);

	_newLayerName = newLayerName;
	_camPath = cameraPath;
	_renderIsStarted = true;
	switchRenderLayer(_oldLayerName, _newLayerName);

	_startRenderTime = GetCurrentChronoTime();

	if (!InitHydraResources())
	{
		return MStatus::kFailure;
	}

	MRenderView::startRender(width, height, false, true);

	_renderProgressBars = std::make_unique<RenderProgressBars>(false);

	ApplySettings();
	Render();

	const float REFRESH_RATE = 1.0f;

	if (!synchronousRender)
	{
		MStatus status;
		_callbackTimerId = MTimerMessage::addTimerCallback(REFRESH_RATE, RPRMainThreadTimerEventCallback, this, &status);
	}
	else
	{
		ProcessSyncRender(REFRESH_RATE);
	}

	return MStatus::kSuccess;
}

void RprUsdProductionRender::ProcessSyncRender(float refreshRate)
{
	do
	{
		std::chrono::duration<float, std::ratio<1>> time(refreshRate);
		std::this_thread::sleep_for(time);
	} while (ProcessTimerMessage());
}

void RprUsdProductionRender::ApplySettings()
{
	ProductionSettings::ApplySettings(_GetRenderDelegate());
}

void RprUsdProductionRender::StopRender()
{
	if (!_renderIsStarted)
	{
		return;
	}

	// call abort render somehow

	RefreshRenderView();
	SaveToFile();
	_renderProgressBars.reset();

	MRenderView::endRender();

	HdRenderDelegate* renderDelegate = _renderIndex->GetRenderDelegate();
	assert(renderDelegate);
	renderDelegate->Stop();

	unsigned long totalRenderMiliseconds = TimeDiffChrono<std::chrono::milliseconds>(GetCurrentChronoTime(), _startRenderTime);
	OutputInfoToMayaConsole("Total Render Time", totalRenderMiliseconds);

	// unregsiter timer callback
	MTimerMessage::removeCallback(_callbackTimerId);
	_callbackTimerId = 0;

	restoreRenderLayer(_oldLayerName, _newLayerName);

	ClearHydraResources();
	_renderIsStarted = false;
	_additionalStatsWasOutput = false;
}

void RprUsdProductionRender::SaveToFile()
{
	MCommonRenderSettingsData settings;
	MRenderUtil::getCommonRenderSettings(settings);

	MString sceneName = settings.name;

	// Populate the scene name with the current namespace - this
	// should be equivalent to the scene name and is how Maya generates
	// it for post render operations such as layered PSD creation.
	if (sceneName.length() <= 0)
	{
		MStringArray results;
		MGlobal::executeCommand("file -q -ns", results);

		if (results.length() > 0)
			sceneName = results[0];
	}

	MString cameraName = MFnDagNode(_camPath.transform()).name();
	unsigned int frame = static_cast<unsigned int>(MAnimControl::currentTime().value());

	MString fullPath = settings.getImageName(MCommonRenderSettingsData::kFullPathImage, frame,
		sceneName, cameraName, "", MFnRenderLayer::currentLayer());

	// remove existing file to avoid file replace confirmation prompt
	MGlobal::executeCommand("sysFile -delete \"" + fullPath + "\"");

	int dotIndex = fullPath.rindex('.');

	if (dotIndex > 0)
	{
		fullPath = fullPath.substring(0, dotIndex - 1);
	}

	std::string cmd = TfStringPrintf("$editor = `renderWindowEditor -q -editorName`; renderWindowEditor -e -writeImage \"%s\" $editor", fullPath.asChar());

	MStatus status = MGlobal::executeCommand(cmd.c_str());

	if (status != MStatus::kSuccess)
	{
		MGlobal::displayError("[hdRPR] Render image could not be saved!");
	}
}

void RprUsdProductionRender::RefreshRenderView()
{
	HdRenderBuffer* bufferPtr = _taskController->GetRenderOutput(HdAovTokens->color);
	assert(bufferPtr);

	RV_PIXEL* rawBuffer = static_cast<RV_PIXEL*>(bufferPtr->Map());

	// _TODO Remove constants
	unsigned int width = _viewport.GetArray()[2];
	unsigned int height = _viewport.GetArray()[3];

	// Update the render view pixels.
	MRenderView::updatePixels(
		0, width - 1,
		0, height - 1,
		rawBuffer, true);

	// Refresh the render view.
	MRenderView::refresh(0, width - 1, 0, height - 1);

	bufferPtr->Unmap();
}

bool RprUsdProductionRender::InitHydraResources()
{
#if PXR_VERSION < 2102
	GlfGlewInit();
#endif
	GlfContextCaps::InitInstance();
	_rendererPlugin
		= HdRendererPluginRegistry::GetInstance().GetRendererPlugin(_rendererName);
	if (!_rendererPlugin)
		return false;

	auto* renderDelegate = _rendererPlugin->CreateRenderDelegate();
	if (!renderDelegate)
		return false;

    // set "metersPerUnit" setting. It can be used by render delegate to produce physically correct images
    // for now unit size is always equal maya internal unit
    static const TfToken metersPerUnitToken("stageMetersPerUnit", TfToken::Immortal);
    renderDelegate->SetRenderSetting(metersPerUnitToken, VtValue(MDistance(1.00, MDistance::internalUnit()).asMeters()));

	_renderIndex = HdRenderIndex::New(renderDelegate, { &_hgiDriver });
	if (!_renderIndex)
		return false;

	_taskController = new HdxTaskController(
		_renderIndex,
		_ID.AppendChild(TfToken(TfStringPrintf(
			"_UsdImaging_%s_%p",
			TfMakeValidIdentifier(_rendererName.GetText()).c_str(),
			this))));
	_taskController->SetEnableShadows(true);

	HdMayaDelegate::InitData delegateInitData(
		TfToken(),
		_engine,
		_renderIndex,
		_rendererPlugin,
		_taskController,
		SdfPath(),
		/*_isUsingHdSt*/false);

	auto delegateNames = HdMayaDelegateRegistry::GetDelegateNames();
	auto creators = HdMayaDelegateRegistry::GetDelegateCreators();
	TF_VERIFY(delegateNames.size() == creators.size());
	for (size_t i = 0, n = creators.size(); i < n; ++i) {
		const auto& creator = creators[i];
		if (creator == nullptr) {
			continue;
		}
		delegateInitData.name = delegateNames[i];
		delegateInitData.delegateID = _ID.AppendChild(
			TfToken(TfStringPrintf("_Delegate_%s_%lu_%p", delegateNames[i].GetText(), i, this)));
		auto newDelegate = creator(delegateInitData);
		if (newDelegate) {
			// Call SetLightsEnabled before the delegate is populated
			newDelegate->SetLightsEnabled(!_hasDefaultLighting);
			_delegates.emplace_back(std::move(newDelegate));
		}
	}
	if (_hasDefaultLighting) {
		delegateInitData.delegateID
			= _ID.AppendChild(TfToken(TfStringPrintf("_DefaultLightDelegate_%p", this)));
		_defaultLightDelegate.reset(new MtohDefaultLightDelegate(delegateInitData));
	}

	for (auto& it : _delegates) {
		it->Populate();
	}
	if (_defaultLightDelegate) {
		_defaultLightDelegate->Populate();
	}

	_initialized = true;
	return true;
}

HdRenderDelegate* RprUsdProductionRender::_GetRenderDelegate()
{
	return _renderIndex ? _renderIndex->GetRenderDelegate() : nullptr;
}

void RprUsdProductionRender::ClearHydraResources()
{
	_delegates.clear();
	_defaultLightDelegate.reset();

	if (_taskController != nullptr) {
		delete _taskController;
		_taskController = nullptr;
	}

	HdRenderDelegate* renderDelegate = nullptr;
	if (_renderIndex != nullptr) {
		renderDelegate = _renderIndex->GetRenderDelegate();
		delete _renderIndex;
		_renderIndex = nullptr;
	}

	if (_rendererPlugin != nullptr) {
		if (renderDelegate != nullptr) {
			_rendererPlugin->DeleteRenderDelegate(renderDelegate);
		}
		HdRendererPluginRegistry::GetInstance().ReleasePlugin(_rendererPlugin);
		_rendererPlugin = nullptr;
	}
}

MStatus RprUsdProductionRender::Render()
{
	auto renderFrame = [&](bool markTime = false) 
	{
		HdTaskSharedPtrVector tasks = _taskController->GetRenderingTasks();

		SdfPath path;
		for (auto it = tasks.begin(); it != tasks.end(); ++it)
		{
			std::shared_ptr<HdxColorizeSelectionTask> selectionTask
				= std::dynamic_pointer_cast<HdxColorizeSelectionTask>(*it);

			if (selectionTask != nullptr)
			{
				tasks.erase(it);
				break;
			}
		}

		_engine.Execute(_renderIndex, &tasks);
	};

	HdxRenderTaskParams params;
	params.enableLighting = true;
	params.enableSceneMaterials = true;

	_taskController->SetRenderViewport(_viewport);

	MStatus  status;

	UsdPrim cameraPrim = ProductionSettings::GetUsdCameraPrim();
	bool isUsdCamera = ProductionSettings::IsUSDCameraToUse() && cameraPrim.IsValid();

	if (!isUsdCamera)
	{
		MFnCamera fnCamera(_camPath.node());

		// From Maya Documentation: Returns the orthographic or perspective projection matrix for the camera.  
		// The projection matrix that maya's software renderer uses is almost identical to the OpenGL projection matrix. 
		// The difference is that maya uses a left hand coordinate system and so the entries [2][2] and [3][2] are negated.
		MFloatMatrix projMatrix = fnCamera.projectionMatrix();
		projMatrix[2][2] = -projMatrix[2][2];
		projMatrix[3][2] = -projMatrix[3][2];

		MMatrix viewMatrix = MFnTransform(_camPath.transform()).transformationMatrix().inverse();

		_taskController->SetFreeCameraMatrices(
			GetGfMatrixFromMaya(viewMatrix),
			GetGfMatrixFromMaya(projMatrix));
	}
	else
	{	
		UsdGeomCamera usdGeomCamera(cameraPrim);

		GfCamera camera = usdGeomCamera.GetCamera(ProductionSettings::GetMayaUsdProxyShapeBase()->getTime());

		GfMatrix4d projectionMatrix = camera.GetFrustum().ComputeProjectionMatrix();
		GfMatrix4d viewMatrix = camera.GetFrustum().ComputeViewMatrix();

		_taskController->SetFreeCameraMatrices(viewMatrix, projectionMatrix);
	}

	_taskController->SetEnablePresentation(false);

	_taskController->SetRenderParams(params);
	if (!params.camera.IsEmpty())
		_taskController->SetCameraPath(params.camera);

	// Default color in usdview.
	_taskController->SetEnableSelection(false);

	_taskController->SetCollection(_renderCollection);
	
	renderFrame(true);

	OutputHardwareSetupAndSyncTime();

	for (auto& it : _delegates) {
		it->PostFrame();
	}

	return MStatus::kSuccess;
}

void RprUsdProductionRender::OutputHardwareSetupAndSyncTime()
{
	HdRenderDelegate* renderDelegate = _renderIndex->GetRenderDelegate();

	VtDictionary renderStats = renderDelegate->GetRenderStats();
	OutputInfoToMayaConsoleCommon("Hardware setup: ");
	std::vector<std::string> gpusUsedNames = renderStats.find("gpuUsedNames")->second.Get<std::vector<std::string>>();
	int threadCount = renderStats.find("threadCountUsed")->second.Get<int>();

	for (const std::string& gpuName : gpusUsedNames) {
		OutputInfoToMayaConsoleCommon(MString("GPU: ") + gpuName.c_str());
	}

	OutputInfoToMayaConsoleCommon(MString("CPU for rendering: threadCount= ") + std::to_string(threadCount).c_str());

	// after renderFrame call we can say how much time sync process took
	unsigned long totalSyncTimeMiliseconds = TimeDiffChrono<std::chrono::milliseconds>(GetCurrentChronoTime(), _startRenderTime);
	OutputInfoToMayaConsole("Sync Time", totalSyncTimeMiliseconds);
}

void RprUsdProductionRender::Initialize()
{
	_rendererName = TfToken(GetRendererName());

	std::string controlCreationCmds;

	ProductionSettings::RegisterCallbacks();
	controlCreationCmds = ProductionSettings::CreateAttributes();

	RprUsdProductionRender::RegisterRenderer(controlCreationCmds);

    // in case if we start the plugin after scene is opened
    ProductionSettings::UsdCameraListRefresh();
}

void RprUsdProductionRender::Uninitialize()
{
	ProductionSettings::UnregisterCallbacks();
}

void RprUsdProductionRender::RegisterRenderer(const std::string& controlCreationCmds)
{
	constexpr auto registerCmd =
		R"mel(global proc registerRprUsdRenderer()
	{
		string $currentRendererName = "hdRPR";

		renderer - rendererUIName $currentRendererName
			- renderProcedure "rprUsdRenderCmd"

			rprUsdRender;

		renderer - edit - addGlobalsNode "RprUsdGlobals" rprUsdRender;
		renderer - edit - addGlobalsNode "defaultRenderGlobals" rprUsdRender;
		renderer - edit - addGlobalsNode "defaultResolution" rprUsdRender;

		renderer - edit - addGlobalsTab "Common" "createMayaSoftwareCommonGlobalsTab" "updateMayaSoftwareCommonGlobalsTab" rprUsdRender;
		renderer - edit - addGlobalsTab "Config" "createRprUsdRenderConfigTab" "updateRprUsdRenderConfigTab" rprUsdRender;
		renderer - edit - addGlobalsTab "General" "createRprUsdRenderGeneralTab" "updateRprUsdRenderGeneralTab" rprUsdRender;
		renderer - edit - addGlobalsTab "Camera" "createRprUsdRenderCameraTab" "updateRprUsdRenderCameraTab" rprUsdRender;
	}

	proc string GetCameraSelectedAttribute()
	{
		string $value = `getAttr defaultRenderGlobals.HdRprPlugin_Prod_Static_usdCameraSelected`;
		return $value;
	}

	proc SetCameraSelectedAttribute(string $value)
	{
		setAttr -type "string" defaultRenderGlobals.HdRprPlugin_Prod_Static_usdCameraSelected $value;
	}

	global proc string rprUsdRenderCmd(int $resolution0, int $resolution1,
		int $doShadows, int $doGlowPass, string $camera, string $option)
	{
		print("hdRPR command " + $resolution0 + " " + $resolution1 + " " + $doShadows + " " + $doGlowPass + " " + $camera + " " + $option + "\n");
		string $cmd = "rprUsdRender -w " + $resolution0 + " -h " + $resolution1 + " -cam " + $camera + " " + $option;
		eval($cmd);
		string $result = "";
		return $result;
	}

	global proc createRprUsdRenderConfigTab()
	{
		columnLayout -w 375 -adjustableColumn true rprmayausd_configcolumn;

		button -label "Configure GPU" -command "onConfigureGPU";

		setParent ..;
	}

	global proc updateRprUsdRenderConfigTab()
	{

	}

	global proc onConfigureGPU()
	{
		python( "import deviceConfigRunner\ndeviceConfigRunner.open_window()" );
	}

	global proc createRprUsdRenderGeneralTab()
	{
		string $parentForm = `setParent -query`;

		scrollLayout -w 375 -horizontalScrollBarThickness 0 rprmayausd_scrollLayout;
		columnLayout -w 375 -adjustableColumn true rprmayausd_tabcolumn;

		{CONTROLS_CREATION_CMDS}

		formLayout
			-edit
			-af rprmayausd_scrollLayout "top" 0
			-af rprmayausd_scrollLayout "bottom" 0
			-af rprmayausd_scrollLayout "left" 0
			-af rprmayausd_scrollLayout "right" 0
			$parentForm;
	}

	global proc updateRprUsdRenderGeneralTab()
	{

	} 

    global string $g_rprHdrUSDCamerasCtrl;
    global string $usdCamerasArray[];

	global proc createRprUsdRenderCameraTab()
	{
		global string $g_rprHdrUSDCamerasCtrl;
		global string $usdCamerasArray[];

		columnLayout -w 375 -adjustableColumn true rprmayausd_cameracolumn;
		attrControlGrp -label "Enable USD Camera" -attribute "defaultRenderGlobals.HdRprPlugin_Prod_Static_useUSDCamera" -changeCommand "OnIsUseUsdCameraChanged";
		$g_rprHdrUSDCamerasCtrl = `optionMenu -l "USD Camera: " -changeCommand "OnUsdCameraChanged"`;
		setParent ..;

		for ($i = 0; $i < size($usdCamerasArray); $i++) 
		{
			$cameraName = $usdCamerasArray[$i];
			menuItem -parent $g_rprHdrUSDCamerasCtrl -label $cameraName;
		}

		$value = GetCameraSelectedAttribute();
		if ($value != "")
		{
			optionMenu -e -v $value $g_rprHdrUSDCamerasCtrl; 
		}

		OnIsUseUsdCameraChanged();
	}

	global proc OnUsdCameraChanged()
	{
		global string $g_rprHdrUSDCamerasCtrl;

		$value = `optionMenu -q -v $g_rprHdrUSDCamerasCtrl`;
		SetCameraSelectedAttribute($value);
	}

	global proc OnIsUseUsdCameraChanged()
	{
		global string $g_rprHdrUSDCamerasCtrl;

		$enabled = `getAttr defaultRenderGlobals.HdRprPlugin_Prod_Static_useUSDCamera`;
		optionMenu -e -en $enabled $g_rprHdrUSDCamerasCtrl;
	}

	global proc updateRprUsdRenderCameraTab()
	{

	}

	global proc int IsUSDCameraCtrlExist()
	{
		global string $g_rprHdrUSDCamerasCtrl;

		if ($g_rprHdrUSDCamerasCtrl == "")
			return 0;

		$exists = `optionMenu -q -exists $g_rprHdrUSDCamerasCtrl`;
		return $exists; 
	}

	global proc HdRpr_clearUSDCameras()
    {
		global string $g_rprHdrUSDCamerasCtrl;
		global string $usdCamerasArray[];

		clear($usdCamerasArray);

		if (!IsUSDCameraCtrlExist())
			return;

        $items = `optionMenu -q -itemListLong $g_rprHdrUSDCamerasCtrl`;
		if (size($items) > 0)
		{
			deleteUI($items);
		}
	}

	global proc HdRpr_AddUsdCamera(string $cameraName)
	{
		global string $g_rprHdrUSDCamerasCtrl;
		global string $usdCamerasArray[];

		$usdCamerasArray[size($usdCamerasArray)] = $cameraName;
		if (IsUSDCameraCtrlExist())
		{
			menuItem -parent $g_rprHdrUSDCamerasCtrl -label $cameraName;
		}

		if (GetCameraSelectedAttribute() == "")
		{
			SetCameraSelectedAttribute($cameraName);
		}
	}

	global proc string GetCurrentUsdCamera()
	{
		return `getAttr defaultRenderGlobals.HdRprPlugin_Prod_Static_usdCameraSelected`;
	} 

	registerRprUsdRenderer();
)mel";

	std::string registerRenderCmd = TfStringReplace(registerCmd, "{CONTROLS_CREATION_CMDS}", controlCreationCmds);

	MString mstringCmd(registerRenderCmd.c_str());
	MStatus status = MGlobal::executeCommand(mstringCmd);
	CHECK_MSTATUS(status);
}

PXR_NAMESPACE_CLOSE_SCOPE
