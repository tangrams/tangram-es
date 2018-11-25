//
// Created by Matt Blair on 11/15/18.
//
#pragma once
#include "IJavaScriptContext.h"
#include <JavaScriptCore/JavaScript.h>
#include <list>
#include <unordered_map>
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

class JSCoreContext : public IJavaScriptContext {

public:

    JSCoreContext();

    ~JSCoreContext() override;

    void setGlobalValue(const std::string& name, JSValue value) override;

    void setCurrentFeature(const Feature* feature) override;

    bool setFunction(JSFunctionIndex index, const std::string& source) override;

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

    JSContextGroupRef _group;
    JSGlobalContextRef _context;

    JSCoreStringCache _strings;

    const Feature* _feature;
};

} // namespace Tangram
