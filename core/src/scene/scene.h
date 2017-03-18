#pragma once

#include "util/color.h"
#include "view/view.h"

#include <atomic>
#include <list>
#include <memory>
#include <string>
#include <vector>
#include <tuple>
#include <unordered_map>

#include "glm/vec2.hpp"
#include "yaml-cpp/yaml.h"
#include "util/yamlHelper.h"


namespace Tangram {

class DataLayer;
class FeatureSelection;
class FontContext;
class Light;
class MapProjection;
class Platform;
class SceneLayer;
class SpriteAtlas;
class Style;
class Texture;
class TileSource;
struct Stops;

// Delimiter used in sceneloader for style params and layer-sublayer naming
const std::string DELIMITER = ":";

/* Singleton container of <Style> information
 *
 * Scene is a singleton containing the styles, lighting, and interactions defining a map scene
 */

class Scene {
public:

    struct Camera {
        CameraType type = CameraType::perspective;

        float maxTilt = 90.f;
        std::shared_ptr<Stops> maxTiltStops;

        // perspective
        glm::vec2 vanishingPoint = {0, 0};
        float fieldOfView = 0.25 * PI;
        std::shared_ptr<Stops> fovStops;

        // isometric
        glm::vec2 obliqueAxis = {0, 1};
    };

    Camera m_camera;

    enum animate {
        yes, no, none
    };

    Scene(std::shared_ptr<const Platform> _platform, const std::string& _path = "");
    Scene(const Scene& _other);
    ~Scene();

    auto& camera() { return m_camera; }

    auto& resourceRoot() { return m_resourceRoot; }
    auto& config() { return m_config; }
    auto& tileSources() { return m_tileSources; };
    auto& layers() { return m_layers; };
    auto& styles() { return m_styles; };
    auto& lights() { return m_lights; };
    auto& lightBlocks() { return m_lightShaderBlocks; };
    auto& textures() { return m_textures; };
    auto& functions() { return m_jsFunctions; };
    auto& spriteAtlases() { return m_spriteAtlases; };
    auto& stops() { return m_stops; }
    auto& background() { return m_background; }
    auto& fontContext() { return m_fontContext; }
    auto& globalRefs() { return m_globalRefs; }
    auto& featureSelection() { return m_featureSelection; }
    Style* findStyle(const std::string& _name);

    const auto& path() const { return m_path; }
    const auto& resourceRoot() const { return m_resourceRoot; }
    const auto& config() const { return m_config; }
    const auto& tileSources() const { return m_tileSources; };
    const auto& layers() const { return m_layers; };
    const auto& styles() const { return m_styles; };
    const auto& lights() const { return m_lights; };
    const auto& lightBlocks() const { return m_lightShaderBlocks; };
    const auto& functions() const { return m_jsFunctions; };
    const auto& mapProjection() const { return m_mapProjection; };
    const auto& fontContext() const { return m_fontContext; }
    const auto& globalRefs() const { return m_globalRefs; }
    const auto& featureSelection() const { return m_featureSelection; }

    const Style* findStyle(const std::string& _name) const;

    const Light* findLight(const std::string& _name) const;

    void updateTime(float _dt) { m_time += _dt; }
    float time() const { return m_time; }

    int addIdForName(const std::string& _name);
    int getIdForName(const std::string& _name) const;

    int addJsFunction(const std::string& _function);

    const int32_t id;

    bool useScenePosition = true;
    glm::dvec2 startPosition = { 0, 0 };
    float startZoom = 0;

    void animated(bool animated) { m_animated = animated ? yes : no; }
    animate animated() const { return m_animated; }

    std::shared_ptr<TileSource> getTileSource(const std::string& name);

    std::shared_ptr<Texture> getTexture(const std::string& name) const;

    float pixelScale() { return m_pixelScale; }
    void setPixelScale(float _scale);

    std::atomic_ushort pendingTextures{0};
    std::atomic_ushort pendingFonts{0};

private:

    // The file path from which this scene was loaded
    std::string m_path;

    std::string m_resourceRoot;

    // The root node of the YAML scene configuration
    YAML::Node m_config;

    std::unique_ptr<MapProjection> m_mapProjection;

    std::vector<DataLayer> m_layers;
    std::vector<std::shared_ptr<TileSource>> m_tileSources;
    std::vector<std::unique_ptr<Style>> m_styles;

    std::vector<std::unique_ptr<Light>> m_lights;
    std::map<std::string, std::string> m_lightShaderBlocks;

    std::unordered_map<std::string, std::shared_ptr<Texture>> m_textures;
    std::unordered_map<std::string, std::shared_ptr<SpriteAtlas>> m_spriteAtlases;

    // Records the YAML Nodes for which global values have been swapped; keys are
    // nodes that referenced globals, values are nodes of globals themselves.
    std::vector<std::pair<YamlPath, YamlPath>> m_globalRefs;

    // Container of all strings used in styling rules; these need to be
    // copied and compared frequently when applying styling, so rules use
    // integer indices into this container to represent strings
    std::vector<std::string> m_names;

    std::vector<std::string> m_jsFunctions;
    std::list<Stops> m_stops;

    Color m_background;

    std::shared_ptr<FontContext> m_fontContext;

    std::unique_ptr<FeatureSelection> m_featureSelection;

    animate m_animated = none;

    float m_pixelScale = 1.0f;

    float m_time = 0.0;

};

}
