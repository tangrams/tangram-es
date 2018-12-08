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

#include "js/JavaScript.h"

#include <array>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

namespace Tangram {

template<class JSContext>
struct StyleContextImpl : public StyleContext::StyleContextImpl {
    using JSValue = typename JSContext::Value;
    using Scope = JavaScriptScope<JSContext>;

    int32_t m_sceneId = -1;

    int m_functionCount = 0;

    JSContext m_jsContext;

    StyleContextImpl() {}

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

    void setSceneGlobals(const YAML::Node& sceneGlobals) override {
        if (!sceneGlobals) { return; }

        Scope jsScope(m_jsContext);
        auto jsValue = parseSceneGlobals(jsScope, sceneGlobals);

        m_jsContext.setGlobalValue("global", std::move(jsValue));
    }

    void initScene(const Scene& _scene) override {
        if (_scene.id == m_sceneId) { return; }
        m_sceneId = _scene.id;

        setSceneGlobals(_scene.config()["global"]);

        setFunctions(_scene.functions().functions);
    }

    bool setFunctions(const std::vector<std::string>& _functions) override {
        uint32_t id = 0;
        bool success = true;
        for (auto& function : _functions) {
            success &= m_jsContext.setFunction(id++, function);
        }
        m_functionCount = id;

        return success;
    }

    bool addFunction(const std::string& _function) override {
        bool success = m_jsContext.setFunction(m_functionCount++, _function);
        return success;
    }

    void setFilterKey(Filter::Key _key, int _val) override {
        m_jsContext.setFilterKey(_key, _val);
    }

    void setFeature(const Feature& _feature) override {
        m_jsContext.setCurrentFeature(&_feature);
    }

    void clear() override {
        m_jsContext.setCurrentFeature(nullptr);
    }

    bool evalFilter(JSFunctionIndex _id) override {
        return m_jsContext.evaluateBooleanFunction(_id);
    }

    bool evalStyle(JSFunctionIndex _id, StyleParamKey _key, StyleParam::Value& _val) override {

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
};

template<class JSContext>
struct StyleContextRecorder : public StyleContextImpl<JSContext> {

    using Base = StyleContextImpl<JSContext>;

    // TODO record geomtype
    std::map<JSFunctionIndex, std::vector<const Feature*>> filterCalls;
    std::map<JSFunctionIndex, std::vector<std::pair<StyleParamKey, const Feature*>>> styleCalls;

    const Feature* feature;
    std::vector<std::string> functions;
    const Scene* scene;

    void setFeature(const Feature& _feature) override {
        feature = &_feature;
        Base::setFeature(_feature);
    }
    void setFilterKey(Filter::Key _key, int _value) override {
        Base::setFilterKey(_key, _value);
    }
    bool evalFilter(JSFunctionIndex _id) override {

        filterCalls[_id].emplace_back(feature);

        return Base::evalFilter(_id);
    }
    bool evalStyle(JSFunctionIndex _id, StyleParamKey _key, StyleParam::Value& _value) override {
        styleCalls[_id].emplace_back(_key, feature);
        return Base::evalStyle(_id, _key, _value);
    }
    void initScene(const Scene& _scene) override {
        scene = &_scene;

        Base::initScene(_scene);
    }
    void clear() override {
        Base::clear();
    }
    bool addFunction(const std::string& _function) override {
        return Base::addFunction(_function);
    }
    void setSceneGlobals(const YAML::Node& _sceneGlobals) override {
        Base::setSceneGlobals(_sceneGlobals);
    }
    bool setFunctions(const std::vector<std::string>& _functions)  override {
        return Base::setFunctions(_functions);
    }

    void recorderLog(bool printFunctions, bool printFeatures) override {
        size_t sum = 0;
        for (auto& entry : filterCalls) {
            sum += entry.second.size();
        }
        printf(">>>>>>>>>>>>>>> [%zu] filter functions calls <<<<<<<<<<<<<<<\n", sum);

        for (auto& entry : filterCalls) {
            printf("--------------------  fn:[%d] features:[%zu] --------------------\n", entry.first, entry.second.size());
            if (printFunctions) {
                printf("%s\n", scene->functions().functions[entry.first].c_str());
            }
            if (printFeatures) {
                for (auto& feature : entry.second) {
                    printf("F:%d %s\n", entry.first, feature->props.toJson().c_str());
                }
            }
        }
        sum = 0;
        for (auto& entry : styleCalls) {
            sum += entry.second.size();
        }
        printf(">>>>>>>>>>>>>>> [%zu] style functions calls <<<<<<<<<<<<<<<\n", sum);
        for (auto& entry : styleCalls) {
            printf("--------------------  fn:[%d] features:[%zu] --------------------\n", entry.first, entry.second.size());
            if (printFunctions) {
                printf("%s\n", scene->functions().functions[entry.first].c_str());
            }
            if (printFeatures) {
                for (auto& keyFeature : entry.second) {
                    printf("S:%d %s\n", entry.first, keyFeature.second->props.toJson().c_str());
                }
            }
        }
    }
    // Recorder interface
    void replayFilters() override {
        for (auto& entry : filterCalls) {
            for (auto& feature : entry.second) {
                Base::setFeature(*feature);
                Base::evalFilter(entry.first);
            }
        }
    }
    // Recorder interface
    void replayStyles() override {
        for (auto& entry : styleCalls) {
            for (auto& keyFeature : entry.second) {
                StyleParam::Value value;
                Base::setFeature(*keyFeature.second);
                Base::evalStyle(entry.first, keyFeature.first, value);
            }
        }
    }
};

using DuktapeStyleContext = StyleContextImpl<Duktape::Context>;
using DuktapeStyleContextRecorder = StyleContextRecorder<Duktape::Context>;
#ifdef TANGRAM_USE_JSCORE
using JSCoreStyleContext = StyleContextImpl<JSCore::Context>;
using JSCoreStyleContextRecorder = StyleContextRecorder<JSCore::Context>;
#endif


StyleContext::StyleContext(bool jscore, bool record) {
    if (record) {
#ifdef TANGRAM_USE_JSCORE
        if (jscore) {
            LOG(">>>>>>>>  record <<<<<");
            impl.reset(new JSCoreStyleContextRecorder());
            return;
        }
#endif
        LOG(">>>>>>>>  record <<<<<");
        impl.reset(new DuktapeStyleContextRecorder());

    } else {
#ifdef TANGRAM_USE_JSCORE
        if (jscore) {
            impl.reset(new JSCoreStyleContext());
            return;
        }
#endif
        impl.reset(new DuktapeStyleContext());
    }
}

#ifdef TANGRAM_USE_JSCORE
StyleContext::StyleContext()
    : impl(std::make_unique<JSCoreStyleContext>()) {}
#else
StyleContext::StyleContext()
    : impl(std::make_unique<DuktapeStyleContext>()) {}
#endif

StyleContext::~StyleContext() {}

void StyleContext::setFeature(const Feature& _feature) {
    impl->setFeature(_feature);
    setFilterKey(Filter::Key::geometry, _feature.geometryType);
}

void StyleContext::setFilterKey(Filter::Key _key, int _value) {
    if (_key == Filter::Key::other) { return; }

    if (m_filterKeys[uint8_t(_key)] == _value) {
        return;
    }
    m_filterKeys[uint8_t(_key)] = _value;

    if (_key == Filter::Key::zoom) {
        m_zoomLevel = _value;
        // scale the filter value with pixelsPerMeter
        // used with `px2` area filtering
        double metersPerPixel = (MapProjection::EARTH_CIRCUMFERENCE_METERS *
                                 exp2(-_value) / MapProjection::tileSize());
        m_pixelAreaScale = metersPerPixel * metersPerPixel;
    }
    impl->setFilterKey(_key, _value);
}

int StyleContext::getFilterKey(Filter::Key _key) const {
    return m_filterKeys[static_cast<uint8_t>(_key)];
}
bool StyleContext::evalFilter(JSFunctionIndex _id) {
    return impl->evalFilter(_id);
}
bool StyleContext::evalStyle(JSFunctionIndex _id, StyleParamKey _key, StyleParam::Value& _value) {
    return impl->evalStyle(_id, _key, _value);
}
void StyleContext::initScene(const Scene& _scene) {
    impl->initScene(_scene);
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

} // namespace Tangram
