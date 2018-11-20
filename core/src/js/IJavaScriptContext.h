//
// Created by Matt Blair on 11/9/18.
//
#pragma once
#include <memory>
#include <string>

namespace Tangram {

struct Feature;

class IJavaScriptValue;
class IJavaScriptContext;
class JavaScriptScope;

using JSValue = std::unique_ptr<IJavaScriptValue>;
using JSContext = std::unique_ptr<IJavaScriptContext>;
using JSScopeMarker = int32_t;
using JSFunctionIndex = uint32_t;

JSContext createJavaScriptContext();

class IJavaScriptValue {

public:

    virtual ~IJavaScriptValue() = default;

    virtual bool isUndefined() = 0;
    virtual bool isNull() = 0;
    virtual bool isBoolean() = 0;
    virtual bool isNumber() = 0;
    virtual bool isString() = 0;
    virtual bool isArray() = 0;
    virtual bool isObject() = 0;

    virtual bool toBool() = 0;
    virtual int toInt() = 0;
    virtual double toDouble() = 0;
    virtual std::string toString() = 0;

    virtual size_t getLength() = 0;
    virtual JSValue getValueAtIndex(size_t index) = 0;
    virtual JSValue getValueForProperty(const std::string& name) = 0;

    virtual void setValueAtIndex(size_t index, JSValue value) = 0;
    virtual void setValueForProperty(const std::string& name, JSValue value) = 0;
};

class IJavaScriptContext {

public:

    virtual ~IJavaScriptContext() = default;

    virtual void setGlobalValue(const std::string& name, JSValue value) = 0;

    virtual void setCurrentFeature(const Feature* feature) = 0;

    // Compiles the given JavaScript string into a function and adds it to the function list at the given index.
    // Returns false if the source cannot be compiled, otherwise returns true.
    virtual bool setFunction(JSFunctionIndex index, const std::string& source) = 0;

    virtual bool evaluateBooleanFunction(JSFunctionIndex index) = 0;

protected:

    virtual JSValue newNull() = 0;
    virtual JSValue newBoolean(bool value) = 0;
    virtual JSValue newNumber(double value) = 0;
    virtual JSValue newString(const std::string& value) = 0;
    virtual JSValue newArray() = 0;
    virtual JSValue newObject() = 0;
    virtual JSValue newFunction(const std::string& value) = 0;
    virtual JSValue getFunctionResult(JSFunctionIndex index) = 0;

    virtual JSScopeMarker getScopeMarker() = 0;
    virtual void resetToScopeMarker(JSScopeMarker marker) = 0;

    friend JavaScriptScope;
};

class JavaScriptScope {

public:

    explicit JavaScriptScope(IJavaScriptContext& context) : _context(context) {
        _scopeMarker = _context.getScopeMarker();
    }

    ~JavaScriptScope() {
        _context.resetToScopeMarker(_scopeMarker);
    }

    JavaScriptScope& operator=(const JavaScriptScope& other) = delete;
    JavaScriptScope& operator=(JavaScriptScope&& other) = delete;

    JSValue newNull() { return _context.newNull(); }
    JSValue newBoolean(bool value) { return _context.newBoolean(value); }
    JSValue newNumber(double value) { return _context.newNumber(value); }
    JSValue newString(const std::string& value) { return _context.newString(value); }
    JSValue newArray() { return _context.newArray(); }
    JSValue newObject() { return _context.newObject(); }
    JSValue newFunction(const std::string& value) { return _context.newFunction(value); }
    JSValue getFunctionResult(JSFunctionIndex index) { return _context.getFunctionResult(index); }

private:

    IJavaScriptContext& _context;
    JSScopeMarker _scopeMarker = 0;
};

} // namespace Tangram
