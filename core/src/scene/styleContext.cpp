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

const static char DATA_ID[] = "\xff""\xff""data";
const static char ATTR_ID[] = "\xff""\xff""attr";
const static char FUNC_ID[] = "\xff""\xff""fns";

static const std::string key_geometry = "$geometry";
static const std::string key_zoom("$zoom");

StyleContext::StyleContext() {
    m_ctx = duk_create_heap_default();

    // add empty feature_object
    duk_push_object(m_ctx);

    // assign instance to feature_object
    duk_push_pointer(m_ctx, this);
    if (!duk_put_prop_string(m_ctx, -2, DATA_ID)) {
        LOGE("Ctx not assigned");
    }

    // put object in global scope
    if (!duk_put_global_string(m_ctx, "feature")) {
        LOGE("Feature not assigned");
    }

    duk_push_number(m_ctx, GeometryType::points);
    duk_put_global_string(m_ctx, "point");

    duk_push_number(m_ctx, GeometryType::lines);
    duk_put_global_string(m_ctx, "line");

    duk_push_number(m_ctx, GeometryType::polygons);
    duk_put_global_string(m_ctx, "polygon");

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
    m_featureIsReady = false;
}

void StyleContext::setGlobalZoom(float _zoom) {
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

    Value& entry = m_globals[globalKey];
    if (entry == _val) { return; }

    entry = _val;

    if (_val.is<float>()) {
        duk_push_number(m_ctx, _val.get<float>());
        duk_put_global_string(m_ctx, _key.c_str());

        if (_key == "$zoom") { m_globalZoom = _val.get<float>(); }

    } else if (_val.is<std::string>()) {
        duk_push_string(m_ctx, _val.get<std::string>().c_str());
        duk_put_global_string(m_ctx, _key.c_str());

    } else if (_val.is<int64_t>()) {
        duk_push_number(m_ctx, (int)_val.get<int64_t>());
        duk_put_global_string(m_ctx, _key.c_str());
    }
}

const Value& StyleContext::getGlobal(FilterGlobal _key) const {
    const static Value NOT_FOUND(none_type{});

    auto it = m_globals.find(_key);
    if (it != m_globals.end()) {
        return it->second;
    }
    return NOT_FOUND;

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
    if (!m_featureIsReady) {
        setAccessors();
    }

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

bool StyleContext::evalFilterFn(const std::string& _name) {
    if (!m_featureIsReady) {
        setAccessors();
    }

    if (!duk_get_global_string(m_ctx, _name.c_str())) {
        LOGE("EvalFilter %s", _name.c_str());
        return false;
    }

    if (duk_pcall(m_ctx, 0) != 0) {
        LOGE("EvalFilterFn: %s", duk_safe_to_string(m_ctx, -1));
    }

    bool result = false;

    if (duk_is_boolean(m_ctx, -1)) {
        result = duk_get_boolean(m_ctx, -1);
    }

    // pop result
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
    if (!m_featureIsReady) {
        setAccessors();
    }

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


bool StyleContext::evalStyleFn(const std::string& name, StyleParamKey _key, StyleParam::Value& _val) {
    if (!m_featureIsReady) {
        setAccessors();
    }

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


void StyleContext::setAccessors() {

    m_featureIsReady = true;

    if (!m_feature) { return; }

    setGlobal(key_geometry, m_feature->geometryType);

    for (auto& item : m_feature->props.items()) {
        addAccessor(item.key);
    }
}

void StyleContext::addAccessor(const std::string& _name) {

    auto& entry = m_accessors[_name];
    if (entry) { return; }

    entry = std::make_unique<Accessor>();
    entry->key = _name;
    entry->ctx = this;

    // push 'feature' obj onto stack
    if (!duk_get_global_string(m_ctx, "feature")) {
        LOGE("'feature' not in global scope");
        return;
    }

    // push property name
    duk_push_string(m_ctx, _name.c_str());

    // push getter function
    duk_push_c_function(m_ctx, jsPropertyGetter, 0 /*nargs*/);
    duk_push_pointer(m_ctx, (void*)entry.get());
    duk_put_prop_string(m_ctx, -2, ATTR_ID);

    // push setter function
    // duk_push_c_function(m_ctx, jsPropertySetter, 1 /*nargs*/);
    // duk_push_pointer(m_ctx, (void*)&attr);
    // duk_put_prop_string(m_ctx, -2, ATTR_ID);

    // stack: [ feature_obj, name, getter, setter ] -> [ feature_obj.name ]
    duk_def_prop(m_ctx, -3,
                 DUK_DEFPROP_HAVE_GETTER |
                 // DUK_DEFPROP_HAVE_SETTER |
                 // DUK_DEFPROP_WRITABLE |
                 // DUK_DEFPROP_HAVE_ENUMERABLE |
                 // DUK_DEFPROP_ENUMERABLE |
                 // DUK_DEFPROP_HAVE_CONFIGURABLE |
                 0);

    // pop feature obj
    duk_pop(m_ctx);

    DUMP("addAccessor\n");
}

duk_ret_t StyleContext::jsPropertyGetter(duk_context *_ctx) {

    // Storing state for a Duktape/C function:
    // http://duktape.org/guide.html#programming.9
    duk_push_current_function(_ctx);
    duk_get_prop_string(_ctx, -1, ATTR_ID);
    auto* attr = static_cast<const Accessor*> (duk_to_pointer(_ctx, -1));

    if (!attr || !attr->ctx || !attr->ctx->m_feature) {
        LOGE("Error: no context set %p %p",
               attr,
               attr ? attr->ctx : nullptr);

        duk_pop(_ctx);
        return 0;
    }

    auto it = attr->ctx->m_feature->props.get(attr->key);

    if (it.is<std::string>()) {
        duk_push_string(_ctx, it.get<std::string>().c_str());
    } else if (it.is<float>()) {
        duk_push_number(_ctx, it.get<float>());
    } else if (it.is<int64_t>()) {
        duk_push_number(_ctx, it.get<int64_t>());
    } else {
        duk_push_undefined(_ctx);
    }

    return 1;
}

duk_ret_t StyleContext::jsPropertySetter(duk_context *_ctx) {
    return 0;
}

}
