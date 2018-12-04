#pragma once

#include "scene/styleContext.h"
#include "duktape/duktape.h"
#include "scene/scene.h"
#include "log.h"
#include <array>

#if 0
#define DUMP() do {                             \
        bool calling = _calling;                \
        _dumping = true;                        \
        duk_push_context_dump(_ctx);            \
        LOG("%s", duk_to_string(_ctx, -1));     \
        duk_pop(_ctx);                          \
        _calling = calling;                     \
        _dumping = false;                       \
    } while(0)
#define DUMPCTX(context) do {                           \
        bool calling = context._calling;                \
        context._dumping = true;                        \
        duk_push_context_dump(context._ctx);            \
        LOG("%s", duk_to_string(context._ctx, -1));     \
        duk_pop(context._ctx);                          \
        context._calling = calling;                     \
        context._dumping = false;                       \
    } while(0)
#define DBG(...) LOG(__VA_ARGS__)
#else
#define DUMP()
#define DBG(...)
#define DUMPCTX(...)
#endif
#define DBGCACHE(...)
//#define DBGCACHE(...) LOG(__VA_ARGS__)

namespace Tangram {

using JSScopeMarker = int32_t;
using JSFunctionIndex = uint32_t;

namespace Duktape {

const static char FUNC_ID[] = "\xff""\xff""fns";
const static char GLOBAL_ID[] = "\xff""\xff""glb";
const static char FEATURE_ID[] = "\xff""\xff""fet";

struct Context;

struct Value {

    duk_context* _ctx = nullptr;
    duk_idx_t _index = 0;

    Value(duk_context* ctx, duk_idx_t index) : _ctx(ctx), _index(index) {}

    Value(Value&& _other) : _ctx(_other._ctx), _index(_other._index) {
        _other._ctx = nullptr;
    }

    Value& operator=(Value&& _other) {
        this->_ctx = _other._ctx;
        this->_index = _other._index;
        _other._ctx = nullptr;
        return *this;
    }

    operator bool() const { return bool(_ctx ); }

    bool isUndefined() {
        return duk_is_undefined(_ctx, _index) != 0;
    }

    bool isNull() {
        return duk_is_null(_ctx, _index) != 0;
    }

    bool isBoolean() {
        return duk_is_boolean(_ctx, _index) != 0;
    }

    bool isNumber() {
        return duk_is_number(_ctx, _index) != 0;
    }

    bool isString() {
        return duk_is_string(_ctx, _index) != 0;
    }

    bool isArray() {
        return duk_is_array(_ctx, _index) != 0;
    }

    bool isObject() {
        return duk_is_object(_ctx, _index) != 0;
    }

    bool toBool() {
        return duk_to_boolean(_ctx, _index) != 0;
    }

    int toInt() {
        return duk_to_int(_ctx, _index);
    }

    double toDouble() {
        return duk_to_number(_ctx, _index);
    }

    std::string toString() {
        std::string result;
        size_t len;
        // NB this requires the value to be a string. Rename to getString()?
        if (const char* data = duk_get_lstring(_ctx, _index, &len)) {
            if (len > 0) { result = std::string(data, len); }
        }
        return result;
    }

    size_t getLength() {
        return duk_get_length(_ctx, _index);
    }

    Value getValueAtIndex(size_t index) {
        duk_get_prop_index(_ctx, _index, static_cast<duk_uarridx_t>(index));
        return {_ctx, duk_normalize_index(_ctx, -1)};
    }

    Value getValueForProperty(const std::string& name) {
        duk_get_prop_lstring(_ctx, _index, name.data(), name.length());
        return {_ctx, duk_normalize_index(_ctx, -1)};
    }

    void setValueAtIndex(size_t index, Value value) {
        value.ensureExistsOnStackTop();
        duk_put_prop_index(_ctx, _index, static_cast<duk_uarridx_t>(index));
    }

    void setValueForProperty(const std::string& name, Value value) {
        value.ensureExistsOnStackTop();
        duk_put_prop_lstring(_ctx, _index, name.data(), name.length());

    }

    void ensureExistsOnStackTop() {
        auto dukTopIndex = duk_get_top_index(_ctx);
        if (_index != dukTopIndex) {
            duk_require_stack_top(_ctx, dukTopIndex + 1);
            duk_dup(_ctx, _index);
        }
    }
};

struct Function {
    JSFunctionIndex idx = 0;
    void* ptr = nullptr;
    struct {
        bool feature = false;
        bool zoom = false;
        bool geom = false;
        bool global = false;
    } context;
    std::string source;
};

static Context& getContext(duk_context* _ctx) {
    duk_memory_functions funcs;
    duk_get_memory_functions(_ctx, &funcs);
    assert(funcs.udata != nullptr);
    return *reinterpret_cast<Context*>(funcs.udata);
}

struct Context {

    duk_context* _ctx = nullptr;
    const Feature* _feature = nullptr;
    void* _featurePtr = nullptr;
    void* _objectPtr = nullptr;
    void* _functionsPtr = nullptr;
    void* _globalPtr = nullptr;
    void* _stringStashPtr = nullptr;

    std::vector<Function> m_functions;
    std::array<int, 4> m_filterKeys {};

    ~Context() {
        duk_destroy_heap(_ctx);
        LOG("gets:%d reused:%d - %f%% / extstr:%d freed:%d\n",
            _fetchCnt+_reuseCnt, _reuseCnt,
            float(_reuseCnt) / float(_fetchCnt + _reuseCnt) * 100,
            _allocCnt, _freeCnt);

        _stringCache8.dump();
        _stringCache32.dump();
        _stringCache128.dump();
    }

    Context() {

        static std::atomic<bool> initialized{false};
        if (!initialized.exchange(true)) {
            duk_extstr_set_handler(jsExtstrInternCheck, jsExtstrFree);
        }

        // Create duktape heap with default allocation functions and
        // custom fatal error handler.
        _ctx = duk_create_heap(nullptr, nullptr, nullptr,
                               this, fatalErrorHandler);

        //// Create global geometry constants
        duk_push_number(_ctx, GeometryType::points);
        duk_put_global_string(_ctx, "point");
        duk_push_number(_ctx, GeometryType::lines);
        duk_put_global_string(_ctx, "line");
        duk_push_number(_ctx, GeometryType::polygons);
        duk_put_global_string(_ctx, "polygon");

        // Set up 'fns' array.
        duk_idx_t functionsObj = duk_push_array(_ctx);
        _functionsPtr = duk_get_heapptr(_ctx, functionsObj);
        if (!_functionsPtr) {
            LOGE("'Function object not set");
            return;
        }
        if(!duk_put_global_string(_ctx, FUNC_ID)) {
            LOGE("'Function object not set 2");
            return;
        }

        // Create 'feature' object and store in global
        // Feature object
        duk_push_object(_ctx);
        _objectPtr = duk_require_heapptr(_ctx, -1);

        // Handler object
        duk_idx_t handlerObj = duk_push_object(_ctx);
        // Add 'get' property to handler
        duk_push_c_function(_ctx, jsGetProperty, 3);
        duk_put_prop_string(_ctx, handlerObj, "get");
        // Add 'has' property to handler
        duk_push_c_function(_ctx, jsHasProperty, 2);
        duk_put_prop_string(_ctx, handlerObj, "has");
        // [{get:func,has:func}]

        duk_push_proxy(_ctx, 0);
        duk_freeze(_ctx, -1);

        _featurePtr = duk_get_heapptr(_ctx, -1);
        // Stash 'feature' proxy object
        if (!duk_put_global_string(_ctx, FEATURE_ID)) {
            LOGE("Initialization failed");
            duk_pop(_ctx);
        }

        {
            duk_push_heap_stash(_ctx);
            duk_push_array(_ctx);
            _stringStashPtr = duk_get_heapptr(_ctx, -1);
            duk_put_prop_string(_ctx, -2, "stringstash");
            duk_pop(_ctx); // pop stash
        }

        DUMP();
        DBG("<<<<<<<<<");
    }

    void addStringProxy() {
        // duk_push_object(_ctx);
        // //_objectPtr = duk_require_heapptr(_ctx, -1);
        // // Handler object
        // duk_idx_t handlerObj = duk_push_object(_ctx);
        // // Add 'get' property to handler
        // duk_push_c_function(_ctx, dukStringProxyLength, 0);
        // duk_put_prop_string(_ctx, handlerObj, "length");
        // // Add 'has' property to handler
        // duk_push_c_function(_ctx, dukStringProxyValueOf, 0);
        // duk_put_prop_string(_ctx, handlerObj, "valueOf");
        // // [{get:func,has:func}]
        // duk_push_proxy(_ctx, 0);
        // _featurePtr = duk_get_heapptr(_ctx, -1);
    }

    void setGlobalValue(const std::string& name, Value value) {
        DBG(">>>>> GLOBAL >>>>>");
        //DUMP();
        value.ensureExistsOnStackTop();

        if (name == "global") {
            _globalPtr = duk_get_heapptr(_ctx, value._index);
            if (!_globalPtr) { LOGE("Global object invalid!"); }

            // Freeze object to not allow modifications
            duk_freeze(_ctx, -1);
            //duk_push_proxy(duk_context *ctx, duk_uint_t proxy_flags)
            // Stash global object
            duk_put_global_string(_ctx, GLOBAL_ID);
        } else {
            duk_put_global_lstring(_ctx, name.data(), name.length());
        }
        DBG("<<<<< GLOBAL <<<<<");
    }

    void setCurrentFeature(const Feature* feature) {
        // if (_was_called) LOG("[%p] Feature", _feature);
        // _was_called = false;

        _feature = feature;
        _lastFeature = nullptr;
        _propertyCacheUse = 0;

        //m_stringCache.clear();
    }

    void setFilterKey(Filter::Key _key, int _val) {
        m_filterKeys[uint8_t(_key)] = _val;
    }

    Value getStackTopValue() {
        return {_ctx, duk_normalize_index(_ctx, -1)};
    }

    JSScopeMarker getScopeMarker() {
        return duk_get_top(_ctx);
    }

    void resetToScopeMarker(JSScopeMarker marker) {
        duk_set_top(_ctx, marker);
    }

    bool evaluateBooleanFunction(JSFunctionIndex index) {
        if (!evaluateFunction(index)) { return false; }
        // Evaluate the "truthiness" of the function result at the top
        //  of the stack.
        bool result = duk_to_boolean(_ctx, -1) != 0;

        // pop result
        duk_pop(_ctx);
        return result;
    }

    Value getFunctionResult(JSFunctionIndex index) {
        if (!evaluateFunction(index)) {
            return {nullptr, 0};
        }
        return getStackTopValue();
    }

    bool setFunction(JSFunctionIndex index, const std::string& source) {
        DBG(">>>> FUNCTION >>>> %d", index);

        if (m_functions.size() == index) {
            m_functions.emplace_back();
        } else {
            m_functions.resize(index+1);
        }

        if (!duk_get_global_string(_ctx, FUNC_ID)) {
            LOGE("AddFunction - functions array not initialized");
            DUMP();
            duk_pop(_ctx); // pop [undefined] sitting at stack top
            return false;
        }
        auto& function = m_functions.back();

        bool append = false;
        std::string args;
        // TODO use proper regex

//#define BIND_GLOBAL_TO_FUNCTION
#ifndef BIND_GLOBAL_TO_FUNCTION
        size_t hasGlobal = source.find("global");
        if (hasGlobal != std::string::npos) {
            args += "global";
            append = true;
            function.context.global = true;
        }
#endif


        size_t hasFeature = source.find("feature");
        if (hasFeature != std::string::npos) {
            if (append) { args += ","; }
            args += "feature";
            append = true;
            function.context.feature = true;
        }
        size_t hasZoom = source.find("$zoom");
        if (hasZoom != std::string::npos) {
            if (append) { args += ","; }
            args += "$zoom";
            append = true;
            function.context.zoom = true;
        }
        size_t hasGeom = source.find("$geometry");
        if (hasGeom != std::string::npos) {
            if (append) { args += ","; }
            args += "$geometry";
            function.context.geom = true;
        }

        size_t beg = source.find('(')+1;
        function.source = source;
        function.source.insert(beg, args);


#ifdef BIND_GLOBAL_TO_FUNCTION
        size_t hasGlobal = source.find("global");
        if (hasGlobal != std::string::npos) {
            function.source = "function(global) { return " + function.source + " }";
            function.context.global = true;
        }
#endif

        if (duk_pcompile_lstring(_ctx, DUK_COMPILE_FUNCTION,
                                 function.source.c_str(), function.source.length()) == 0) {

#ifdef BIND_GLOBAL_TO_FUNCTION
            if (function.context.global) {
                duk_push_heapptr(_ctx, _globalPtr);

                if (duk_pcall(_ctx, 1) != 0) {
                    LOG("ERRRRRRRRRRRRR");
                }
            }
#endif

            function.ptr = duk_get_heapptr(_ctx, -1);

            // Store function in global.functions to make sure it will not be
            // garbage collected.
            duk_put_prop_index(_ctx, -2, index);

        } else {
            LOGW("Compile failed: %s\n%s\n---",
                 duk_safe_to_string(_ctx, -1), function.source.c_str());
            // Pop error
            duk_pop(_ctx);
            // Pop array
            duk_pop(_ctx);
            return false;
        }
        // Pop the functions array off the stack
        duk_pop(_ctx);
        DBG("<<<< FUNCTION <<<<");
        return true;
    }

    bool evaluateFunction(JSFunctionIndex index) {
        //DBG(">>>>> EVAL >>>>> %d", index);

        if (m_functions.size() <= index) {
            LOGE("Functions array not initialized. index:%d size:%d",
                 index, m_functions.size());
            return false;
        }
        if (m_functions[index].ptr == nullptr) {
            LOGE("Function not set. index:%d size:%d", index, m_functions.size());
            return false;
        }

        auto& function = m_functions[index];
        duk_push_heapptr(_ctx, function.ptr);
        int args = 0;
#ifndef BIND_GLOBAL_TO_FUNCTION
        if (function.context.global) {
             args++;
             duk_push_heapptr(_ctx, _globalPtr);
        }
#endif
        if (function.context.feature) {
            args++;
            duk_push_heapptr(_ctx, _featurePtr);
        }
        if (function.context.zoom) {
            args++;
            duk_push_number(_ctx, m_filterKeys[uint8_t(Filter::Key::zoom)]);
        }
        if (function.context.geom) {
            args++;
            duk_push_number(_ctx, m_filterKeys[uint8_t(Filter::Key::geometry)]);
        }

        _calling = true;
        _was_called = true;

        DBG(">>>>> Calling %d", index);
        DUMP();

        // DUMP();
        if (duk_pcall(_ctx, args) != 0) {
            _calling = false;

            LOG("Error: %s, function:%d feature:%p\n%s", duk_safe_to_string(_ctx, -1),
                index, _feature, function.source.c_str());

            // Pop error
            duk_pop(_ctx);
            return false;
        }
        _calling = false;
        // -- why not return value here?
        //DBG("<<<<< EVAL <<<<<");
        return true;
    }

    Value newNull() {
        duk_push_null(_ctx);
        return getStackTopValue();
    }

    Value newBoolean(bool value) {
        duk_push_boolean(_ctx, static_cast<duk_bool_t>(value));
        return getStackTopValue();
    }

    Value newNumber(double value) {
        duk_push_number(_ctx, value);
        return getStackTopValue();
    }

    Value newString(const std::string& value) {
        duk_push_lstring(_ctx, value.data(), value.length());
        return getStackTopValue();
    }

    Value newArray() {
        duk_push_array(_ctx);
        return getStackTopValue();
    }

    Value newObject() {
        duk_push_object(_ctx);
        return getStackTopValue();
    }

    Value newFunction(const std::string& value) {
        if (duk_pcompile_lstring(_ctx, DUK_COMPILE_FUNCTION,
                                 value.data(), value.length()) != 0) {

            auto error = duk_safe_to_string(_ctx, -1);
            LOGW("Compile failed in global function: %s\n%s\n---", error, value.c_str());
            duk_pop(_ctx); // Pop error.
            return {nullptr, 0};
        }
        return getStackTopValue();
    }
private:

    // Cache Feature property indexes
    const Feature* _lastFeature = nullptr;

    // Map strings to duk heapPtr
    //std::vector<std::pair<const std::string&, const void*>> _stringCache {};

    using prop_key = const char*;
    using prop_val = const Tangram::Value*;
    using heap_ptr = void*;


    struct cache_entry {
        prop_key key = nullptr;
        prop_val val = &NOT_A_VALUE;
        heap_ptr ptr = nullptr;
    };
    std::array<cache_entry, 16> _propertyCache {};

    uint32_t _propertyCacheUse = 0;
    int _reuseCnt = 0;
    int _fetchCnt = 0;
    int _allocCnt = 0;
    int _freeCnt = 0;

    bool _calling = false;
    bool _was_called = false;
    bool _dumping = false;

    const char* _lastPushed = nullptr;

    //std::set<std::string> _stringCache;
    std::string _buffer;

    template<size_t SIZE, size_t ENTRIES>
    struct StringCache {
        // 16KB on x64, 8KB on 32bit
        //  char=

        static constexpr size_t entrybytes = SIZE;
        static constexpr size_t stroffset = 1;

        //std::array<std::array<size_t, entrybytes / sizeof(size_t)>, 64> strings;
        // first char holds length!

        std::array<std::array<char, entrybytes>, ENTRIES> strings{};

        //std::array<uint8_t, SIZE> lengths{};
        std::array<int32_t, SIZE> refs{};

        int usage = 0;

        void free(Context& context, const void* ptr) {
            const char* str = (const char*)(ptr);
            const char* arr = (const char*)(&strings);
            const ptrdiff_t diff = str - arr;

            if ((diff > 0) && (diff < sizeof(strings))) {
                context._freeCnt++;

                int pos = (diff - stroffset) / ptrdiff_t(entrybytes);
                refs[pos]--;

                DBGCACHE("[%d] freed  %.*s", refs[pos], strings[pos][0], &strings[pos][stroffset]);
            }
        }

        const char* add(Context& context, void* ptr, size_t length) {
            const char* str = (const char*)ptr;
            int slot = -1;
            for (int i = 0; i < usage; i++) {
                DBGCACHE("[%d/%d]", i, usage);
                assert(int64_t(ENTRIES) - int64_t(i) > 0);

                if (length <= strings[i][0] && std::memcmp(&strings[i][1], str, length) == 0) {
                    refs[i]++;
                    DBGCACHE("[%d] found '%.*s'", refs[i], length, &strings[i][stroffset]);
                    return &strings[i][stroffset];
                }
                if (slot < 0 && refs[i] == 0) {
                    slot = i;
                }
            }
            if (slot == -1) {
                if (usage < ENTRIES) {
                    slot = usage++;
                } else {
                    LOG("cache full: %.*s len:%d - usage:%d, slot:%d", length, str, length, usage, slot);
                    return nullptr;
                }
            }

            std::memcpy(&strings[slot][stroffset], str, length);
            strings[slot][0] = uint8_t(length);

            refs[slot]++;

            context._allocCnt++;

            DBGCACHE("[%d] added: '%.*s'", refs[slot], length, str);

            return &strings[slot][stroffset];
        }
        void dump() {
            DBGCACHE("CACHE usage %d", usage);
            for (size_t i = 0; i < usage; i++) {
                 DBGCACHE("[%d] refs:%d '%.*s'", i, refs[i], strings[i][0], &strings[i][stroffset]);
            }
        }
    };

    StringCache<8, 128> _stringCache8;
    StringCache<32, 64> _stringCache32;
    StringCache<128, 16> _stringCache128;

    static const char* jsExtstrInternCheck(void* udata, void* str, unsigned long blen) {
        if (blen < 2) { return nullptr; } // TODO duk should have it's own small string cache!

        if (blen > 127) {
            LOG("[%p] >>>>> not my business %.*s - %d", str, blen, str, blen);
        }

        Context& context = *reinterpret_cast<Context*>(udata);
        if (context._calling) {
            //DBG("[%p / %p] >>>>> check %.*s - %d", str, context._lastPushed, blen, str, blen);

            if (context._lastPushed == str) {
                // We hold it - safe!
                DBGCACHE("[%p] >>>>> found %.*s - %d", str, blen, str, blen);
                return context._lastPushed;
            }

            if (blen < 8){
                // todo could one stash the heapptr in this call?
                return context._stringCache8.add(context, str, blen);
            } else if (blen < 64){
                return context._stringCache32.add(context, str, blen);
            } else  {
                return context._stringCache128.add(context, str, blen);
            }
        }
        return nullptr;
    }

    static void jsExtstrFree(void* udata, const void *extdata) {
        Context& context = *reinterpret_cast<Context*>(udata);

        context._stringCache8.free(context, extdata);
        context._stringCache32.free(context, extdata);
        context._stringCache128.free(context, extdata);
    }

    static int getProperty(Context& context) {
        // Get the requested object key
        const char* key = duk_require_string(context._ctx, 1);

        auto& cache = context._propertyCache;
        size_t use = context._propertyCacheUse;

        DBG("get prop %d %s", use, key);

        for (auto i = 0; i < use; i++) {
            DBG("entry cached");
            if (cache[i].key == key) {
                context._reuseCnt++;
                return i;
            }
        }

        context._fetchCnt++;

        if (use == cache.size()) {
            LOG("overflowing cache!");
            use = context._propertyCacheUse = 0;
        }

        //if (use < context->_propertyCache.size()) {
        DBG("caching");

        auto& entry= cache[use];
        entry.key = key;
        entry.val = &(context._feature->props.get(key));
        entry.ptr = nullptr;

        ++context._propertyCacheUse;
        //}
        return use;
    }

    // Implements Proxy handler.has(target_object, key)
    static int jsHasProperty(duk_context *_ctx) {
        auto& context = getContext(_ctx);
        if (context._feature) {

            int id = getProperty(context);
            auto& entry = context._propertyCache[id];

            bool hasProp = !(entry.val->is<none_type>());

            duk_push_boolean(_ctx, static_cast<duk_bool_t>(hasProp));
            return 1;
        }
        return 0;
    }

    // Implements Proxy handler.get(target_object, key)
    static int jsGetProperty(duk_context *_ctx) {
        DBG("jsGetProperty");

        auto& context = getContext(_ctx);
        if (context._dumping) { return 0; }

        DUMPCTX(context);

        if (!context._feature) { return 0; }

#ifdef STASH_PROPERTIES
        if (context._feature == context._lastFeature) {
            duk_push_heap_stash(_ctx);

            // duk_idx_t featureIdx = duk_push_heapptr(_ctx, context._objectPtr);
            duk_dup(_ctx, 1);

            if (duk_get_prop(_ctx, -2)) {
                DBG("stashed");
                return 1;
            } else {
                duk_pop(_ctx); // pop undefined
            }
            duk_pop(_ctx); // pop stash
        }
        context._lastFeature = context._feature;
#endif
        //DUMP();

        // Get the property name (second parameter)
        int id = getProperty(context);
        auto& entry = context._propertyCache[id];

        if (entry.val->is<std::string>()) {
            const auto& str = entry.val->get<std::string>();
            if (entry.ptr) {
                DBG("push pointer %p - %s", entry.ptr, str.c_str());
                duk_push_heapptr(_ctx, entry.ptr);
            } else {
                context._lastPushed = str.c_str();

#ifdef STASH_PROPERTIES
                duk_push_heap_stash(_ctx);
#else
                //duk_idx_t featureIdx = duk_push_heapptr(_ctx, context._featurePtr);
               duk_idx_t stringStash = duk_push_heapptr(_ctx, context._stringStashPtr);
#endif
                DBG("push string  %p %s -> %p", str.c_str(), str.c_str(), entry.ptr);

                // TODO check if we get an interned ref!
                duk_push_lstring(_ctx, str.c_str(), str.length());
                entry.ptr = duk_get_heapptr(_ctx, -1);

                DUMPCTX(context);

                // Duplicate to stash string
                duk_dup_top(_ctx);

#ifdef STASH_PROPERTIES
                duk_put_prop_heapptr(_ctx, -3, duk_get_heapptr(_ctx, 1));
#else
                //duk_put_prop_heapptr(_ctx, featureIdx, duk_get_heapptr(_ctx, 1));
                duk_put_prop_index(_ctx, stringStash, id);
#endif

                DUMPCTX(context);
            }
        } else if (entry.val->is<double>()) {
            duk_push_number(_ctx, entry.val->get<double>());
        } else {
            duk_push_undefined(_ctx);
        }
        //DUMP();
        return 1;
    }

    static void fatalErrorHandler(void* udata, const char* message) {
        //Context* context = reinterpret_cast<Context>(udata);

        LOGE("Fatal Error in DuktapeJavaScriptContext: %s", message);
        abort();
    }
};

} // namespace Duktape
} // namespace Tangram
