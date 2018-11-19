//
// Created by Matt Blair on 11/15/18.
//
#pragma once
#include "IJavaScriptContext.h"
#include <JavaScriptCore/JavaScriptCore.h>
#include <vector>

namespace Tangram {

class JSCoreValue : public IJavaScriptValue {

public:

    JSCoreValue(JSContextRef ctx, JSValueRef value);

    ~JSCoreValue() override;

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

    JSValueRef getValueRef() { return _value; }

private:

    JSContextRef _ctx;
    JSValueRef _value;
};

class JSCoreContext : public IJavaScriptContext {

public:

    JSCoreContext();

    ~JSCoreContext() override;

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

    static bool jsHasPropertyCallback(JSContextRef context, JSObjectRef object, JSStringRef property);
    static JSValueRef jsGetPropertyCallback(JSContextRef context, JSObjectRef object, JSStringRef property, JSValueRef* exception);

    JSObjectRef compileFunction(const std::string& source);

    std::vector<JSObjectRef> _functions;

    JSGlobalContextRef _context;


    const Feature* _feature;
};

} // namespace Tangram
