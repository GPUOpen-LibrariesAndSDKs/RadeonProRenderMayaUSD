from client import MatlibClient

matlibClient = MatlibClient("https://api.matlib.gpuopen.com")

#for category in list :
#    print(category)
#    print(type(category).__name__) 

#categoryListData = matlibClient.categories.get_list(1000, 0)
#print(categoryListData)
#categoryDict = dict()

print ("111")
matlibClient.renders.download_thumbnail("5dba30e0-68d4-4bfa-8a76-1f354cf1f1a5", None, "C:\\Projects\\RadeonProRenderMayaUSD\\RprUsd\\src\\python\\test", "filename.png")
print ("222")


