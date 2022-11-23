#include "RprUsdBindMtlxCmd.h"
#include "common.h"
#include <maya/MGlobal.h>


#include <pxr/usd/usd/references.h>
#include <pxr/usd/usdShade/materialBindingAPI.h>



PXR_NAMESPACE_OPEN_SCOPE

MString RprUsdBiodMtlxCmd::s_commandName = "rprUsdBindMtlx";

RprUsdBiodMtlxCmd::RprUsdBiodMtlxCmd()
{

}

// MPxCommand Implementation
// -----------------------------------------------------------------------------
void* RprUsdBiodMtlxCmd::creator()
{
	return new RprUsdBiodMtlxCmd;
}

// -----------------------------------------------------------------------------
MSyntax RprUsdBiodMtlxCmd::newSyntax()
{
	MSyntax syntax;

	CHECK_MSTATUS(syntax.addFlag(kPrimPathFlag, kPrimPathFlagLong, MSyntax::kString));
	CHECK_MSTATUS(syntax.addFlag(kMtlxFilePathFlag, kMtlxFilePathFlagLong, MSyntax::kString));
	CHECK_MSTATUS(syntax.addFlag(kMaterialNameFlag, kMaterialNameFlagLong, MSyntax::kString));

	return syntax;
}

// -----------------------------------------------------------------------------
MStatus RprUsdBiodMtlxCmd::doIt(const MArgList & args)
{
	// Parse arguments.
	MArgDatabase argData(syntax(), args);

	MString primPath;
	if (argData.isFlagSet(kPrimPathFlag))
	{
		argData.getFlagArgument(kPrimPathFlag, 0, primPath);
	}
	else
	{
		MGlobal::displayError("RprUsd: primPath is not defined");
		return MS::kFailure;
	}

	MString mtlxFileName;
	if (argData.isFlagSet(kMtlxFilePathFlag))
	{
		argData.getFlagArgument(kMtlxFilePathFlag, 0, mtlxFileName);
	}
	else
	{
		MGlobal::displayError("RprUsd: mtlxFileName is not defined");
		return MS::kFailure;
	}

	MString materialName;

	if (argData.isFlagSet(kMaterialNameFlag))
	{
		argData.getFlagArgument(kMaterialNameFlag, 0, materialName);
	}
	else
	{
		MGlobal::displayError("RprUsd: materialName is not defined");
	}

	// TODO temp code
	if (materialName.isEmpty())
	{
		materialName = "MaterialX_Graph";
	}

	SdfReference sdfRef = SdfReference(mtlxFileName.asChar(), SdfPath("/MaterialX"));
	//SdfReference sdfRef = SdfReference("C:\\Projects\\Scenes\\WebMatXFolder\\Stone_Wall_Combination_1k_8b\\Stone_Wall_Combination.mtlx", SdfPath("/MaterialX"));

	//SdfPath path = SdfPath("/pSphere1");
	SdfPath path = SdfPath(primPath.asChar());

	UsdStageRefPtr stage = GetUsdStage();

	if (!stage)
	{
		MGlobal::displayError("RprUsd: USD stage does not exist!");
		return MS::kFailure;
	}

	UsdPrim prim = stage->GetPrimAtPath(path);

	UsdReferences primRefs = prim.GetReferences();
	primRefs.AddReference(sdfRef);

	SdfPath materialPath = SdfPath((primPath + "/Materials/" + materialName).asChar());
	SdfPath previousMaterialPath;

	UsdShadeMaterial material(stage->GetPrimAtPath(materialPath));
	if (prim.IsValid() && material) {
		UsdShadeMaterialBindingAPI bindingAPI;
		if (prim.HasAPI<UsdShadeMaterialBindingAPI>()) {
			bindingAPI = UsdShadeMaterialBindingAPI(prim);
			previousMaterialPath = bindingAPI.GetDirectBinding().GetMaterialPath();
		}
		else {
			bindingAPI = UsdShadeMaterialBindingAPI::Apply(prim);
		}

		bindingAPI.Bind(material);
	}
	else
	{
		MGlobal::displayError(MString("RprUsd: Cannot bind prim ") + primPath + " with material " + materialPath.GetString().c_str());
		return MS::kFailure;
	}

	return MStatus::kSuccess;
}

// Static Methods
// -----------------------------------------------------------------------------
void RprUsdBiodMtlxCmd::cleanUp()
{
}

void RprUsdBiodMtlxCmd::Initialize()
{

}

void RprUsdBiodMtlxCmd::Uninitialize()
{

}

PXR_NAMESPACE_CLOSE_SCOPE
