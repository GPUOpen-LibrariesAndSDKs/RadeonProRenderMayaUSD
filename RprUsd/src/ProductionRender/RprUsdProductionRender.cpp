#include "RprUsdProductionRender.h" 
#include "ProductionSettings.h"
#include "common.h"

// For getting versions of RPR SDK and RIF SDK
#include "RadeonProRenderUSD/deps/RIF/include/RadeonImageFilters_version.h"
#include "RadeonProRenderUSD/deps/RPR/RadeonProRender/inc/RadeonProRender.h"

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
#include <hdMaya/adapters/proxyAdapter.h>

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

	HdMayaSceneDelegate* pMayaSceneDelegate = nullptr;

	SdfPath proxyAdapterPath;
	MayaUsdProxyShapeBase* pShapeBase = GetMayaUsdProxyShapeBase();

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

			if (delegateInitData.name == "HdMayaSceneDelegate") {
				pMayaSceneDelegate = static_cast<HdMayaSceneDelegate*> (newDelegate.get());

				if (pShapeBase != nullptr) {
					MFnDagNode fn(pShapeBase->thisMObject());
					MDagPath   proxyTransformPath;
					fn.getPath(proxyTransformPath);

					proxyAdapterPath = pMayaSceneDelegate->GetPrimPath(proxyTransformPath, false);
				}
			}

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

	if (pMayaSceneDelegate) {
		HdMayaProxyAdapter* pProxyAdapter = static_cast<HdMayaProxyAdapter*> (pMayaSceneDelegate->GetShapeAdapter(proxyAdapterPath).get());

		if (pProxyAdapter != nullptr) {
			UsdImagingDelegate* pImagingDelegate = nullptr;

			// Temporary Hack! We access private variable here because we cannot modify MtoH directly.
			// I want to contribute getter method for this variable in MtoH. If they approve we will remove the hack.
			void* addr = (char*) pProxyAdapter + sizeof(HdMayaShapeAdapter) + sizeof(TfWeakBase) + sizeof(MayaUsdProxyShapeBase*);
			pImagingDelegate = ((std::unique_ptr<HdMayaProxyUsdImagingDelegate>*)(addr))->get();

			pImagingDelegate->SetTime(pShapeBase->getTime());
			pImagingDelegate->SetSceneMaterialsEnabled(true);
		}
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

	UsdPrim cameraPrim;
	bool isUsdCamera = false;

	if (ProductionSettings::IsUSDCameraToUse()) {
		cameraPrim = ProductionSettings::GetUsdCameraPrim();
		isUsdCamera = cameraPrim.IsValid();
	}

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

		GfCamera camera = usdGeomCamera.GetCamera(GetMayaUsdProxyShapeBase()->getTime());

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

	std::map<std::string, std::string> controlCreationCmds;

	ProductionSettings::RegisterCallbacks();
	ProductionSettings::CreateAttributes(&controlCreationCmds);

	RprUsdProductionRender::RegisterRenderer(controlCreationCmds);

    // in case if we start the plugin after scene is opened
    ProductionSettings::UsdCameraListRefresh();
}

void RprUsdProductionRender::Uninitialize()
{
	ProductionSettings::UnregisterCallbacks();
	RprUsdProductionRender::UnregisterRenderer();
}

void RprUsdProductionRender::UnregisterRenderer()
{
	MStatus status = MGlobal::executeCommand("unregisterRprUsdRenderer();");
}

void RprUsdProductionRender::RegisterRenderer(const std::map<std::string, std::string>& controlCreationCmds)
{
	std::string rifSdkVersion = std::to_string(RIF_VERSION_MAJOR) + "." + std::to_string(RIF_VERSION_MINOR) + "." + std::to_string(RIF_VERSION_REVISION);
	std::string rprSdkVersion = std::to_string(RPR_VERSION_MAJOR) + "." + std::to_string(RPR_VERSION_MINOR) + "." + std::to_string(RPR_VERSION_REVISION);

	constexpr auto registerCmd =
		R"mel(global proc registerRprUsdRenderer()
	{
		string $currentRendererName = "hdRPR";

		renderer - rendererUIName $currentRendererName
			- renderProcedure "rprUsdRenderCmd" rprUsdRender;

		renderer - edit - addGlobalsNode "RprUsdGlobals" rprUsdRender;
		renderer - edit - addGlobalsNode "defaultRenderGlobals" rprUsdRender;
		renderer - edit - addGlobalsNode "defaultResolution" rprUsdRender;

		renderer - edit - addGlobalsTab "Common" "createMayaSoftwareCommonGlobalsTab" "updateMayaSoftwareCommonGlobalsTab" rprUsdRender;
		renderer - edit - addGlobalsTab "Config" "createRprUsdRenderConfigTab" "updateRprUsdRenderConfigTab" rprUsdRender;
		renderer - edit - addGlobalsTab "Quality" "createRprUsdRenderQualityTab" "updateRprUsdRenderQualityTab" rprUsdRender;
		renderer - edit - addGlobalsTab "Camera" "createRprUsdRenderCameraTab" "updateRprUsdRenderCameraTab" rprUsdRender;
		renderer - edit - addGlobalsTab "About" "createRprUsdAboutTab" "updateRprUsdAboutTab" rprUsdRender;
	}

	global proc unregisterRprUsdRenderer()
	{
		renderer -unregisterRenderer rprUsdRender;
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

		button -label "Configure Hardware" -command "onConfigureGPU";

		setParent ..;
	}

	global proc updateRprUsdRenderConfigTab()
	{

	}

	global proc onConfigureGPU()
	{
		python( "import deviceConfigRunner\ndeviceConfigRunner.open_window()" );
	}

	global proc createRprUsdRenderQualityTab()
	{
		string $parentForm = `setParent -query`;

		scrollLayout -w 375 -horizontalScrollBarThickness 0 rprmayausd_scrollLayout;
		columnLayout -w 375 -adjustableColumn true rprmayausd_tabcolumn;

		{QUALITY_CONTROLS_CREATION_CMDS}

		formLayout
			-edit
			-af rprmayausd_scrollLayout "top" 0
			-af rprmayausd_scrollLayout "bottom" 0
			-af rprmayausd_scrollLayout "left" 0
			-af rprmayausd_scrollLayout "right" 0
			$parentForm;
	}

	global proc updateRprUsdRenderQualityTab()
	{

	} 

    global string $g_rprHdrUSDCamerasCtrl;
    global string $usdCamerasArray[];

	global proc createRprUsdRenderCameraTab()
	{
		string $parentForm = `setParent -query`;

		global string $g_rprHdrUSDCamerasCtrl;
		global string $usdCamerasArray[];

		scrollLayout -horizontalScrollBarThickness 0 rprmayausd_camerascroll;
		columnLayout -adjustableColumn true;

		frameLayout - label "Usd Camera" - cll true - cl false;

			columnLayout  -adjustableColumn false;

				columnLayout  -adjustableColumn false -co "left" 140;
					button -label "Refresh USD Camera List" -width 160 -command "OnUSDCameraListRefresh";
				setParent ..;

				attrControlGrp -label "Enable USD Camera" -attribute "defaultRenderGlobals.HdRprPlugin_Prod_Static_useUSDCamera" -changeCommand "OnIsUseUsdCameraChanged";

				columnLayout  -adjustableColumn false -co "left" 140;
					$g_rprHdrUSDCamerasCtrl = `optionMenu -l "USD Camera: " -changeCommand "OnUsdCameraChanged"`;
					setParent ..;
				setParent ..;

				$value = GetCameraSelectedAttribute();

				$valueExist = 0;
				for ($i = 0; $i < size($usdCamerasArray); $i++) 
				{
					$cameraName = $usdCamerasArray[$i];
					menuItem -parent $g_rprHdrUSDCamerasCtrl -label $cameraName;

					if ($cameraName == $value)
					{
						$valueExist = 1;
					}
				}

				if ($value != "" && $valueExist > 0)
				{
					optionMenu -e -v $value $g_rprHdrUSDCamerasCtrl; 
				}
			setParent ..; // columnLayout

		setParent ..; // frameLayout

		{CAMERA_CONTROLS_CREATION_CMDS}

		OnIsUseUsdCameraChanged();

		formLayout
			-edit
			-af rprmayausd_camerascroll "top" 0
			-af rprmayausd_camerascroll "bottom" 0
			-af rprmayausd_camerascroll "left" 0
			-af rprmayausd_camerascroll "right" 0
			$parentForm;
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

	global proc OnUSDCameraListRefresh()
	{
		rprUsdRender -ucr;
	}

	global proc updateRprUsdRenderCameraTab()
	{

	}

	global proc string getRprPluginVersion()
	{
		return `pluginInfo -query -version RprUsd`;		
	}

	global proc string getRprCoreVersion()
	{
		return "{RPRSDK_VERSION}";
	}

	global proc string getRifSdkVersion()
	{
		return "{RIFSDK_VERSION}";
	}

 	global proc createRprUsdAboutTab()
	{
		columnLayout
			-adjustableColumn true
			-columnAttach "both" 5
			-columnWidth 250
			-columnAlign "center"
			-rowSpacing 5;

			separator -style "none";

			image
				-width 250
				-image "RadeonProRenderLogo.png";

			frameLayout
				-marginHeight 8
				-labelVisible false
				-borderVisible true
				-collapsable false
				-collapse false;

				string $name = "Radeon ProRender USD Render Delegate (hdRPR) for Maya(R)\n" + 
						"Plugin: " + getRprPluginVersion() + " (Core: " + getRprCoreVersion() + ", RIF:" + getRifSdkVersion() + ")\n" +
						"Copyright (C) 2022 Advanced Micro Devices, Inc. (AMD).\n" +
						"Portions of this software are created and copyrighted\n" +
						"to other third parties.";

				button -label $name -height 70;
			setParent..;

		setParent ..;
	}

	global proc updateRprUsdAboutTab()
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

	std::string qualityTabData;
	std::string cameraTabData;

	auto it = controlCreationCmds.find("Quality");
	if (it != controlCreationCmds.end())
	{
		qualityTabData = it->second;
	}
	else
	{
		assert(false);
	}

	it = controlCreationCmds.find("Camera");
	if (it != controlCreationCmds.end())
	{
		cameraTabData = it->second;
	}
	else
	{
		assert(false);
	}

	std::string registerRenderCmd = TfStringReplace(registerCmd, "{QUALITY_CONTROLS_CREATION_CMDS}", qualityTabData);
	registerRenderCmd = TfStringReplace(registerRenderCmd, "{CAMERA_CONTROLS_CREATION_CMDS}", cameraTabData);
	registerRenderCmd = TfStringReplace(registerRenderCmd, "{RPRSDK_VERSION}", rprSdkVersion);
	registerRenderCmd = TfStringReplace(registerRenderCmd, "{RIFSDK_VERSION}", rifSdkVersion);

	MString mstringCmd(registerRenderCmd.c_str());
	MStatus status = MGlobal::executeCommand(mstringCmd);
	CHECK_MSTATUS(status);
}

PXR_NAMESPACE_CLOSE_SCOPE
