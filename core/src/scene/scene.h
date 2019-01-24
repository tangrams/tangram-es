#pragma once

#include "map.h"
#include "platform.h"
#include "stops.h"
#include "util/color.h"
#include "util/url.h"
#include "util/yamlPath.h"
#include "view/view.h"

#include <atomic>
#include <list>
#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include <tuple>
#include <unordered_map>

#include "glm/vec2.hpp"
#include "yaml-cpp/yaml.h"

namespace Tangram {

class DataLayer;
class FeatureSelection;
class FontContext;
class Light;
class MapProjection;
class Platform;
class SceneLayer;
class Style;
class Texture;
class TileSource;
class ZipArchive;

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

    Scene();
    Scene(Platform& _platform, const Url& _url);
    Scene(Platform& _platform, const std::string& _yaml, const Url& _url);
    Scene(const Scene& _other) = delete;

    ~Scene();

    const int32_t id;

    void copyConfig(const Scene& _other);

    auto& camera() { return m_camera; }
    auto& config() { return m_config; }
    auto& tileSources() { return m_tileSources; }
    auto& layers() { return m_layers; }
    auto& styles() { return m_styles; }
    auto& lights() { return m_lights; }
    auto& lightBlocks() { return m_lightShaderBlocks; }
    auto& textures() { return m_textures; }
    auto& functions() { return m_jsFunctions; }
    auto& stops() { return m_stops; }
    auto& background() { return m_background; }
    auto& backgroundStops() { return m_backgroundStops; }
    auto& fontContext() { return m_fontContext; }
    auto& globalRefs() { return m_globalRefs; }
    auto& featureSelection() { return m_featureSelection; }
    Style* findStyle(const std::string& _name);

    const auto& url() const { return m_url; }
    const auto& yaml() { return m_yaml; }
    const auto& config() const { return m_config; }
    const auto& tileSources() const { return m_tileSources; }
    const auto& layers() const { return m_layers; }
    const auto& styles() const { return m_styles; }
    const auto& lights() const { return m_lights; }
    const auto& lightBlocks() const { return m_lightShaderBlocks; }
    const auto& functions() const { return m_jsFunctions; }
    const auto& fontContext() const { return m_fontContext; }
    const auto& globalRefs() const { return m_globalRefs; }
    const auto& featureSelection() const { return m_featureSelection; }

    const Style* findStyle(const std::string& _name) const;

    const Light* findLight(const std::string& _name) const;

    // Start an asynchronous request for the scene resource at the given URL.
    // In addition to the URL types supported by the platform instance, this
    // also supports a custom ZIP URL scheme. ZIP URLs are of the form:
    //   zip://path/to/file.txt#http://host.com/some/archive.zip
    // The fragment (#http...) of the URL is the location of the archive and the
    // relative portion of the URL (path/...) is the path of the target file
    // within the archive (this allows relative path operations on URLs to work
    // as expected within zip archives). This function expects that all required
    // zip archives will be added to the scene with addZipArchive before being
    // requested.
    UrlRequestHandle startUrlRequest(Platform& platform, Url url, UrlCallback callback);

    void addZipArchive(Url url, std::shared_ptr<ZipArchive> zipArchive);

    void updateTime(float _dt) { m_time += _dt; }
    float time() const { return m_time; }

    int addIdForName(const std::string& _name);
    int getIdForName(const std::string& _name) const;

    int addJsFunction(const std::string& _function);

    bool useScenePosition = true;
    glm::dvec2 startPosition = { 0, 0 };
    float startZoom = 0;

    void animated(bool animated) { m_animated = animated ? yes : no; }
    animate animated() const { return m_animated; }

    std::shared_ptr<TileSource> getTileSource(int32_t id);
    std::shared_ptr<TileSource> getTileSource(const std::string& name);

    std::shared_ptr<Texture> getTexture(const std::string& name) const;

    float pixelScale() { return m_pixelScale; }
    void setPixelScale(float _scale);

    std::atomic_ushort pendingTextures{0};
    std::atomic_ushort pendingFonts{0};

    std::vector<SceneError> errors;

private:

    // The URL from which this scene was loaded
    Url m_url;

    std::string m_yaml;

    // The root node of the YAML scene configuration
    YAML::Node m_config;

    std::vector<DataLayer> m_layers;
    std::vector<std::shared_ptr<TileSource>> m_tileSources;
    std::vector<std::unique_ptr<Style>> m_styles;

    std::vector<std::unique_ptr<Light>> m_lights;
    std::map<std::string, std::string> m_lightShaderBlocks;

    std::unordered_map<std::string, std::shared_ptr<Texture>> m_textures;

    // Container for any zip archives needed for the scene. For each entry, the
    // key is the original URL from which the zip archive was retrieved and the
    // value is a ZipArchive initialized with the compressed archive data.
    std::unordered_map<Url, std::shared_ptr<ZipArchive>> m_zipArchives;

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
    Stops m_backgroundStops;

    std::shared_ptr<FontContext> m_fontContext;

    std::unique_ptr<FeatureSelection> m_featureSelection;

    animate m_animated = none;

    float m_pixelScale = 1.0f;

    float m_time = 0.0;

};

}
