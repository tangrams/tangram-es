//
// Created by Matt Blair on 11/15/18.
//

#include "JSCoreContext.h"
#include "log.h"
#include "data/tileData.h"
#include "util/variant.h"
#include <array>

namespace Tangram {

JSContext createJavaScriptContext() {
    return JSContext(new JSCoreContext());
}

JSCoreValue::JSCoreValue(JSContextRef ctx, JSValueRef value) : _ctx(ctx), _value(value) {
    JSValueProtect(_ctx, _value);
}

JSCoreValue::~JSCoreValue() {
    JSValueUnprotect(_ctx, _value);
}

bool JSCoreValue::isUndefined() {
    return JSValueIsUndefined(_ctx, _value);
}

bool JSCoreValue::isNull() {
    return JSValueIsNull(_ctx, _value);
}

bool JSCoreValue::isBoolean() {
    return JSValueIsBoolean(_ctx, _value);
}

bool JSCoreValue::isNumber() {
    return JSValueIsNumber(_ctx, _value);
}

bool JSCoreValue::isString() {
    return JSValueIsString(_ctx, _value);
}

bool JSCoreValue::isArray() {
    return JSValueIsArray(_ctx, _value);
}

bool JSCoreValue::isObject() {
    return JSValueIsObject(_ctx, _value);
}

bool JSCoreValue::toBool() {
    return JSValueToBoolean(_ctx, _value);
}

int JSCoreValue::toInt() {
    return static_cast<int>(JSValueToNumber(_ctx, _value, nullptr));
}

double JSCoreValue::toDouble() {
    return JSValueToNumber(_ctx, _value, nullptr);
}

std::string JSCoreValue::toString() {
    JSStringRef jsString = JSValueToStringCopy(_ctx, _value, nullptr);
    std::string result(JSStringGetMaximumUTF8CStringSize(jsString), '\0');
    size_t bytesWrittenIncludingNull = JSStringGetUTF8CString(jsString, &result[0], result.capacity());
    result.resize(bytesWrittenIncludingNull - 1);
    JSStringRelease(jsString);
    return result;
}

size_t JSCoreValue::getLength() {
    JSStringRef jsLengthProperty = JSStringCreateWithUTF8CString("length");
    JSObjectRef jsObject = JSValueToObject(_ctx, _value, nullptr);
    JSValueRef jsLengthValue = JSObjectGetProperty(_ctx, jsObject, jsLengthProperty, nullptr);
    return static_cast<size_t>(JSValueToNumber(_ctx, jsLengthValue, nullptr));
}

JSValue JSCoreValue::getValueAtIndex(size_t index) {
    JSObjectRef jsObject = JSValueToObject(_ctx, _value, nullptr);
    JSValueRef jsValue = JSObjectGetPropertyAtIndex(_ctx, jsObject, static_cast<uint32_t>(index), nullptr);
    return JSValue(new JSCoreValue(_ctx, jsValue));
}

JSValue JSCoreValue::getValueForProperty(const std::string& name) {
    JSObjectRef jsObject = JSValueToObject(_ctx, _value, nullptr);
    JSStringRef jsPropertyName = JSStringCreateWithUTF8CString(name.c_str());
    JSValueRef jsPropertyValue = JSObjectGetProperty(_ctx, jsObject, jsPropertyName, nullptr);
    return JSValue(new JSCoreValue(_ctx, jsPropertyValue));
}

void JSCoreValue::setValueAtIndex(size_t index, JSValue value) {
    JSObjectRef jsObject = JSValueToObject(_ctx, _value, nullptr);
    JSValueRef jsValueForIndex = reinterpret_cast<JSCoreValue*>(value.get())->getValueRef();
    JSObjectSetPropertyAtIndex(_ctx, jsObject, static_cast<uint32_t>(index), jsValueForIndex, nullptr);
}

void JSCoreValue::setValueForProperty(const std::string& name, JSValue value) {
    JSObjectRef jsObject = JSValueToObject(_ctx, _value, nullptr);
    JSStringRef jsPropertyName = JSStringCreateWithUTF8CString(name.c_str());
    JSValueRef jsValueForProperty = reinterpret_cast<JSCoreValue*>(value.get())->getValueRef();
    JSObjectSetProperty(_ctx, jsObject, jsPropertyName, jsValueForProperty, kJSPropertyAttributeNone, nullptr);
}

JSCoreContext::JSCoreContext() {
    _context = JSGlobalContextCreate(nullptr);

    // Create the global 'feature' object.
    JSClassDefinition jsFeatureClassDefinition = kJSClassDefinitionEmpty;
    jsFeatureClassDefinition.hasProperty = jsHasPropertyCallback;
    jsFeatureClassDefinition.getProperty = jsGetPropertyCallback;
    JSClassRef jsFeatureClass = JSClassCreate(&jsFeatureClassDefinition);
    JSObjectRef jsFeatureObject = JSObjectMake(_context, jsFeatureClass, this);
    JSClassRelease(jsFeatureClass);
    JSObjectRef jsGlobalObject = JSContextGetGlobalObject(_context);
    JSStringRef jsFeatureName = JSStringCreateWithUTF8CString("feature");
    JSObjectSetProperty(_context, jsGlobalObject, jsFeatureName, jsFeatureObject, kJSPropertyAttributeNone, nullptr);
    JSStringRelease(jsFeatureName);

    // Create geometry constants.
    std::array<std::pair<const char*, int>, 3> geometryConstants{
        {{"point", GeometryType::points}, {"line", GeometryType::lines}, {"polygon", GeometryType::polygons}}
    };
    for (const auto& pair : geometryConstants) {
        JSStringRef jsPropertyName = JSStringCreateWithUTF8CString(pair.first);
        JSValueRef jsPropertyValue = JSValueMakeNumber(_context, pair.second);
        JSObjectSetProperty(_context, jsGlobalObject, jsPropertyName, jsPropertyValue, kJSPropertyAttributeReadOnly, nullptr);
        JSStringRelease(jsPropertyName);
    }
}

JSCoreContext::~JSCoreContext() {
    JSGlobalContextRelease(_context);
}

void JSCoreContext::setGlobalValue(const std::string& name, JSValue value) {
    JSObjectRef jsGlobalObject = JSContextGetGlobalObject(_context);
    JSStringRef jsPropertyName = JSStringCreateWithUTF8CString(name.c_str());
    JSValueRef jsValueForProperty = reinterpret_cast<JSCoreValue*>(value.get())->getValueRef();
    JSObjectSetProperty(_context, jsGlobalObject, jsPropertyName, jsValueForProperty, kJSPropertyAttributeNone, nullptr);
    JSStringRelease(jsPropertyName);
}

void JSCoreContext::setCurrentFeature(const Feature* feature) {
    _feature = feature;
}

bool JSCoreContext::setFunction(JSFunctionIndex index, const std::string& source) {
    JSObjectRef jsFunctionObject = compileFunction(source);
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

bool JSCoreContext::evaluateBooleanFunction(JSFunctionIndex index) {
    auto resultValue = getFunctionResult(index);
    if (resultValue) {
        return resultValue->toBool();
    }
    return false;
}

JSValue JSCoreContext::newNull() {
    JSValueRef jsValue = JSValueMakeNull(_context);
    return JSValue(new JSCoreValue(_context, jsValue));
}

JSValue JSCoreContext::newBoolean(bool value) {
    JSValueRef jsValue = JSValueMakeBoolean(_context, value);
    return JSValue(new JSCoreValue(_context, jsValue));
}

JSValue JSCoreContext::newNumber(double value) {
    JSValueRef jsValue = JSValueMakeNumber(_context, value);
    return JSValue(new JSCoreValue(_context, jsValue));
}

JSValue JSCoreContext::newString(const std::string& value) {
    JSStringRef jsString = JSStringCreateWithUTF8CString(value.c_str());
    JSValueRef jsValue = JSValueMakeString(_context, jsString);
    return JSValue(new JSCoreValue(_context, jsValue));
}

JSValue JSCoreContext::newArray() {
    JSObjectRef jsObject = JSObjectMakeArray(_context, 0, nullptr, nullptr);
    return JSValue(new JSCoreValue(_context, jsObject));
}

JSValue JSCoreContext::newObject() {
    JSObjectRef jsObject = JSObjectMake(_context, nullptr, nullptr);
    return JSValue(new JSCoreValue(_context, jsObject));
}

JSValue JSCoreContext::newFunction(const std::string& value) {
    JSObjectRef jsFunctionObject = compileFunction(value);
    if (jsFunctionObject == nullptr) {
        return nullptr;
    }
    return JSValue(new JSCoreValue(_context, jsFunctionObject));
}

JSValue JSCoreContext::getFunctionResult(JSFunctionIndex index) {
    if (index > _functions.size()) {
        return nullptr;
    }
    JSObjectRef jsFunctionObject = _functions[index];
    JSValueRef jsException = nullptr;
    JSValueRef jsResultValue = JSObjectCallAsFunction(_context, jsFunctionObject, nullptr, 0, nullptr, &jsException);
    if (jsException != nullptr) {
        char buffer[128];
        JSStringRef jsExceptionString = JSValueToStringCopy(_context, jsException, nullptr);
        JSStringGetUTF8CString(jsExceptionString, buffer, sizeof(buffer));
        LOGE("Error evaluating JavaScript function - %s", buffer);
        JSStringRelease(jsExceptionString);
        return nullptr;
    }
    return JSValue(new JSCoreValue(_context, jsResultValue));
}

JSScopeMarker JSCoreContext::getScopeMarker() {
    // Not needed for JSCore implementation.
    return 0;
}

void JSCoreContext::resetToScopeMarker(JSScopeMarker) {
    // Not needed for JSCore implementation.
}

JSObjectRef JSCoreContext::compileFunction(const std::string& source) {
    std::string expression("(" + source + ")");
    JSStringRef jsFunctionSource = JSStringCreateWithUTF8CString(expression.c_str());
    JSValueRef jsException = nullptr;
    JSValueRef jsFunction = JSEvaluateScript(_context, jsFunctionSource, nullptr, nullptr, 0, &jsException);
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

bool JSCoreContext::jsHasPropertyCallback(JSContextRef, JSObjectRef object, JSStringRef property) {
    auto jsCoreContext = reinterpret_cast<JSCoreContext*>(JSObjectGetPrivate(object));
    if (!jsCoreContext) {
        return false;
    }
    auto feature = jsCoreContext->_feature;
    if (!feature) {
        return false;
    }
    char nameBuffer[128]; // This should be enough for all the names we use - could make it dynamically-sized if needed.
    JSStringGetUTF8CString(property, nameBuffer, sizeof(nameBuffer));
    return feature->props.contains(nameBuffer);
}

JSValueRef JSCoreContext::jsGetPropertyCallback(JSContextRef context, JSObjectRef object, JSStringRef property, JSValueRef*) {
    auto jsCoreContext = reinterpret_cast<JSCoreContext*>(JSObjectGetPrivate(object));
    if (!jsCoreContext) {
        return nullptr;
    }
    auto feature = jsCoreContext->_feature;
    if (!feature) {
        return nullptr;
    }
    JSValueRef jsValue = nullptr;
    char nameBuffer[128]; // This should be enough for all the names we use - could make it dynamically-sized if needed.
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

} // namespace Tangram
