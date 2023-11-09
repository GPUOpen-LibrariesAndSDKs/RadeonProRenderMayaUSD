from client import MatlibClient

g_WebMatXServerUrl = "https://api.matlib.gpuopen.com"

def getMatXNameByIdWithoutBrowserRunning(uid):
    matlibClient = MatlibClient(g_WebMatXServerUrl)    
    return matlibClient.materials._get_by_id(uid)["mtlx_material_name"]
