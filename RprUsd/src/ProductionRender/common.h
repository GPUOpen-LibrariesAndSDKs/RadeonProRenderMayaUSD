#ifndef HDRPR_COMMON_H
#define HDRPR_COMMON_H

#include <string>

#include <pxr/base/gf/vec4f.h>
#include <pxr/base/tf/token.h>
#include <pxr/base/vt/value.h>
#include <pxr/pxr.h>
#include <pxr/imaging/hd/renderDelegate.h>
#include <pxr/usd/usd/primRange.h>
#include <mayaUsd/nodes/proxyShapeBase.h>

std::string GetRendererName();

MayaUsdProxyShapeBase* GetMayaUsdProxyShapeBase();
UsdStageRefPtr GetUsdStage();

MObject GetSettingsNode();

#endif
