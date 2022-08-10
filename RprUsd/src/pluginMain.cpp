#include <maya/MFnPlugin.h>
#include <maya/MGlobal.h>

#include "version.h"


PLUGIN_EXPORT MStatus initializePlugin(MObject obj)
{
    MStatus ret = MS::kSuccess;

    MFnPlugin plugin(obj, "AMD", PLUGIN_VERSION, "Any");

    MGlobal::executePythonCommand("import menu\ncreateFireRenderMenu()");

    return ret;
}

PLUGIN_EXPORT MStatus uninitializePlugin(MObject obj)
{
    MFnPlugin plugin(obj, "AMD", PLUGIN_VERSION, "Any");

    MStatus ret = MS::kSuccess;

    return ret; 
} 
