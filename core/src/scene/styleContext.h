#pragma once
#include <memory>
#include <string>

#include "scene/styleParam.h"
#include "scene/filters.h"
#include "scene/scene.h"

namespace Tangram {

struct Feature;
class Scene;

using JSFunctionIndex = uint32_t;

class StyleContext {
public:

    StyleContext();
    StyleContext(bool jscore);

    ~StyleContext() = default;

    StyleContext(StyleContext&&) = default;
    StyleContext& operator=(StyleContext&&) = default;

    // Set currently processed Feature
    void setFeature(const Feature& _feature);

    // Set keyword for currently processed Tile
    void setKeywordZoom(int _zoom);

    // Called from Filter::eval
    float getKeywordZoom() const;
    const Value& getKeyword(FilterKeyword _key) const;

    // returns meters per pixels at current style zoom
    float getPixelAreaScale();

    // Called from Filter::eval
    bool evalFilter(JSFunctionIndex idx);

    // Called from DrawRule::eval
    bool evalStyle(JSFunctionIndex idx, StyleParamKey _key, StyleParam::Value& _val);

    // Setup filter and style functions from @_scene
    void initFunctions(const Scene& _scene);

    // Unset Feature handle
    void clear();

    // Set keyword for currently processed Tile
    void setKeyword(const std::string& _key, Value _value);
    const Value& getKeyword(const std::string& _key) const;

    // Set currently processed Feature
    void setCurrentFeature(const Feature* feature);

    // Used by MarkerManager
    bool addFunction(const std::string& _function);

    // Only for testing
    auto& getImpl() { return *(impl.get()); }
    bool setFunctions(const std::vector<std::string>& _functions) ;
    void setSceneGlobals(const YAML::Node& sceneGlobals);

    struct DynamicStyleContext {
        virtual ~DynamicStyleContext() = default;
        virtual void setFeature(const Feature& _feature) = 0;
        virtual void setKeywordZoom(int _zoom) = 0;
        virtual float getKeywordZoom() const = 0;
        virtual const Value& getKeyword(FilterKeyword _key) const = 0;
        virtual bool evalFilter(JSFunctionIndex id) = 0;
        virtual bool evalStyle(JSFunctionIndex id, StyleParamKey _key, StyleParam::Value& _val) = 0;
        virtual void initFunctions(const Scene& _scene) = 0;
        virtual void clear() = 0;
        virtual void setKeyword(const std::string& _key, Value _value) = 0;
        virtual const Value& getKeyword(const std::string& _key) const = 0;
        virtual float getPixelAreaScale() = 0;
        virtual bool addFunction(const std::string& _function) = 0;
        virtual void setSceneGlobals(const YAML::Node& sceneGlobals) = 0;
        virtual bool setFunctions(const std::vector<std::string>& _functions)  = 0;
    };

    std::unique_ptr<DynamicStyleContext> impl;
};

} // namespace Tangram
