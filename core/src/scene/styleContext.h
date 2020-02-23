#pragma once

#include "js/JavaScriptFwd.h"
#include "scene/styleParam.h"

#include <array>
#include <memory>
#include <string>

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

    /// Set current Feature being evaluated.
    void setFeature(const Feature& feature);

    /// Set current zoom level being evaluated.
    void setZoom(double zoom);

    double getZoom() const {
        return m_zoom;
    }

    /// Squared meters per pixels at current zoom.
    double getPixelAreaScale();

    const Value& getKeyword(FilterKeyword keyword) const {
        return m_keywordValues[static_cast<uint8_t>(keyword)];
    }

    /// Called from Filter::eval
    bool evalFilter(FunctionID id);

    /// Called from DrawRule::eval
    bool evalStyle(FunctionID id, StyleParamKey key, StyleParam::Value& value);

    /// Setup filter and style functions from a Scene.
    void initFunctions(const Scene& scene);

    /// Unset the current Feature.
    void clear();

    bool setFunctions(const std::vector<std::string>& functions);
    bool addFunction(const std::string& function);
    void setSceneGlobals(const YAML::Node& sceneGlobals);

private:

    void setKeyword(FilterKeyword keyword, Value value);

    std::array<Value, 4> m_keywordValues;

    // Cache zoom separately from keywords for easier access.
    double m_zoom = -1;

    // Geometry keyword is accessed as a string, but internally cached as an int.
    int m_keywordGeometry = -1;

    int m_functionCount = 0;

    int32_t m_sceneId = -1;

    const Feature* m_feature = nullptr;

    std::unique_ptr<JSContext> m_jsContext;
};

}
