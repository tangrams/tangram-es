#pragma once

#include "util/color.h"
#include <list>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

#include "util/fastmap.h"
#include "glm/vec2.hpp"
#include "yaml-cpp/yaml.h"

namespace Tangram {

class Style;
class Texture;
class DataSource;
class DataLayer;
class FontContext;
class Light;
class MapProjection;
class SpriteAtlas;
class View;
struct Stops;

/* Singleton container of <Style> information
 *
 * Scene is a singleton containing the styles, lighting, and interactions defining a map scene
 */

enum class StyleComponent {
    scene,
    global,
    cameras,
    lights,
    textures,
    styles,
    sources,
    layers
};

#define COMPONENT_PATH_DELIMITER '.'

using StyleComponents = fastmap<std::string, std::string>;

class Scene {
public:
    Scene(std::string path);
    Scene(std::string path, std::map<StyleComponent, StyleComponents> userDefined);
    ~Scene();

    auto& view() { return m_view; }
    auto& dataSources() { return m_dataSources; };
    auto& layers() { return m_layers; };
    auto& styles() { return m_styles; };
    auto& lights() { return m_lights; };
    auto& textures() { return m_textures; };
    auto& functions() { return m_jsFunctions; };
    auto& spriteAtlases() { return m_spriteAtlases; };
    auto& stops() { return m_stops; }
    auto& background() { return m_background; }
    auto& fontContext() { return m_fontContext; }
    auto& globals() { return m_globals; }

    const auto& dataSources() const { return m_dataSources; };
    const auto& layers() const { return m_layers; };
    const auto& styles() const { return m_styles; };
    const auto& lights() const { return m_lights; };
    const auto& functions() const { return m_jsFunctions; };
    const auto& mapProjection() const { return m_mapProjection; };
    const auto& fontContext() const { return m_fontContext; }

    const Style* findStyle(const std::string& _name) const;
    const Light* findLight(const std::string& _name) const;

    int addIdForName(const std::string& _name);
    int getIdForName(const std::string& _name) const;

    bool texture(const std::string& textureName, std::shared_ptr<Texture>& texture) const;

    const int32_t id;

    glm::dvec2 startPosition = { 0, 0 };
    float startZoom = 0;

    enum animate {
        yes, no, none
    };

    void animated(bool animated) { m_animated = animated ? yes : no; }
    animate animated() const { return m_animated; }

    void setComponent(std::string componentName, std::string value);

    bool getComponentValue(StyleComponent component, const std::string& componentPath, std::string& value);

    std::string path() const { return m_path; }

    const std::map<StyleComponent, StyleComponents>& userDefines() const { return m_userDefinedValues; }

    void clearUserDefines() { m_userDefinedValues.clear(); }

private:

    std::unique_ptr<MapProjection> m_mapProjection;
    std::shared_ptr<View> m_view;

    std::vector<DataLayer> m_layers;
    std::vector<std::shared_ptr<DataSource>> m_dataSources;
    std::vector<std::unique_ptr<Style>> m_styles;
    std::vector<std::unique_ptr<Light>> m_lights;
    std::unordered_map<std::string, std::shared_ptr<Texture>> m_textures;
    std::unordered_map<std::string, std::shared_ptr<SpriteAtlas>> m_spriteAtlases;
    std::unordered_map<std::string, YAML::Node> m_globals;

    std::map<StyleComponent, StyleComponents> m_userDefinedValues;

    std::string m_path;

    // Container of all strings used in styling rules; these need to be
    // copied and compared frequently when applying styling, so rules use
    // integer indices into this container to represent strings
    std::vector<std::string> m_names;

    std::vector<std::string> m_jsFunctions;
    std::list<Stops> m_stops;

    Color m_background;

    std::shared_ptr<FontContext> m_fontContext;

    animate m_animated = none;
};

}
