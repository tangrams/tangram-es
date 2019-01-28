//
// Created by Matt Blair on 11/9/18.
//
#include "DuktapeContext.h"

#include "log.h"
#include "data/tileData.h"
#include "util/variant.h"

#include "duktape/duktape.h"
#include "glm/vec2.hpp"

namespace Tangram {

const static char INSTANCE_ID[] = "\xff""\xff""obj";
const static char FUNC_ID[] = "\xff""\xff""fns";

DuktapeContext::DuktapeContext() {
    // Create duktape heap with default allocation functions and custom fatal error handler.
    _ctx = duk_create_heap(nullptr, nullptr, nullptr, nullptr, fatalErrorHandler);

    //// Create global geometry constants
    // TODO make immutable
    duk_push_number(_ctx, GeometryType::points);
    duk_put_global_string(_ctx, "point");

    duk_push_number(_ctx, GeometryType::lines);
    duk_put_global_string(_ctx, "line");

    duk_push_number(_ctx, GeometryType::polygons);
    duk_put_global_string(_ctx, "polygon");

    //// Create global 'feature' object
    // Get Proxy constructor
    // -> [cons]
    duk_eval_string(_ctx, "Proxy");

    // Add feature object
    // -> [cons, { __obj: this }]
    duk_idx_t featureObj = duk_push_object(_ctx);
    duk_push_pointer(_ctx, this);
    duk_put_prop_string(_ctx, featureObj, INSTANCE_ID);

    // Add handler object
    // -> [cons, {...}, { get: func, has: func }]
    duk_idx_t handlerObj = duk_push_object(_ctx);
    // Add 'get' property to handler
    duk_push_c_function(_ctx, jsGetProperty, 3 /*nargs*/);
    duk_put_prop_string(_ctx, handlerObj, "get");
    // Add 'has' property to handler
    duk_push_c_function(_ctx, jsHasProperty, 2 /*nargs*/);
    duk_put_prop_string(_ctx, handlerObj, "has");

    // Call proxy constructor
    // [cons, feature, handler ] -> [obj|error]
    if (duk_pnew(_ctx, 2) == 0) {
        // put feature proxy object in global scope
        if (!duk_put_global_string(_ctx, "feature")) {
            LOGE("Initialization failed");
        }
    } else {
        LOGE("Failure: %s", duk_safe_to_string(_ctx, -1));
        duk_pop(_ctx);
    }

    // Set up 'fns' array.
    duk_push_array(_ctx);
    if (!duk_put_global_string(_ctx, FUNC_ID)) {
        LOGE("'fns' object not set");
    }
}

DuktapeContext::~DuktapeContext() {
    duk_destroy_heap(_ctx);
}

void DuktapeContext::setGlobalValue(const std::string& name, DuktapeValue value) {
    value.ensureExistsOnStackTop();
    duk_put_global_lstring(_ctx, name.data(), name.length());
}

void DuktapeContext::setCurrentFeature(const Feature* feature) {
    _feature = feature;
}

bool DuktapeContext::setFunction(JSFunctionIndex index, const std::string& source) {
    // Get all functions (array) in context
    if (!duk_get_global_string(_ctx, FUNC_ID)) {
        LOGE("AddFunction - functions array not initialized");
        duk_pop(_ctx); // pop [undefined] sitting at stack top
        return false;
    }

    duk_push_string(_ctx, source.c_str());
    duk_push_string(_ctx, "");

    if (duk_pcompile(_ctx, DUK_COMPILE_FUNCTION) == 0) {
        duk_put_prop_index(_ctx, -2, index);
    } else {
        LOGW("Compile failed: %s\n%s\n---",
             duk_safe_to_string(_ctx, -1),
             source.c_str());
        duk_pop(_ctx);
        return false;
    }

    // Pop the functions array off the stack
    duk_pop(_ctx);

    return true;
}

bool DuktapeContext::evaluateBooleanFunction(uint32_t index) {
    if (!evaluateFunction(index)) {
        return false;
    }

    // Evaluate the "truthiness" of the function result at the top of the stack.
    bool result = duk_to_boolean(_ctx, -1) != 0;

    // pop result
    duk_pop(_ctx);

    return result;
}

DuktapeValue DuktapeContext::getFunctionResult(uint32_t index) {
    if (!evaluateFunction(index)) {
        return DuktapeValue();
    }
    return getStackTopValue();
}

DuktapeValue DuktapeContext::newNull() {
    duk_push_null(_ctx);
    return getStackTopValue();
}

DuktapeValue DuktapeContext::newBoolean(bool value) {
    duk_push_boolean(_ctx, static_cast<duk_bool_t>(value));
    return getStackTopValue();
}

DuktapeValue DuktapeContext::newNumber(double value) {
    duk_push_number(_ctx, value);
    return getStackTopValue();
}

DuktapeValue DuktapeContext::newString(const std::string& value) {
    duk_push_lstring(_ctx, value.data(), value.length());
    return getStackTopValue();
}

DuktapeValue DuktapeContext::newArray() {
    duk_push_array(_ctx);
    return getStackTopValue();
}

DuktapeValue DuktapeContext::newObject() {
    duk_push_object(_ctx);
    return getStackTopValue();
}

DuktapeValue DuktapeContext::newFunction(const std::string& value) {
    if (duk_pcompile_lstring(_ctx, DUK_COMPILE_FUNCTION, value.data(), value.length()) != 0) {
        auto error = duk_safe_to_string(_ctx, -1);
        LOGW("Compile failed in global function: %s\n%s\n---", error, value.c_str());
        duk_pop(_ctx); // Pop error.
        return DuktapeValue();
    }
    return getStackTopValue();
}

JSScopeMarker DuktapeContext::getScopeMarker() {
    return duk_get_top(_ctx);
}

void DuktapeContext::resetToScopeMarker(JSScopeMarker marker) {
    duk_set_top(_ctx, marker);
}

// Implements Proxy handler.has(target_object, key)
int DuktapeContext::jsHasProperty(duk_context *_ctx) {

    duk_get_prop_string(_ctx, 0, INSTANCE_ID);
    auto context = static_cast<const DuktapeContext*>(duk_to_pointer(_ctx, -1));
    if (!context || !context->_feature) {
        LOGE("Error: no context set %p %p", context, context ? context->_feature : nullptr);
        duk_pop(_ctx);
        return 0;
    }

    const char* key = duk_require_string(_ctx, 1);
    auto result = static_cast<duk_bool_t>(context->_feature->props.contains(key));
    duk_push_boolean(_ctx, result);

    return 1;
}

// Implements Proxy handler.get(target_object, key)
int DuktapeContext::jsGetProperty(duk_context *_ctx) {

    // Get the JavaScriptContext instance from JS Feature object (first parameter).
    duk_get_prop_string(_ctx, 0, INSTANCE_ID);
    auto context = static_cast<const DuktapeContext*>(duk_to_pointer(_ctx, -1));
    if (!context || !context->_feature) {
        LOGE("Error: no context set %p %p",  context, context ? context->_feature : nullptr);
        duk_pop(_ctx);
        return 0;
    }

    // Get the property name (second parameter)
    const char* key = duk_require_string(_ctx, 1);

    auto it = context->_feature->props.get(key);
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

void DuktapeContext::fatalErrorHandler(void*, const char* message) {
    LOGE("Fatal Error in DuktapeJavaScriptContext: %s", message);
    abort();
}

bool DuktapeContext::evaluateFunction(uint32_t index) {
    // Get all functions (array) in context
    if (!duk_get_global_string(_ctx, FUNC_ID)) {
        LOGE("EvalFilterFn - functions array not initialized");
        duk_pop(_ctx); // pop [undefined] sitting at stack top
        return false;
    }

    // Get function at index `id` from functions array, put it at stack top
    if (!duk_get_prop_index(_ctx, -1, index)) {
        LOGE("EvalFilterFn - function %d not set", index);
        duk_pop(_ctx); // pop "undefined" sitting at stack top
        duk_pop(_ctx); // pop functions (array) now sitting at stack top
        return false;
    }

    // pop fns array
    duk_remove(_ctx, -2);

    // call popped function (sitting at stack top), evaluated value is put on stack top
    if (duk_pcall(_ctx, 0) != 0) {
        LOGE("EvalFilterFn: %s", duk_safe_to_string(_ctx, -1));
        duk_pop(_ctx);
        return false;
    }

    return true;
}

} // namespace Tangram
