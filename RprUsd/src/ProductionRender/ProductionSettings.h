//
// Copyright 2019 Luma Pictures
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
#ifndef PRODUCTION_SETTINGS_H
#define PRODUCTION_SETTINGS_H

//#include "tokens.h"
//#include "utils.h"

//#include <hdMaya/delegates/params.h>

#include <pxr/base/gf/vec4f.h>
#include <pxr/base/tf/token.h>
#include <pxr/base/vt/value.h>
#include <pxr/pxr.h>
#include <pxr/imaging/hd/renderDelegate.h>
#include <pxr/usd/usd/primRange.h>
#include <mayaUsd/nodes/proxyShapeBase.h>

#include <maya/MObject.h>
#include <maya/MMessage.h>
#include <maya/MDGMessage.h>
#include <maya/MNodeMessage.h>


#include <unordered_map>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE


class ProductionSettings
{
public:
	ProductionSettings();
    ~ProductionSettings() = default;
 
    // Creating render globals attributes on "defaultRenderGlobals"
	static std::string CreateAttributes();
	static void ClearUsdCameraAttributes();
	static void ApplySettings(HdRenderDelegate* renderDelegate);

	static void CheckRenderGlobals();
	static void UsdCameraListRefresh();
	static void OnSceneCallback(void* );

	static void RegisterCallbacks();
	static void UnregisterCallbacks();

	static UsdPrim GetUsdCameraPrim();
	static bool IsUSDCameraToUse();

	static MayaUsdProxyShapeBase* GetMayaUsdProxyShapeBase();

private:
	static UsdStageRefPtr GetUsdStage();
	static void attributeChangedCallback(MNodeMessage::AttributeMessage msg, MPlug & plug, MPlug & otherPlug, void* clientData);
	static void nodeAddedCallback(MObject& node, void* pData);

	static void OnBeforeOpenCallback(void* );

private:
	//static MString _attributePrefix;
	static MCallbackId _newSceneCallback;
	static MCallbackId _openSceneCallback;
	static MCallbackId _importSceneCallback;

	static MCallbackId _beforeOpenSceneCallback;
	static MCallbackId _nodeAddedCallback;

	static bool _usdCameraListRefreshed;
	static bool _IsOpeningScene;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PRODUCTION_SETTINGS_H
