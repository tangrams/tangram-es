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
    explicit StyleContext(bool jscore);

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
    auto& getImpl() { return *(impl.get()); }
    bool setFunctions(const std::vector<std::string>& _functions) ;
    void setSceneGlobals(const YAML::Node& sceneGlobals);

    struct StyleContextImpl;
    std::unique_ptr<StyleContextImpl> impl;

    std::array<int, 4> m_filterKeys {};
    float m_zoomLevel = 0;
    float m_pixelAreaScale = 0;
};

} // namespace Tangram
