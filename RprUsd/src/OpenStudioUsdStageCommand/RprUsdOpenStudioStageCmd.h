#ifndef __RPRUSDOPENSTUDIOSTAGECMD__
#define __RPRUSDOPENSTUDIOSTAGECMD__

#include <maya/MPxCommand.h>
#include <maya/MSyntax.h>
#include <maya/MArgDatabase.h>

#include <pxr/pxr.h>

// Command arguments.
// required
#define kFilePathFlag "-f"
#define kFilePathFlagLong "-filePath"


PXR_NAMESPACE_OPEN_SCOPE

/** Perform a single frame */
class RprUsdOpenStudioStageCmd : public MPxCommand
{
public:
	RprUsdOpenStudioStageCmd();
	// MPxCommand Implementation
	// -----------------------------------------------------------------------------

	MStatus doIt(const MArgList& args) override;

	/** Used by Maya to create the command instance. */
	static void* creator();

	/** Return the command syntax object. */
	static MSyntax newSyntax();

	// Static Methods
	// -----------------------------------------------------------------------------

	/** Clean up before plug-in shutdown. */
	static void cleanUp();

	static void Initialize();

	static void Uninitialize();

public:
	static MString s_commandName;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif //__RPRUSDOPENSTUDIOSTAGECMD__
