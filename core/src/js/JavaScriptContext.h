//
// Created by Matt Blair on 11/9/18.
//
#pragma once
#include <memory>
#include <string>
#include "js/JavaScriptFwd.h"

#ifdef TANGRAM_USE_DUKTAPE
#include "js/DuktapeContext.h"
#endif

#ifdef TANGRAM_USE_JAVASCRIPTCORE
#include "js/JSCoreContext.h"
#endif

namespace Tangram {

class JavaScriptScope {

public:

    explicit JavaScriptScope(JSContext& context) : _context(context) {
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

    JSContext& _context;
    JSScopeMarker _scopeMarker = 0;
};

} // namespace Tangram
