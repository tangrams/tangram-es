//
// Created by Matt Blair on 11/9/18.
//
#include "DuktapeJavaScriptContext.h"

#include "log.h"
#include "data/tileData.h"
#include "util/variant.h"

#include "duktape/duktape.h"
#include "glm/vec2.hpp"

namespace Tangram {

JSContext createJavaScriptContext() {
    return JSContext(new DuktapeJavaScriptContext());
}

DuktapeJavaScriptValue::DuktapeJavaScriptValue(duk_context* ctx, duk_idx_t index) : _ctx(ctx), _index(index) {}

bool DuktapeJavaScriptValue::isUndefined() {
    return duk_is_undefined(_ctx, _index) != 0;
}

bool DuktapeJavaScriptValue::isNull() {
    return duk_is_null(_ctx, _index) != 0;
}

bool DuktapeJavaScriptValue::isBoolean() {
    return duk_is_boolean(_ctx, _index) != 0;
}

bool DuktapeJavaScriptValue::isNumber() {
    return duk_is_number(_ctx, _index) != 0;
}

bool DuktapeJavaScriptValue::isString() {
    return duk_is_string(_ctx, _index) != 0;
}

bool DuktapeJavaScriptValue::isArray() {
    return duk_is_array(_ctx, _index) != 0;
}

bool DuktapeJavaScriptValue::isObject() {
    return duk_is_object(_ctx, _index) != 0;
}

bool DuktapeJavaScriptValue::toBool() {
    return duk_to_boolean(_ctx, _index) != 0;
}

int DuktapeJavaScriptValue::toInt() {
    return duk_to_int(_ctx, _index);
}

double DuktapeJavaScriptValue::toDouble() {
    return duk_to_number(_ctx, _index);
}

std::string DuktapeJavaScriptValue::toString() {
    return std::string(duk_to_string(_ctx, _index));
}

size_t DuktapeJavaScriptValue::getLength() {
    return duk_get_length(_ctx, _index);
}

JSValue DuktapeJavaScriptValue::getValueAtIndex(size_t index) {
    duk_get_prop_index(_ctx, _index, static_cast<duk_uarridx_t>(index));
    return JSValue(new DuktapeJavaScriptValue(_ctx, duk_normalize_index(_ctx, -1)));
}

JSValue DuktapeJavaScriptValue::getValueForProperty(const std::string& name) {
    duk_get_prop_lstring(_ctx, _index, name.data(), name.length());
    return JSValue(new DuktapeJavaScriptValue(_ctx, duk_normalize_index(_ctx, -1)));
}

void DuktapeJavaScriptValue::setValueAtIndex(size_t index, JSValue value) {
    auto dukValue = reinterpret_cast<DuktapeJavaScriptValue*>(value.get());
    dukValue->ensureExistsOnStackTop();
    duk_put_prop_index(_ctx, _index, static_cast<duk_uarridx_t>(index));
}

void DuktapeJavaScriptValue::setValueForProperty(const std::string& name, JSValue value) {
    auto dukValue = reinterpret_cast<DuktapeJavaScriptValue*>(value.get());
    dukValue->ensureExistsOnStackTop();
    duk_put_prop_lstring(_ctx, _index, name.data(), name.length());

}

void DuktapeJavaScriptValue::ensureExistsOnStackTop() {
    auto dukTopIndex = duk_get_top_index(_ctx);
    if (_index != dukTopIndex) {
        duk_require_stack_top(_ctx, dukTopIndex + 1);
        duk_dup(_ctx, _index);
    }
}

const static char INSTANCE_ID[] = "\xff""\xff""obj";
const static char FUNC_ID[] = "\xff""\xff""fns";

DuktapeJavaScriptContext::DuktapeJavaScriptContext() {
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

DuktapeJavaScriptContext::~DuktapeJavaScriptContext() {
    duk_destroy_heap(_ctx);
}

void DuktapeJavaScriptContext::setGlobalValue(const std::string& name, JSValue value) {
    auto dukValue = reinterpret_cast<DuktapeJavaScriptValue*>(value.get());
    dukValue->ensureExistsOnStackTop();
    duk_put_global_lstring(_ctx, name.data(), name.length());
}

void DuktapeJavaScriptContext::setCurrentFeature(const Feature* feature) {
    _feature = feature;
}

uint32_t DuktapeJavaScriptContext::addFunction(const std::string& source, bool& error) {
    // Get all functions (array) in context
    if (!duk_get_global_string(_ctx, FUNC_ID)) {
        LOGE("AddFunction - functions array not initialized");
        duk_pop(_ctx); // pop [undefined] sitting at stack top
        error = true;
        return 0;
    }

    auto index = static_cast<uint32_t>(duk_get_length(_ctx, -1));

    duk_push_string(_ctx, source.c_str());
    duk_push_string(_ctx, "");

    if (duk_pcompile(_ctx, DUK_COMPILE_FUNCTION) == 0) {
        duk_put_prop_index(_ctx, -2, index);
    } else {
        LOGW("Compile failed: %s\n%s\n---",
             duk_safe_to_string(_ctx, -1),
             source.c_str());
        duk_pop(_ctx);
        error = true;
    }

    // Pop the functions array off the stack
    duk_pop(_ctx);

    return index;
}

bool DuktapeJavaScriptContext::evaluateBooleanFunction(uint32_t index) {
    if (!evaluateFunction(index)) {
        return false;
    }

    // Evaluate the "truthiness" of the function result at the top of the stack.
    bool result = duk_to_boolean(_ctx, -1) != 0;

    // pop result
    duk_pop(_ctx);

    return result;
}

JSValue DuktapeJavaScriptContext::getFunctionResult(uint32_t index) {
    if (!evaluateFunction(index)) {
        return nullptr;
    }
    return getStackTopValue();
}

JSValue DuktapeJavaScriptContext::newNull() {
    duk_push_null(_ctx);
    return getStackTopValue();
}

JSValue DuktapeJavaScriptContext::newBoolean(bool value) {
    duk_push_boolean(_ctx, static_cast<duk_bool_t>(value));
    return getStackTopValue();
}

JSValue DuktapeJavaScriptContext::newNumber(double value) {
    duk_push_number(_ctx, value);
    return getStackTopValue();
}

JSValue DuktapeJavaScriptContext::newString(const std::string& value) {
    duk_push_lstring(_ctx, value.data(), value.length());
    return getStackTopValue();
}

JSValue DuktapeJavaScriptContext::newArray() {
    duk_push_array(_ctx);
    return getStackTopValue();
}

JSValue DuktapeJavaScriptContext::newObject() {
    duk_push_object(_ctx);
    return getStackTopValue();
}

JSValue DuktapeJavaScriptContext::newFunction(const std::string& value) {
    if (duk_pcompile_lstring(_ctx, DUK_COMPILE_FUNCTION, value.data(), value.length()) != 0) {
        auto error = duk_safe_to_string(_ctx, -1);
        LOGW("Compile failed in global function: %s\n%s\n---", error, value.c_str());
        duk_pop(_ctx); // Pop error.
        return nullptr;
    }
    return getStackTopValue();
}

JSScopeMarker DuktapeJavaScriptContext::getScopeMarker() {
    return duk_get_top(_ctx);
}

void DuktapeJavaScriptContext::resetToScopeMarker(JSScopeMarker marker) {
    duk_set_top(_ctx, marker);
}

// Implements Proxy handler.has(target_object, key)
int DuktapeJavaScriptContext::jsHasProperty(duk_context *_ctx) {

    duk_get_prop_string(_ctx, 0, INSTANCE_ID);
    auto context = static_cast<const DuktapeJavaScriptContext*>(duk_to_pointer(_ctx, -1));
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
int DuktapeJavaScriptContext::jsGetProperty(duk_context *_ctx) {

    // Get the JavaScriptContext instance from JS Feature object (first parameter).
    duk_get_prop_string(_ctx, 0, INSTANCE_ID);
    auto context = static_cast<const DuktapeJavaScriptContext*>(duk_to_pointer(_ctx, -1));
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

void DuktapeJavaScriptContext::fatalErrorHandler(void*, const char* message) {
    LOGE("Fatal Error in DuktapeJavaScriptContext: %s", message);
    abort();
}

bool DuktapeJavaScriptContext::evaluateFunction(uint32_t index) {
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
