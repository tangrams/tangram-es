//
// Created by Matt Blair on 11/9/18.
//
#pragma once
#include <memory>
#include <string>
#include "js/JavaScriptFwd.h"

#ifdef TANGRAM_USE_JAVASCRIPTCORE
#include "js/JSCoreContext.h"
#else
#include "js/DuktapeContext.h"
#endif

namespace Tangram {

template<class Context, class Value>
class JavaScriptScope {

public:

    explicit JavaScriptScope(Context& context) : _context(context) {
        _scopeMarker = _context.getScopeMarker();
    }

    ~JavaScriptScope() {
        _context.resetToScopeMarker(_scopeMarker);
    }

    JavaScriptScope& operator=(const JavaScriptScope& other) = delete;
    JavaScriptScope& operator=(JavaScriptScope&& other) = delete;

    Value newNull() { return _context.newNull(); }
    Value newBoolean(bool value) { return _context.newBoolean(value); }
    Value newNumber(double value) { return _context.newNumber(value); }
    Value newString(const std::string& value) { return _context.newString(value); }
    Value newArray() { return _context.newArray(); }
    Value newObject() { return _context.newObject(); }
    Value newFunction(const std::string& value) { return _context.newFunction(value); }
    Value getFunctionResult(JSFunctionIndex index) { return _context.getFunctionResult(index); }

private:

    Context& _context;
    JSScopeMarker _scopeMarker = 0;
};

} // namespace Tangram
