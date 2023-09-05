//
// Copyright 2023 Advanced Micro Devices, Inc
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//    http://www.apache.org/licenses/LICENSE-2.0
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//


#include "common.h"

#include <maya/MItDependencyNodes.h>
#include <mayaUsd/nodes/layerManager.h>

#include <maya/MSelectionList.h>

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

MObject GetSettingsNode()
{
	MSelectionList slist;
	MString nodeName = "defaultRenderGlobals";
	slist.add(nodeName);

	MObject mayaObject;
	if (slist.length() == 0 || !slist.getDependNode(0, mayaObject)) {
		return mayaObject;
	}

	MStatus           status;
	MFnDependencyNode node(mayaObject, &status);
	if (!status) {
		return MObject();
	}

	return mayaObject;
}
