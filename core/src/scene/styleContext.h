#pragma once

#include "duktape.h"
#include "tileData.h"
#include "scene/drawRule.h"
#include "scene/styleParam.h"

#include <string>
#include <functional>
#include <unordered_map>

namespace Tangram {

class Scene;

class StyleContext {

public:

    using FunctionID = uint32_t;

    StyleContext();
    ~StyleContext();

    void addAccessor(const std::string& _name);

    bool addFunction(const std::string& _name, const std::string& _func);

    bool evalFilterFn(const std::string& _name);
    bool evalFilter(FunctionID id) const;

    bool evalStyleFn(const std::string& _name, StyleParamKey _key, StyleParam::Value& _val);
    bool evalStyle(FunctionID id, StyleParamKey _key, StyleParam::Value& _val) const;

    void setFeature(const Feature& _feature);

    void setGlobal(const std::string& _key, const Value& _value);

    const Value& getGlobal(const std::string& _key) const;

    void clear();

    void initFunctions(const Scene& _scene);


private:
    static duk_ret_t jsPropertyGetter(duk_context *_ctx);
    static duk_ret_t jsPropertySetter(duk_context *_ctx);

    bool parseStyleResult(StyleParamKey _key, StyleParam::Value& _val) const;

    mutable duk_context *m_ctx;

    const Feature* m_feature = nullptr;

    struct Accessor {
        std::string key;
        StyleContext* ctx;
    };

    std::unordered_map<std::string, Accessor> m_accessors;

    std::unordered_map<std::string, Value> m_globals;

    int32_t m_sceneId = -1;
};

}
