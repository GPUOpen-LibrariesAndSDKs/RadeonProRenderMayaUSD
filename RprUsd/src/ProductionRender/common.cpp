#include "common.h"

#include <maya/MItDependencyNodes.h>
#include <mayaUsd/nodes/layerManager.h>

std::string GetRendererName()
{
	return "HdRprPlugin";
}

MayaUsdProxyShapeBase* GetMayaUsdProxyShapeBase()
{
	MFnDependencyNode  fn;
	MItDependencyNodes iter(MFn::kPluginDependNode);
	for (; !iter.isDone(); iter.next()) {
		MObject mobj = iter.thisNode();
		fn.setObject(mobj);

		MString origRTypeName = MayaUsdProxyShapeBase::typeName;
		if (!fn.isFromReferencedFile() && (MayaUsd::LayerManager::supportedNodeType(fn.typeId()))) {
			MayaUsdProxyShapeBase* pShape = static_cast<MayaUsdProxyShapeBase*>(fn.userNode());
			if (pShape) {
				return pShape;
			}
		}
	}

	return nullptr;
}

UsdStageRefPtr GetUsdStage()
{
	MayaUsdProxyShapeBase* pShape = GetMayaUsdProxyShapeBase();

	if (pShape)
	{
		return pShape->getUsdStage();
	}

	return nullptr;
}
