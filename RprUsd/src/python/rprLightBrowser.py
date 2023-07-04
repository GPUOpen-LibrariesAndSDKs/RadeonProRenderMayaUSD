import maya.cmds as cmds
import maya.mel as mel
import os
import json

def show() :
    RPRLightBrowser().show()

# A material browser window for Radeon Pro Render materials.
# -----------------------------------------------------------------------------
class RPRLightBrowser(object) :

    # Constructor.
    # -----------------------------------------------------------------------------
    def __init__(self, *args) :
        pass

    # Show the material browser.
    # -----------------------------------------------------------------------------
    def show(self) :

        #load all material at one time. It's possible to rework to load in chunks if needs           	     		
        self.createLayout()

    def createLayout(self) :
        
        windowName = "RPRLightBrowserWindow"
        # Delete any existing window.
        if (cmds.window(windowName, exists = True)) :
            cmds.deleteUI(windowName)

        # Create a new window.
        self.window = cmds.window(windowName,
                                  widthHeight=(512, 512),
                                  title="Radeon ProRender Light Browser")

#        if (cmds.layout("RPRMaterialsFlow", exists=True)) :
#            cmds.deleteUI("RPRMaterialsFlow", layout=True)

        # Ensure that the material container is the current parent.
        cmds.setParent(self.window)

        self.iconSize = 128

        self.cellWidth = self.iconSize + 10
        self.cellHeight = self.iconSize + 30

        # Create the new flow layout.
        cmds.flowLayout("RPRMaterialsFlow", columnSpacing=0, wrap=True, width = 3 * self.cellWidth, height = 3 * self.cellHeight)

        self.lights = list()

        for i in range(9) :
            light = dict()
            light["name"] = i
            self.lights.append(light)

            # Vertical layout for large icons.
        for light in self.lights : 
            cmds.columnLayout(width=self.cellWidth, height=self.cellHeight)
            cmds.iconTextButton(style='iconOnly', image="", width=self.iconSize, height=self.iconSize)
            cmds.text(label=light["name"], align="center", width=self.iconSize)
            cmds.setParent('..')

         # Show the material browser window.
        cmds.showWindow(self.window)
