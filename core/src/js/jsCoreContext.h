#pragma once

#include <JavaScriptCore/JavaScript.h>
#include "log.h"
#include "scene/scene.h"

#include <vector>

namespace Tangram {

using JSScopeMarker = int32_t;
using JSFunctionIndex = uint32_t;

namespace JSCore {

struct Value {

    JSContextRef _ctx;
    JSValueRef _value;

    Value(JSContextRef ctx, JSValueRef value)
        : _ctx(ctx), _value(value) {
        JSValueProtect(_ctx, _value);
    }

    Value() : _ctx(nullptr), _value(nullptr) {}

    Value(Value&& _other) : _ctx(_other._ctx), _value(_other._value) {
        _other._ctx = nullptr;
    }

    ~Value() {
        if (_ctx) { JSValueUnprotect(_ctx, _value); }
    }

    Value& operator=(Value&& _other) {
        this->_ctx = _other._ctx;
        this->_value = _other._value;
        _other._ctx = nullptr;
        return *this;
    }

    operator bool() const { return bool(_ctx); }

    bool isUndefined() {
        return JSValueIsUndefined(_ctx, _value);
    }

    bool isNull() {
        return JSValueIsNull(_ctx, _value);
    }

    bool isBoolean() {
        return JSValueIsBoolean(_ctx, _value);
    }

    bool isNumber() {
        return JSValueIsNumber(_ctx, _value);
    }

    bool isString() {
        return JSValueIsString(_ctx, _value);
    }

    bool isArray() {
        return JSValueIsArray(_ctx, _value);
    }

    bool isObject() {
        return JSValueIsObject(_ctx, _value);
    }

    bool toBool() {
        return JSValueToBoolean(_ctx, _value);
    }

    int toInt() {
        return static_cast<int>(JSValueToNumber(_ctx, _value, nullptr));
    }

    double toDouble() {
        return JSValueToNumber(_ctx, _value, nullptr);
    }

    std::string toString() {
        JSStringRef jsString = JSValueToStringCopy(_ctx, _value, nullptr);
        std::string result(JSStringGetMaximumUTF8CStringSize(jsString), '\0');
        size_t bytesWrittenIncludingNull = JSStringGetUTF8CString(jsString, &result[0], result.capacity());
        result.resize(bytesWrittenIncludingNull - 1);
        JSStringRelease(jsString);
        return result;
    }

    size_t getLength() {
        JSStringRef jsLengthProperty = JSStringCreateWithUTF8CString("length");
        JSObjectRef jsObject = JSValueToObject(_ctx, _value, nullptr);
        JSValueRef jsLengthValue = JSObjectGetProperty(_ctx, jsObject, jsLengthProperty, nullptr);
        return static_cast<size_t>(JSValueToNumber(_ctx, jsLengthValue, nullptr));
    }

    Value getValueAtIndex(size_t index) {
        JSObjectRef jsObject = JSValueToObject(_ctx, _value, nullptr);
        JSValueRef jsValue = JSObjectGetPropertyAtIndex(_ctx, jsObject, static_cast<uint32_t>(index), nullptr);
        return {_ctx, jsValue};
    }

    Value getValueForProperty(const std::string& name) {
        JSObjectRef jsObject = JSValueToObject(_ctx, _value, nullptr);
        JSStringRef jsPropertyName = JSStringCreateWithUTF8CString(name.c_str());
        JSValueRef jsPropertyValue = JSObjectGetProperty(_ctx, jsObject, jsPropertyName, nullptr);
        return {_ctx, jsPropertyValue};
    }

    void setValueAtIndex(size_t index, Value value) {
        JSObjectRef jsObject = JSValueToObject(_ctx, _value, nullptr);
        JSValueRef jsValueForIndex = value.getValueRef();
        JSObjectSetPropertyAtIndex(_ctx, jsObject, static_cast<uint32_t>(index), jsValueForIndex, nullptr);
    }

    void setValueForProperty(const std::string& name, Value value) {
        JSObjectRef jsObject = JSValueToObject(_ctx, _value, nullptr);
        JSStringRef jsPropertyName = JSStringCreateWithUTF8CString(name.c_str());
        JSValueRef jsValueForProperty = value.getValueRef();
        JSObjectSetProperty(_ctx, jsObject, jsPropertyName, jsValueForProperty,
                            kJSPropertyAttributeNone, nullptr);
    }

    JSValueRef getValueRef() { return _value; }
};


struct Context {

    const Feature* _feature;
    std::vector<JSObjectRef> _functions;
    JSGlobalContextRef _context;

    Context() : _context(JSGlobalContextCreate(nullptr)) {
        // Create the global 'feature' object.
        JSClassDefinition jsFeatureClassDefinition = kJSClassDefinitionEmpty;
        jsFeatureClassDefinition.hasProperty = &jsHasPropertyCallback;
        jsFeatureClassDefinition.getProperty = &jsGetPropertyCallback;
        JSClassRef jsFeatureClass = JSClassCreate(&jsFeatureClassDefinition);
        JSObjectRef jsFeatureObject = JSObjectMake(_context, jsFeatureClass, this);
        JSClassRelease(jsFeatureClass);
        JSObjectRef jsGlobalObject = JSContextGetGlobalObject(_context);
        JSStringRef jsFeatureName = JSStringCreateWithUTF8CString("feature");
        JSObjectSetProperty(_context, jsGlobalObject, jsFeatureName, jsFeatureObject,
                            kJSPropertyAttributeNone, nullptr);
        JSStringRelease(jsFeatureName);

        // Create geometry constants.
        std::array<std::pair<const char*, int>, 3> geometryConstants{
            {{"point", GeometryType::points},
             {"line", GeometryType::lines},
             {"polygon", GeometryType::polygons}}
        };
        for (const auto& pair : geometryConstants) {
            JSStringRef jsPropertyName = JSStringCreateWithUTF8CString(pair.first);
            JSValueRef jsPropertyValue = JSValueMakeNumber(_context, pair.second);
            JSObjectSetProperty(_context, jsGlobalObject, jsPropertyName, jsPropertyValue,
                                kJSPropertyAttributeReadOnly, nullptr);
            JSStringRelease(jsPropertyName);
        }
    }

    ~Context() {
        JSGlobalContextRelease(_context);
    }

    void setGlobalValue(const std::string& name, Value value) {
        JSObjectRef jsGlobalObject = JSContextGetGlobalObject(_context);
        JSStringRef jsPropertyName = JSStringCreateWithUTF8CString(name.c_str());
        JSValueRef jsValueForProperty = value.getValueRef();
        JSObjectSetProperty(_context, jsGlobalObject, jsPropertyName, jsValueForProperty,
                            kJSPropertyAttributeNone, nullptr);
        JSStringRelease(jsPropertyName);
    }

    void setCurrentFeature(const Feature* feature) {
        _feature = feature;
    }

    bool setFunction(JSFunctionIndex index, const std::string& fn) {
        JSObjectRef jsFunctionObject = compileFunction(fn);
        if (!jsFunctionObject) {
            return false;
        }
        if (index >= _functions.size()) {
            _functions.resize(index + 1);
        }
        auto& functionEntry = _functions[index];
        if (functionEntry != nullptr) {
            JSValueUnprotect(_context, functionEntry);
        }
        JSValueProtect(_context, jsFunctionObject);
        functionEntry = jsFunctionObject;
        return true;
    }

    bool evaluateBooleanFunction(JSFunctionIndex index) {
        auto resultValue = getFunctionResult(index);
        if (resultValue) {
            return resultValue.toBool();
        }
        return false;
    }

    Value newNull() {
        JSValueRef jsValue = JSValueMakeNull(_context);
        return {_context, jsValue};
    }

    Value newBoolean(bool value) {
        JSValueRef jsValue = JSValueMakeBoolean(_context, value);
        return {_context, jsValue};
    }

    Value newNumber(double value) {
        JSValueRef jsValue = JSValueMakeNumber(_context, value);
        return {_context, jsValue};
    }

    Value newString(const std::string& value) {
        JSStringRef jsString = JSStringCreateWithUTF8CString(value.c_str());
        JSValueRef jsValue = JSValueMakeString(_context, jsString);
        return {_context, jsValue};
    }

    Value newArray() {
        JSObjectRef jsObject = JSObjectMakeArray(_context, 0, nullptr, nullptr);
        return {_context, jsObject};
    }

    Value newObject() {
        JSObjectRef jsObject = JSObjectMake(_context, nullptr, nullptr);
        return {_context, jsObject};
    }

    Value newFunction(const std::string& value) {
        JSObjectRef jsFunctionObject = compileFunction(value);
        if (jsFunctionObject == nullptr) {
            return {};
        }
        return {_context, jsFunctionObject};
    }

    Value getFunctionResult(JSFunctionIndex index) {
        if (index > _functions.size()) {
            return {};
        }
        JSObjectRef jsFunctionObject = _functions[index];
        JSValueRef jsException = nullptr;
        JSValueRef jsResultValue = JSObjectCallAsFunction(_context, jsFunctionObject, nullptr,
                                                          0, nullptr, &jsException);
        if (jsException != nullptr) {
            char buffer[128];
            JSStringRef jsExceptionString = JSValueToStringCopy(_context, jsException, nullptr);
            JSStringGetUTF8CString(jsExceptionString, buffer, sizeof(buffer));
            LOGE("Error evaluating JavaScript function - %s", buffer);
            JSStringRelease(jsExceptionString);
            return {};
        }
        return {_context, jsResultValue};
    }

    JSScopeMarker getScopeMarker() {
        // Not needed for JSCore implementation.
        return 0;
    }

    void resetToScopeMarker(JSScopeMarker) {
        // Not needed for JSCore implementation.
    }

    JSObjectRef compileFunction(const std::string& fn) {
        std::string expression("(" + fn + ")");
        JSStringRef jsFunctionSource = JSStringCreateWithUTF8CString(expression.c_str());
        JSValueRef jsException = nullptr;
        JSValueRef jsFunction = JSEvaluateScript(_context, jsFunctionSource, nullptr,
                                                 nullptr, 0, &jsException);
        JSStringRelease(jsFunctionSource);
        if (jsException) {
            char buffer[128];
            JSStringRef jsExceptionString = JSValueToStringCopy(_context, jsException, nullptr);
            JSStringGetUTF8CString(jsExceptionString, buffer, sizeof(buffer));
            LOGE("Error compiling JavaScript function - %s", buffer);
            JSStringRelease(jsExceptionString);
        }
        return JSValueToObject(_context, jsFunction, nullptr);
    }

    static bool jsHasPropertyCallback(JSContextRef, JSObjectRef object, JSStringRef property) {
        auto jsCoreContext = reinterpret_cast<Context*>(JSObjectGetPrivate(object));
        if (!jsCoreContext) {
            return false;
        }
        auto feature = jsCoreContext->_feature;
        if (!feature) {
            return false;
        }
        // This should be enough for all the names we use - could make it dynamically-sized if needed.
        char nameBuffer[128];
        JSStringGetUTF8CString(property, nameBuffer, sizeof(nameBuffer));
        return feature->props.contains(nameBuffer);
    }

    static JSValueRef jsGetPropertyCallback(JSContextRef context, JSObjectRef object,
                                            JSStringRef property, JSValueRef*) {
        auto jsCoreContext = reinterpret_cast<Context*>(JSObjectGetPrivate(object));
        if (!jsCoreContext) {
            return nullptr;
        }
        auto feature = jsCoreContext->_feature;
        if (!feature) {
            return nullptr;
        }
        JSValueRef jsValue = nullptr;
        // This should be enough for all the names we use - could make it dynamically-sized if needed.
        char nameBuffer[128];
        JSStringGetUTF8CString(property, nameBuffer, sizeof(nameBuffer));
        auto it = feature->props.get(nameBuffer);
        if (it.is<std::string>()) {
            JSStringRef jsString = JSStringCreateWithUTF8CString(it.get<std::string>().c_str());
            jsValue = JSValueMakeString(context, jsString);
            JSStringRelease(jsString);
        } else if (it.is<double>()) {
            jsValue = JSValueMakeNumber(context, it.get<double>());
        }
        return jsValue;
    }
};

} // namespace JSCore
} // namespace Tangram
