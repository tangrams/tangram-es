#pragma once

#include "duktape/duktape.h"
#include "scene/scene.h"
#include "log.h"

#define DUMP(...) // do { logMsg(__VA_ARGS__); duk_dump_context_stderr(m_ctx); } while(0)

namespace Tangram {

using JSScopeMarker = int32_t;
using JSFunctionIndex = uint32_t;

namespace Duktape {

const static char INSTANCE_ID[] = "\xff""\xff""obj";
const static char FUNC_ID[] = "\xff""\xff""fns";

struct Value {

    duk_context* _ctx = nullptr;
    duk_idx_t _index = 0;

    Value(duk_context* ctx, duk_idx_t index) : _ctx(ctx), _index(index) {}

    Value(Value&& _other) : _ctx(_other._ctx), _index(_other._index) {
        _other._ctx = nullptr;
    }

    Value& operator=(Value&& _other) {
        this->_ctx = _other._ctx;
        this->_index = _other._index;
        _other._ctx = nullptr;
        return *this;
    }

    operator bool() const { return bool(_ctx ); }

    bool isUndefined() {
        return duk_is_undefined(_ctx, _index) != 0;
    }

    bool isNull() {
        return duk_is_null(_ctx, _index) != 0;
    }

    bool isBoolean() {
        return duk_is_boolean(_ctx, _index) != 0;
    }

    bool isNumber() {
        return duk_is_number(_ctx, _index) != 0;
    }

    bool isString() {
        return duk_is_string(_ctx, _index) != 0;
    }

    bool isArray() {
        return duk_is_array(_ctx, _index) != 0;
    }

    bool isObject() {
        return duk_is_object(_ctx, _index) != 0;
    }

    bool toBool() {
        return duk_to_boolean(_ctx, _index) != 0;
    }

    int toInt() {
        return duk_to_int(_ctx, _index);
    }

    double toDouble() {
        return duk_to_number(_ctx, _index);
    }

    std::string toString() {
        return std::string(duk_to_string(_ctx, _index));
    }

    size_t getLength() {
        return duk_get_length(_ctx, _index);
    }

    Value getValueAtIndex(size_t index) {
        duk_get_prop_index(_ctx, _index, static_cast<duk_uarridx_t>(index));
        return {_ctx, duk_normalize_index(_ctx, -1)};
    }

    Value getValueForProperty(const std::string& name) {
        duk_get_prop_lstring(_ctx, _index, name.data(), name.length());
        return {_ctx, duk_normalize_index(_ctx, -1)};
    }

    void setValueAtIndex(size_t index, Value value) {
        value.ensureExistsOnStackTop();
        duk_put_prop_index(_ctx, _index, static_cast<duk_uarridx_t>(index));
    }

    void setValueForProperty(const std::string& name, Value value) {
        value.ensureExistsOnStackTop();
        duk_put_prop_lstring(_ctx, _index, name.data(), name.length());

    }

    void ensureExistsOnStackTop() {
        auto dukTopIndex = duk_get_top_index(_ctx);
        if (_index != dukTopIndex) {
            duk_require_stack_top(_ctx, dukTopIndex + 1);
            duk_dup(_ctx, _index);
        }
    }
};


struct Context {

    duk_context* _ctx = nullptr;
    const Feature* _feature = nullptr;

    Value getStackTopValue() {
        return {_ctx, duk_normalize_index(_ctx, -1)};
    }

    Context() {
        // Create duktape heap with default allocation functions and
        // custom fatal error handler.
        _ctx = duk_create_heap(nullptr, nullptr, nullptr, nullptr,
                               fatalErrorHandler);

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

    ~Context() {
        duk_destroy_heap(_ctx);
    }

    void setGlobalValue(const std::string& name, Value value) {
        value.ensureExistsOnStackTop();
        duk_put_global_lstring(_ctx, name.data(), name.length());
    }

    void setCurrentFeature(const Feature* feature) {
        _feature = feature;
    }

    bool setFunction(JSFunctionIndex index, const std::string& fn) {
        // Get all functions (array) in context
        if (!duk_get_global_string(_ctx, FUNC_ID)) {
            LOGE("AddFunction - functions array not initialized");
            duk_pop(_ctx); // pop [undefined] sitting at stack top
            return false;
        }

        duk_push_string(_ctx, fn.c_str());
        duk_push_string(_ctx, "");

        if (duk_pcompile(_ctx, DUK_COMPILE_FUNCTION) == 0) {
            duk_put_prop_index(_ctx, -2, index);
        } else {
            LOGW("Compile failed: %s\n%s\n---",
                 duk_safe_to_string(_ctx, -1), fn.c_str());
            duk_pop(_ctx);
            return false;
        }
        // Pop the functions array off the stack
        duk_pop(_ctx);

        return true;
    }

    bool evaluateBooleanFunction(uint32_t index) {
        if (!evaluateFunction(index)) {
            return false;
        }
        // Evaluate the "truthiness" of the function result at the top
        //  of the stack.
        bool result = duk_to_boolean(_ctx, -1) != 0;

        // pop result
        duk_pop(_ctx);
        return result;
    }

    Value getFunctionResult(uint32_t index) {
        if (!evaluateFunction(index)) {
            return {nullptr, 0};
        }
        return getStackTopValue();
    }

    Value newNull() {
        duk_push_null(_ctx);
        return getStackTopValue();
    }

    Value newBoolean(bool value) {
        duk_push_boolean(_ctx, static_cast<duk_bool_t>(value));
        return getStackTopValue();
    }

    Value newNumber(double value) {
        duk_push_number(_ctx, value);
        return getStackTopValue();
    }

    Value newString(const std::string& value) {
        duk_push_lstring(_ctx, value.data(), value.length());
        return getStackTopValue();
    }

    Value newArray() {
        duk_push_array(_ctx);
        return getStackTopValue();
    }

    Value newObject() {
        duk_push_object(_ctx);
        return getStackTopValue();
    }

    Value newFunction(const std::string& value) {
        if (duk_pcompile_lstring(_ctx, DUK_COMPILE_FUNCTION,
                                 value.data(), value.length()) != 0) {
            auto error = duk_safe_to_string(_ctx, -1);
            LOGW("Compile failed in global function: %s\n%s\n---",
                 error, value.c_str());
            duk_pop(_ctx); // Pop error.
            return {nullptr, 0};
        }
        return getStackTopValue();
    }

    JSScopeMarker getScopeMarker() {
        return duk_get_top(_ctx);
    }

    void resetToScopeMarker(JSScopeMarker marker) {
        duk_set_top(_ctx, marker);
    }

   // Implements Proxy handler.has(target_object, key)
   static int jsHasProperty(duk_context *_ctx) {

        duk_get_prop_string(_ctx, 0, INSTANCE_ID);
        auto context = static_cast<const Context*>(duk_to_pointer(_ctx, -1));
        if (!context || !context->_feature) {
            LOGE("Error: no context set %p %p", context,
                 context ? context->_feature : nullptr);
            duk_pop(_ctx);
            return 0;
        }

        const char* key = duk_require_string(_ctx, 1);
        auto result = static_cast<duk_bool_t>(context->_feature->props.contains(key));
        duk_push_boolean(_ctx, result);

        return 1;
    }

   // Implements Proxy handler.get(target_object, key)
   static int jsGetProperty(duk_context *_ctx) {

        // Get the JavaScriptContext instance from JS Feature object
        //  (first parameter).
        duk_get_prop_string(_ctx, 0, INSTANCE_ID);
        auto context = static_cast<const Context*>(duk_to_pointer(_ctx, -1));
        if (!context || !context->_feature) {
            LOGE("Error: no context set %p %p",  context,
                 context ? context->_feature : nullptr);
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

    static void fatalErrorHandler(void*, const char* message) {
        LOGE("Fatal Error in DuktapeJavaScriptContext: %s", message);
        abort();
    }

    bool evaluateFunction(uint32_t index) {
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

        // call popped function (sitting at stack top), evaluated value is put
        // on stack top
        if (duk_pcall(_ctx, 0) != 0) {
            LOGE("EvalFilterFn: %s", duk_safe_to_string(_ctx, -1));
            duk_pop(_ctx);
            return false;
        }

        return true;
    }
};

} // namespace Duktape
} // namespace Tangram
