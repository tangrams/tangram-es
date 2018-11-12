#include "scene/styleContext.h"

#include "data/propertyItem.h"
#include "data/tileData.h"
#include "log.h"
#include "platform.h"
#include "scene/filters.h"
#include "scene/scene.h"
#include "util/mapProjection.h"
#include "util/builders.h"

#include "duktape.h"

#define DUMP(...) // do { logMsg(__VA_ARGS__); duk_dump_context_stderr(m_ctx); } while(0)
#define DBG(...) do { logMsg(__VA_ARGS__); duk_dump_context_stderr(m_ctx); } while(0)


namespace Tangram {

const static char INSTANCE_ID[] = "\xff""\xff""obj";
const static char FUNC_ID[] = "\xff""\xff""fns";

static const std::string key_geom("$geometry");
static const std::string key_zoom("$zoom");

static const std::vector<std::string> s_geometryStrings = {
    "", // unknown
    "point",
    "line",
    "polygon",
};

StyleContext::StyleContext() {
    m_jsContext = JsContext::create();
}

StyleContext::~StyleContext() {
    JsContext::destroy(m_jsContext);
}

// Convert a scalar node to a boolean, double, or string (in that order)
// and for the first conversion that works, push it to the top of the JS stack.
void pushYamlScalarAsJsPrimitive(duk_context* ctx, const YAML::Node& node) {
    bool booleanValue = false;
    double numberValue = 0.;
    if (YAML::convert<bool>::decode(node, booleanValue)) {
        duk_push_boolean(ctx, booleanValue);
    } else if (YAML::convert<double>::decode(node, numberValue)) {
        duk_push_number(ctx, numberValue);
    } else {
        duk_push_string(ctx, node.Scalar().c_str());
    }
}

void pushYamlScalarAsJsFunctionOrString(duk_context* ctx, const YAML::Node& node) {
    auto scalar = node.Scalar().c_str();
    duk_push_string(ctx, scalar); // Push function source.
    duk_push_string(ctx, ""); // Push a "filename".
    if (duk_pcompile(ctx, DUK_COMPILE_FUNCTION) != 0) { // Compile function.
        auto error = duk_safe_to_string(ctx, -1);
        LOGW("Compile failed in global function: %s\n%s\n---", error, scalar);
        duk_pop(ctx); // Pop error.
        duk_push_string(ctx, scalar); // Push property as a string.
    }
}

void StyleContext::parseSceneGlobals(const YAML::Node& node) {
    switch(node.Type()) {
    case YAML::NodeType::Scalar:
        {
            auto& scalar = node.Scalar();
            if (scalar.compare(0, 8, "function") == 0) {
                pushYamlScalarAsJsFunctionOrString(m_ctx, node);
            } else {
                pushYamlScalarAsJsPrimitive(m_ctx, node);
            }
            break;
        }
    case YAML::NodeType::Sequence:
        {
            auto seqObj = duk_push_array(m_ctx);
            for (size_t i = 0; i < node.size(); i++) {
                parseSceneGlobals(node[i]);
                duk_put_prop_index(m_ctx, seqObj, i);
            }
            break;
        }
    case YAML::NodeType::Map:
        {
            auto mapObj = duk_push_object(m_ctx);
            for (const auto& entry : node) {
                if (!entry.first.IsScalar()) {
                    continue; // Can't put non-scalar keys in JS objects.
                }
                parseSceneGlobals(entry.second);
                duk_put_prop_string(m_ctx, mapObj, entry.first.Scalar().c_str());
            }
            break;
        }
    default:
        duk_push_null(m_ctx);
        break;
    }
}

void StyleContext::setSceneGlobals(const YAML::Node& sceneGlobals) {

    if (!sceneGlobals) { return; }

    //[ "ctx" ]
    // globalObject
    duk_push_object(m_ctx);

    parseSceneGlobals(sceneGlobals);

    duk_put_global_string(m_ctx, "global");
}

void StyleContext::initFunctions(const Scene& _scene) {

    if (_scene.id == m_sceneId) {
        return;
    }
    m_sceneId = _scene.id;

    setSceneGlobals(_scene.config()["global"]);
    setFunctions(_scene.functions());
}

bool StyleContext::setFunctions(const std::vector<std::string>& _functions) {
    uint32_t id = 0;
    bool error = false;
    for (auto& function : _functions) {
        id = JsContext::addFunction(m_jsContext, function, error);
    }

    m_functionCount = id;

    DUMP("setFunctions\n");
    return !error;
}

bool StyleContext::addFunction(const std::string& _function) {
    bool error = false;
    JsContext::addFunction(m_jsContext, _function, error);
    m_functionCount++;

    return !error;
}

void StyleContext::setFeature(const Feature& _feature) {

    m_feature = &_feature;

    if (m_keywordGeom != m_feature->geometryType) {
        setKeyword(key_geom, s_geometryStrings[m_feature->geometryType]);
        m_keywordGeom = m_feature->geometryType;
    }
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

    if (_val.is<std::string>()) {
        JsContext::setGlobalString(m_jsContext, _key, _val.get<std::string>());
    } else if (_val.is<double>()) {
        JsContext::setGlobalNumber(m_jsContext, _key, _val.get<double>());
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
    JsContext::setCurrentFeature(m_jsContext, nullptr);
}

bool StyleContext::evalFilter(FunctionID _id) {
    bool result = JsContext::evaluateBooleanFunction(m_jsContext, _id);
    return result;
}

bool StyleContext::evalStyle(FunctionID _id, StyleParamKey _key, StyleParam::Value& _val) {
    _val = none_type{};

    auto jsValue = JsContext::getFunctionResult(m_jsContext, _id);
    if (!jsValue) {
        return false;
    }

    if (JsContext::valueIsString(m_jsContext, jsValue)) {
        std::string value = JsContext::valueGetString(m_jsContext, jsValue);

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

    } else if (JsContext::valueIsBool(m_jsContext, jsValue)) {
        bool value = JsContext::valueGetBool(m_jsContext, jsValue);

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

    } else if (JsContext::valueIsArray(m_jsContext, jsValue)) {
        auto len = JsContext::valueGetArraySize(m_jsContext, jsValue);

        switch (_key) {
            case StyleParamKey::extrude: {
                if (len != 2) {
                    LOGW("Wrong array size for extrusion: '%d'.", len);
                    break;
                }

                auto valueAt0 = JsContext::valueGetArrayElement(m_jsContext, jsValue, 0);
                double v1 = JsContext::valueGetDouble(m_jsContext, valueAt0);
                JsContext::releaseValue(m_jsContext, valueAt0);

                auto valueAt1 = JsContext::valueGetArrayElement(m_jsContext, jsValue, 1);
                double v2 = JsContext::valueGetDouble(m_jsContext, valueAt1);
                JsContext::releaseValue(m_jsContext, valueAt1);

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
                auto valueAt0 = JsContext::valueGetArrayElement(m_jsContext, jsValue, 0);
                double r = JsContext::valueGetDouble(m_jsContext, valueAt0);
                JsContext::releaseValue(m_jsContext, valueAt0);

                auto valueAt1 = JsContext::valueGetArrayElement(m_jsContext, jsValue, 1);
                double g = JsContext::valueGetDouble(m_jsContext, valueAt1);
                JsContext::releaseValue(m_jsContext, valueAt1);

                auto valueAt2 = JsContext::valueGetArrayElement(m_jsContext, jsValue, 2);
                double b = JsContext::valueGetDouble(m_jsContext, valueAt2);
                JsContext::releaseValue(m_jsContext, valueAt2);

                double a = 1.0;
                if (len == 4) {
                    auto valueAt3 = JsContext::valueGetArrayElement(m_jsContext, jsValue, 3);
                    a = JsContext::valueGetDouble(m_jsContext, valueAt3);
                    JsContext::releaseValue(m_jsContext, valueAt3);
                }

                _val = ColorF(r, g, b, a).toColor().abgr;
                break;
            }
            default:
                break;
        }
    } else if (JsContext::valueIsNumber(m_jsContext, jsValue)) {
        double number = JsContext::valueGetDouble(m_jsContext, jsValue);
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
    } else if (JsContext::valueIsUndefined(m_jsContext, jsValue)) {
        // Explicitly set value as 'undefined'. This is important for some styling rules.
        _val = Undefined();
    } else {
        LOGW("Unhandled return type from Javascript style function for %d.", _key);
    }

    JsContext::releaseValue(m_jsContext, jsValue);

    DUMP("parseStyleResult\n");

    return !_val.is<none_type>();
}

} // namespace Tangram
