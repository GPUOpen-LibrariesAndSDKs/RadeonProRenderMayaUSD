#include "RprUsdOpenStudioStageCmd.h"
#include "common.h"
#include "../RenderStudioResolverHelper.h"

#include <filesystem>
#include <maya/MGlobal.h>

#include <combaseapi.h>

PXR_NAMESPACE_OPEN_SCOPE

MString RprUsdOpenStudioStageCmd::s_commandName = "rprUsdOpenStudioStage";
MString RprUsdOpenStudioStageCmd::s_LastRencetUsedFilePath = "";

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
	CHECK_MSTATUS(syntax.addFlag(kGetRecentFilePathUsedFlag, kGetRecentFilePathUsedFlagLong, MSyntax::kString));

	return syntax;
}

std::string GenerateGUID()
{
	GUID guid;
	HRESULT hCreateGuid = CoCreateGuid(&guid);

	char szGuid[64] = { 0 };

	sprintf(szGuid, "%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X", guid.Data1, guid.Data2, guid.Data3, guid.Data4[0], guid.Data4[1], 
							guid.Data4[2], guid.Data4[3], guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);

	return szGuid;
}

// -----------------------------------------------------------------------------
MStatus RprUsdOpenStudioStageCmd::doIt(const MArgList & args)
{
	// Parse arguments.
	MArgDatabase argData(syntax(), args);

	if (argData.isFlagSet(kGetRecentFilePathUsedFlag))
	{
		setResult(s_LastRencetUsedFilePath);
		return MStatus::kSuccess;
	}

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

	if (RenderStudioResolverHelper::IsUnresovableToRenderStudioPath(filePath)) {
		std::string studioFilePath = RenderStudioResolverHelper::Unresolve(filePath);

		MString cmd = MString("RprUsdDoCreateStage(\"") + studioFilePath.c_str() + "\")";

		MGlobal::executeCommand(cmd);

		LiveModeInfo liveModeInfo;
		liveModeInfo.liveUrl = "wss://renderstudio.luxoft.com/livecpp";
		liveModeInfo.channelId = "Maya";

		MPlug channelNamePlug = MFnDependencyNode(GetSettingsNode()).findPlug("HdRprPlugin_LiveModeChannelName", false);
		if (!channelNamePlug.isNull()){
			MString channelName = channelNamePlug.asString();
			if (channelName != "") {
				liveModeInfo.channelId = channelName.asChar();
			}
		}

		liveModeInfo.userId = "MayaUser_" + GenerateGUID();

		RenderStudioResolverHelper::StartLiveMode(liveModeInfo);

		s_LastRencetUsedFilePath = filePath.c_str();
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
