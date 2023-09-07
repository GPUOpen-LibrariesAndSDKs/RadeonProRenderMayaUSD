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

#include "RprUsdSetIBLCmd.h"

#include "common.h"

#include <maya/MGlobal.h>

#pragma warning(push, 0)
#include <pxr/usd/usd/references.h>
#include <pxr/usd/usdGeom/xform.h>
#include <pxr/usd/usdLux/domeLight.h>
#pragma warning(pop)

PXR_NAMESPACE_OPEN_SCOPE

MString RprUsdSetIBLCmd::s_commandName = "rprUsdSetIBL";

RprUsdSetIBLCmd::RprUsdSetIBLCmd() { }

// MPxCommand Implementation
// -----------------------------------------------------------------------------
void* RprUsdSetIBLCmd::creator() { return new RprUsdSetIBLCmd; }

// -----------------------------------------------------------------------------
MSyntax RprUsdSetIBLCmd::newSyntax()
{
    MSyntax syntax;

    CHECK_MSTATUS(syntax.addFlag(kNameFlag, kNameFlagLong, MSyntax::kString));

    return syntax;
}

// -----------------------------------------------------------------------------
MStatus RprUsdSetIBLCmd::doIt(const MArgList& args)
{
    // Parse arguments.
    MArgDatabase argData(syntax(), args);

    MString iblName;
    if (argData.isFlagSet(kNameFlag)) {
        argData.getFlagArgument(kNameFlag, 0, iblName);
    } else {
        MGlobal::displayWarning("RprUsd: IBL is not set, please provide a name!");
        return MStatus::kFailure;
    }

    UsdStageRefPtr stage = GetUsdStage();

    if (!stage) {
        MGlobal::displayError("RprUsd: USD stage does not exist!");
        return MStatus::kFailure;
    }

    SdfPath primRootPrimPath = SdfPath("/RenderStudioPrimitives");

    UsdPrim primRootPrim = UsdGeomXform::Define(stage, primRootPrimPath).GetPrim();

    if (primRootPrim.IsValid()) {
        primRootPrim.SetActive(true);

        SdfPath childrenPath
            = primRootPrim.GetPath().AppendChild(TfToken { TfMakeValidIdentifier("Environment") });
        UsdPrim envPrim = stage->DefinePrim(childrenPath);

        if (stage->GetPrimAtPath(childrenPath).IsValid()) {
            stage->RemovePrim(childrenPath);
            envPrim = stage->DefinePrim(childrenPath);
            UsdLuxDomeLight(envPrim).OrientToStageUpAxis();
        }

        if (envPrim.IsValid()) {
            UsdReferences references = envPrim.GetReferences();
            references.ClearReferences();
            references.AddReference(SdfReference(("storage://" + iblName).asChar()));
        }
    }

    return MStatus::kSuccess;
}

// Static Methods
// -----------------------------------------------------------------------------
void RprUsdSetIBLCmd::cleanUp() { }

void RprUsdSetIBLCmd::Initialize() { }

void RprUsdSetIBLCmd::Uninitialize() { }

PXR_NAMESPACE_CLOSE_SCOPE
