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

#include <pxr/base/gf/vec4f.h>
#include <pxr/base/tf/token.h>
#include <pxr/base/vt/value.h>
#include <pxr/pxr.h>
#include <pxr/imaging/hd/renderDelegate.h>
#include <pxr/usd/usd/primRange.h>
#include <mayaUsd/nodes/proxyShapeBase.h>

#pragma warning(pop)

std::string GetRendererName();

MayaUsdProxyShapeBase* GetMayaUsdProxyShapeBase();
UsdStageRefPtr GetUsdStage();

MObject GetSettingsNode();

#endif
