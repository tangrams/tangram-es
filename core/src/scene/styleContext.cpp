#include "scene/styleContext.h"

#include "data/propertyItem.h"
#include "data/tileData.h"
#include "js/JavaScript.h"
#include "log.h"
#include "platform.h"
#include "scene/filters.h"
#include "scene/scene.h"
#include "util/mapProjection.h"
#include "util/builders.h"
#include "util/yamlUtil.h"

namespace Tangram {

static const std::string key_geom("$geometry");
static const std::string key_zoom("$zoom");

static const std::vector<std::string> s_geometryStrings = {
    "", // unknown
    "point",
    "line",
    "polygon",
};

StyleContext::StyleContext() {
    m_jsContext = std::make_unique<JSContext>();
}

StyleContext::~StyleContext() = default;

// Convert a scalar node to a boolean, double, or string (in that order)
// and for the first conversion that works, push it to the top of the JS stack.
JSValue pushYamlScalarAsJsPrimitive(JSScope& jsScope, const YAML::Node& node) {
    bool booleanValue = false;
    double numberValue = 0.;
    if (YamlUtil::getBool(node, booleanValue)) {
        return jsScope.newBoolean(booleanValue);
    } else if (YamlUtil::getDouble(node, numberValue)) {
        return jsScope.newNumber(numberValue);
    } else {
        return jsScope.newString(node.Scalar());
    }
}

JSValue pushYamlScalarAsJsFunctionOrString(JSScope& jsScope, const YAML::Node& node) {
    auto value = jsScope.newFunction(node.Scalar());
    if (value) {
        return value;
    }
    return jsScope.newString(node.Scalar());
}

JSValue parseSceneGlobals(JSScope& jsScope, const YAML::Node& node) {
    switch(node.Type()) {
    case YAML::NodeType::Scalar: {
        auto& scalar = node.Scalar();
        if (scalar.compare(0, 8, "function") == 0) {
            return pushYamlScalarAsJsFunctionOrString(jsScope, node);
        }
        return pushYamlScalarAsJsPrimitive(jsScope, node);
    }
    case YAML::NodeType::Sequence: {
        auto jsArray = jsScope.newArray();
        for (size_t i = 0; i < node.size(); i++) {
            jsArray.setValueAtIndex(i, parseSceneGlobals(jsScope, node[i]));
        }
        return jsArray;
    }
    case YAML::NodeType::Map: {
        auto jsObject = jsScope.newObject();
        for (const auto& entry : node) {
            if (!entry.first.IsScalar()) {
                continue; // Can't put non-scalar keys in JS objects.
            }
            jsObject.setValueForProperty(entry.first.Scalar(), parseSceneGlobals(jsScope, entry.second));
        }
        return jsObject;
    }
    default:
        return jsScope.newNull();
    }
}

void StyleContext::setSceneGlobals(const YAML::Node& sceneGlobals) {

    if (!sceneGlobals) { return; }

    JSScope jsScope(*m_jsContext);

    auto jsValue = parseSceneGlobals(jsScope, sceneGlobals);

    m_jsContext->setGlobalValue("global", std::move(jsValue));
}

void StyleContext::initFunctions(const Scene& _scene) {

    if (_scene.id == m_sceneId) {
        return;
    }
    m_sceneId = _scene.id;

    setSceneGlobals(_scene.globals());
    setFunctions(_scene.functions());
}

bool StyleContext::setFunctions(const std::vector<std::string>& _functions) {
    uint32_t id = 0;
    bool success = true;
    for (auto& function : _functions) {
        success &= m_jsContext->setFunction(id++, function);
    }

    m_functionCount = id;

    return success;
}

bool StyleContext::addFunction(const std::string& _function) {
    bool success = m_jsContext->setFunction(m_functionCount++, _function);
    return success;
}

void StyleContext::setFeature(const Feature& _feature) {

    m_feature = &_feature;

    if (m_keywordGeom != m_feature->geometryType) {
        setKeyword(key_geom, s_geometryStrings[m_feature->geometryType]);
        m_keywordGeom = m_feature->geometryType;
    }

    m_jsContext->setCurrentFeature(&_feature);
}

void StyleContext::setKeywordZoom(int _zoom) {
    if (m_keywordZoom != _zoom) {
        setKeyword(key_zoom, _zoom);
        m_keywordZoom = _zoom;
    }
}

void StyleContext::setKeyword(const std::string& _key, Value _val) {
    auto keywordKey = Filter::keywordType(_key);
    if (keywordKey == FilterKeyword::undefined) {
        LOG("Undefined Keyword: %s", _key.c_str());
        return;
    }

    // Unset shortcuts in case setKeyword was not called by
    // the helper functions above.
    if (_key == key_zoom) { m_keywordZoom = -1; }
    if (_key == key_geom) { m_keywordGeom = -1; }

    Value& entry = m_keywords[static_cast<uint8_t>(keywordKey)];
    if (entry == _val) { return; }

    {
        JSScope jsScope(*m_jsContext);
        JSValue value;
        if (_val.is<std::string>()) {
            value = jsScope.newString(_val.get<std::string>());
        } else if (_val.is<double>()) {
            value = jsScope.newNumber(_val.get<double>());
        }
        m_jsContext->setGlobalValue(_key, std::move(value));
    }


    entry = std::move(_val);
}

float StyleContext::getPixelAreaScale() {
    // scale the filter value with pixelsPerMeter
    // used with `px2` area filtering
    double metersPerPixel = MapProjection::EARTH_CIRCUMFERENCE_METERS * exp2(-m_keywordZoom) / MapProjection::tileSize();
    return metersPerPixel * metersPerPixel;
}

const Value& StyleContext::getKeyword(const std::string& _key) const {
    return getKeyword(Filter::keywordType(_key));
}

void StyleContext::clear() {
    m_jsContext->setCurrentFeature(nullptr);
}

bool StyleContext::evalFilter(FunctionID _id) {
    bool result = m_jsContext->evaluateBooleanFunction(_id);
    return result;
}

bool StyleContext::evalStyle(FunctionID _id, StyleParamKey _key, StyleParam::Value& _val) {
    _val = none_type{};

    JSScope jsScope(*m_jsContext);
    auto jsValue = jsScope.getFunctionResult(_id);
    if (!jsValue) {
        return false;
    }

    if (jsValue.isString()) {
        std::string value = jsValue.toString();

        switch (_key) {
            case StyleParamKey::outline_style:
            case StyleParamKey::repeat_group:
            case StyleParamKey::sprite:
            case StyleParamKey::sprite_default:
            case StyleParamKey::style:
            case StyleParamKey::text_align:
            case StyleParamKey::text_repeat_group:
            case StyleParamKey::text_source:
            case StyleParamKey::text_source_left:
            case StyleParamKey::text_source_right:
            case StyleParamKey::text_transform:
            case StyleParamKey::texture:
                _val = value;
                break;
            case StyleParamKey::color:
            case StyleParamKey::outline_color:
            case StyleParamKey::text_font_fill:
            case StyleParamKey::text_font_stroke_color: {
                Color result;
                if (StyleParam::parseColor(value, result)) {
                    _val = result.abgr;
                } else {
                    LOGW("Invalid color value: %s", value.c_str());
                }
                break;
            }
            default:
                _val = StyleParam::parseString(_key, value);
                break;
        }

    } else if (jsValue.isBoolean()) {
        bool value = jsValue.toBool();

        switch (_key) {
            case StyleParamKey::interactive:
            case StyleParamKey::text_interactive:
            case StyleParamKey::visible:
                _val = value;
                break;
            case StyleParamKey::extrude:
                _val = value ? glm::vec2(NAN, NAN) : glm::vec2(0.0f, 0.0f);
                break;
            default:
                break;
        }

    } else if (jsValue.isArray()) {
        auto len = jsValue.getLength();

        switch (_key) {
            case StyleParamKey::extrude: {
                if (len != 2) {
                    LOGW("Wrong array size for extrusion: '%d'.", len);
                    break;
                }

                double v1 = jsValue.getValueAtIndex(0).toDouble();
                double v2 = jsValue.getValueAtIndex(1).toDouble();

                _val = glm::vec2(v1, v2);
                break;
            }
            case StyleParamKey::color:
            case StyleParamKey::outline_color:
            case StyleParamKey::text_font_fill:
            case StyleParamKey::text_font_stroke_color: {
                if (len < 3 || len > 4) {
                    LOGW("Wrong array size for color: '%d'.", len);
                    break;
                }
                double r = jsValue.getValueAtIndex(0).toDouble();
                double g = jsValue.getValueAtIndex(1).toDouble();
                double b = jsValue.getValueAtIndex(2).toDouble();
                double a = 1.0;
                if (len == 4) {
                    a = jsValue.getValueAtIndex(3).toDouble();
                }
                _val = ColorF(r, g, b, a).toColor().abgr;
                break;
            }
            default:
                break;
        }
    } else if (jsValue.isNumber()) {
        double number = jsValue.toDouble();
        if (std::isnan(number)) {
            LOGD("duk evaluates JS method to NAN.\n");
        }
        switch (_key) {
            case StyleParamKey::text_source:
            case StyleParamKey::text_source_left:
            case StyleParamKey::text_source_right:
                _val = doubleToString(number);
                break;
            case StyleParamKey::extrude:
                _val = glm::vec2(0.f, number);
                break;
            case StyleParamKey::placement_spacing: {
                _val = StyleParam::Width{static_cast<float>(number), Unit::pixel};
                break;
            }
            case StyleParamKey::width:
            case StyleParamKey::outline_width: {
                // TODO more efficient way to return pixels.
                // atm this only works by return value as string
                _val = StyleParam::Width{static_cast<float>(number)};
                break;
            }
            case StyleParamKey::angle:
            case StyleParamKey::text_font_stroke_width:
            case StyleParamKey::placement_min_length_ratio: {
                _val = static_cast<float>(number);
                break;
            }
            case StyleParamKey::size: {
                StyleParam::SizeValue vec;
                vec.x.value = static_cast<float>(number);
                _val = vec;
                break;
            }
            case StyleParamKey::order:
            case StyleParamKey::outline_order:
            case StyleParamKey::priority:
            case StyleParamKey::color:
            case StyleParamKey::outline_color:
            case StyleParamKey::text_font_fill:
            case StyleParamKey::text_font_stroke_color: {
                _val = static_cast<uint32_t>(number);
                break;
            }
            default:
                break;
        }
    } else if (jsValue.isUndefined()) {
        // Explicitly set value as 'undefined'. This is important for some styling rules.
        _val = Undefined();
    } else {
        LOGW("Unhandled return type from Javascript style function for %d.", _key);
    }

    return !_val.is<none_type>();
}

} // namespace Tangram
