#pragma once

#include "scene/styleParam.h"
#include "util/fastmap.h"

#include <string>
#include <functional>
#include <memory>
#include <array>

struct duk_hthread;
typedef struct duk_hthread duk_context;

namespace Tangram {

class DataLayer;
class Scene;
struct Feature;
struct StyleParam;
class StyleBuilder;

enum class StyleParamKey : uint8_t;
enum class FilterGlobal : uint8_t;


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
     * Set global for currently processed Tile
     */
    void setGlobalZoom(int _zoom);

    /* Called from Filter::eval */
    float getGlobalZoom() const { return m_globalZoom; }

    const Value& getGlobal(FilterGlobal _key) const;

    /* Called from Filter::eval */
    bool evalFilter(FunctionID id);

    /* Called from DrawRule::eval */
    bool evalStyle(FunctionID id, StyleParamKey _key, StyleParam::Value& _val);

    /*
     * Setup filter and style functions from @_scene
     */
    void setScene(const Scene& _scene);

    /*
     * Unset Feature handle
     */
    void clear();

    // Public for testing
    bool addFunction(const std::string& _name, const std::string& _func);
    bool evalFilterFn(const std::string& _name);
    bool evalStyleFn(const std::string& _name, StyleParamKey _key, StyleParam::Value& _val);
    void addAccessor(const std::string& _name);
    void setGlobal(const std::string& _key, const Value& _value);
    const Value& getGlobal(const std::string& _key) const;

    StyleBuilder* getStyleBuilder(const std::string& _name);

    const auto& styleBuilders() { return m_styleBuilder; }

    const auto& sceneLayers() const { return *m_sceneLayers; };

private:
    static int jsGetProperty(duk_context *_ctx);
    static int jsHasProperty(duk_context *_ctx);

    bool parseStyleResult(StyleParamKey _key, StyleParam::Value& _val) const;

    mutable duk_context *m_ctx;

    const Feature* m_feature = nullptr;

    std::array<Value, 4> m_globals;

    int32_t m_sceneId = -1;

    double m_globalZoom = -1;
    double m_globalGeom = -1;

    fastmap<std::string, std::unique_ptr<StyleBuilder>> m_styleBuilder;

    std::shared_ptr<std::vector<DataLayer>> m_sceneLayers;
};

}
