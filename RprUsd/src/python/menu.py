import maya.cmds
import maya.mel
import os
import maya.mel as mel

def showRPRMaterialXLibrary(value):
    import rprMaterialXBrowser
    rprMaterialXBrowser.show()

def createRprUsdMenu():
    if not maya.cmds.menu("rprUsdMenuCtrl", exists=1):
        gMainWindow = "MayaWindow"
        rprUsdMenuCtrl = maya.cmds.menu("rprUsdMenuCtrl", label="RPR USD", p=gMainWindow)
        maya.cmds.menuItem("materialXLibraryCtrl", label="MaterialX Library", p=rprUsdMenuCtrl, c=showRPRMaterialXLibrary)
