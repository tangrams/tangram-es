#include "filterContext.h"
#include "platform.h"
#include "builders.h"

// TODO
// - add new properties from js? Not needed for filter I guess

#define DUMP(...) do { logMsg(__VA_ARGS__); duk_dump_context_stderr(m_ctx); } while(0)


namespace Tangram {

const static char DATA_ID[] = "\xff""\xff""data";
const static char ATTR_ID[] = "\xff""\xff""attr";

FilterContext::FilterContext() {
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

FilterContext::~FilterContext() {
    duk_destroy_heap(m_ctx);
}

void FilterContext::setFeature(const Feature& feature) {
    m_feature = &feature;

    for (auto& item : feature.props.items()) {
        addAccessor(item.key);
    }
}

void FilterContext::setGlobal(const std::string& key, const Value& value) {
    if (value.is<float>()) {
        duk_push_number(m_ctx, value.get<float>());
        duk_put_global_string(m_ctx, key.c_str());

    } else if (value.is<std::string>()) {
        duk_push_string(m_ctx, value.get<std::string>().c_str());
        duk_put_global_string(m_ctx, key.c_str());
    }
}

void FilterContext::clear() {
    m_feature = nullptr;
    m_dirty = true;
}

bool FilterContext::addFilterFn(const std::string& name, const std::string& function) {

    duk_push_string(m_ctx, function.c_str());
    duk_push_string(m_ctx, name.c_str());

    if (duk_pcompile(m_ctx, DUK_COMPILE_FUNCTION) != 0) {
        logMsg("Error: compile failed: %s\n", duk_safe_to_string(m_ctx, -1));
        return false;
    }

    // DUMP("add1\n");

    // if (!duk_get_global_string(m_ctx, "$feature")) {
    //     logMsg("Error: 'feature' not in global scope\n");
    //     return false;
    // }

    // DUMP("add2\n");

    // // Assign $feature to function
    // if (!duk_put_prop_string(m_ctx, -2, "feature")) {
    //     logMsg("Error: feature not assigned to function\n");
    //     return false;
    // }
    // DUMP("add3\n");

    // Put function in global scope
    duk_put_global_string(m_ctx, name.c_str());


    DUMP("add4\n");

    return true;
}

bool FilterContext::evalFilterFn(const std::string& name) {
    if (!duk_get_global_string(m_ctx, name.c_str())) {
        logMsg("Error: evalFilter %s\n", name.c_str());
        return false;
    }

    // Use function also as 'this' obj
    if (duk_pcall(m_ctx, 0) != 0) {
        logMsg("Error: evalFilterFn: %s\n", duk_safe_to_string(m_ctx, -1));
    }

    if (duk_is_boolean(m_ctx, -1)) {
        return duk_get_boolean(m_ctx, -1);
    }

    DUMP("evalFilterFn\n");

    return false;
}

bool FilterContext::evalStyleFn(const std::string& name, StyleParamKey key, StyleParam::Value& value) {
    if (!duk_get_global_string(m_ctx, name.c_str())) {
        logMsg("Error: evalFilter %s\n", name.c_str());
        return false;
    }

    // Use function also as 'this' obj
    if (duk_pcall(m_ctx, 0) != 0) {
        logMsg("Error: evalStyleFn: %s\n", duk_safe_to_string(m_ctx, -1));
    }

    // if (duk_is_boolean(m_ctx, -1)) {
    //     return duk_get_boolean(m_ctx, -1);
    // }

    DUMP("evalFilterFn\n");

    switch (key) {
        case StyleParamKey::order:
        {
            if (duk_is_number(m_ctx, -1)) {
                int v = duk_get_int(m_ctx, -1);
                value = static_cast<int32_t>(v);
                return true;
            }
            break;
        }
        case StyleParamKey::width:
        case StyleParamKey::outline_width:
        {
            if (duk_is_number(m_ctx, -1)) {
                double v = duk_get_number(m_ctx, -1);
                value = static_cast<float>(v);
                return true;
            }
            break;
        }
        case StyleParamKey::color:
        case StyleParamKey::outline_color:
        {
            if (duk_is_string(m_ctx, -1)) {
                std::string v(duk_get_string(m_ctx, -1));
                value = DrawRule::parseColor(v);
                return true;
            } else if (duk_is_number(m_ctx, -1)) {
                int v = duk_get_int(m_ctx, -1);
                // TODO
                return true;
            }
            // TODO color object
            break;
        }
        case StyleParamKey::cap:
        case StyleParamKey::outline_cap:
        {
            std::string v(duk_get_string(m_ctx, -1));
            value = CapTypeFromString(v);
            return true;
        }
        case StyleParamKey::join:
        case StyleParamKey::outline_join:
        {
            std::string v(duk_get_string(m_ctx, -1));
            value = JoinTypeFromString(v);
            return true;
        }
        default:
            value = none_type{};
    }

    return false;
}


void FilterContext::addAccessor(const std::string& name) {

    auto entry = m_accessors.emplace(name);
    if (!entry.second) {
        return; // already added
    }
    auto& attr = *entry.first;

    // push 'feature' obj onto stack
    // duk_push_global_object(m_ctx);
    // duk_get_prop_string(m_ctx, -1, "feature");
    if (!duk_get_global_string(m_ctx, "feature")) {
        logMsg("Error: 'feature' not in global scope\n");
        return;
    }

    // push property name
    duk_push_string(m_ctx, name.c_str());

    // push getter function
    duk_push_c_function(m_ctx, jsPropertyGetter, 0 /*nargs*/);
    duk_push_pointer(m_ctx, (void*)&attr);
    duk_put_prop_string(m_ctx, -2, ATTR_ID);

    // push setter function
    duk_push_c_function(m_ctx, jsPropertySetter, 1 /*nargs*/);
    duk_push_pointer(m_ctx, (void*)&attr);
    duk_put_prop_string(m_ctx, -2, ATTR_ID);

    // stack: [ feature_obj, name, getter, setter ] -> [ feature_obj.name ]
    duk_def_prop(m_ctx, -4,
                 DUK_DEFPROP_HAVE_GETTER |
                 DUK_DEFPROP_HAVE_SETTER |
                 DUK_DEFPROP_WRITABLE |
                 /* Note: ignored, no "have writable" flag */
                 /* enumerable defaults */
                 //DUK_DEFPROP_HAVE_ENUMERABLE | DUK_DEFPROP_ENUMERABLE |
                 DUK_DEFPROP_HAVE_CONFIGURABLE | 0);

    duk_pop(m_ctx);

    DUMP("addAccessor\n");

}

duk_ret_t FilterContext::jsPropertyGetter(duk_context *ctx) {

    // get data from object to which this function belongs
    duk_push_this(ctx);

    if (!duk_get_prop_string(ctx, -1, DATA_ID)) {
        printf("no data\n");
        duk_pop(ctx);
        return 0;
    }

    auto* filterCtx = static_cast<FilterContext*> (duk_to_pointer(ctx, -1));
    duk_pop(ctx);

    if (filterCtx == nullptr) {
        printf("no context set %p\n", filterCtx);
        return 0;
    }
    if (filterCtx->m_feature == nullptr) {
        printf("no feature set %p\n", filterCtx);
        return 0;
    }

    // Storing state for a Duktape/C function:
    // http://duktape.org/guide.html#programming.9
    duk_push_current_function(ctx);
    duk_get_prop_string(ctx, -1, ATTR_ID);
    auto* key = static_cast<const std::string*> (duk_to_pointer(ctx, -1));

    auto it = filterCtx->m_feature->props.get(*key);

    if (it.is<std::string>()) {
        duk_push_string(ctx, it.get<std::string>().c_str());
    } else if (it.is<float>()) {
        duk_push_number(ctx, it.get<float>());
    } else {
        duk_push_undefined(ctx);
    }

    return 1;
}

duk_ret_t FilterContext::jsPropertySetter(duk_context *ctx) {
#if 0
    // get data from object to which this function belongs
    duk_push_this(ctx);

    if (!duk_get_prop_string(ctx, -1, DATA_ID)) {
        printf("no data\n");
        duk_pop(ctx);
        return 0;
    }

    auto* filterCtx = static_cast<FilterContext*> (duk_to_pointer(ctx, -1));
    duk_pop(ctx);

    if (filterCtx == nullptr) {
        printf("no context set %p\n", filterCtx);
        return 0;
    }
    if (filterCtx->m_feature == nullptr) {
        printf("no feature set %p\n", filterCtx);
        return 0;
    }

    // Storing state for a Duktape/C function:
    // http://duktape.org/guide.html#programming.9
    duk_push_current_function(ctx);
    duk_get_prop_string(ctx, -1, ATTR_ID);
    auto* key = static_cast<const std::string*> (duk_to_pointer(ctx, -1));

    auto it = filterCtx->m_feature->props.get(*key);

    if (duk_is_string(ctx, 0)) {
    } else if (duk_is_number(ctx, 0)) {
    } else {
    }
#endif
    return 0;
}

}
