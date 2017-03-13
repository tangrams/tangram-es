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
    m_ctx = duk_create_heap_default();

    //// Create global geometry constants
    // TODO make immutable
    duk_push_number(m_ctx, GeometryType::points);
    duk_put_global_string(m_ctx, "point");

    duk_push_number(m_ctx, GeometryType::lines);
    duk_put_global_string(m_ctx, "line");

    duk_push_number(m_ctx, GeometryType::polygons);
    duk_put_global_string(m_ctx, "polygon");

    //// Create global 'feature' object
    // Get Proxy constructor
    // -> [cons]
    duk_eval_string(m_ctx, "Proxy");

    // Add feature object
    // -> [cons, { __obj: this }]
    duk_idx_t featureObj = duk_push_object(m_ctx);
    duk_push_pointer(m_ctx, this);
    duk_put_prop_string(m_ctx, featureObj, INSTANCE_ID);

    // Add handler object
    // -> [cons, {...}, { get: func, has: func }]
    duk_idx_t handlerObj = duk_push_object(m_ctx);
    // Add 'get' property to handler
    duk_push_c_function(m_ctx, jsGetProperty, 3 /*nargs*/);
    duk_put_prop_string(m_ctx, handlerObj, "get");
    // Add 'has' property to handler
    duk_push_c_function(m_ctx, jsHasProperty, 2 /*nargs*/);
    duk_put_prop_string(m_ctx, handlerObj, "has");

    // Call proxy constructor
    // [cons, feature, handler ] -> [obj|error]
    if (duk_pnew(m_ctx, 2) == 0) {
        // put feature proxy object in global scope
        if (!duk_put_global_string(m_ctx, "feature")) {
            LOGE("Initialization failed");
        }
    } else {
        LOGE("Failure: %s", duk_safe_to_string(m_ctx, -1));
        duk_pop(m_ctx);
    }

    DUMP("init\n");
}

StyleContext::~StyleContext() {
    duk_destroy_heap(m_ctx);
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

    auto arr_idx = duk_push_array(m_ctx);
    int id = 0;

    bool ok = true;

    for (auto& function : _functions) {
        duk_push_string(m_ctx, function.c_str());
        duk_push_string(m_ctx, "");

        if (duk_pcompile(m_ctx, DUK_COMPILE_FUNCTION) == 0) {
            duk_put_prop_index(m_ctx, arr_idx, id);
        } else {
            LOGW("Compile failed: %s\n%s\n---",
                 duk_safe_to_string(m_ctx, -1),
                 function.c_str());
            duk_pop(m_ctx);
            ok = false;
        }
        id++;
    }

    if (!duk_put_global_string(m_ctx, FUNC_ID)) {
        LOGE("'fns' object not set");
    }

    m_functionCount = id;

    DUMP("setFunctions\n");
    return ok;
}

bool StyleContext::addFunction(const std::string& _function) {
    // Get all functions (array) in context
    if (!duk_get_global_string(m_ctx, FUNC_ID)) {
        LOGE("AddFunction - functions array not initialized");
        duk_pop(m_ctx); // pop [undefined] sitting at stack top
        return false;
    }

    int id = m_functionCount++;
    bool ok = true;

    duk_push_string(m_ctx, _function.c_str());
    duk_push_string(m_ctx, "");

    if (duk_pcompile(m_ctx, DUK_COMPILE_FUNCTION) == 0) {
        duk_put_prop_index(m_ctx, -2, id);
    } else {
        LOGW("Compile failed: %s\n%s\n---",
             duk_safe_to_string(m_ctx, -1),
             _function.c_str());
        duk_pop(m_ctx);
        ok = false;
    }

    // Pop the functions array off the stack
    duk_pop(m_ctx);

    return ok;
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
        duk_push_string(m_ctx, _val.get<std::string>().c_str());
        duk_put_global_string(m_ctx, _key.c_str());
    } else if (_val.is<double>()) {
        duk_push_number(m_ctx, _val.get<double>());
        duk_put_global_string(m_ctx, _key.c_str());
    }

    entry = std::move(_val);
}

float StyleContext::getPixelAreaScale() {
    // scale the filter value with pixelsPerMeter
    // used with `px2` area filtering
    double metersPerPixel = 2.f * MapProjection::HALF_CIRCUMFERENCE * exp2(-m_keywordZoom) / View::s_pixelsPerTile;
    return metersPerPixel * metersPerPixel;
}

const Value& StyleContext::getKeyword(const std::string& _key) const {
    return getKeyword(Filter::keywordType(_key));
}

void StyleContext::clear() {
    m_feature = nullptr;
}

bool StyleContext::evalFunction(FunctionID id) {
    // Get all functions (array) in context
    if (!duk_get_global_string(m_ctx, FUNC_ID)) {
        LOGE("EvalFilterFn - functions array not initialized");
        duk_pop(m_ctx); // pop [undefined] sitting at stack top
        return false;
    }

    // Get function at index `id` from functions array, put it at stack top
    if (!duk_get_prop_index(m_ctx, -1, id)) {
        LOGE("EvalFilterFn - function %d not set", id);
        duk_pop(m_ctx); // pop "undefined" sitting at stack top
        duk_pop(m_ctx); // pop functions (array) now sitting at stack top
        return false;
    }

    // pop fns array
    duk_remove(m_ctx, -2);

    // call popped function (sitting at stack top), evaluated value is put on stack top
    if (duk_pcall(m_ctx, 0) != 0) {
        LOGE("EvalFilterFn: %s", duk_safe_to_string(m_ctx, -1));
        duk_pop(m_ctx);
        return false;
    }

    return true;
}

bool StyleContext::evalFilter(FunctionID _id) {

    if (!evalFunction(_id)) { return false; };

    // Evaluate the "truthiness" of the function result at the top of the stack.
    bool result = duk_to_boolean(m_ctx, -1);

    // pop result
    duk_pop(m_ctx);

    return result;
}

bool StyleContext::evalStyle(FunctionID _id, StyleParamKey _key, StyleParam::Value& _val) {

    if (!evalFunction(_id)) { return false; }

    // parse evaluated result at stack top
    parseStyleResult(_key, _val);

    // pop result, empty stack
    duk_pop(m_ctx);

    return !_val.is<none_type>();
}

void StyleContext::parseStyleResult(StyleParamKey _key, StyleParam::Value& _val) const {
    _val = none_type{};

    if (duk_is_string(m_ctx, -1)) {
        std::string value(duk_get_string(m_ctx, -1));
        _val = StyleParam::parseString(_key, value);

    } else if (duk_is_boolean(m_ctx, -1)) {
        bool value = duk_get_boolean(m_ctx, -1);

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

    } else if (duk_is_array(m_ctx, -1)) {
        duk_get_prop_string(m_ctx, -1, "length");
        int len = duk_get_int(m_ctx, -1);
        duk_pop(m_ctx);

        switch (_key) {
            case StyleParamKey::extrude: {
                if (len != 2) {
                    LOGW("Wrong array size for extrusion: '%d'.", len);
                    break;
                }

                duk_get_prop_index(m_ctx, -1, 0);
                double v1 = duk_get_number(m_ctx, -1);
                duk_pop(m_ctx);

                duk_get_prop_index(m_ctx, -1, 1);
                double v2 = duk_get_number(m_ctx, -1);
                duk_pop(m_ctx);

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
                duk_get_prop_index(m_ctx, -1, 0);
                double r = duk_get_number(m_ctx, -1);
                duk_pop(m_ctx);

                duk_get_prop_index(m_ctx, -1, 1);
                double g = duk_get_number(m_ctx, -1);
                duk_pop(m_ctx);

                duk_get_prop_index(m_ctx, -1, 2);
                double b = duk_get_number(m_ctx, -1);
                duk_pop(m_ctx);

                double a = 1.0;
                if (len == 4) {
                    duk_get_prop_index(m_ctx, -1, 3);
                    a = duk_get_number(m_ctx, -1);
                    duk_pop(m_ctx);
                }

                _val = (((uint32_t)(255.0 * a) & 0xff) << 24) |
                       (((uint32_t)(255.0 * r) & 0xff)<< 16) |
                       (((uint32_t)(255.0 * g) & 0xff)<< 8) |
                       (((uint32_t)(255.0 * b) & 0xff));
                break;
            }
            default:
                break;
        }

    } else if (duk_is_nan(m_ctx, -1)) {
        // Ignore setting value
        LOGD("duk evaluates JS method to NAN.\n");
    } else if (duk_is_number(m_ctx, -1)) {

        switch (_key) {
            case StyleParamKey::extrude:
                _val = glm::vec2(0.f, static_cast<float>(duk_get_number(m_ctx, -1)));
                break;
            case StyleParamKey::placement_spacing: {
                double v = duk_get_number(m_ctx, -1);
                _val = StyleParam::Width{static_cast<float>(v), Unit::pixel};
                break;
            }
            case StyleParamKey::width:
            case StyleParamKey::outline_width: {
                // TODO more efficient way to return pixels.
                // atm this only works by return value as string
                double v = duk_get_number(m_ctx, -1);
                _val = StyleParam::Width{static_cast<float>(v)};
                break;
            }
            case StyleParamKey::text_font_stroke_width:
            case StyleParamKey::placement_min_length_ratio: {
                _val = static_cast<float>(duk_get_number(m_ctx, -1));
                break;
            }
            case StyleParamKey::order:
            case StyleParamKey::outline_order:
            case StyleParamKey::priority:
            case StyleParamKey::color:
            case StyleParamKey::outline_color:
            case StyleParamKey::text_font_fill:
            case StyleParamKey::text_font_stroke_color: {
                _val = static_cast<uint32_t>(duk_get_uint(m_ctx, -1));
                break;
            }
            default:
                break;
        }
    } else if (duk_is_null_or_undefined(m_ctx, -1)) {
        // Explicitly set value as 'undefined'. This is important for some styling rules.
        _val = Undefined();
    } else {
        LOGW("Unhandled return type from Javascript style function for %d.", _key);
    }

    DUMP("parseStyleResult\n");
}

// Implements Proxy handler.has(target_object, key)
duk_ret_t StyleContext::jsHasProperty(duk_context *_ctx) {

    duk_get_prop_string(_ctx, 0, INSTANCE_ID);
    auto* attr = static_cast<const StyleContext*> (duk_to_pointer(_ctx, -1));
    if (!attr || !attr->m_feature) {
        LOGE("Error: no context set %p %p", attr, attr ? attr->m_feature : nullptr);
        duk_pop(_ctx);
        return 0;
    }

    const char* key = duk_require_string(_ctx, 1);
    duk_push_boolean(_ctx, attr->m_feature->props.contains(key));

    return 1;
}

// Implements Proxy handler.get(target_object, key)
duk_ret_t StyleContext::jsGetProperty(duk_context *_ctx) {

    // Get the StyleContext instance from JS Feature object (first parameter).
    duk_get_prop_string(_ctx, 0, INSTANCE_ID);
    auto* attr = static_cast<const StyleContext*> (duk_to_pointer(_ctx, -1));
    if (!attr || !attr->m_feature) {
        LOGE("Error: no context set %p %p",  attr, attr ? attr->m_feature : nullptr);
        duk_pop(_ctx);
        return 0;
    }

    // Get the property name (second parameter)
    const char* key = duk_require_string(_ctx, 1);

    auto it = attr->m_feature->props.get(key);
    if (it.is<std::string>()) {
        duk_push_string(_ctx, it.get<std::string>().c_str());
    } else if (it.is<double>()) {
        duk_push_number(_ctx, it.get<double>());
    } else {
        duk_push_undefined(_ctx);
    }
    // FIXME: Distinguish Booleans here as well

    return 1;
}

}
