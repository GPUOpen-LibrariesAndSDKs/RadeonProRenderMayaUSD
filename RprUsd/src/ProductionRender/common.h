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

#ifndef HDRPR_COMMON_H
#define HDRPR_COMMON_H

#include <string>

#pragma warning(push, 0)

#include <mayaUsd/nodes/proxyShapeBase.h>

#include <pxr/base/gf/vec4f.h>
#include <pxr/base/tf/token.h>
#include <pxr/base/vt/value.h>
#include <pxr/imaging/hd/renderDelegate.h>
#include <pxr/pxr.h>
#include <pxr/usd/usd/primRange.h>

#include <maya/MFnEnumAttribute.h>
#include <maya/MGlobal.h>

#pragma warning(pop)

std::string GetRendererName();

MayaUsdProxyShapeBase* GetMayaUsdProxyShapeBase();
UsdStageRefPtr         GetUsdStage();

MObject GetSettingsNode();

template <typename T> bool _SetOptionVar(const MString& attrName, const T& value)
{
    return MGlobal::setOptionVarValue(attrName, value);
}

bool _SetOptionVar(const MString& attrName, const bool& value);
bool _SetOptionVar(const MString& attrName, const float& value);
bool _SetOptionVar(const MString& attrName, const TfToken& value);
bool _SetOptionVar(const MString& attrName, const std::string& value);
bool _SetOptionVar(const MString& attrName, const SdfAssetPath& value);
bool _SetOptionVar(const MString& attrName, const TfEnum& value);


template <typename T> void _GetFromPlug(const MPlug& plug, T& out) { assert(false); }
template <> void _GetFromPlug<bool>(const MPlug& plug, bool& out);
template <> void _GetFromPlug<int>(const MPlug& plug, int& out);
template <> void _GetFromPlug<float>(const MPlug& plug, float& out);
template <> void _GetFromPlug<std::string>(const MPlug& plug, std::string& out);
template <> void _GetFromPlug<TfEnum>(const MPlug& plug, TfEnum& out);
template <> void _GetFromPlug<SdfAssetPath>(const MPlug& plug, SdfAssetPath& out);
template <> void _GetFromPlug<TfToken>(const MPlug& plug, TfToken& out);


template <typename T>
bool _GetAttribute(
    const MFnDependencyNode& node,
    const MString& attrName,
    T& out,
    bool                     storeUserSetting)
{
    const auto plug = node.findPlug(attrName, true);
    if (plug.isNull()) {
        return false;
    }
    _GetFromPlug<T>(plug, out);

    if (storeUserSetting) {
        _SetOptionVar(attrName, out);
    }

    return true;
}


#endif
