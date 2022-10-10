#include "ProductionSettings.h"
#include "common.h"

#include "pxr/usd/usdRender/settings.h"

#include <pxr/imaging/hd/rendererPlugin.h>
#include <pxr/imaging/hd/rendererPluginRegistry.h>
#include <pxr/pxr.h>

#include <maya/MFnDependencyNode.h>
#include <maya/MFnEnumAttribute.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnStringData.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MGlobal.h>
#include <maya/MPlug.h>
#include <maya/MSelectionList.h>
#include <maya/MStatus.h>
#include <maya/MSceneMessage.h>
#include <maya/MItDependencyNodes.h>

#include <mayaUsd/nodes/layerManager.h>


#include <functional>
#include <sstream>

PXR_NAMESPACE_OPEN_SCOPE

static TfToken g_attributePrefix;

static constexpr const char* kMtohNSToken = "_mtohns_";
static const std::string     kMtohRendererPostFix("__");

MCallbackId ProductionSettings::_newSceneCallback = 0;
MCallbackId ProductionSettings::_openSceneCallback = 0;
MCallbackId ProductionSettings::_importSceneCallback = 0;
MCallbackId ProductionSettings::_beforeOpenSceneCallback = 0;
MCallbackId ProductionSettings::_nodeAddedCallback = 0;

bool ProductionSettings::_usdCameraListRefreshed = true;
bool ProductionSettings::_IsOpeningScene = false;

TfToken _MangleString(
	const std::string& settingKey,
	const std::string& token,
	const std::string& replacement,
	std::string        str = {})
{
	std::size_t pos = 0;
	auto        delim = settingKey.find(token);
	while (delim != std::string::npos) {
		str += settingKey.substr(pos, delim - pos).c_str();
		str += replacement;
		pos = delim + token.size();
		delim = settingKey.find(token, pos);
	}
	str += settingKey.substr(pos, settingKey.size() - pos).c_str();
	return TfToken(str, TfToken::Immortal);
}

std::string _MangleRenderer(const TfToken& rendererName)
{
	return rendererName.IsEmpty() ? "" : (rendererName.GetString() + kMtohRendererPostFix);
}

TfToken _MangleString(const TfToken& settingKey, const TfToken& rendererName)
{
	return _MangleString(settingKey.GetString(), ":", kMtohNSToken, _MangleRenderer(rendererName));
}

TfToken _DeMangleString(const TfToken& settingKey, const TfToken& rendererName)
{
	assert(!rendererName.IsEmpty() && "No condition for this");
	return _MangleString(
		settingKey.GetString().substr(rendererName.size() + kMtohRendererPostFix.size()),
		kMtohNSToken,
		":");
}

TfToken _MangleName(const TfToken& settingKey, const TfToken& rendererName = {})
{
	assert(
		rendererName.GetString().find(':') == std::string::npos
		&& "Unexpected : token in plug-in name");
	return _MangleString(settingKey, rendererName);
}

static MString _MangleColorAttribute(const MString& attrName, unsigned i)
{
    static const MString                kMtohCmptToken("_mtohc_");
    static const std::array<MString, 4> kColorComponents = { "R", "G", "B", "A" };
    if (i < kColorComponents.size()) {
        return attrName + kMtohCmptToken + kColorComponents[i];
    }

	TF_CODING_ERROR("[mtoh] Cannot mangle component: %u", i);
	return attrName + kMtohCmptToken + MString("INVALID");
}

static MString _AlphaAttribute(const MString& attrName)
{
    return _MangleColorAttribute(attrName, 3);
}

template <typename HydraType, typename PrefType>
bool _RestoreValue(
    MFnDependencyNode& node,
    const MString&     attrName,
    PrefType (*getter)(const MString&, bool* valid))
{
    bool     valid = false;
    PrefType mayaPref = getter(attrName, &valid);
    if (valid) {
        auto plug = node.findPlug(attrName, false);
        plug.setValue(HydraType(mayaPref));
    }
    return valid;
}

void _CreateEnumAttribute(
    MFnDependencyNode&   node,
    const MString&       attrName,
    const TfTokenVector& values,
    const TfToken&       defValue,
    bool                 useUserOptions)
{
    const auto attr = node.attribute(attrName);
    const bool existed = !attr.isNull();
    if (existed) {
        const auto sameOrder = [&attr, &values]() -> bool {
            MStatus          status;
            MFnEnumAttribute eAttr(attr, &status);
            if (!status) {
                return false;
            }
            short id = 0;
            for (const auto& v : values) {
                if (eAttr.fieldName(id++) != v.GetText()) {
                    return false;
                }
            }
            return true;
        };
        if (sameOrder()) {
            return;
        }

        node.removeAttribute(attr);
    }

    MFnEnumAttribute eAttr;
    auto             o = eAttr.create(attrName, attrName);
    short            id = 0;
    for (const auto& v : values) {
        eAttr.addField(v.GetText(), id++);
    }
    eAttr.setDefault(defValue.GetText());
    node.addAttribute(o);

    if (existed || !useUserOptions) {
        return;
    }

    // Enums stored as string to allow re-ordering
    // Why MPlug::setValue doesn't handle this ?
    //
    bool    valid = false;
    TfToken mayaPref(MGlobal::optionVarStringValue(attrName, &valid).asChar());
    if (!valid) {
        return;
    }

    for (int i = 0, n = values.size(); i < n; ++i) {
        if (mayaPref == values[i]) {
            auto plug = node.findPlug(attrName, false);
            plug.setValue(i);
            return;
        }
    }
    TF_WARN("[mtoh] Cannot restore enum '%s'", mayaPref.GetText());
}

void _CreateEnumAttribute(
    MFnDependencyNode& node,
    const MString&     attrName,
    const TfEnum&      defValue,
    bool               useUserOptions)
{
    std::vector<std::string> names = TfEnum::GetAllNames(defValue);
    TfTokenVector            tokens(names.begin(), names.end());
    return _CreateEnumAttribute(
        node, attrName, tokens, TfToken(TfEnum::GetDisplayName(defValue)), useUserOptions);
}

void _CreateStringAttribute(
    MFnDependencyNode& node,
    const MString&     attrName,
    const std::string& defValue,
    bool               useUserOptions)
{

    const auto attr = node.attribute(attrName);
    const bool existed = !attr.isNull();
    if (existed) {
        MStatus           status;
        MFnTypedAttribute tAttr(attr, &status);
        if (status && tAttr.attrType() == MFnData::kString) {
            return;
        }
        node.removeAttribute(attr);
    }

    MFnTypedAttribute tAttr;
    const auto        obj = tAttr.create(attrName, attrName, MFnData::kString);
    if (!defValue.empty()) {
        MFnStringData strData;
        MObject       defObj = strData.create(defValue.c_str());
        tAttr.setDefault(defObj);
    }
    node.addAttribute(obj);

    if (!existed && useUserOptions) {
        _RestoreValue<MString>(node, attrName, MGlobal::optionVarStringValue);
    }
}

template <typename T, typename MayaType>
void _CreateNumericAttribute(
    MFnDependencyNode&                                      node,
    const MString&                                          attrName,
    MFnNumericData::Type                                    type,
    typename std::enable_if<std::is_pod<T>::value, T>::type defValue,
    bool                                                    useUserOptions,
    MayaType (*getter)(const MString&, bool* valid),
    std::function<void(MFnNumericAttribute& nAttr)> postCreate = {})
{

    const auto attr = node.attribute(attrName);
    const bool existed = !attr.isNull();
    if (existed) {
        MStatus             status;
        MFnNumericAttribute nAttr(attr, &status);
        if (status && nAttr.unitType() == type) {
            return;
        }
        node.removeAttribute(attr);
    }

    MFnNumericAttribute nAttr;
    const auto          obj = nAttr.create(attrName, attrName, type);
    nAttr.setDefault(defValue);
    if (postCreate) {
        postCreate(nAttr);
    }
    node.addAttribute(obj);

    if (!existed && useUserOptions) {
        _RestoreValue<T, MayaType>(node, attrName, getter);
    }
}

template <typename T>
void _CreateColorAttribute(
    MFnDependencyNode&                                             node,
    const MString&                                                 attrName,
    T                                                              defValue,
    bool                                                           useUserOptions,
    std::function<bool(MFnNumericAttribute& nAttr, bool doCreate)> alphaOp = {})
{
    const auto attr = node.attribute(attrName);
    if (!attr.isNull()) {
        MStatus             status;
        MFnNumericAttribute nAttr(attr, &status);
        if (status && nAttr.isUsedAsColor() && (!alphaOp || alphaOp(nAttr, false))) {
            return;
        }
        node.removeAttribute(attr);
    }
    MFnNumericAttribute nAttr;
    const auto          o = nAttr.createColor(attrName, attrName);
    nAttr.setDefault(defValue[0], defValue[1], defValue[2]);
    node.addAttribute(o);

    if (alphaOp) {
        alphaOp(nAttr, true);
    }

    if (useUserOptions) {
        for (unsigned i = 0; i < T::dimension; ++i) {
            _RestoreValue<float>(
                node, _MangleColorAttribute(attrName, i), MGlobal::optionVarDoubleValue);
        }
    }
}

void _CreateColorAttribute(
    MFnDependencyNode& node,
    const MString&     attrName,
    GfVec4f            defVec4,
    bool               useUserOptions)
{
    _CreateColorAttribute(
        node,
        attrName,
        GfVec3f(defVec4.data()),
        useUserOptions,
        [&](MFnNumericAttribute& nAttr, bool doCreate) -> bool {
            const MString attrAName = _AlphaAttribute(attrName);
            const auto    attrA = node.attribute(attrAName);
            // If we previously found the color attribute, make sure the Alpha attribute
            // is also a match (MFnNumericData::kFloat), otherwise delete it and signal to re-create
            // the color too.
            if (!doCreate) {
                if (!attrA.isNull()) {
                    MStatus             status;
                    MFnNumericAttribute nAttr(attrA, &status);
                    if (status && nAttr.unitType() == MFnNumericData::kFloat) {
                        return true;
                    }
                    node.removeAttribute(attrA);
                }
                return false;
            }

            const auto o = nAttr.create(attrAName, attrAName, MFnNumericData::kFloat);
            nAttr.setDefault(defVec4[3]);
            node.addAttribute(o);
            return true;
        });
}

void _CreateBoolAttribute(
    MFnDependencyNode& node,
    const MString&     attrName,
    bool               defValue,
    bool               useUserOptions)
{
    _CreateNumericAttribute<bool>(
        node,
        attrName,
        MFnNumericData::kBoolean,
        defValue,
        useUserOptions,
        MGlobal::optionVarIntValue);
}

void _CreateIntAttribute(
    MFnDependencyNode&                              node,
    const MString&                                  attrName,
    int                                             defValue,
    bool                                            useUserOptions,
    std::function<void(MFnNumericAttribute& nAttr)> postCreate = {})
{
    _CreateNumericAttribute<int>(
        node,
        attrName,
        MFnNumericData::kInt,
        defValue,
        useUserOptions,
        MGlobal::optionVarIntValue,
        std::move(postCreate));
}

void _CreateFloatAttribute(
    MFnDependencyNode& node,
    const MString&     attrName,
    float              defValue,
    bool               useUserOptions)
{
    _CreateNumericAttribute<float>(
        node,
        attrName,
        MFnNumericData::kFloat,
        defValue,
        useUserOptions,
        MGlobal::optionVarDoubleValue);
}

template <typename T> void _GetFromPlug(const MPlug& plug, T& out) { assert(false); }

template <> void _GetFromPlug<bool>(const MPlug& plug, bool& out) { out = plug.asBool(); }

template <> void _GetFromPlug<int>(const MPlug& plug, int& out) { out = plug.asInt(); }

template <> void _GetFromPlug<float>(const MPlug& plug, float& out) { out = plug.asFloat(); }

template <> void _GetFromPlug<std::string>(const MPlug& plug, std::string& out)
{
    out = plug.asString().asChar();
}

template <> void _GetFromPlug<TfEnum>(const MPlug& plug, TfEnum& out)
{
    out = TfEnum(out.GetType(), plug.asInt());
}

template <> void _GetFromPlug<TfToken>(const MPlug& plug, TfToken& out)
{
    MObject attribute = plug.attribute();

    if (attribute.hasFn(MFn::kEnumAttribute)) {
        MFnEnumAttribute enumAttr(attribute);
        MString          value = enumAttr.fieldName(plug.asShort());
        out = TfToken(value.asChar());
    } else {
        out = TfToken(plug.asString().asChar());
    }
}

template <typename T>
bool _GetAttribute(
    const MFnDependencyNode& node,
    const MString&           attrName,
    T&                       out,
    bool                     storeUserSetting)
{
    const auto plug = node.findPlug(attrName, true);
    if (plug.isNull()) {
        return false;
    }
    _GetFromPlug<T>(plug, out);

    return true;
}

void _GetColorAttribute(
    const MFnDependencyNode&            node,
    const MString&                      attrName,
    GfVec3f&                            out,
    bool                                storeUserSetting,
    std::function<void(GfVec3f&, bool)> alphaOp = {})
{
    const auto plug = node.findPlug(attrName, true);
    if (plug.isNull()) {
        return;
    }

    out[0] = plug.child(0).asFloat();
    out[1] = plug.child(1).asFloat();
    out[2] = plug.child(2).asFloat();
    if (alphaOp) {
        // if alphaOp is provided, it is responsible for storing the settings
        alphaOp(out, storeUserSetting);
    }
}

void _GetColorAttribute(
    const MFnDependencyNode& node,
    const MString&           attrName,
    GfVec4f&                 out,
    bool                     storeUserSetting)
{
    GfVec3f color3;
    _GetColorAttribute(
        node, attrName, color3, storeUserSetting, [&](GfVec3f& color3, bool storeUserSetting) {
            const auto plugA = node.findPlug(_AlphaAttribute(attrName), true);
            if (plugA.isNull()) {
                TF_WARN("[mtoh] No Alpha plug for GfVec4f");
                return;
            }
            out[0] = color3[0];
            out[1] = color3[1];
            out[2] = color3[2];
            out[3] = plugA.asFloat();
        });
}

bool _IsSupportedAttribute(const VtValue& v)
{
    return v.IsHolding<bool>() || v.IsHolding<int>() || v.IsHolding<float>()
        || v.IsHolding<GfVec3f>() || v.IsHolding<GfVec4f>() || v.IsHolding<TfToken>()
        || v.IsHolding<std::string>() || v.IsHolding<TfEnum>();
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

std::string ProductionSettings::CreateAttributes()
{
	std::string rendererName = GetRendererName();
	HdRendererPlugin* rendererPlugin = HdRendererPluginRegistry::GetInstance().GetRendererPlugin(TfToken(rendererName));
	if (!rendererPlugin)
		return "";

	HdRenderDelegate* renderDelegate = rendererPlugin->CreateRenderDelegate();
	if (!renderDelegate)
		return "";

	MObject nodeObj = GetSettingsNode();
	if (nodeObj.isNull())
	{
		return "";
	}

	MFnDependencyNode node(nodeObj);
	const bool        userDefaults = false;
	std::string controlsCreationCalls;

	HdRenderSettingDescriptorList rendererSettingDescriptors = renderDelegate->GetRenderSettingDescriptors();

	// We only needed the delegate for the settings, so release
	rendererPlugin->DeleteRenderDelegate(renderDelegate);
	// Null it out to make any possible usage later obv, wrong!
	renderDelegate = nullptr;

	if (g_attributePrefix.GetString().empty())
	{
		g_attributePrefix = TfToken(rendererName + "_Prod_");
	}

	std::string controlCreationCmdTemplate = "attrControlGrp -label \"%s\" -attribute \"%s.%s\";";

	// Create attrbiutes for usd camera

	_CreateBoolAttribute(node, MString(g_attributePrefix.GetText()) + "Static_useUSDCamera", false, userDefaults);

	TfTokenVector vec;
	TfToken token;
	_CreateStringAttribute(node, MString(g_attributePrefix.GetText()) + "Static_usdCameraSelected", "", userDefaults);

	for (const HdRenderSettingDescriptor& attr : rendererSettingDescriptors) {
		MString attrName = _MangleName(attr.key, g_attributePrefix).GetText();

		std::string controlCreationCmd = TfStringPrintf(controlCreationCmdTemplate.c_str(), attr.name.c_str(), node.name().asChar(), attrName.asChar());
		
		bool addControlCreatioCmd = true;

		if (attr.defaultValue.IsHolding<bool>()) {
			_CreateBoolAttribute(
				node,
				attrName,
				attr.defaultValue.UncheckedGet<bool>(),
				userDefaults);
		}
		else if (attr.defaultValue.IsHolding<int>()) {
			_CreateIntAttribute(
				node, attrName, attr.defaultValue.UncheckedGet<int>(), userDefaults);
		}
		else if (attr.defaultValue.IsHolding<float>()) {
			_CreateFloatAttribute(
				node,
				attrName,
				attr.defaultValue.UncheckedGet<float>(),
				userDefaults);
		}
		else if (attr.defaultValue.IsHolding<GfVec3f>()) {
			_CreateColorAttribute(
				node,
				attrName,
				attr.defaultValue.UncheckedGet<GfVec3f>(),
				userDefaults);
		}
		else if (attr.defaultValue.IsHolding<GfVec4f>()) {
			_CreateColorAttribute(
				node,
				attrName,
				attr.defaultValue.UncheckedGet<GfVec4f>(),
				userDefaults);
		}
		else if (attr.defaultValue.IsHolding<TfToken>()) {
			// If this attribute type has AllowedTokens set, we treat it as an enum instead, so
			// that only valid values are available.```
			bool createdAsEnum = false;
			if (auto primDef = UsdRenderSettings().GetSchemaClassPrimDefinition()) {
				VtTokenArray allowedTokens;

				if (primDef->GetPropertyMetadata(
					TfToken(attr.key), SdfFieldKeys->AllowedTokens, &allowedTokens)) {
					// Generate dropdown from allowedTokens
					TfTokenVector tokens(allowedTokens.begin(), allowedTokens.end());
					_CreateEnumAttribute(
						node,
						attrName,
						tokens,
						attr.defaultValue.UncheckedGet<TfToken>(),
						userDefaults);
					createdAsEnum = true;
				}
			}

			if (!createdAsEnum) {
				_CreateStringAttribute(
					node,
					attrName,
					attr.defaultValue.UncheckedGet<TfToken>().GetString(),
					userDefaults);
			}
		}
		else if (attr.defaultValue.IsHolding<std::string>()) {
			_CreateStringAttribute(
				node,
				attrName,
				attr.defaultValue.UncheckedGet<std::string>(),
				userDefaults);
		}
		else if (attr.defaultValue.IsHolding<TfEnum>()) {
			_CreateEnumAttribute(
				node,
				attrName,
				attr.defaultValue.UncheckedGet<TfEnum>(),
				userDefaults);
		}
		else {
			assert(
				!_IsSupportedAttribute(attr.defaultValue)
				&& "_IsSupportedAttribute out of synch");

			TF_WARN(
				"[mtoh] Ignoring setting: '%s' for %s",
				attr.key.GetText(),
				rendererName.c_str());

			addControlCreatioCmd = false;
		}

		if (addControlCreatioCmd)
		{
			controlsCreationCalls += controlCreationCmd + "\n";
		}
	}

	return controlsCreationCalls;
}

void ProductionSettings::ApplySettings(HdRenderDelegate* renderDelegate)
{
	HdRenderSettingDescriptorList rendererSettingDescriptors = renderDelegate->GetRenderSettingDescriptors();

	MObject nodeObj = GetSettingsNode();
	if (nodeObj.isNull())
	{
		TF_WARN("[hdRPR production] render settings node was not found");
		return;
	}

	MFnDependencyNode node(nodeObj);

	bool storeUserSetting = false;

	for (const HdRenderSettingDescriptor& attr : rendererSettingDescriptors) {
		MString attrName = _MangleName(attr.key, g_attributePrefix).GetText();

		bool valueGot = true;

		VtValue vtValue;
		if (attr.defaultValue.IsHolding<bool>()) {
			auto v = attr.defaultValue.UncheckedGet<bool>();
			_GetAttribute(node, attrName, v, storeUserSetting);
			vtValue = v;
		}
		else if (attr.defaultValue.IsHolding<int>()) {
			auto v = attr.defaultValue.UncheckedGet<int>();
			_GetAttribute(node, attrName, v, storeUserSetting);
			vtValue = v;
		}
		else if (attr.defaultValue.IsHolding<float>()) {
			auto v = attr.defaultValue.UncheckedGet<float>();
			_GetAttribute(node, attrName, v, storeUserSetting);
			vtValue = v;
		}
		else if (attr.defaultValue.IsHolding<GfVec3f>()) {
			auto v = attr.defaultValue.UncheckedGet<GfVec3f>();
			_GetAttribute(node, attrName, v, storeUserSetting);
			vtValue = v;
		}
		else if (attr.defaultValue.IsHolding<GfVec4f>()) {
			auto v = attr.defaultValue.UncheckedGet<GfVec4f>();
			_GetAttribute(node, attrName, v, storeUserSetting);
			vtValue = v;
		}
		else if (attr.defaultValue.IsHolding<TfToken>()) {
			auto v = attr.defaultValue.UncheckedGet<TfToken>();
			_GetAttribute(node, attrName, v, storeUserSetting);
			vtValue = v;
		}
		else if (attr.defaultValue.IsHolding<std::string>()) {
			auto v = attr.defaultValue.UncheckedGet<std::string>();
			_GetAttribute(node, attrName, v, storeUserSetting);
			vtValue = v;
		}
		else if (attr.defaultValue.IsHolding<TfEnum>()) {
			auto v = attr.defaultValue.UncheckedGet<TfEnum>();
			_GetAttribute(node, attrName, v, storeUserSetting);
			vtValue = v;
		}
		else {
			assert(
				!_IsSupportedAttribute(attr.defaultValue)
				&& "_IsSupportedAttribute out of synch");

			TF_WARN(
				"[mtoh] Can't get setting: '%s' for %s",
				attr.key.GetText(),
				"HdRprPlugin");

			valueGot = false;
		}

		if (valueGot)
		{
			renderDelegate->SetRenderSetting(attr.key, vtValue);
		}
	}
}

void ProductionSettings::CheckRenderGlobals()
{
	CreateAttributes();
}

void ProductionSettings::ClearUsdCameraAttributes()
{
	MGlobal::executeCommand("HdRpr_clearUSDCameras();"); 
}

MayaUsdProxyShapeBase* ProductionSettings::GetMayaUsdProxyShapeBase()
{
	MFnDependencyNode  fn;
	MItDependencyNodes iter(MFn::kPluginDependNode);
	for (; !iter.isDone(); iter.next()) {
		MObject mobj = iter.thisNode();
		fn.setObject(mobj);
		MTypeId id = fn.typeId();
		MString typeName = fn.typeName();
		MString str = fn.absoluteName();

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

UsdStageRefPtr ProductionSettings::GetUsdStage()
{
	MayaUsdProxyShapeBase* pShape = GetMayaUsdProxyShapeBase();

	if (pShape)
	{
		return pShape->getUsdStage();
	}

	return nullptr;
}


void ProductionSettings::UsdCameraListRefresh()
{
	ClearUsdCameraAttributes();

	UsdStageRefPtr usdStageRefPtr = GetUsdStage();

	if (usdStageRefPtr == nullptr)
	{
 		return;
	}

	UsdPrimRange allPrims = usdStageRefPtr->TraverseAll();

	UsdPrimRange::const_iterator it;

	for (it = allPrims.begin(); it != allPrims.end(); ++it)
	{
		if (it->GetTypeName() != "Camera")
		{
			continue;
		}

		std::string usdPath = it->GetPrimPath().GetAsString();

		MString cmd;
		cmd.format("HdRpr_AddUsdCamera(\"^1s\")", MString(usdPath.c_str()));
		MGlobal::executeCommand(cmd);
	}
}

void ProductionSettings::OnSceneCallback(void* pbCallCheckRenderGlobals)
{
	if (_IsOpeningScene)
	{
		_IsOpeningScene = false;
	}

	if (pbCallCheckRenderGlobals)
	{
		CheckRenderGlobals();
	}

	UsdCameraListRefresh();
}

bool ProductionSettings::IsUSDCameraToUse()
{
	MObject nodeObj = GetSettingsNode();
	if (nodeObj.isNull())
	{
		TF_WARN("[hdRPR production] render settings node was not found");
		return false;
	}

	MFnDependencyNode node(nodeObj);
	return node.findPlug("HdRprPlugin_Prod_Static_useUSDCamera", false).asBool();
}


UsdPrim ProductionSettings::GetUsdCameraPrim()
{
	UsdStageRefPtr usdStage = GetUsdStage();

	if (usdStage == nullptr)
	{
		return UsdPrim();
	}

	MString path = MGlobal::executeCommandStringResult("GetCurrentUsdCamera();");
	SdfPath sdfpath(path.asChar());

	return usdStage->GetPrimAtPath(sdfpath);
}

void ProductionSettings::attributeChangedCallback(MNodeMessage::AttributeMessage msg, MPlug & plug, MPlug & otherPlug, void* clientData)
{
	if (_IsOpeningScene)
	{
		return;
	}

	std::string plugName = plug.name().asChar();

	if ((plugName.find(".outTime") != std::string::npos) && (!_usdCameraListRefreshed) && (msg & MNodeMessage::AttributeMessage::kIncomingDirection))
	{
		_usdCameraListRefreshed = true;
		UsdCameraListRefresh();	
	}
}

void ProductionSettings::nodeAddedCallback(MObject& node, void* pData)
{
	if (_IsOpeningScene)
	{
		return;
	}

	MFnDependencyNode depNode(node);

	if (MayaUsd::LayerManager::supportedNodeType(depNode.typeId()))
	{
 		MNodeMessage::addAttributeChangedCallback(node, attributeChangedCallback);
		_usdCameraListRefreshed = false;
	}
}

void ProductionSettings::OnBeforeOpenCallback(void* pData)
{
	_IsOpeningScene = true;
}

void ProductionSettings::RegisterCallbacks()
{
	MStatus status;

	_newSceneCallback = MSceneMessage::addCallback(MSceneMessage::kAfterNew, OnSceneCallback, (void*) true, &status);
	CHECK_MSTATUS(status);

	_openSceneCallback = MSceneMessage::addCallback(MSceneMessage::kAfterOpen, OnSceneCallback, (void*) true, &status);
	CHECK_MSTATUS(status);

	_importSceneCallback = MSceneMessage::addCallback(MSceneMessage::kAfterImport, OnSceneCallback, NULL, &status);
	CHECK_MSTATUS(status);

	_beforeOpenSceneCallback = MSceneMessage::addCallback(MSceneMessage::kBeforeOpen, OnBeforeOpenCallback, NULL, &status);
	CHECK_MSTATUS(status);

	_nodeAddedCallback = MDGMessage::addNodeAddedCallback(nodeAddedCallback);
	CHECK_MSTATUS(status);
}

void ProductionSettings::UnregisterCallbacks()
{
	if (0 != _newSceneCallback) {
		MSceneMessage::removeCallback(_newSceneCallback);
		MSceneMessage::removeCallback(_openSceneCallback);
		MSceneMessage::removeCallback(_importSceneCallback);
		MSceneMessage::removeCallback(_beforeOpenSceneCallback);
		MSceneMessage::removeCallback(_nodeAddedCallback);

		_newSceneCallback = 0;
		_openSceneCallback = 0;
		_importSceneCallback = 0;
		_beforeOpenSceneCallback = 0;
		_nodeAddedCallback = 0;
	}
}

PXR_NAMESPACE_CLOSE_SCOPE
