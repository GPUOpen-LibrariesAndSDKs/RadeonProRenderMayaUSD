import maya.cmds as cmds
import maya.mel as mel
import os
import json
import urllib.request
import threading


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
        self.pathRootThumbnail = os.environ["USERPROFILE"] + "/Documents/Maya/RprUsd/WebMatlibCache"
        os.makedirs(self.pathRootThumbnail, exist_ok=True)

        self.baseUrl = "https://renderstudio.luxoft.com/storage/api/lights/";
        self.downloadMetadata()
        self.downloadThumbnails()

        #load all material at one time. It's possible to rework to load in chunks if needs           	     		
        self.createLayout()

    def downloadMetadata(self) :
        url = self.baseUrl + "?limit=50&type=environment"
        request = urllib.request.Request(url)
        response = urllib.request.urlopen(request)
        response_content = json.loads(response.read().decode("utf-8"))
        self.lights = response_content['results']

    def threadProcDownloadThumbnail(self, light_id, fullFilePath) :
        self.downloadThumbnail(light_id, fullFilePath)

    def downloadThumbnails(self) :
        threads = []        
        for light in self.lights :
            fileName = self.getMaterialFileName(light)
            imageFileName = self.getMaterialFullPath(fileName)

            # Checks if end condition has been reached
            light_id = light["id"]

            if (not os.path.isfile(imageFileName)) :
                thread = threading.Thread(target=self.threadProcDownloadThumbnail, args=(light_id, imageFileName))
                threads.append(thread)
                thread.start()

        for thread in threads:
            thread.join()
#            percent = int(100 * threadCount / len(threads))
#            cmds.progressWindow( edit=True, progress=percent, status=('opening: ' + str(percent) + '%' ) )

    def downloadThumbnail(self, light_id: str, fullFilePath: str):
        url = self.baseUrl + light_id + "/thumbnail"
        response = urllib.request.urlopen(url)
        length = response.getheader('content-length')
        if length:
            length = int(length)
            blocksize = length
        else:
            blocksize = 8 * 1024 * 1024
        
        with open(fullFilePath, 'wb') as file:
            size = 0
            while True:
                buf = response.read(blocksize)
                if not buf:
                    break
                file.write(buf)
                size += len(buf)
        if length and size != length and os.path.exists(fullFilePath):
            os.remove(fullFilePath)
            raise EOFError

    def getMaterialFileName(self, light) :
        lightId = light["id"]
        return lightId + ".png"

    def getMaterialFullPath(self, fileName) :
        return os.path.join(self.pathRootThumbnail, fileName)

    def createLayout(self) :
        
        windowName = "RPRLightBrowserWindow"
        # Delete any existing window.
        if (cmds.window(windowName, exists = True)) :
            cmds.deleteUI(windowName)

        # Create a new window.
        self.window = cmds.window(windowName,
                                  widthHeight=(400, 400),
                                  title="Radeon ProRender Light Browser")

        # Ensure that the material container is the current parent.
        cmds.setParent(self.window)

        self.iconSize = 128

        self.cellWidth = self.iconSize + 10
        self.cellHeight = self.iconSize + 30

        # Create the new flow layout.
        cmds.flowLayout("RPRMaterialsFlow", columnSpacing=0, wrap=True, width = 3 * self.cellWidth, height = 3 * self.cellHeight)

            # Vertical layout for large icons.
        for light in self.lights : 
            fileName = self.getMaterialFileName(light)
            imageFileName = self.getMaterialFullPath(fileName)

            cmd = partial(selectIBL, light["name"])

            cmds.columnLayout(width=self.cellWidth, height=self.cellHeight)
            cmds.iconTextButton(style='iconOnly', image=imageFileName, width=self.iconSize, height=self.iconSize, command=cmd)
            cmds.text(label=light["name"], align="center", width=self.iconSize)
            cmds.setParent('..')

         # Show the material browser window.
        cmds.showWindow(self.window)

    def selectIBL(self, lightName) :




        