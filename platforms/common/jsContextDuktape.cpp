//
// Created by Matt Blair on 11/9/18.
//
#include <log.h>
#include "jsContext.h"

#include "data/tileData.h"
#include "util/variant.h"

#include "duktape.h"

namespace Tangram {
namespace JsContext {

const static char INSTANCE_ID[] = "\xff""\xff""obj";
const static char FUNC_ID[] = "\xff""\xff""fns";

struct Instance {
    duk_context* ctx = nullptr;
    Feature* feature = nullptr;
};

// Used for proxy object.
static int jsGetProperty(duk_context *_ctx);
static int jsHasProperty(duk_context *_ctx);

Instance* create() {
    auto ctx = duk_create_heap_default();
    auto instance = new Instance();
    instance->ctx = ctx;

    //// Create global geometry constants
    // TODO make immutable
    duk_push_number(ctx, GeometryType::points);
    duk_put_global_string(ctx, "point");

    duk_push_number(ctx, GeometryType::lines);
    duk_put_global_string(ctx, "line");

    duk_push_number(ctx, GeometryType::polygons);
    duk_put_global_string(ctx, "polygon");

    //// Create global 'feature' object
    // Get Proxy constructor
    // -> [cons]
    duk_eval_string(ctx, "Proxy");

    // Add feature object
    // -> [cons, { __obj: this }]
    duk_idx_t featureObj = duk_push_object(ctx);
    duk_push_pointer(ctx, instance);
    duk_put_prop_string(ctx, featureObj, INSTANCE_ID);

    // Add handler object
    // -> [cons, {...}, { get: func, has: func }]
    duk_idx_t handlerObj = duk_push_object(ctx);
    // Add 'get' property to handler
    duk_push_c_function(ctx, jsGetProperty, 3 /*nargs*/);
    duk_put_prop_string(ctx, handlerObj, "get");
    // Add 'has' property to handler
    duk_push_c_function(ctx, jsHasProperty, 2 /*nargs*/);
    duk_put_prop_string(ctx, handlerObj, "has");

    // Call proxy constructor
    // [cons, feature, handler ] -> [obj|error]
    if (duk_pnew(ctx, 2) == 0) {
        // put feature proxy object in global scope
        if (!duk_put_global_string(ctx, "feature")) {
            LOGE("Initialization failed");
        }
    } else {
        LOGE("Failure: %s", duk_safe_to_string(ctx, -1));
        duk_pop(ctx);
    }

    // Set up 'fns' array.
    duk_push_array(ctx);
    if (!duk_put_global_string(ctx, FUNC_ID)) {
        LOGE("'fns' object not set");
    }

    return instance;
}

void destroy(Instance* instance) {
    duk_destroy_heap(instance->ctx);
    delete instance;
}

void setGlobalString(Instance* instance, const std::string& name, const std::string& value) {
    auto ctx = instance->ctx;
    duk_push_string(ctx, value.c_str());
    duk_put_global_string(ctx, name.c_str());
}

void setGlobalNumber(Instance* instance, const std::string& name, double value) {
    auto ctx = instance->ctx;
    duk_push_number(ctx, value);
    duk_put_global_string(ctx, name.c_str());
}

void setCurrentFeature(Instance* instance, Feature* feature) {
    instance->feature = feature;
}

uint32_t addFunction(Instance* instance, const std::string& source, bool& error) {
    auto ctx = instance->ctx;
    // Get all functions (array) in context
    if (!duk_get_global_string(ctx, FUNC_ID)) {
        LOGE("AddFunction - functions array not initialized");
        duk_pop(ctx); // pop [undefined] sitting at stack top
        return false;
    }

    uint32_t index = duk_get_length(ctx, -1);

    duk_push_string(ctx, source.c_str());
    duk_push_string(ctx, "");

    if (duk_pcompile(ctx, DUK_COMPILE_FUNCTION) == 0) {
        duk_put_prop_index(ctx, -2, index);
    } else {
        LOGW("Compile failed: %s\n%s\n---",
             duk_safe_to_string(ctx, -1),
             source.c_str());
        duk_pop(ctx);
        error = true;
    }

    // Pop the functions array off the stack
    duk_pop(ctx);

    return index;
}

bool evalulateFunction(duk_context* ctx, uint32_t index) {
    // Get all functions (array) in context
    if (!duk_get_global_string(ctx, FUNC_ID)) {
        LOGE("EvalFilterFn - functions array not initialized");
        duk_pop(ctx); // pop [undefined] sitting at stack top
        return false;
    }

    // Get function at index `id` from functions array, put it at stack top
    if (!duk_get_prop_index(ctx, -1, index)) {
        LOGE("EvalFilterFn - function %d not set", index);
        duk_pop(ctx); // pop "undefined" sitting at stack top
        duk_pop(ctx); // pop functions (array) now sitting at stack top
        return false;
    }

    // pop fns array
    duk_remove(ctx, -2);

    // call popped function (sitting at stack top), evaluated value is put on stack top
    if (duk_pcall(ctx, 0) != 0) {
        LOGE("EvalFilterFn: %s", duk_safe_to_string(ctx, -1));
        duk_pop(ctx);
        return false;
    }

    return true;
}

bool evaluateBooleanFunction(Instance* instance, uint32_t index) {
    auto ctx = instance->ctx;

    if (!evalulateFunction(ctx, index)) {
        return false;
    }

    // Evaluate the "truthiness" of the function result at the top of the stack.
    bool result = duk_to_boolean(ctx, -1) != 0;

    // pop result
    duk_pop(ctx);

    return result;
}

struct Value {
    duk_idx_t index = 0;
};

Value* getFunctionResult(Instance* instance, uint32_t index) {
    auto ctx = instance->ctx;
    if (!evalulateFunction(ctx, index)) {
        return nullptr;
    }

    auto value = new Value;
    value->index = duk_get_top_index(ctx);
    return value;
}

void releaseValue(Instance* instance, Value* value) {
    auto ctx = instance->ctx;
    if (value) {
        duk_remove(ctx, value->index);
    }
    delete value;
}

bool valueIsNull(Instance* instance, Value* value) {
    auto ctx = instance->ctx;
    return duk_is_null(ctx, value->index) != 0;
}

bool valueIsBool(Instance* instance, Value* value) {
    auto ctx = instance->ctx;
    return duk_is_boolean(ctx, value->index) != 0;
}

bool valueIsNumber(Instance* instance, Value* value) {
    auto ctx = instance->ctx;
    return duk_is_number(ctx, value->index) != 0;
}

bool valueIsString(Instance* instance, Value* value) {
    auto ctx = instance->ctx;
    return duk_is_string(ctx, value->index) != 0;
}

bool valueIsArray(Instance* instance, Value* value) {
    auto ctx = instance->ctx;
    return duk_is_array(ctx, value->index) != 0;
}

bool valueIsObject(Instance* instance, Value* value) {
    auto ctx = instance->ctx;
    return duk_is_object(ctx, value->index) != 0;
}

bool valueIsUndefined(Instance* instance, Value* value) {
    auto ctx = instance->ctx;
    return duk_is_undefined(ctx, value->index) != 0;
}

bool valueGetBool(Instance* instance, Value* value) {
    auto ctx = instance->ctx;
    return duk_get_boolean(ctx, value->index) != 0;
}

double valueGetDouble(Instance* instance, Value* value) {
    auto ctx = instance->ctx;
    return duk_get_number(ctx, value->index);
}

std::string valueGetString(Instance* instance, Value* value) {
    auto ctx = instance->ctx;
    auto str = duk_get_string(ctx, value->index);
    return std::string(str);
}

size_t valueGetArraySize(Instance* instance, Value* value) {
    auto ctx = instance->ctx;
    return duk_get_length(ctx, value->index);
}

Value* valueGetArrayElement(Instance* instance, Value* value, size_t index) {
    auto ctx = instance->ctx;
    duk_get_prop_index(ctx, value->index, index);
    auto element = new Value();
    element->index = duk_get_top_index(ctx);
    return element;
}


// Implements Proxy handler.has(target_object, key)
int jsHasProperty(duk_context *_ctx) {

    duk_get_prop_string(_ctx, 0, INSTANCE_ID);
    auto* instance = static_cast<const Instance*>(duk_to_pointer(_ctx, -1));
    if (!instance || !instance->feature) {
        LOGE("Error: no context set %p %p", instance, instance ? instance->feature : nullptr);
        duk_pop(_ctx);
        return 0;
    }

    const char* key = duk_require_string(_ctx, 1);
    duk_push_boolean(_ctx, instance->feature->props.contains(key));

    return 1;
}

// Implements Proxy handler.get(target_object, key)
int jsGetProperty(duk_context *_ctx) {

    // Get the StyleContext instance from JS Feature object (first parameter).
    duk_get_prop_string(_ctx, 0, INSTANCE_ID);
    auto* instance = static_cast<const Instance*>(duk_to_pointer(_ctx, -1));
    if (!instance || !instance->feature) {
        LOGE("Error: no context set %p %p",  instance, instance ? instance->feature : nullptr);
        duk_pop(_ctx);
        return 0;
    }

    // Get the property name (second parameter)
    const char* key = duk_require_string(_ctx, 1);

    auto it = instance->feature->props.get(key);
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

} // namespace JsContext
} // namespace Tangram
