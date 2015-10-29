#pragma once

#include "scene/styleParam.h"
#include "util/fastmap.h"

#include <string>
#include <functional>
#include <memory>

struct duk_hthread;
typedef struct duk_hthread duk_context;

namespace Tangram {

class Scene;
enum class StyleParamKey : uint8_t;
struct Feature;
struct StyleParam;

class StyleContext {

public:

    using FunctionID = uint32_t;

    StyleContext();
    ~StyleContext();

    bool addFunction(const std::string& _name, const std::string& _func);

    bool evalFilterFn(const std::string& _name);
    bool evalFilter(FunctionID id);

    bool evalStyleFn(const std::string& _name, StyleParamKey _key, StyleParam::Value& _val);
    bool evalStyle(FunctionID id, StyleParamKey _key, StyleParam::Value& _val);

    void setFeature(const Feature& _feature);

    void setGlobal(const std::string& _key, const Value& _value);
    void setGlobalZoom(float _zoom);

    const Value& getGlobal(const std::string& _key) const;
    float getGlobalZoom() const { return m_globalZoom; }

    void clear();

    void initFunctions(const Scene& _scene);


private:
    static int jsPropertyGetter(duk_context *_ctx);
    static int jsPropertySetter(duk_context *_ctx);

    bool parseStyleResult(StyleParamKey _key, StyleParam::Value& _val) const;

    void setAccessors();
    void addAccessor(const std::string& _name);

    mutable duk_context *m_ctx;

    const Feature* m_feature = nullptr;
    bool m_featureIsReady;

    struct Accessor {
        std::string key;
        StyleContext* ctx;
    };

    fastmap<std::string, std::unique_ptr<Accessor>> m_accessors;
    fastmap<std::string, Value> m_globals;

    int32_t m_sceneId = -1;

    float m_globalZoom = -1;
};

}
