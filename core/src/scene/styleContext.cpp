#include "styleContext.h"

#include "data/propertyItem.h"
#include "data/tileData.h"
#include "log.h"
#include "platform.h"
#include "scene/filters.h"
#include "scene/scene.h"
#include "scene/styleParam.h"
#include "util/builders.h"
#include "util/fastmap.h"
#include "util/mapProjection.h"
#include "util/yamlUtil.h"
#include "yaml-cpp/yaml.h"

#ifdef TANGRAM_USE_DUKTAPE
#include "js/DuktapeContext.h"
#endif

#ifdef TANGRAM_USE_JAVASCRIPTCORE
#include "js/JSCoreContext.h"
#endif

#include <array>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

#ifdef RUNTIME_STYLE_CONTEXT
#define OVERRIDE
#else
#define OVERRIDE override
#endif

namespace Tangram {

using JSScopeMarker = int32_t;

static const std::string key_geom("$geometry");
static const std::string key_zoom("$zoom");
static const std::vector<std::string> s_geometryStrings = {
    "", // unknown
    "point",
    "line",
    "polygon",
};

template<class JSContext, class JSValue>
class JavaScriptScope {
public:
    explicit JavaScriptScope(JSContext& context) : _context(context) {
        _scopeMarker = _context.getScopeMarker();
    }

    ~JavaScriptScope() {
        _context.resetToScopeMarker(_scopeMarker);
    }

    JavaScriptScope& operator=(const JavaScriptScope& other) = delete;
    JavaScriptScope& operator=(JavaScriptScope&& other) = delete;

    JSValue newNull() { return _context.newNull(); }
    JSValue newBoolean(bool value) { return _context.newBoolean(value); }
    JSValue newNumber(double value) { return _context.newNumber(value); }
    JSValue newString(const std::string& value) { return _context.newString(value); }
    JSValue newArray() { return _context.newArray(); }
    JSValue newObject() { return _context.newObject(); }
    JSValue newFunction(const std::string& value) { return _context.newFunction(value); }
    JSValue getFunctionResult(JSFunctionIndex index) { return _context.getFunctionResult(index); }
private:
    JSContext& _context;
    JSScopeMarker _scopeMarker = 0;
};


template<class JSContext, class JSValue>
struct StyleContextBase : public StyleContext::DynamicStyleContext {

    using Scope = JavaScriptScope<JSContext, JSValue>;

    int m_keywordZoom = -1;
    std::array<Value, 4> m_keywords {};
    int m_keywordGeom= -1;
    int m_functionCount = 0;
    int32_t m_sceneId = -1;
    const Feature* m_feature = nullptr;
    JSContext m_jsContext;

    StyleContextBase() {}

    static JSValue pushYamlScalarAsJsPrimitive(Scope& jsScope, const YAML::Node& node) {
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

    static JSValue pushYamlScalarAsJsFunctionOrString(Scope& jsScope, const YAML::Node& node) {
        auto value = jsScope.newFunction(node.Scalar());
        if (value) {
            return value;
        }
        return jsScope.newString(node.Scalar());
    }

    static JSValue parseSceneGlobals(Scope& jsScope, const YAML::Node& node) {
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
                jsObject.setValueForProperty(entry.first.Scalar(),
                                             parseSceneGlobals(jsScope, entry.second));
            }
            return jsObject;
        }
        default:
            return jsScope.newNull();
        }
    }

    void setSceneGlobals(const YAML::Node& sceneGlobals) OVERRIDE {
        if (!sceneGlobals) { return; }

        Scope jsScope(m_jsContext);
        auto jsValue = parseSceneGlobals(jsScope, sceneGlobals);

        m_jsContext.setGlobalValue("global", std::move(jsValue));
    }

    void initFunctions(const Scene& _scene) OVERRIDE {
        if (_scene.id == m_sceneId) {
            return;
        }
        m_sceneId = _scene.id;

        setSceneGlobals(_scene.config()["global"]);
        setFunctions(_scene.functions());
    }

    bool setFunctions(const std::vector<std::string>& _functions) OVERRIDE {
        uint32_t id = 0;
        bool success = true;
        for (auto& function : _functions) {
            success &= m_jsContext.setFunction(id++, function);
        }

        m_functionCount = id;

        return success;
    }

    bool addFunction(const std::string& _function) OVERRIDE {
        bool success = m_jsContext.setFunction(m_functionCount++, _function);
        return success;
    }

    void setFeature(const Feature& _feature) OVERRIDE {
        m_feature = &_feature;

        if (m_keywordGeom != m_feature->geometryType) {
            setKeyword(key_geom, s_geometryStrings[m_feature->geometryType]);
            m_keywordGeom = m_feature->geometryType;
        }
        m_jsContext.setCurrentFeature(&_feature);
    }

    void setKeywordZoom(int _zoom) OVERRIDE {
        if (m_keywordZoom != _zoom) {
            setKeyword(key_zoom, _zoom);
            m_keywordZoom = _zoom;
        }
    }

    void setKeyword(const std::string& _key, Value _val) OVERRIDE {
        auto keywordKey = Filter::keywordType(_key);
        if (keywordKey == FilterKeyword::undefined) {
            LOG("Undefined Keyword: %s", _key.c_str());
            return;
        }

        // Unset shortcuts in case setKeyword was not called by
        // the helper functions above.
        if (_key == key_zoom) { m_keywordZoom = -1; }
        if (_key == key_geom) { m_keywordGeom = -1; }

        {
            Scope jsScope(m_jsContext);
            Value& entry = m_keywords[static_cast<uint8_t>(keywordKey)];
            if (entry == _val) { return; }

            if (_val.is<std::string>()) {
                m_jsContext.setGlobalValue(_key, jsScope.newString(_val.get<std::string>()));
            } else if (_val.is<double>()) {
                m_jsContext.setGlobalValue(_key, jsScope.newNumber(_val.get<double>()));
            }
            entry = std::move(_val);
        }
    }

    const Value& getKeyword(FilterKeyword _key) const OVERRIDE {
        return m_keywords[static_cast<uint8_t>(_key)];
    }

    const Value& getKeyword(const std::string& _key) const OVERRIDE {
        return getKeyword(Filter::keywordType(_key));
    }

    float getKeywordZoom() const OVERRIDE { return m_keywordZoom; }


    void clear() OVERRIDE {
        m_jsContext.setCurrentFeature(nullptr);
    }

    bool evalFilter(JSFunctionIndex _id) OVERRIDE {
        return m_jsContext.evaluateBooleanFunction(_id);
    }

    bool evalStyle(JSFunctionIndex _id, StyleParamKey _key, StyleParam::Value& _val) OVERRIDE {

        Scope jsScope(m_jsContext);

        auto jsValue = jsScope.getFunctionResult(_id);
        if (!jsValue) { return false; }

        _val = none_type{};

        // TODO check if duk/jscore provide functions to get a typeid and
        // switch on that instead of calling multiple times into them
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

    // Convert a scalar node to a boolean, double, or string (in that order)
    // and for the first conversion that works, push it to the top of the JS stack.
    float getPixelAreaScale() OVERRIDE {
        // scale the filter value with pixelsPerMeter
        // used with `px2` area filtering
        double metersPerPixel = (MapProjection::EARTH_CIRCUMFERENCE_METERS *
                                 exp2(-m_keywordZoom) / MapProjection::tileSize());
        return metersPerPixel * metersPerPixel;
    }
};

#ifdef TANGRAM_USE_DUKTAPE
using DuktapeStyleContext = StyleContextBase<Duktape::Context, Duktape::Value>;
#endif

#ifdef TANGRAM_USE_JAVASCRIPTCORE
using JSCoreStyleContext = StyleContextBase<JSCore::Context, JSCore::Value>;
#endif

#ifdef TANGRAM_USE_JAVASCRIPTCORE
StyleContext::StyleContext()
    : impl(std::make_unique<JSCoreStyleContext>()) {}
#elif TANGRAM_USE_DUKTAPE
StyleContext::StyleContext()
    : impl(std::make_unique<DuktapeStyleContext>()) {}
#endif

StyleContext::StyleContext(bool jscore) {
#ifdef TANGRAM_USE_JAVASCRIPTCORE
    if (jscore) {
        impl.reset(new JSCoreStyleContext());
        return;
    }
#endif
#if TANGRAM_USE_DUKTAPE
    impl.reset(new DuktapeStyleContext());
#endif
}

void StyleContext::setFeature(const Feature& _feature) {
    impl->setFeature(_feature);
}
void StyleContext::setKeywordZoom(int _zoom) {
    impl->setKeywordZoom(_zoom);
}
float StyleContext::getKeywordZoom() const {
    return impl->getKeywordZoom();
}
float StyleContext::getPixelAreaScale() {
    return impl->getPixelAreaScale();
}
const Value& StyleContext::getKeyword(FilterKeyword _key) const {
    return impl->getKeyword(_key);
}
bool StyleContext::evalFilter(JSFunctionIndex _id) {
    return impl->evalFilter(_id);
}
bool StyleContext::evalStyle(JSFunctionIndex _id, StyleParamKey _key, StyleParam::Value& _value) {
    return impl->evalStyle(_id, _key, _value);
}
void StyleContext::initFunctions(const Scene& _scene) {
    impl->initFunctions(_scene);
}
void StyleContext::clear() {
    impl->clear();
}
bool StyleContext::setFunctions(const std::vector<std::string>& _functions) {
    return impl->setFunctions(_functions);
}
bool StyleContext::addFunction(const std::string& _function) {
    return impl->addFunction(_function);
}
void StyleContext::setSceneGlobals(const YAML::Node& _sceneGlobals) {
    impl->setSceneGlobals(_sceneGlobals);
}
void StyleContext::setKeyword(const std::string& _key, Value _value) {
    impl->setKeyword(_key, _value);
}
const Value& StyleContext::getKeyword(const std::string& _key) const {
    return impl->getKeyword(_key);
}


} // namespace Tangram
