//
// Created by Matt Blair on 11/9/18.
//
#pragma once
#include <memory>
#include <string>

#if TANGRAM_USE_JSCORE
#include "js/jsCoreContext.h"
#endif
#include "js/duktapeContext.h"

namespace Tangram {

template<class Context>
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

    auto newNull() { return _context.newNull(); }
    auto newBoolean(bool value) { return _context.newBoolean(value); }
    auto newNumber(double value) { return _context.newNumber(value); }
    auto newString(const std::string& value) { return _context.newString(value); }
    auto newArray() { return _context.newArray(); }
    auto newObject() { return _context.newObject(); }
    auto newFunction(const std::string& value) { return _context.newFunction(value); }
    auto getFunctionResult(JSFunctionIndex index) { return _context.getFunctionResult(index); }

private:

    Context& _context;
    JSScopeMarker _scopeMarker = 0;
};

} // namespace Tangram
