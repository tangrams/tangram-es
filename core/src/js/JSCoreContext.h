//
// Created by Matt Blair on 11/15/18.
//
#pragma once
#include "js/JavaScriptFwd.h"
#include <JavaScriptCore/JavaScript.h>
#include <list>
#include <unordered_map>
#include <string>
#include <vector>

namespace Tangram {

class JSCoreValue {

public:

    JSCoreValue() = default;

    JSCoreValue(JSContextRef ctx, JSValueRef value) : _ctx(ctx), _value(value) {
        JSValueProtect(_ctx, _value);
    }

    JSCoreValue(JSCoreValue&& other) noexcept : _ctx(other._ctx), _value(other._value) {
        other._ctx = nullptr;
        other._value = nullptr;
    }

    ~JSCoreValue() {
        if (_ctx) {
            JSValueUnprotect(_ctx, _value);
        }
    }

    JSCoreValue& operator=(const JSCoreValue& other) = delete;

    JSCoreValue& operator=(JSCoreValue&& other) noexcept {
        _ctx = other._ctx;
        _value = other._value;
        other._ctx = nullptr;
        other._value = nullptr;
        return *this;
    }

    operator bool() const { return _ctx != nullptr; }

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
        JSStringRelease(jsLengthProperty);
        return static_cast<size_t>(JSValueToNumber(_ctx, jsLengthValue, nullptr));
    }

    JSCoreValue getValueAtIndex(size_t index) {
        JSObjectRef jsObject = JSValueToObject(_ctx, _value, nullptr);
        JSValueRef jsValue = JSObjectGetPropertyAtIndex(_ctx, jsObject, static_cast<uint32_t>(index), nullptr);
        return JSCoreValue(_ctx, jsValue);
    }

    JSCoreValue getValueForProperty(const std::string& name) {
        JSObjectRef jsObject = JSValueToObject(_ctx, _value, nullptr);
        JSStringRef jsPropertyName = JSStringCreateWithUTF8CString(name.c_str());
        JSValueRef jsPropertyValue = JSObjectGetProperty(_ctx, jsObject, jsPropertyName, nullptr);
        JSStringRelease(jsPropertyName);
        return JSCoreValue(_ctx, jsPropertyValue);
    }

    void setValueAtIndex(size_t index, JSCoreValue value) {
        JSObjectRef jsObject = JSValueToObject(_ctx, _value, nullptr);
        JSObjectSetPropertyAtIndex(_ctx, jsObject, static_cast<uint32_t>(index), value.getValueRef(), nullptr);
    }

    void setValueForProperty(const std::string& name, JSCoreValue value) {
        JSObjectRef jsObject = JSValueToObject(_ctx, _value, nullptr);
        JSStringRef jsPropertyName = JSStringCreateWithUTF8CString(name.c_str());
        JSObjectSetProperty(_ctx, jsObject, jsPropertyName, value.getValueRef(), kJSPropertyAttributeNone, nullptr);
        JSStringRelease(jsPropertyName);
    }

    JSValueRef getValueRef() { return _value; }

private:

    JSContextRef _ctx = nullptr;
    JSValueRef _value = nullptr;
};

class JSCoreStringCache {

public:

    explicit JSCoreStringCache(size_t capacity) : _capacity(capacity) {
        // Must reserve the maximum number of keys we'll need so no rehashing occurs, otherwise we'll invalidate the
        // iterators in CacheEntry.
        _map.reserve(capacity + 1);
    };

    JSValueRef get(JSContextRef context, const std::string& key) {
        bool inserted;
        CacheMap::iterator mapIt;
        std::tie(mapIt, inserted) = _map.emplace(key, CacheList::iterator{});
        if (inserted) {
            // New entry - create JS string and add to front of list.
            JSStringRef jsString = JSStringCreateWithUTF8CString(key.c_str());
            JSValueRef jsValue = JSValueMakeString(context, jsString);
            JSValueProtect(context, jsValue);
            JSStringRelease(jsString);
            _list.emplace_front(CacheEntry{mapIt, jsValue});
            mapIt->second = _list.begin();
        } else {
            // Existing entry - move to front of list.
            auto listIt = mapIt->second;
            if (listIt != _list.begin()) {
                _list.splice(_list.begin(), _list, listIt);
            }
        }
        while (_list.size() > _capacity) {
            auto& leastRecentEntry = _list.back();
            JSValueUnprotect(context, leastRecentEntry.jsValue);
            _map.erase(leastRecentEntry.mapIt);
            _list.pop_back();
        }
        return mapIt->second->jsValue;
    }

private:

    struct CacheEntry;
    using CacheList = std::list<CacheEntry>;
    using CacheMap = std::unordered_map<std::string, typename CacheList::iterator>;

    struct CacheEntry {
        CacheMap::iterator mapIt;
        JSValueRef jsValue;
    };

    CacheMap _map;
    CacheList _list;
    size_t _capacity;
};

class JSCoreContext {

public:

    JSCoreContext();

    ~JSCoreContext();

    void setGlobalValue(const std::string& name, JSCoreValue value);

    void setCurrentFeature(const Feature* feature);

    bool setFunction(JSFunctionIndex index, const std::string& source);

    bool evaluateBooleanFunction(JSFunctionIndex index);

protected:

    JSCoreValue newNull();
    JSCoreValue newBoolean(bool value);
    JSCoreValue newNumber(double value);
    JSCoreValue newString(const std::string& value);
    JSCoreValue newArray();
    JSCoreValue newObject();
    JSCoreValue newFunction(const std::string& value);
    JSCoreValue getFunctionResult(JSFunctionIndex index);

    JSScopeMarker getScopeMarker();
    void resetToScopeMarker(JSScopeMarker marker);

private:

    static bool jsHasPropertyCallback(JSContextRef context, JSObjectRef object, JSStringRef property);
    static JSValueRef jsGetPropertyCallback(JSContextRef context, JSObjectRef object, JSStringRef property, JSValueRef* exception);

    JSObjectRef compileFunction(const std::string& source);

    std::vector<JSObjectRef> _functions;

    JSContextGroupRef _group;
    JSGlobalContextRef _context;

    JSCoreStringCache _strings;

    const Feature* _feature;

    friend JavaScriptScope<JSCoreContext>;
};

} // namespace Tangram
