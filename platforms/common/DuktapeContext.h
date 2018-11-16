//
// Created by Matt Blair on 11/13/18.
//

#pragma once

#include "util/IJavaScriptContext.h"
#include "duktape/duktape.h"

namespace Tangram {

class DuktapeValue : public IJavaScriptValue {

public:

    DuktapeValue(duk_context* ctx, duk_idx_t index);

    ~DuktapeValue() override = default;

    bool isUndefined() override;
    bool isNull() override;
    bool isBoolean() override;
    bool isNumber() override;
    bool isString() override;
    bool isArray() override;
    bool isObject() override;

    bool toBool() override;
    int toInt() override;
    double toDouble() override;
    std::string toString() override;

    size_t getLength() override;
    JSValue getValueAtIndex(size_t index) override;
    JSValue getValueForProperty(const std::string& name) override;

    void setValueAtIndex(size_t index, JSValue value) override;
    void setValueForProperty(const std::string& name, JSValue value) override;

    auto getStackIndex() { return _index; }

    void ensureExistsOnStackTop();

private:

    duk_context* _ctx = nullptr;

    duk_idx_t _index = 0;
};

class DuktapeContext : public IJavaScriptContext {

public:

    DuktapeContext();

    ~DuktapeContext() override;

    void setGlobalValue(const std::string& name, JSValue value) override;

    void setCurrentFeature(const Feature* feature) override;

    JSFunctionIndex addFunction(const std::string& source, bool& error) override;

    bool evaluateBooleanFunction(JSFunctionIndex index) override;

protected:
    JSValue newNull() override;

    JSValue newBoolean(bool value) override;
    JSValue newNumber(double value) override;
    JSValue newString(const std::string& value) override;
    JSValue newArray() override;
    JSValue newObject() override;
    JSValue newFunction(const std::string& value) override;
    JSValue getFunctionResult(JSFunctionIndex index) override;

    JSScopeMarker getScopeMarker() override;
    void resetToScopeMarker(JSScopeMarker marker) override;

private:

    // Used for proxy object.
    static int jsGetProperty(duk_context *_ctx);
    static int jsHasProperty(duk_context *_ctx);

    static void fatalErrorHandler(void* userData, const char* message);

    bool evaluateFunction(uint32_t index);

    JSValue getStackTopValue() {
        return JSValue(new DuktapeValue(_ctx, duk_normalize_index(_ctx, -1)));
    }

    duk_context* _ctx = nullptr;

    const Feature* _feature = nullptr;

};

} // namespace Tangram
