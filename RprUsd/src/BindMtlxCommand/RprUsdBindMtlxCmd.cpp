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

#include "RprUsdBindMtlxCmd.h"

#include "common.h"

#include <maya/MGlobal.h>

#pragma warning(push, 0)

#include <pxr/usd/usd/references.h>
#include <pxr/usd/usdShade/materialBindingAPI.h>

#pragma warning(pop)

#include <MaterialXCore/Document.h>
#include <MaterialXFormat/XmlIo.h>

PXR_NAMESPACE_OPEN_SCOPE

MString RprUsdBiodMtlxCmd::s_commandName = "rprUsdBindMtlx";

RprUsdBiodMtlxCmd::RprUsdBiodMtlxCmd() { }

// MPxCommand Implementation
// -----------------------------------------------------------------------------
void* RprUsdBiodMtlxCmd::creator() { return new RprUsdBiodMtlxCmd; }

// -----------------------------------------------------------------------------
MSyntax RprUsdBiodMtlxCmd::newSyntax()
{
    MSyntax syntax;

    CHECK_MSTATUS(syntax.addFlag(kPrimPathFlag, kPrimPathFlagLong, MSyntax::kString));
    CHECK_MSTATUS(syntax.addFlag(kMtlxFilePathFlag, kMtlxFilePathFlagLong, MSyntax::kString));
    CHECK_MSTATUS(syntax.addFlag(kMaterialNameFlag, kMaterialNameFlagLong, MSyntax::kString));

    CHECK_MSTATUS(
        syntax.addFlag(kClearAllReferencesFlag, kClearAllReferencesFlagLong, MSyntax::kNoArg));

    // LiveMode
    CHECK_MSTATUS(syntax.addFlag(kLiveModeFlag, kLiveModeFlagLong, MSyntax::kNoArg));
    CHECK_MSTATUS(syntax.addFlag(kGpuOpenMatIdFlag, kGpuOpenMatIdFlagLong, MSyntax::kString));

    return syntax;
}

void AddColorSpaceAttribute(UsdShadeMaterial& material)
{
    // Make colorspace hack
    UsdPrimSubtreeRange range = material.GetPrim().GetDescendants();
    for (const auto& prim : range) {
        // Work only with shaders
        if (!prim.IsA<UsdShadeShader>()) {
            continue;
        }

        // Find colorspace attribute
        for (const auto& attribute : prim.GetAttributes()) {
            if (!attribute.HasColorSpace()) {
                continue;
            }
            prim.CreateAttribute(TfToken { "inputs:rs:colorspace" }, SdfValueTypeNames->String)
                .Set(attribute.GetColorSpace().GetString());
        }
    }
}

MStatus AssignMatXMaterial(
    UsdStageRefPtr      stage,
    const MString&      primPath,
    const SdfReference& sdfRef,
    const MString&      inMaterialName)
{
    SdfPath       path = SdfPath(primPath.asChar());
    UsdPrim       prim = stage->GetPrimAtPath(path);
    UsdReferences primRefs = prim.GetReferences();

    primRefs.ClearReferences();
    primRefs.AddReference(sdfRef);

    MString materialName = inMaterialName;

    UsdPrim materialPrim;
    if (materialName == "") {
        UsdPrim appendedPrim = stage->GetPrimAtPath(path.AppendChild(TfToken{ "Materials" }));

        if (!appendedPrim.IsValid()) {
            MGlobal::displayError("RprUsd: 'Materials' child was not created!");
            return MStatus::kFailure;
        }

        materialPrim = appendedPrim.GetChildren().front();
    } else {
        materialPrim
            = stage->GetPrimAtPath(SdfPath((primPath + "/Materials/" + materialName).asChar()));
    }

    UsdShadeMaterial material(materialPrim);
    if (material) {
        UsdShadeMaterialBindingAPI bindingAPI;
        if (prim.HasAPI<UsdShadeMaterialBindingAPI>()) {
            bindingAPI = UsdShadeMaterialBindingAPI(prim);
            bindingAPI.UnbindAllBindings();
        } else {
            bindingAPI = UsdShadeMaterialBindingAPI::Apply(prim);
        }

        bindingAPI.Bind(material, UsdShadeTokens->strongerThanDescendants);

        AddColorSpaceAttribute(material);
    } else {
        MGlobal::displayError(
            MString("RprUsd: Cannot bind prim ") + primPath + " with material "
            + materialName.asChar());
        return MS::kFailure;
    }

    MGlobal::displayInfo("RprUsd: MaterialX applied to prim!");
    return MStatus::kSuccess;
}

// -----------------------------------------------------------------------------
MStatus RprUsdBiodMtlxCmd::doIt(const MArgList& args)
{
    // Parse arguments.
    MArgDatabase argData(syntax(), args);

    MString primPath;
    if (argData.isFlagSet(kPrimPathFlag)) {
        argData.getFlagArgument(kPrimPathFlag, 0, primPath);
    }

    if (primPath.isEmpty()) {
        MGlobal::displayError("RprUsd: primPath is not defined");
        return MS::kFailure;
    }

    SdfPath path = SdfPath(primPath.asChar());

    UsdStageRefPtr stage = GetUsdStage();

    if (!stage) {
        MGlobal::displayError("RprUsd: USD stage does not exist!");
        return MS::kFailure;
    }

    UsdPrim prim = stage->GetPrimAtPath(path);

    if (!prim.IsValid()) {
        MGlobal::displayError("RprUsd: Prim is not valid!");
        return MS::kFailure;
    }

    const std::string& primTypeName = prim.GetTypeName().GetString();
    if (primTypeName != "Mesh") {
        MGlobal::displayError("RprUsd: Selected prim is not a mesh !");
        return MS::kFailure;
    }

    UsdReferences primRefs = prim.GetReferences();

    // If ClearAllReferences flag is set
    if (argData.isFlagSet(kClearAllReferencesFlag)) {
        primRefs.ClearReferences();
        return MStatus::kSuccess;
    }

    if (argData.isFlagSet(kLiveModeFlag)) {
        if (argData.isFlagSet(kGpuOpenMatIdFlag)) {
            MString id;
            argData.getFlagArgument(kGpuOpenMatIdFlag, 0, id);
            SdfReference sdfRef = SdfReference(("gpuopen://" + id).asChar(), SdfPath("/MaterialX"));

            return AssignMatXMaterial(stage, primPath, sdfRef, "");
        } else {
            MGlobal::displayError("RprUsd: -id parameter is required for MatX Bind in LiveMode");
            return MS::kFailure;
        }
    }

    // otherwise assing new material

    MString mtlxFileName;
    if (argData.isFlagSet(kMtlxFilePathFlag)) {
        argData.getFlagArgument(kMtlxFilePathFlag, 0, mtlxFileName);
    }

    if (mtlxFileName.isEmpty()) {
        MGlobal::displayError("RprUsd: mtlxFileName is not defined");
        return MS::kFailure;
    }

    MString materialName;

    if (argData.isFlagSet(kMaterialNameFlag)) {
        argData.getFlagArgument(kMaterialNameFlag, 0, materialName);
    }

    if (materialName.isEmpty()) {
        MaterialX::DocumentPtr mtlxDocumentPtr = MaterialX::createDocument();

        try {
            MaterialX::readFromXmlFile(mtlxDocumentPtr, mtlxFileName.asChar());
        } catch (const MaterialX::ExceptionParseError&) {
            MGlobal::displayError("RprUsd: mtlxFileName cannot be parsed");
            return MS::kFailure;
        } catch (const MaterialX::ExceptionFileMissing&) {
            MGlobal::displayError("RprUsd: mtlxFileName does not exist");
            return MS::kFailure;
        }

        std::vector<MaterialX::NodePtr> materialVector = mtlxDocumentPtr->getMaterialNodes();

        if (materialVector.empty()) {
            MGlobal::displayError("RprUsd: mtlxFileName does not contain amy materials");
            return MS::kFailure;
        }

        MaterialX::NodePtr output = materialVector[0];

        materialName = output->getName().c_str();
    }

    if (materialName.isEmpty()) {
        MGlobal::displayError("RprUsd: materialName is not defined");
        return MS::kFailure;
    }

    SdfReference sdfRef = SdfReference(mtlxFileName.asChar(), SdfPath("/MaterialX"));

    return AssignMatXMaterial(stage, primPath, sdfRef, materialName);
}

// Static Methods
// -----------------------------------------------------------------------------
void RprUsdBiodMtlxCmd::cleanUp() { }

void RprUsdBiodMtlxCmd::Initialize() { }

void RprUsdBiodMtlxCmd::Uninitialize() { }

PXR_NAMESPACE_CLOSE_SCOPE
