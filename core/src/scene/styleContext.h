#pragma once

#include "scene/styleParam.h"
#include "util/fastmap.h"

#include <string>
#include <functional>
#include <memory>
#include <array>
#include <unordered_map>

struct duk_hthread;
typedef struct duk_hthread duk_context;

namespace YAML {
    class Node;
}

namespace Tangram {

class Scene;
struct Feature;
struct StyleParam;

enum class StyleParamKey : uint8_t;
enum class FilterKeyword : uint8_t;


class StyleContext {

public:

    using FunctionID = uint32_t;

    StyleContext();
    ~StyleContext();

    /*
     * Set currently processed Feature
     */
    void setFeature(const Feature& _feature);

    /*
     * Set keyword for currently processed Tile
     */
    void setKeywordZoom(int _zoom);

    /* Called from Filter::eval */
    float getKeywordZoom() const { return m_keywordZoom; }

    const Value& getKeyword(FilterKeyword _key) const {
        return m_keywords[static_cast<uint8_t>(_key)];
    }

    /* Called from Filter::eval */
    bool evalFilter(FunctionID id);

    /* Called from DrawRule::eval */
    bool evalStyle(FunctionID id, StyleParamKey _key, StyleParam::Value& _val);

    /*
     * Setup filter and style functions from @_scene
     */
    void initFunctions(const Scene& _scene);

    /*
     * Unset Feature handle
     */
    void clear();

    bool setFunctions(const std::vector<std::string>& _functions);
    void setSceneGlobals(const std::unordered_map<std::string, YAML::Node>& sceneGlobals);

    void setKeyword(const std::string& _key, Value _value);
    const Value& getKeyword(const std::string& _key) const;

private:
    static int jsGetProperty(duk_context *_ctx);
    static int jsHasProperty(duk_context *_ctx);

    bool evalFunction(FunctionID id);
    void parseStyleResult(StyleParamKey _key, StyleParam::Value& _val) const;
    void parseSceneGlobals(const YAML::Node& node, const std::string& key, int seqIndex, int dukObject);

    std::array<Value, 4> m_keywords;
    int m_keywordGeom= -1;
    int m_keywordZoom = -1;

    int32_t m_sceneId = -1;

    const Feature* m_feature = nullptr;

    mutable duk_context *m_ctx;
};

}
