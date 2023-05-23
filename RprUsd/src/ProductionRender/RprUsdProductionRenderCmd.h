#ifndef __RPRUSDPRODUCTIONRENDERCMD__
#define __RPRUSDPRODUCTIONRENDERCMD__

#include <maya/MPxCommand.h>
#include <maya/MSyntax.h>
#include <maya/MArgDatabase.h>
#include <maya/MDagPath.h>
#include <maya/MCommonRenderSettingsData.h>

#include <pxr/pxr.h>

// Command arguments.
#define kCameraFlag "-cam"
#define kCameraFlagLong "-camera"
#define kRenderLayerFlag "-l"
#define kRenderLayerFlagLong "-layer"
#define kWidthFlag "-w"
#define kWidthFlagLong "-width"
#define kHeightFlag "-h"
#define kHeightFlagLong "-height"

#define kWaitForIt "-wfi"
#define kWaitForItLong "-waitForIt"

#define kWaitForItTwoStep "-wft"
#define kWaitForItTwoStepLong "-waitForItTwo"

// Misc flag. Its not related to rendering itself
#define kUSDCameraListRefreshFlag "-ucr"
#define kUSDCameraListRefreshFlagLong "-usdCameraListRefresh"

PXR_NAMESPACE_OPEN_SCOPE

class RprUsdProductionRender;

/** Perform a single frame */
class RprUsdProductionRenderCmd : public MPxCommand
{
public:
	RprUsdProductionRenderCmd();
	// MPxCommand Implementation
	// -----------------------------------------------------------------------------

	/**
	 * Perform a render command to either start or
	 * communicate with a frame, IPR or batch render.
	 */
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

	static void RegisterEnvVariables();

	static void Uninitialize();

public:
	static MString s_commandName;

private:
	static std::unique_ptr<RprUsdProductionRender> s_productionRender;

	
 /* Set in case we need to wait for single frame render to be finish,
 * and we still want to use the built in render function call hierarchy
 * (for ep.: renderIntoNewWindow)
 * so wait for it flag and actual rendering is in two separate calls.
 */
	static bool s_waitForIt;

};

PXR_NAMESPACE_CLOSE_SCOPE

#endif //__RPRUSDPRODUCTIONRENDERCMD__
