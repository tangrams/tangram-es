#pragma once

#include "duktape.h"
#include "tileData.h"
#include "scene/drawRule.h"

#include <string>
#include <functional>
#include <set>

namespace Tangram {

class FilterContext {

public:

    struct Stash {
        duk_context *ctx;
    };

    FilterContext();
    ~FilterContext();

    void addAccessor(const std::string& name);
    duk_context *jsContext() { return m_ctx; }

    bool addFilterFn(const std::string& name, const std::string& function);

    bool evalFilterFn(const std::string& name);
    bool evalStyleFn(const std::string& name, StyleParamKey key, StyleParam::Value& value);

    void setFeature(const Feature& feature);

    void setGlobal(const std::string& key, const Value& value);

    void clear();

private:
    static duk_ret_t jsPropertyGetter(duk_context *ctx);
    static duk_ret_t jsPropertySetter(duk_context *ctx);

    duk_context *m_ctx;

    const Feature* m_feature = nullptr;
    bool m_dirty;

    std::set<std::string> m_accessors;
};

}
