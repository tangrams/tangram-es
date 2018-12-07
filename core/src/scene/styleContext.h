#pragma once
#include <memory>
#include <string>

#include "scene/styleParam.h"
#include "scene/filters.h"
#include "scene/scene.h"

namespace Tangram {

struct Feature;

using JSFunctionIndex = uint32_t;
using JSScopeMarker = int32_t;

class StyleContext {
public:

    StyleContext();
    ~StyleContext();

    StyleContext(StyleContext&&) = default;
    StyleContext& operator=(StyleContext&&) = default;

    // Set currently processed Feature
    void setFeature(const Feature& _feature);

    // Set keyword for currently processed Tile
    void setFilterKey(Filter::Key _key, int _value);

    // Called from Filter::eval and used by JS functions: $zoom
    float getZoomLevel() const { return m_zoomLevel; }
    int getFilterKey(Filter::Key _key) const;

    // Returns meters per pixels at current style zoom
    float getPixelAreaScale() const { return m_pixelAreaScale; }

    // Called from Filter::eval
    bool evalFilter(JSFunctionIndex idx);

    // Called from DrawRule::eval
    bool evalStyle(JSFunctionIndex idx, StyleParamKey _key, StyleParam::Value& _val);

    // Setup filter and style functions from @_scene
    void initScene(const Scene& _scene);

    // Unset Feature handle
    void clear();

    // Set currently processed Feature
    void setCurrentFeature(const Feature* feature);

    // Used by MarkerManager
    bool addFunction(const std::string& _function);

    // Only for testing
    StyleContext(bool jscore, bool record = false);
    bool setFunctions(const std::vector<std::string>& _functions) ;
    void setSceneGlobals(const YAML::Node& sceneGlobals);

    struct StyleContextImpl {
        virtual ~StyleContextImpl() = default;
        virtual void setFeature(const Feature& _feature) = 0;
        virtual void setFilterKey(Filter::Key _key, int _value) = 0;
        virtual bool evalFilter(JSFunctionIndex id) = 0;
        virtual bool evalStyle(JSFunctionIndex id, StyleParamKey _key, StyleParam::Value& _val) = 0;
        virtual void initScene(const Scene& _scene) = 0;
        virtual void clear() = 0;
        virtual bool addFunction(const std::string& _function) = 0;
        virtual void setSceneGlobals(const YAML::Node& sceneGlobals) = 0;
        virtual bool setFunctions(const std::vector<std::string>& _functions)  = 0;
        // Recorder
        virtual void recorderLog() {}
        virtual void replayFilters() {}
        virtual void replayStyles() {}
    };

    std::unique_ptr<StyleContextImpl> impl;

    std::array<int, 4> m_filterKeys {};
    float m_zoomLevel = 0;
    float m_pixelAreaScale = 0;
};

} // namespace Tangram
