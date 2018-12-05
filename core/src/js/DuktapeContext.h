//
// Created by Matt Blair on 11/13/18.
//

#pragma once

#include "js/JavaScriptFwd.h"
#include "duktape/duktape.h"

#include <string>

namespace Tangram {

class DuktapeValue {

public:

    DuktapeValue() = default;

    DuktapeValue(duk_context* ctx, duk_idx_t index) : _ctx(ctx), _index(index) {}

    DuktapeValue(DuktapeValue&& other) noexcept : _ctx(other._ctx), _index(other._index) {
        other._ctx = nullptr;
    }

    ~DuktapeValue() = default;

    DuktapeValue& operator=(const DuktapeValue& other) = delete;

    DuktapeValue& operator=(DuktapeValue&& other) noexcept {
        _ctx = other._ctx;
        _index = other._index;
        other._ctx = nullptr;
        return *this;
    }

    operator bool() const {
        return _ctx != nullptr;
    }

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

    DuktapeValue getValueAtIndex(size_t index) {
        duk_get_prop_index(_ctx, _index, static_cast<duk_uarridx_t>(index));
        return DuktapeValue(_ctx, duk_normalize_index(_ctx, -1));
    }

    DuktapeValue getValueForProperty(const std::string& name) {
        duk_get_prop_lstring(_ctx, _index, name.data(), name.length());
        return DuktapeValue(_ctx, duk_normalize_index(_ctx, -1));
    }

    void setValueAtIndex(size_t index, DuktapeValue value) {
        value.ensureExistsOnStackTop();
        duk_put_prop_index(_ctx, _index, static_cast<duk_uarridx_t>(index));
    }

    void setValueForProperty(const std::string& name, DuktapeValue value) {
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

    auto getStackIndex() {
        return _index;
    }

private:

    duk_context* _ctx = nullptr;

    duk_idx_t _index = 0;
};

class DuktapeContext {

public:

    DuktapeContext();

    ~DuktapeContext();

    void setGlobalValue(const std::string& name, DuktapeValue value);

    void setCurrentFeature(const Feature* feature);

    bool setFunction(JSFunctionIndex index, const std::string& source);

    bool evaluateBooleanFunction(JSFunctionIndex index);

protected:
    DuktapeValue newNull();

    DuktapeValue newBoolean(bool value);
    DuktapeValue newNumber(double value);
    DuktapeValue newString(const std::string& value);
    DuktapeValue newArray();
    DuktapeValue newObject();
    DuktapeValue newFunction(const std::string& value);
    DuktapeValue getFunctionResult(JSFunctionIndex index);

    JSScopeMarker getScopeMarker();
    void resetToScopeMarker(JSScopeMarker marker);

private:

    // Used for proxy object.
    static int jsGetProperty(duk_context *_ctx);
    static int jsHasProperty(duk_context *_ctx);

    static void fatalErrorHandler(void* userData, const char* message);

    bool evaluateFunction(uint32_t index);

    DuktapeValue getStackTopValue() {
        return DuktapeValue(_ctx, duk_normalize_index(_ctx, -1));
    }

    duk_context* _ctx = nullptr;

    const Feature* _feature = nullptr;

    friend JavaScriptScope<DuktapeContext>;
};

} // namespace Tangram
