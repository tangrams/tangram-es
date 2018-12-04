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

    DuktapeValue(duk_context* ctx, duk_idx_t index);

    ~DuktapeValue() = default;

    operator bool() const { return _ctx != nullptr; }

    bool isUndefined();
    bool isNull();
    bool isBoolean();
    bool isNumber();
    bool isString();
    bool isArray();
    bool isObject();

    bool toBool();
    int toInt();
    double toDouble();
    std::string toString();

    size_t getLength();
    DuktapeValue getValueAtIndex(size_t index);
    DuktapeValue getValueForProperty(const std::string& name);

    void setValueAtIndex(size_t index, DuktapeValue value);
    void setValueForProperty(const std::string& name, DuktapeValue value);

    auto getStackIndex() { return _index; }

    void ensureExistsOnStackTop();

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

    friend JavaScriptScope;
};

} // namespace Tangram
