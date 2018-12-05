//
// Created by Matt Blair on 11/15/18.
//

#include "JSCoreContext.h"
#include "log.h"
#include "data/tileData.h"
#include "util/variant.h"
#include <array>

namespace Tangram {

JSCoreContext::JSCoreContext() : _strings(256) {

    _group = JSContextGroupCreate();
    _context = JSGlobalContextCreateInGroup(_group, nullptr);

    // Create the global 'feature' object.
    JSClassDefinition jsFeatureClassDefinition = kJSClassDefinitionEmpty;
    // Empirically, feature property access is slightly faster when only 'getProperty' is provided.
    //jsFeatureClassDefinition.hasProperty = jsHasPropertyCallback;
    jsFeatureClassDefinition.getProperty = jsGetPropertyCallback;
    JSClassRef jsFeatureClass = JSClassCreate(&jsFeatureClassDefinition);
    JSObjectRef jsFeatureObject = JSObjectMake(_context, jsFeatureClass, this);
    JSClassRelease(jsFeatureClass);
    JSObjectRef jsGlobalObject = JSContextGetGlobalObject(_context);
    JSStringRef jsFeatureName = JSStringCreateWithUTF8CString("feature");
    JSObjectSetProperty(_context, jsGlobalObject, jsFeatureName, jsFeatureObject, kJSPropertyAttributeReadOnly, nullptr);
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
    JSContextGroupRelease(_group);
}

void JSCoreContext::setGlobalValue(const std::string& name, JSCoreValue value) {
    JSObjectRef jsGlobalObject = JSContextGetGlobalObject(_context);
    JSStringRef jsPropertyName = JSStringCreateWithUTF8CString(name.c_str());
    JSValueRef jsValueForProperty = value.getValueRef();
    JSObjectSetProperty(_context, jsGlobalObject, jsPropertyName, jsValueForProperty, kJSPropertyAttributeReadOnly, nullptr);
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
        return resultValue.toBool();
    }
    return false;
}

JSCoreValue JSCoreContext::newNull() {
    JSValueRef jsValue = JSValueMakeNull(_context);
    return JSCoreValue(_context, jsValue);
}

JSCoreValue JSCoreContext::newBoolean(bool value) {
    JSValueRef jsValue = JSValueMakeBoolean(_context, value);
    return JSCoreValue(_context, jsValue);
}

JSCoreValue JSCoreContext::newNumber(double value) {
    JSValueRef jsValue = JSValueMakeNumber(_context, value);
    return JSCoreValue(_context, jsValue);
}

JSCoreValue JSCoreContext::newString(const std::string& value) {
    JSStringRef jsString = JSStringCreateWithUTF8CString(value.c_str());
    JSValueRef jsValue = JSValueMakeString(_context, jsString);
    return JSCoreValue(_context, jsValue);
}

JSCoreValue JSCoreContext::newArray() {
    JSObjectRef jsObject = JSObjectMakeArray(_context, 0, nullptr, nullptr);
    return JSCoreValue(_context, jsObject);
}

JSCoreValue JSCoreContext::newObject() {
    JSObjectRef jsObject = JSObjectMake(_context, nullptr, nullptr);
    return JSCoreValue(_context, jsObject);
}

JSCoreValue JSCoreContext::newFunction(const std::string& value) {
    JSObjectRef jsFunctionObject = compileFunction(value);
    if (jsFunctionObject == nullptr) {
        return JSCoreValue();
    }
    return JSCoreValue(_context, jsFunctionObject);
}

JSCoreValue JSCoreContext::getFunctionResult(JSFunctionIndex index) {
    if (index > _functions.size()) {
        return JSCoreValue();
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
        return JSCoreValue();
    }
    return JSCoreValue(_context, jsResultValue);
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
        jsValue = jsCoreContext->_strings.get(context, it.get<std::string>());
    } else if (it.is<double>()) {
        jsValue = JSValueMakeNumber(context, it.get<double>());
    }
    return jsValue;
}

} // namespace Tangram
