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

    JSCoreValue(JSContextRef ctx, JSValueRef value);

    ~JSCoreValue();

    operator bool() const { return _ctx != nullptr; }

    bool isUndefined();
    bool isNull();
    bool isBoolean();
    bool isNumber();
    bool isString();
    bool isArray();
    bool isObject();

    bool toBool();
    int toInt();
    double toDouble();
    std::string toString();

    size_t getLength();
    JSCoreValue getValueAtIndex(size_t index);
    JSCoreValue getValueForProperty(const std::string& name);

    void setValueAtIndex(size_t index, JSCoreValue value);
    void setValueForProperty(const std::string& name, JSCoreValue value);

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

    friend JavaScriptScope;
};

} // namespace Tangram
