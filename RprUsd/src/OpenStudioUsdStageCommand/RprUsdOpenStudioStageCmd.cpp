#include "RprUsdOpenStudioStageCmd.h"
#include "common.h"

#include <filesystem>
#include <maya/MGlobal.h>

#include "../RenderStudioResolverHelper.h"

PXR_NAMESPACE_OPEN_SCOPE

MString RprUsdOpenStudioStageCmd::s_commandName = "rprUsdOpenStudioStage";

RprUsdOpenStudioStageCmd::RprUsdOpenStudioStageCmd()
{

}

// MPxCommand Implementation
// -----------------------------------------------------------------------------
void* RprUsdOpenStudioStageCmd::creator()
{
	return new RprUsdOpenStudioStageCmd;
}

// -----------------------------------------------------------------------------
MSyntax RprUsdOpenStudioStageCmd::newSyntax()
{
	MSyntax syntax;

	CHECK_MSTATUS(syntax.addFlag(kFilePathFlag, kFilePathFlagLong, MSyntax::kString));

	return syntax;
}

// -----------------------------------------------------------------------------
MStatus RprUsdOpenStudioStageCmd::doIt(const MArgList & args)
{
	// Parse arguments.
	MArgDatabase argData(syntax(), args);

	std::string filePath;
	if (argData.isFlagSet(kFilePathFlag))
	{
		MString path;
		argData.getFlagArgument(kFilePathFlag, 0, path);
		filePath = path.asChar();
	}
	
	if (filePath.empty())
	{
		MGlobal::displayError("RprUsd: RprUsdOpenStudioStageCmd: filePath is not defined");
		return MS::kFailure;
	}

	if (RenderStudioResolverHelper::IsRenderStudioPath(filePath)) {
		std::string studioFilePath = RenderStudioResolverHelper::Unresolve(filePath);

		MString cmd = MString("RprUsdDoCreateStage(\"") + studioFilePath.c_str() + "\")";

		MGlobal::executeCommand(cmd);

		LiveModeInfo liveModeInfo;
		liveModeInfo.liveUrl = "wss://renderstudio.luxoft.com/livecpp";
		liveModeInfo.channelId = "Maya";
		liveModeInfo.userId = "MayaUser";

		RenderStudioResolverHelper::StartLiveMode(liveModeInfo);
	}
	else {
		MGlobal::displayError("RprUsd: RprUsdOpenStudioStageCmd: wrong filePath is specified");
		return MS::kFailure;
	}

	MGlobal::displayInfo("RprUsd: usd stage for synchronization is opened");
	return MStatus::kSuccess;
}

// Static Methods
// -----------------------------------------------------------------------------
void RprUsdOpenStudioStageCmd::cleanUp()
{
}

void RprUsdOpenStudioStageCmd::Initialize()
{

}

void RprUsdOpenStudioStageCmd::Uninitialize()
{

}

PXR_NAMESPACE_CLOSE_SCOPE
