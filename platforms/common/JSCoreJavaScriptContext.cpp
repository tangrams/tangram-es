//
// Created by Matt Blair on 11/15/18.
//

#include "JSCoreJavaScriptContext.h"
#include "log.h"
#include "data/tileData.h"
#include "util/variant.h"

namespace Tangram {

JSContext createJavaScriptContext() {
    return JSContext(new JSCoreJavaScriptContext());
}

JSCoreJavaScriptValue::JSCoreJavaScriptValue(JSContextRef ctx, JSValueRef value) : _ctx(ctx), _value(value) {
    JSValueProtect(_ctx, _value);
}

JSCoreJavaScriptValue::~JSCoreJavaScriptValue() {
    JSValueUnprotect(_ctx, _value);
}

bool JSCoreJavaScriptValue::isUndefined() {
    return JSValueIsUndefined(_ctx, _value);
}

bool JSCoreJavaScriptValue::isNull() {
    return JSValueIsNull(_ctx, _value);
}

bool JSCoreJavaScriptValue::isBoolean() {
    return JSValueIsBoolean(_ctx, _value);
}

bool JSCoreJavaScriptValue::isNumber() {
    return JSValueIsNumber(_ctx, _value);
}

bool JSCoreJavaScriptValue::isString() {
    return JSValueIsString(_ctx, _value);
}

bool JSCoreJavaScriptValue::isArray() {
    return JSValueIsArray(_ctx, _value);
}

bool JSCoreJavaScriptValue::isObject() {
    return JSValueIsObject(_ctx, _value);
}

bool JSCoreJavaScriptValue::toBool() {
    return JSValueToBoolean(_ctx, _value);
}

int JSCoreJavaScriptValue::toInt() {
    return static_cast<int>(JSValueToNumber(_ctx, _value, nullptr));
}

double JSCoreJavaScriptValue::toDouble() {
    return JSValueToNumber(_ctx, _value, nullptr);
}

std::string JSCoreJavaScriptValue::toString() {
    JSStringRef jsString = JSValueToStringCopy(_ctx, _value, nullptr);
    std::string result(JSStringGetMaximumUTF8CStringSize(jsString), '\0');
    size_t bytesWrittenIncludingNull = JSStringGetUTF8CString(jsString, &result[0], result.capacity());
    result.resize(bytesWrittenIncludingNull - 1);
    JSStringRelease(jsString);
    return result;
}

size_t JSCoreJavaScriptValue::getLength() {
    JSStringRef jsLengthProperty = JSStringCreateWithUTF8CString("length");
    JSObjectRef jsObject = JSValueToObject(_ctx, _value, nullptr);
    JSValueRef jsLengthValue = JSObjectGetProperty(_ctx, jsObject, jsLengthProperty, nullptr);
    return static_cast<size_t>(JSValueToNumber(_ctx, jsLengthValue, nullptr));
}

JSValue JSCoreJavaScriptValue::getValueAtIndex(size_t index) {
    JSObjectRef jsObject = JSValueToObject(_ctx, _value, nullptr);
    JSValueRef jsValue = JSObjectGetPropertyAtIndex(_ctx, jsObject, static_cast<uint32_t>(index), nullptr);
    return JSValue(new JSCoreJavaScriptValue(_ctx, jsValue));
}

JSValue JSCoreJavaScriptValue::getValueForProperty(const std::string& name) {
    JSObjectRef jsObject = JSValueToObject(_ctx, _value, nullptr);
    JSStringRef jsPropertyName = JSStringCreateWithUTF8CString(name.c_str());
    JSValueRef jsPropertyValue = JSObjectGetProperty(_ctx, jsObject, jsPropertyName, nullptr);
    return JSValue(new JSCoreJavaScriptValue(_ctx, jsPropertyValue));
}

void JSCoreJavaScriptValue::setValueAtIndex(size_t index, JSValue value) {
    JSObjectRef jsObject = JSValueToObject(_ctx, _value, nullptr);
    JSValueRef jsValueForIndex = reinterpret_cast<JSCoreJavaScriptValue*>(value.get())->getValueRef();
    JSObjectSetPropertyAtIndex(_ctx, jsObject, static_cast<uint32_t>(index), jsValueForIndex, nullptr);
}

void JSCoreJavaScriptValue::setValueForProperty(const std::string& name, JSValue value) {
    JSObjectRef jsObject = JSValueToObject(_ctx, _value, nullptr);
    JSStringRef jsPropertyName = JSStringCreateWithUTF8CString(name.c_str());
    JSValueRef jsValueForProperty = reinterpret_cast<JSCoreJavaScriptValue*>(value.get())->getValueRef();
    JSObjectSetProperty(_ctx, jsObject, jsPropertyName, jsValueForProperty, kJSPropertyAttributeNone, nullptr);
}

JSCoreJavaScriptContext::JSCoreJavaScriptContext() {
    _contextGroup = JSContextGroupCreate();
    _context = JSGlobalContextCreateInGroup(_contextGroup, nullptr);

    // Create the global 'feature' object.
    JSClassDefinition jsFeatureClassDefinition = kJSClassDefinitionEmpty;
    jsFeatureClassDefinition.hasProperty = jsHasPropertyCallback;
    jsFeatureClassDefinition.getProperty = jsGetPropertyCallback;
    JSClassRef jsFeatureClass = JSClassCreate(&jsFeatureClassDefinition);
    JSObjectRef jsFeatureObject = JSObjectMake(_context, jsFeatureClass, this);
    JSClassRelease(jsFeatureClass);
    JSObjectRef jsGlobalObject = JSContextGetGlobalObject(_context);
    JSStringRef jsFeatureName = JSStringCreateWithUTF8CString("feature");
    JSObjectSetProperty(_context, jsGlobalObject, jsFeatureName, jsFeatureObject, kJSPropertyAttributeReadOnly, nullptr);
    JSStringRelease(jsFeatureName);
}

JSCoreJavaScriptContext::~JSCoreJavaScriptContext() {
    JSGlobalContextRelease(_context);
    JSContextGroupRelease(_contextGroup);
}

void JSCoreJavaScriptContext::setGlobalValue(const std::string& name, JSValue value) {
    JSObjectRef jsGlobalObject = JSContextGetGlobalObject(_context);
    JSStringRef jsPropertyName = JSStringCreateWithUTF8CString(name.c_str());
    JSValueRef jsValueForProperty = reinterpret_cast<JSCoreJavaScriptValue*>(value.get())->getValueRef();
    JSObjectSetProperty(_context, jsGlobalObject, jsPropertyName, jsValueForProperty, kJSPropertyAttributeNone, nullptr);
    JSStringRelease(jsPropertyName);
}

void JSCoreJavaScriptContext::setCurrentFeature(const Feature* feature) {
    _feature = feature;
}

JSFunctionIndex JSCoreJavaScriptContext::addFunction(const std::string& source, bool& error) {
    auto newFunctionIndex = static_cast<JSFunctionIndex>(_functions.size());
    JSObjectRef jsFunctionObject = compileFunction(source);
    if (!jsFunctionObject) {
        error = true;
    }
    // Function objects will live for the entire duration of the context, so we can retain them once and never release
    // them. They will be released when the context is released.
    JSValueProtect(_context, jsFunctionObject);
    _functions.push_back(jsFunctionObject);
    return newFunctionIndex;
}

bool JSCoreJavaScriptContext::evaluateBooleanFunction(JSFunctionIndex index) {
    auto resultValue = getFunctionResult(index);
    if (resultValue) {
        return resultValue->toBool();
    }
    return false;
}

JSValue JSCoreJavaScriptContext::newNull() {
    JSValueRef jsValue = JSValueMakeNull(_context);
    return JSValue(new JSCoreJavaScriptValue(_context, jsValue));
}

JSValue JSCoreJavaScriptContext::newBoolean(bool value) {
    JSValueRef jsValue = JSValueMakeBoolean(_context, value);
    return JSValue(new JSCoreJavaScriptValue(_context, jsValue));
}

JSValue JSCoreJavaScriptContext::newNumber(double value) {
    JSValueRef jsValue = JSValueMakeNumber(_context, value);
    return JSValue(new JSCoreJavaScriptValue(_context, jsValue));
}

JSValue JSCoreJavaScriptContext::newString(const std::string& value) {
    JSStringRef jsString = JSStringCreateWithUTF8CString(value.c_str());
    JSValueRef jsValue = JSValueMakeString(_context, jsString);
    return JSValue(new JSCoreJavaScriptValue(_context, jsValue));
}

JSValue JSCoreJavaScriptContext::newArray() {
    JSObjectRef jsObject = JSObjectMakeArray(_context, 0, nullptr, nullptr);
    return JSValue(new JSCoreJavaScriptValue(_context, jsObject));
}

JSValue JSCoreJavaScriptContext::newObject() {
    JSObjectRef jsObject = JSObjectMake(_context, nullptr, nullptr);
    return JSValue(new JSCoreJavaScriptValue(_context, jsObject));
}

JSValue JSCoreJavaScriptContext::newFunction(const std::string& value) {
    JSObjectRef jsFunctionObject = compileFunction(value);
    return JSValue(new JSCoreJavaScriptValue(_context, jsFunctionObject));
}

JSValue JSCoreJavaScriptContext::getFunctionResult(JSFunctionIndex index) {
    if (index > _functions.size()) {
        return nullptr;
    }
    JSObjectRef jsFunctionObject = _functions[index];
    JSValueRef jsResultValue = JSObjectCallAsFunction(_context, jsFunctionObject, nullptr, 0, nullptr, nullptr);
    return JSValue(new JSCoreJavaScriptValue(_context, jsResultValue));
}

JSScopeMarker JSCoreJavaScriptContext::getScopeMarker() {
    // Not needed for JSCore implementation.
    return 0;
}

void JSCoreJavaScriptContext::resetToScopeMarker(JSScopeMarker) {
    // Not needed for JSCore implementation.
}

JSObjectRef JSCoreJavaScriptContext::compileFunction(const std::string& source) {
    // Get the function body within the enclosing {}'s.
    auto bodyStart = source.find('{') + 1;
    auto bodyEnd = source.rfind('}') - 1;
    auto body = source.substr(bodyStart, bodyEnd - bodyStart);
    JSStringRef jsFunctionSource = JSStringCreateWithUTF8CString(body.c_str());
    JSValueRef jsException = nullptr;
    JSObjectRef jsObject = JSObjectMakeFunction(_context, nullptr, 0, nullptr, jsFunctionSource, nullptr, 0, &jsException);
    JSStringRelease(jsFunctionSource);
    if (jsException) {
        char buffer[128];
        JSStringRef jsExceptionString = JSValueToStringCopy(_context, jsException, nullptr);
        JSStringGetUTF8CString(jsExceptionString, buffer, sizeof(buffer));
        LOGE("Error compiling JavaScript function - %s", buffer);
        JSStringRelease(jsExceptionString);
    }
    return jsObject;
}

bool JSCoreJavaScriptContext::jsHasPropertyCallback(JSContextRef context, JSObjectRef object, JSStringRef property) {
    auto jsCoreContext = reinterpret_cast<JSCoreJavaScriptContext*>(JSObjectGetPrivate(object));
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

JSValueRef JSCoreJavaScriptContext::jsGetPropertyCallback(JSContextRef context, JSObjectRef object, JSStringRef property, JSValueRef* exception) {
    auto jsCoreContext = reinterpret_cast<JSCoreJavaScriptContext*>(JSObjectGetPrivate(object));
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
