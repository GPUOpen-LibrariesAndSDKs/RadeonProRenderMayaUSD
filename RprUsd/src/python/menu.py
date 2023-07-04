import maya.cmds
import maya.mel
import os
import maya.mel as mel
import ufe

import winreg
import subprocess

mel.eval("source loadUsdStageForSharing.mel")

def showRPRMaterialXLibrary(value) :
    import rprMaterialXBrowser
    rprMaterialXBrowser.show()

def BindMaterialXFromFile(value) :

    gsel = ufe.GlobalSelection.get()
    if gsel.empty() : 
        maya.OpenMaya.MGlobal.displayError("RprUsd: Nothing is selected in USD layout!")
        return

    optionVarNameRecentDirectory = "RprUsd_RecentBinxMatXDirectory"
    previousDirectoryUsed = ""
    if maya.cmds.optionVar(exists=optionVarNameRecentDirectory) :
        previousDirectoryUsed = maya.cmds.optionVar(query=optionVarNameRecentDirectory)
        
    ret = maya.cmds.fileDialog2(startingDirectory=previousDirectoryUsed, fm = 1, fileFilter="MaterialX (*.mtlx)")

    if ret is None :
        return

    filePath = ret[0]
    maya.cmds.optionVar(sv=(optionVarNameRecentDirectory, filePath))

    pathList = list(gsel)    
    while len(pathList) > 0 :
        selected_path = str(pathList.pop().path())
        selected_path = selected_path[selected_path.find("/"):len(selected_path)]
        maya.cmds.rprUsdBindMtlx(pp=selected_path, mp=filePath)

def createRprUsdMenu():
    if not maya.cmds.menu("rprUsdMenuCtrl", exists=1):
        gMainWindow = "MayaWindow"
        rprUsdMenuCtrl = maya.cmds.menu("rprUsdMenuCtrl", label="RPR USD", p=gMainWindow)
        maya.cmds.menuItem("materialXLibraryCtrl", label="MaterialX Library", p=rprUsdMenuCtrl, c=showRPRMaterialXLibrary)
        maya.cmds.menuItem("bindMaterialXCtrl",label="Bind MaterialX To Selected Mesh", p=rprUsdMenuCtrl, c=BindMaterialXFromFile)
        maya.cmds.menuItem("loadUsdForSharing",label="Load Usd Stage For Sharing", p=rprUsdMenuCtrl, c=LoadUsdStageForSharing)
        maya.cmds.menuItem("runRenderStudio",label="Run RenderStudio", p=rprUsdMenuCtrl, c=RunRenderStudio)

def LoadUsdStageForSharing(value):  
    mel.eval("RprUsd_CreateStageFromFile();")

def removeRprUsdMenu():
    if maya.cmds.menu("rprUsdMenuCtrl", exists=1):
        maya.cmds.deleteUI("rprUsdMenuCtrl")

def RunRenderStudio(value) :
    try:
        key = winreg.OpenKeyEx(winreg.HKEY_LOCAL_MACHINE, "SOFTWARE\\AMD\\RenderStudio")
        renderStudioExecPath = winreg.QueryValueEx(key, "ExecCmd")[0]
        
        filePath = maya.cmds.rprUsdOpenStudioStage(gr=1)
        subprocess.Popen([renderStudioExecPath, "--scene=" + filePath])
    except OSError:
        maya.cmds.confirmDialog(title="Cannot run Render Studio", message="Unable to run Render Studio. Make sure it is installed!", button="OK")
