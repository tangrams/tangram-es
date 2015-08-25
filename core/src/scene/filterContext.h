#pragma once

#include "duktape.h"
#include "tileData.h"
#include "scene/drawRule.h"

#include <string>
#include <functional>
#include <unordered_set>

namespace Tangram {

class Scene;

class FilterContext {

public:

    using FunctionID = uint32_t;

    FilterContext();
    ~FilterContext();

    void addAccessor(const std::string& _name);

    bool addFilterFn(const std::string& _name, const std::string& _func);

    bool evalFilterFn(const std::string& _name);
    bool evalFilter(FunctionID id) const;

    bool evalStyleFn(const std::string& _name, StyleParamKey _key, StyleParam::Value& _val);
    bool evalStyle(FunctionID id, StyleParamKey _key, StyleParam::Value& _val) const;

    void setFeature(const Feature& _feature);

    void setGlobal(const std::string& _key, const Value& _value);

    void clear();

    void initFunctions(const Scene& _scene);

    std::unordered_map<std::string, Value> globals;

private:
    static duk_ret_t jsPropertyGetter(duk_context *_ctx);
    static duk_ret_t jsPropertySetter(duk_context *_ctx);

    bool parseStyleResult(StyleParamKey _key, StyleParam::Value& _val) const;

    mutable duk_context *m_ctx;

    const Feature* m_feature = nullptr;

    std::unordered_set<std::string> m_accessors;

    int32_t m_sceneId = -1;
};

}
