#include "styleContext.h"

#include "platform.h"
#include "data/propertyItem.h"
#include "data/tileData.h"
#include "scene/filters.h"
#include "scene/scene.h"
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

void StyleContext::initFunctions(const Scene& _scene) {

    if (_scene.id == m_sceneId) {
        return;
    }
    m_sceneId = _scene.id;

    auto arr_idx = duk_push_array(m_ctx);
    int id = 0;

    for (auto& function : _scene.functions()) {
        LOGD("compile '%s'", function.c_str());
        duk_push_string(m_ctx, function.c_str());
        duk_push_string(m_ctx, "");

        if (duk_pcompile(m_ctx, DUK_COMPILE_FUNCTION) == 0) {
            duk_put_prop_index(m_ctx, arr_idx, id);
        } else {
            LOGE("Compile failed: %s", duk_safe_to_string(m_ctx, -1));
            duk_pop(m_ctx);
        }
        id++;
    }

    if (!duk_put_global_string(m_ctx, FUNC_ID)) {
        LOGE("'fns' object not set");
    }

    DUMP("setScene - %d functions\n", id);
}

void StyleContext::setFeature(const Feature& _feature) {

    m_feature = &_feature;

    if (m_feature->geometryType != m_globalGeom) {
        //setGlobal(key_geom, m_feature->geometryType);
        setGlobal(key_geom, s_geometryStrings[m_feature->geometryType]);
    }
}

void StyleContext::setGlobalZoom(int _zoom) {
    if (_zoom != m_globalZoom) {
        setGlobal(key_zoom, _zoom);
    }
}

void StyleContext::setGlobal(const std::string& _key, const Value& _val) {
    auto globalKey = Filter::globalType(_key);
    if (globalKey == FilterGlobal::undefined) {
        LOG("Undefined Global: %s", _key.c_str());
        return;
    }

    Value& entry = m_globals[static_cast<uint8_t>(globalKey)];
    if (entry == _val) { return; }

    entry = _val;

    if (_val.is<double>()) {
        duk_push_number(m_ctx, _val.get<double>());
        duk_put_global_string(m_ctx, _key.c_str());

        if (_key == key_zoom) { m_globalZoom = _val.get<double>(); }
        if (_key == key_geom) { m_globalGeom = _val.get<double>(); }

    } else if (_val.is<std::string>()) {
        duk_push_string(m_ctx, _val.get<std::string>().c_str());
        duk_put_global_string(m_ctx, _key.c_str());
    }
}

const Value& StyleContext::getGlobal(const std::string& _key) const {
    return getGlobal(Filter::globalType(_key));
}

void StyleContext::clear() {
    m_feature = nullptr;
}

bool StyleContext::addFunction(const std::string& _name, const std::string& _func) {

    duk_push_string(m_ctx, _func.c_str());
    duk_push_string(m_ctx, _name.c_str());

    if (duk_pcompile(m_ctx, DUK_COMPILE_FUNCTION) != 0) {
        LOGE("Compile failed: %s", duk_safe_to_string(m_ctx, -1));
        return false;
    }

    // Put function in global scope
    duk_put_global_string(m_ctx, _name.c_str());

    DUMP("addFunction\n");
    return true;
}

bool StyleContext::evalFilter(FunctionID _id) {

    if (!duk_get_global_string(m_ctx, FUNC_ID)) {
        LOGE("EvalFilterFn - functions not initialized");
        return false;
    }

    if (!duk_get_prop_index(m_ctx, -1, _id)) {
        LOGE("EvalFilterFn - function %d not set", _id);
        duk_pop(m_ctx);
        DBG("evalFilterFn\n");
        return false;
    }

    if (duk_pcall(m_ctx, 0) != 0) {
        LOGE("EvalFilterFn: %s", duk_safe_to_string(m_ctx, -1));
        duk_pop(m_ctx);
        duk_pop(m_ctx);
        DBG("evalFilterFn\n");
        return false;
    }

    bool result = false;

    if (duk_is_boolean(m_ctx, -1)) {
        result = duk_get_boolean(m_ctx, -1);
    }

    // pop result
    duk_pop(m_ctx);
    // pop fns obj
    duk_pop(m_ctx);

    DUMP("evalFilterFn\n");
    return result;
}

bool StyleContext::parseStyleResult(StyleParamKey _key, StyleParam::Value& _val) const {
    _val = none_type{};

    if (duk_is_string(m_ctx, -1)) {
        std::string value(duk_get_string(m_ctx, -1));
        _val = StyleParam::parseString(_key, value);

    } else if (duk_is_boolean(m_ctx, -1)) {
        bool value = duk_get_boolean(m_ctx, -1);

        switch (_key) {
            case StyleParamKey::interactive:
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
            case StyleParamKey::font_fill:
            case StyleParamKey::font_stroke_color: {
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
            case StyleParamKey::width:
            case StyleParamKey::outline_width: {
                // TODO more efficient way to return pixels.
                // atm this only works by return value as string
                double v = duk_get_number(m_ctx, -1);
                _val = StyleParam::Width{static_cast<float>(v)};
                break;
            }
            case StyleParamKey::font_stroke_width: {
                _val = static_cast<float>(duk_get_number(m_ctx, -1));
                break;
            }
            case StyleParamKey::order:
            case StyleParamKey::outline_order:
            case StyleParamKey::priority:
            case StyleParamKey::color:
            case StyleParamKey::outline_color:
            case StyleParamKey::font_fill:
            case StyleParamKey::font_stroke_color: {
                _val = static_cast<uint32_t>(duk_get_uint(m_ctx, -1));
                break;
            }
            default:
                break;
        }
    } else if (duk_is_null_or_undefined(m_ctx, -1)) {
        // Ignore setting value
        LOGD("duk evaluates JS method to null or undefined.\n");
    } else {
        LOGW("Unhandled return type from Javascript style function for %d.", _key);
    }

    duk_pop(m_ctx);

    DUMP("parseStyleResult\n");
    return !_val.is<none_type>();
}

bool StyleContext::evalStyle(FunctionID _id, StyleParamKey _key, StyleParam::Value& _val) {

    if (!duk_get_global_string(m_ctx, FUNC_ID)) {
        LOGE("EvalFilterFn - functions array not initialized");
        return false;
    }

    if (!duk_get_prop_index(m_ctx, -1, _id)) {
        LOGE("EvalFilterFn - function %d not set", _id);
    }

    // pop fns array
    duk_remove(m_ctx, -2);

    if (duk_pcall(m_ctx, 0) != 0) {
        LOGE("EvalFilterFn: %s", duk_safe_to_string(m_ctx, -1));
        duk_pop(m_ctx);
        return false;
    }

    return parseStyleResult(_key, _val);
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

    return 1;
}

/* This function is only used by tests - Remove? */
bool StyleContext::evalFilterFn(const std::string& _name) {

    if (!duk_get_global_string(m_ctx, _name.c_str())) {
        LOGE("EvalFilter %s", _name.c_str());
        return false;
    }

    if (duk_pcall(m_ctx, 0) != 0) {
        LOGE("EvalFilterFn: %s", duk_safe_to_string(m_ctx, -1));
        duk_pop(m_ctx);
        return false;
    }

    bool result = false;

    if (duk_is_boolean(m_ctx, -1)) {
        result = duk_get_boolean(m_ctx, -1);
    } else {
        LOGE("EvalFilterFn: invalid return type");
    }

    // pop result
    duk_pop(m_ctx);

    DUMP("evalFilterFn\n");
    return result;
}

/* This function is only used by tests - Remove? */
bool StyleContext::evalStyleFn(const std::string& name, StyleParamKey _key, StyleParam::Value& _val) {

    if (!duk_get_global_string(m_ctx, name.c_str())) {
        LOGE("EvalFilter %s", name.c_str());
        return false;
    }

    if (duk_pcall(m_ctx, 0) != 0) {
        LOGE("EvalStyleFn: %s", duk_safe_to_string(m_ctx, -1));
        duk_pop(m_ctx);
        return false;
    }

    return parseStyleResult(_key, _val);
}

}
