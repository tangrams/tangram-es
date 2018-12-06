//
// Created by Matt Blair on 2018-12-03.
//
#pragma once

#include <cstdint>

namespace Tangram {

#if TANGRAM_USE_JSCORE
class JSCoreValue;
class JSCoreContext;
using JSValue = JSCoreValue;
using JSContext = JSCoreContext;
#else
class DuktapeValue;
class DuktapeContext;
using JSValue = DuktapeValue;
using JSContext = DuktapeContext;
#endif

struct Feature;

using JSScopeMarker = int32_t;
using JSFunctionIndex = uint32_t;

template<class Context> class JavaScriptScope;

using JSScope = JavaScriptScope<JSContext>;

} // namespace Tangram
