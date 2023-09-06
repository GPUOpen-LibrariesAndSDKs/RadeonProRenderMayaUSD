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
