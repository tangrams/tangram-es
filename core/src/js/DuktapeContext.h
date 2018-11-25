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

struct Context;

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
        std::string result;
        size_t len;
        // NB this requires the value to be a string. Rename to getString()?
        if (const char* data = duk_get_lstring(_ctx, _index, &len)) {
            if (len > 0) { result = std::string(data, len); }
        }
        return result;
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


static Context* getContext(duk_context* _ctx) {
    duk_memory_functions funcs;
    duk_get_memory_functions(_ctx, &funcs);
    return reinterpret_cast<Context*>(funcs.udata);
}

struct Context {

    duk_context* _ctx = nullptr;
    const Feature* _feature = nullptr;

    std::vector<std::pair<JSFunctionIndex, void*>> m_functions;

    ~Context() {
        duk_destroy_heap(_ctx);
        //printf("gets:%d reused:%d - %f%%\n", fetchCnt+_reuseCnt, _reuseCnt, float(_reuseCnt) / float(fetchCnt + _reuseCnt) * 100 );
    }

    Context() {
        // Create duktape heap with default allocation functions and
        // custom fatal error handler.
        _ctx = duk_create_heap(nullptr, nullptr, nullptr,
                               this, fatalErrorHandler);

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

    void setGlobalValue(const std::string& name, Value value) {
        value.ensureExistsOnStackTop();
        duk_put_global_lstring(_ctx, name.data(), name.length());
    }

    void setCurrentFeature(const Feature* feature) {
        _feature = feature;
        _lastFeature = nullptr;
    }

    Value getStackTopValue() {
        return {_ctx, duk_normalize_index(_ctx, -1)};
    }

    JSScopeMarker getScopeMarker() {
        return duk_get_top(_ctx);
    }

    void resetToScopeMarker(JSScopeMarker marker) {
        duk_set_top(_ctx, marker);
    }

    bool evaluateBooleanFunction(JSFunctionIndex index) {
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

    Value getFunctionResult(JSFunctionIndex index) {
        if (!evaluateFunction(index)) {
            return {nullptr, 0};
        }
        return getStackTopValue();
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
            void* fnPtr = duk_get_heapptr(_ctx, -1);
            if (m_functions.size() == index) {
                m_functions.emplace_back(index, fnPtr);
            } else {
                m_functions.resize(index+1);
                m_functions[index] = {index, fnPtr};
            }
            // Store function in global.functions to make sure it will not be
            // garbage collected.
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

    bool evaluateFunction(JSFunctionIndex index) {

        if (m_functions.size() <= index) {
            LOGN("Functions array not initialized. index:%d size:%d",
                 index, m_functions.size());
            return false;
        }
        if (m_functions[index].second == nullptr) {
            LOGN("Function not set. index:%d size:%d",
                 index, m_functions.size());
            return false;
        }

        duk_push_heapptr(_ctx, m_functions[index].second);

        if (duk_pcall(_ctx, 0) != 0) {
            LOGN("Error: %s, function:%d feaure:%p",
                 duk_safe_to_string(_ctx, -1), index, _feature);

            duk_pop(_ctx);
            return false;
        }
        return true;
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

private:

    // Cache Feature property indexes
    const Feature* _lastFeature = nullptr;
    using prop_key = const char*;
    using prop_val = const Tangram::Value*;
    std::array<std::pair<prop_key, prop_val>, 16> _propertyCache {};
    uint32_t _propertyCacheUse = 0;

    int _reuseCnt = 0;
    int fetchCnt = 0;

    static const Tangram::Value* getProperty(Context* context) {
        // Get the requested object key
        const char* key = duk_require_string(context->_ctx, 1);

        if (context->_lastFeature == context->_feature) {
            auto used = context->_propertyCacheUse;
            auto cache = context->_propertyCache;
            for (auto i = 0; i < used; i++) {
                if (cache[i].first == key) {
                    context->_reuseCnt++;
                    return cache[i].second;
                }
            }
        } else {
            context->_propertyCacheUse = 0;
        }

        context->fetchCnt++;
        context->_lastFeature = context->_feature;
        auto val = &context->_feature->props.get(key);

        auto use = context->_propertyCacheUse;
        if (use < context->_propertyCache.size()) {
            context->_propertyCache[use] = {key, val};
            context->_propertyCacheUse++;
        }
        return val;
    }

    // Implements Proxy handler.has(target_object, key)
    static int jsHasProperty(duk_context *_ctx) {

        auto context = getContext(_ctx);

        if (context && context->_feature) {
            bool hasProp = !(getProperty(context)->is<none_type>());

            duk_push_boolean(_ctx, static_cast<duk_bool_t>(hasProp));
            return 1;
        }
        LOGN("Error: no context found for %p / %p %p",  _ctx, context,
             context ? context->_feature : nullptr);
        return 0;
    }

    // Implements Proxy handler.get(target_object, key)
    static int jsGetProperty(duk_context *_ctx) {

        auto context = getContext(_ctx);

        if (context && context->_feature) {
            // Get the property name (second parameter)
            const Tangram::Value* val = getProperty(context);

            if (val->is<std::string>()) {
                const auto& str = val->get<std::string>();

                duk_push_lstring(_ctx, str.c_str(), str.length());

            } else if (val->is<double>()) {
                duk_push_number(_ctx, val->get<double>());
            } else {
                duk_push_undefined(_ctx);
            }
            // FIXME: Distinguish Booleans here as well
            return 1;
        }
        LOGN("Error: no context found for %p / %p %p",  _ctx, context,
             context ? context->_feature : nullptr);
        return 0;
    }

    static void fatalErrorHandler(void* udata, const char* message) {
        //Context* context = reinterpret_cast<Context>(udata);

        LOGE("Fatal Error in DuktapeJavaScriptContext: %s", message);
        abort();
    }
};

} // namespace Duktape
} // namespace Tangram
