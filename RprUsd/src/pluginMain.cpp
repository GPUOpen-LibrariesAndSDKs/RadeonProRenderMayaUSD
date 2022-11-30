#include <maya/MFnPlugin.h>
#include <maya/MGlobal.h>

#include "version.h"

#include "ProductionRender\RprUsdProductionRenderCmd.h"

PXR_NAMESPACE_USING_DIRECTIVE

PLUGIN_EXPORT MStatus initializePlugin(MObject obj)
{
    MStatus ret = MS::kSuccess;

    MFnPlugin plugin(obj, "AMD", PLUGIN_VERSION, "Any");

    MGlobal::executePythonCommand("import menu\nmenu.createRprUsdMenu()");

	MStatus status = plugin.registerCommand(RprUsdProductionRenderCmd::s_commandName, RprUsdProductionRenderCmd::creator, RprUsdProductionRenderCmd::newSyntax);
	CHECK_MSTATUS(status);
	RprUsdProductionRenderCmd::Initialize();

    return ret;
}

PLUGIN_EXPORT MStatus uninitializePlugin(MObject obj)
{
    MFnPlugin plugin(obj, "AMD", PLUGIN_VERSION, "Any");

    MStatus ret = MS::kSuccess; 

	plugin.deregisterCommand(RprUsdProductionRenderCmd::s_commandName);

    return ret;
}
