#include "RprUsdSetIBLCmd.h"
#include "common.h"
#include <maya/MGlobal.h>


#include <pxr/usd/usd/references.h>


PXR_NAMESPACE_OPEN_SCOPE

MString RprUsdSetIBLCmd::s_commandName = "rprUsdSetIBL";

RprUsdSetIBLCmd::RprUsdSetIBLCmd()
{

}

// MPxCommand Implementation
// -----------------------------------------------------------------------------
void* RprUsdSetIBLCmd::creator()
{
	return new RprUsdSetIBLCmd;
}

// -----------------------------------------------------------------------------
MSyntax RprUsdSetIBLCmd::newSyntax()
{
	MSyntax syntax;
	
	CHECK_MSTATUS(syntax.addFlag(kNameFlag, kNameFlagLong, MSyntax::kString));

	return syntax;
}

// -----------------------------------------------------------------------------
MStatus RprUsdSetIBLCmd::doIt(const MArgList & args)
{
	// Parse arguments.
	MArgDatabase argData(syntax(), args);

	MString iblName;
	if (argData.isFlagSet(kNameFlag)) {
		argData.getFlagArgument(kNameFlag, 0, iblName);
	}
	else {
		MGlobal::displayWarning("RprUsd: IBL is not set, please provide a name!");
		return MS::kFailure;
	}

	UsdStageRefPtr stage = GetUsdStage();
	
	if (!stage)
	{
		MGlobal::displayError("RprUsd: USD stage does not exist!");
		return MS::kFailure;
	}

	SdfPath envRootPrimPath = SdfPath("/RenderStudioPrimitives");

	UsdPrim envRootPrim = stage->GetPrimAtPath(envRootPrimPath);
	
	if (!envRootPrim.IsValid()) {
		envRootPrim = stage->DefinePrim(envRootPrimPath, TfToken("Xform"));
	}

	if (envRootPrim.IsValid()) {
		envRootPrim.SetActive(true);
		UsdReferences references = envRootPrim.GetReferences();
		references.ClearReferences();
		references.AddReference(SdfReference(("storage://" + iblName).asChar()));
	}
}

// Static Methods
// -----------------------------------------------------------------------------
void RprUsdSetIBLCmd::cleanUp()
{
}

void RprUsdSetIBLCmd::Initialize()
{

}

void RprUsdSetIBLCmd::Uninitialize()
{

}

PXR_NAMESPACE_CLOSE_SCOPE
