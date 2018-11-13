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

using JSValue = std::unique_ptr<IJavaScriptValue>;
using JSContext = std::unique_ptr<IJavaScriptContext>;

class IJavaScriptValue {

public:

    virtual ~IJavaScriptValue() = 0;

    virtual bool isUndefined() = 0;
    virtual bool isNull() = 0;
    virtual bool isBool() = 0;
    virtual bool isNumber() = 0;
    virtual bool isString() = 0;
    virtual bool isArray() = 0;
    virtual bool isObject() = 0;

    virtual bool toBool() = 0;
    virtual int toInt() = 0;
    virtual double toDouble() = 0;
    virtual std::string toString() = 0;

    virtual JSValue getValueAtIndex(size_t index) = 0;
    virtual JSValue getValueForProperty(const std::string& name) = 0;

};

class IJavaScriptContext {

public:

    virtual ~IJavaScriptContext() = 0;

    virtual void setGlobalString(const std::string& name, const std::string& value) = 0;

    virtual void setGlobalNumber(const std::string& name, double value) = 0;

    virtual void setCurrentFeature(Feature* feature) = 0;

    // Compiles the given JavaScript string into a function, adds it to the function list, and returns its index in the
    // list. If the string could not be compiled, sets error to true.
    virtual uint32_t addFunction(const std::string& source, bool& error) = 0;

    virtual bool evaluateBooleanFunction(uint32_t index) = 0;

    virtual JSValue getFunctionResult(uint32_t index) = 0;

};

} // namespace Tangram
