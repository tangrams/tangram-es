#include "styleContext.h"
#include "platform.h"
#include "builders.h"
#include "scene/scene.h"

#define DUMP(...) //do { logMsg(__VA_ARGS__); duk_dump_context_stderr(m_ctx); } while(0)


namespace Tangram {

const static char DATA_ID[] = "\xff""\xff""data";
const static char ATTR_ID[] = "\xff""\xff""attr";
const static char FUNC_ID[] = "\xff""\xff""fns";

StyleContext::StyleContext() {
    m_ctx = duk_create_heap_default();

    // add empty feature_object
    duk_push_object(m_ctx);

    // assign instance to feature_object
    duk_push_pointer(m_ctx, this);
    if (!duk_put_prop_string(m_ctx, -2, DATA_ID)) {
        logMsg("Error: ctx not assigned\n");
    }

    // put object in global scope
    if (!duk_put_global_string(m_ctx, "feature")) {
        logMsg("Error: feature not assigned\n");
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
        //logMsg("compile '%s'\n", function.c_str());
        duk_push_string(m_ctx, function.c_str());
        duk_push_string(m_ctx, "");

        if (duk_pcompile(m_ctx, DUK_COMPILE_FUNCTION) == 0) {
            duk_put_prop_index(m_ctx, arr_idx, id);
        } else {
            logMsg("Error: compile failed: %s\n", duk_safe_to_string(m_ctx, -1));
            duk_pop(m_ctx);
        }
        id++;
    }

    if (!duk_put_global_string(m_ctx, FUNC_ID)) {
        logMsg("Error: 'fns' object not set\n");
    }

    DUMP("setScene - %d functions\n", id);
}

void StyleContext::setFeature(const Feature& _feature) {
    m_feature = &_feature;

    for (auto& item : _feature.props.items()) {
        addAccessor(item.key);
    }
}

void StyleContext::setGlobalZoom(float _zoom) {
    static const std::string _key("$zoom");
    if (_zoom != m_globalZoom) {
        setGlobal(_key, _zoom);
    }
}

void StyleContext::setGlobal(const std::string& _key, const Value& _val) {
    Value& entry = m_globals[_key];
    if (entry == _val) { return; }

    entry = _val;

    if (_val.is<float>()) {
        duk_push_number(m_ctx, _val.get<float>());
        duk_put_global_string(m_ctx, _key.c_str());

        if (_key == "$zoom") { m_globalZoom = _val.get<float>(); }

    } else if (_val.is<std::string>()) {
        duk_push_string(m_ctx, _val.get<std::string>().c_str());
        duk_put_global_string(m_ctx, _key.c_str());
    }
}

const Value& StyleContext::getGlobal(const std::string& _key) const {

    const static Value NOT_FOUND(none_type{});

    auto it = m_globals.find(_key);
    if (it != m_globals.end()) {
        return it->second;
    }
    return NOT_FOUND;
}


void StyleContext::clear() {
    m_feature = nullptr;
}

bool StyleContext::addFunction(const std::string& _name, const std::string& _func) {

    duk_push_string(m_ctx, _func.c_str());
    duk_push_string(m_ctx, _name.c_str());

    if (duk_pcompile(m_ctx, DUK_COMPILE_FUNCTION) != 0) {
        logMsg("Error: compile failed: %s\n", duk_safe_to_string(m_ctx, -1));
        return false;
    }

    // Put function in global scope
    duk_put_global_string(m_ctx, _name.c_str());


    DUMP("addFunction\n");
    return true;
}

bool StyleContext::evalFilter(FunctionID _id) const {
    if (!duk_get_global_string(m_ctx, FUNC_ID)) {
        logMsg("Error: evalFilterFn - functions not initialized\n");
        return false;
    }

    if (!duk_get_prop_index(m_ctx, -1, _id)) {
        logMsg("Error: evalFilterFn - function %d not set\n", _id);
    }

    if (duk_pcall(m_ctx, 0) != 0) {
        logMsg("Error: evalFilterFn: %s\n", duk_safe_to_string(m_ctx, -1));
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
    if (!duk_get_global_string(m_ctx, _name.c_str())) {
        logMsg("Error: evalFilter %s\n", _name.c_str());
        return false;
    }

    if (duk_pcall(m_ctx, 0) != 0) {
        logMsg("Error: evalFilterFn: %s\n", duk_safe_to_string(m_ctx, -1));
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
                    logMsg("Warning: Wrong array size for extrusion: '%d'.\n", len);
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
            case StyleParamKey::font_stroke:
            case StyleParamKey::font_stroke_color: {
                if (len < 3 || len > 4) {
                    logMsg("Warning: Wrong array size for color: '%d'.\n", len);
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

    } else if (duk_is_number(m_ctx, -1)) {

        switch (_key) {
            case StyleParamKey::width:
            case StyleParamKey::outline_width:
            case StyleParamKey::font_stroke_width: {
                double v = duk_get_number(m_ctx, -1);
                _val = static_cast<float>(v);
                break;
            }
            case StyleParamKey::order:
            case StyleParamKey::priority:
            case StyleParamKey::color:
            case StyleParamKey::outline_color:
            case StyleParamKey::font_fill:
            case StyleParamKey::font_stroke:
            case StyleParamKey::font_stroke_color: {
                _val = static_cast<uint32_t>(duk_get_uint(m_ctx, -1));
                break;
            }
            default:
                break;
        }
    } else {
        logMsg("Warning: Unhandled return type from Javascript function.\n");
    }

    duk_pop(m_ctx);

    DUMP("parseStyleResult\n");
    return !_val.is<none_type>();
}

bool StyleContext::evalStyle(FunctionID _id, StyleParamKey _key, StyleParam::Value& _val) const {
    if (!duk_get_global_string(m_ctx, FUNC_ID)) {
        logMsg("Error: evalFilterFn - functions array not initialized\n");
        return false;
    }

    if (!duk_get_prop_index(m_ctx, -1, _id)) {
        logMsg("Error: evalFilterFn - function %d not set\n", _id);
    }

    // pop fns array
    duk_remove(m_ctx, -2);

    if (duk_pcall(m_ctx, 0) != 0) {
        logMsg("Error: evalFilterFn: %s\n", duk_safe_to_string(m_ctx, -1));
        duk_pop(m_ctx);
        return false;
    }

    return parseStyleResult(_key, _val);
}


bool StyleContext::evalStyleFn(const std::string& name, StyleParamKey _key, StyleParam::Value& _val) {
    if (!duk_get_global_string(m_ctx, name.c_str())) {
        logMsg("Error: evalFilter %s\n", name.c_str());
        return false;
    }

    if (duk_pcall(m_ctx, 0) != 0) {
        logMsg("Error: evalStyleFn: %s\n", duk_safe_to_string(m_ctx, -1));
        duk_pop(m_ctx);
        return false;
    }

    return parseStyleResult(_key, _val);
}


void StyleContext::addAccessor(const std::string& _name) {

    auto it = m_accessors.find(_name);
    if (it != m_accessors.end()) {
        return;
    }

    auto entry = m_accessors.emplace(_name, Accessor{_name, this});
    if (!entry.second) {
        return; // hmm, already added..
    }

    Accessor& attr = (*entry.first).second;

    // push 'feature' obj onto stack
    if (!duk_get_global_string(m_ctx, "feature")) {
        logMsg("Error: 'feature' not in global scope\n");
        return;
    }

    // push property name
    duk_push_string(m_ctx, _name.c_str());

    // push getter function
    duk_push_c_function(m_ctx, jsPropertyGetter, 0 /*nargs*/);
    duk_push_pointer(m_ctx, (void*)&attr);
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
        logMsg("Error: no context set %p %p\n",
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
    } else {
        duk_push_undefined(_ctx);
    }

    return 1;
}

duk_ret_t StyleContext::jsPropertySetter(duk_context *_ctx) {
    return 0;
}

}
