#ifndef __RPRUSDBINDMTLXCMD__
#define __RPRUSDBINDMTLXCMD__

#include <maya/MPxCommand.h>
#include <maya/MSyntax.h>
#include <maya/MArgDatabase.h>

#include <pxr/pxr.h>

// Command arguments.
// required
#define kPrimPathFlag "-pp"
#define kPrimPathFlagLong "-primPath"

// required
#define kMtlxFilePathFlag "-mp"
#define kMtlxFilePathFlagLong "-mtlxFilePath"

// optional
#define kMaterialNameFlag "-mn"
#define kMaterialNameFlagLong "-materialName"

PXR_NAMESPACE_OPEN_SCOPE

class RprUsdProductionRender;

/** Perform a single frame */
class RprUsdBiodMtlxCmd : public MPxCommand
{
public:
	RprUsdBiodMtlxCmd();
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

#endif //__RPRUSDBINDMTLXCMD__
