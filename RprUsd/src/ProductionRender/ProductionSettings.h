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

#ifndef PRODUCTION_SETTINGS_H
#define PRODUCTION_SETTINGS_H

#pragma warning(push, 0)

#include <mayaUsd/nodes/proxyShapeBase.h>

#include <pxr/base/gf/vec4f.h>
#include <pxr/base/tf/token.h>
#include <pxr/base/vt/value.h>
#include <pxr/imaging/hd/renderDelegate.h>
#include <pxr/pxr.h>
#include <pxr/usd/usd/primRange.h>

#pragma warning(pop)

#include <maya/MDGMessage.h>
#include <maya/MMessage.h>
#include <maya/MNodeMessage.h>
#include <maya/MObject.h>

#include <unordered_map>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

// Logical structure of attributes - Tabs and Groups

typedef std::shared_ptr<HdRenderSettingDescriptor> HdRenderSettingDescriptorPtr;

struct AttributeDescription
{
    AttributeDescription(HdRenderSettingDescriptorPtr ptr, bool visible)
    {
        hdRenderSettingDescriptorPtr = ptr;
        visibleAsCtrl = visible;
    }

    HdRenderSettingDescriptorPtr hdRenderSettingDescriptorPtr;
    bool visibleAsCtrl = true;
};

typedef std::shared_ptr<AttributeDescription> AttributeDescriptionPtr;


struct GroupDescription
{
    GroupDescription(const std::string& groupName)
        : name(groupName)
    {
    }

    std::string                          name;
    std::vector<AttributeDescriptionPtr> attributeVector;
};

typedef std::shared_ptr<GroupDescription> GroupDescriptionPtr;

struct TabDescription
{
    TabDescription(const std::string& tabName)
        : name(tabName)
    {
    }

    std::string                      name;
    std::vector<GroupDescriptionPtr> groupVector;
};

typedef std::shared_ptr<TabDescription> TabDescriptionPtr;

class ProductionSettings
{
public:
    ProductionSettings();
    ~ProductionSettings() = default;

    // Creating render globals attributes on "defaultRenderGlobals"
    static void
                CreateAttributes(std::map<std::string, std::string>* pMapCtrlCreationForTabs = nullptr);
    static void ClearUsdCameraAttributes();
    static void ApplySettings(HdRenderDelegate* renderDelegate);

    static void CheckRenderGlobals();
    static void UsdCameraListRefresh();
    static void OnSceneCallback(void*);

    static void RegisterCallbacks();
    static void UnregisterCallbacks();

    static UsdPrim GetUsdCameraPrim();
    static bool    IsUSDCameraToUse();

    static void CheckUnsupportedAttributeAndDisplayWarning(
        const std::string&       attrName,
        const VtValue&           value,
        const MFnDependencyNode& depNode);

private:
    static void attributeChangedCallback(
        MNodeMessage::AttributeMessage msg,
        MPlug&                         plug,
        MPlug&                         otherPlug,
        void*                          clientData);
    static void nodeAddedCallback(MObject& node, void* pData);

    static void OnBeforeOpenCallback(void*);

    static void MakeAttributeLogicalStructure();
    static void
    AddAttributeToGroupIfExist(GroupDescriptionPtr groupPtr, const std::string& attrSchemaName, bool visibleAsCtrl = true, const std::string& patchedDispplayName = "");

private:
    static MCallbackId _newSceneCallback;
    static MCallbackId _openSceneCallback;
    static MCallbackId _importSceneCallback;

    static MCallbackId _beforeOpenSceneCallback;
    static MCallbackId _nodeAddedCallback;

    static bool _usdCameraListRefreshed;
    static bool _isOpeningScene;

    static std::map<std::string, HdRenderSettingDescriptorPtr> _attributeMap;
    static std::vector<TabDescriptionPtr>                 _tabsLogicalStructure;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PRODUCTION_SETTINGS_H
