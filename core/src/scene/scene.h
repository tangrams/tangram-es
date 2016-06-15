#pragma once

#include "util/color.h"
#include "util/fastmap.h"
#include <list>
#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include <unordered_map>

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

class Scene {
public:
    struct Update {
        std::string keys;
        std::string value;
    };

    enum animate {
        yes, no, none
    };

    Scene(const std::string& _path = "");
    Scene(const Scene& _other);
    ~Scene();

    auto& config() { return m_config; }
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
    Style* findStyle(const std::string& _name);

    const auto& path() const { return m_path; }
    const auto& config() const { return m_config; }
    const auto& dataSources() const { return m_dataSources; };
    const auto& layers() const { return m_layers; };
    const auto& styles() const { return m_styles; };
    const auto& lights() const { return m_lights; };
    const auto& functions() const { return m_jsFunctions; };
    const auto& mapProjection() const { return m_mapProjection; };
    const auto& fontContext() const { return m_fontContext; }
    const auto& globals() const { return m_globals; }

    const Style* findStyle(const std::string& _name) const;
    const Light* findLight(const std::string& _name) const;

    int addIdForName(const std::string& _name);
    int getIdForName(const std::string& _name) const;

    const int32_t id;

    glm::dvec2 startPosition = { 0, 0 };
    float startZoom = 0;

    void animated(bool animated) { m_animated = animated ? yes : no; }
    animate animated() const { return m_animated; }

    std::shared_ptr<DataSource> getDataSource(const std::string& name);

    std::shared_ptr<Texture> getTexture(const std::string& name) const;

private:

    // The file path from which this scene was loaded
    std::string m_path;

    // The root node of the YAML scene configuration
    YAML::Node m_config;

    std::unique_ptr<MapProjection> m_mapProjection;
    std::shared_ptr<View> m_view;

    std::vector<DataLayer> m_layers;
    std::vector<std::shared_ptr<DataSource>> m_dataSources;
    std::vector<std::unique_ptr<Style>> m_styles;
    std::vector<std::unique_ptr<Light>> m_lights;
    std::unordered_map<std::string, std::shared_ptr<Texture>> m_textures;
    std::unordered_map<std::string, std::shared_ptr<SpriteAtlas>> m_spriteAtlases;
    std::unordered_map<std::string, YAML::Node> m_globals;

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
