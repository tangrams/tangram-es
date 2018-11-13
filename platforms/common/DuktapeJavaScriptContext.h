//
// Created by Matt Blair on 11/13/18.
//

#pragma once

#include "IJavaScriptContext.h"
#include "duktape/duktape.h"

namespace Tangram {

class DuktapeJavaScriptValue : public IJavaScriptValue {

public:

    ~DuktapeJavaScriptValue() override;

    bool isUndefined() override;
    bool isNull() override;
    bool isBool() override;
    bool isNumber() override;
    bool isString() override;
    bool isArray() override;
    bool isObject() override;

    bool toBool() override;
    int toInt() override;
    double toDouble() override;
    std::string toString() override;

    JSValue getValueAtIndex(size_t index) override;
    JSValue getValueForProperty(const std::string& name) override;

};

class DuktapeJavaScriptContext : public IJavaScriptContext {

public:

    DuktapeJavaScriptContext();

    ~DuktapeJavaScriptContext() override;

    void setGlobalString(const std::string& name, const std::string& value) override;

    void setGlobalNumber(const std::string& name, double value) override;

    void setCurrentFeature(Feature* feature) override;

    uint32_t addFunction(const std::string& source, bool& error) override;

    bool evaluateBooleanFunction(uint32_t index) override;

    JSValue getFunctionResult(uint32_t index) override;

private:

    // Used for proxy object.
    static int jsGetProperty(duk_context *_ctx);
    static int jsHasProperty(duk_context *_ctx);

    duk_context* _ctx = nullptr;

    Feature* _feature = nullptr;

};

} // namespace Tangram
