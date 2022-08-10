import maya.cmds
import maya.mel
import os
import maya.mel as mel

def createFireRenderMenu():
    gMainWindow = "MayaWindow";
    rprUsdMenuItem = maya.cmds.menuItem("FrRPRMaterialLibrary", label="RPR USD", p=gMainWindow)
    maya.cmds.menuItem("FrRPRMaterialLibrary", label="showRPRMaterialXLibrary", p=rprUsdMenuItem, c=showRPRMaterialXLibrary)
