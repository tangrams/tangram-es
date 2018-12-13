#pragma once

#include "map.h"
#include "marker/markerManager.h"
#include "platform.h"
#include "stops.h"
#include "text/fontContext.h" // For FontDescription
#include "tile/tileManager.h"
#include "util/color.h"
#include "util/url.h"
#include "util/yamlPath.h"
#include "view/view.h"

#include <atomic>
#include <forward_list>
#include <memory>
#include <condition_variable>
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
class FrameBuffer;
class Importer;
class LabelManager;
class Light;
class MapProjection;
class Platform;
class SceneLayer;
class SelectionQuery;
class Style;
class Texture;
class TileSource;
struct SceneLoader;


/* Container of <Style> information
 *
 * Scene is a singleton containing the styles, lighting, and interactions defining a map scene
 */

class SceneOptions {
public:
    explicit SceneOptions(const Url& _url) : url(_url) {}

    explicit SceneOptions(const std::string& _yaml, const Url& _resources)
        : yaml(_yaml), url(_resources) {}

    SceneOptions() {}

    std::string yaml;
    /// The URL from which this scene was loaded
    Url url;
    /// SceneUpdates to apply to the scene
    std::vector<SceneUpdate> updates;
    /// Set the view to the position provided by the scene
    bool useScenePosition = true;
    /// Add styles toggled by DebguFlags
    bool debugStyles = false;

    /// Start loading tiles as soon as possible
    bool prefetchTiles = true;

    /// Start loading tiles as soon as possible
    uint32_t numTileWorkers = 2;

    /// 16MB default in-memory DataSource cache
    static constexpr size_t CACHE_SIZE = 16 * (1024 * 1024);
    size_t memoryTileCacheSize = CACHE_SIZE;

    std::function<void(Scene*)> asyncCallback = nullptr;
};


class Scene {
public:

    enum animate {
        yes, no, none
    };

    explicit Scene(Platform& platform, SceneOptions&& = SceneOptions{""});

    Scene(const Scene& _other) = delete;
    Scene(Scene&& _other) = delete;

    ~Scene();

    const int32_t id;

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

    const auto& config() const { return m_config; }
    const auto& tileSources() const { return m_tileSources; }
    const auto& styles() const { return m_styles; }
    const auto& lights() const { return m_lights; }
    const auto& lightBlocks() const { return m_lightShaderBlocks; }
    const auto& functions() const { return m_jsFunctions; }
    const auto& fontContext() const { return m_fontContext; }
    const auto& globalRefs() const { return m_globalRefs; }
    const auto& featureSelection() const { return m_featureSelection; }

    const Style* findStyle(const std::string& _name) const;

    const Light* findLight(const std::string& _name) const;

    float time() const { return m_time; }

    int addIdForName(const std::string& _name);
    int getIdForName(const std::string& _name) const;

    int addJsFunction(const std::string& _function);

    void animated(bool animated) { m_animated = animated ? yes : no; }
    animate animated() const { return m_animated; }

    std::shared_ptr<TileSource> getTileSource(int32_t id);
    std::shared_ptr<TileSource> getTileSource(const std::string& name);

    std::shared_ptr<Texture> getTexture(const std::string& name) const;

    float pixelScale() { return m_pixelScale; }
    void setPixelScale(float _scale);

    // Returns:
    // - hasLoadingTiles
    // - labelsNeedUpdate
    // - markersNeedUpdate
    std::tuple<bool,bool,bool> update(const View& _view, float _dt);

    void renderBeginFrame(RenderState& _rs);
    bool render(RenderState& _rs, View& _view);
    void renderSelection(RenderState& _rs, View& _view,
                         FrameBuffer& _selectionBuffer,
                         std::vector<SelectionQuery>& _selectionQueries);

    Color background(int _zoom) {
        if (m_backgroundStops.frames.size() > 0) {
            return m_backgroundStops.evalColor(_zoom);
        }
        return m_background;
    }

    TileManager* tileManager() { return m_tileManager.get(); }
    MarkerManager* markerManager() { return m_markerManager.get(); }
    LabelManager* labelManager() { return m_labelManager.get(); }

    bool load();

    const SceneError* errors() const {
        return (m_errors.empty() ? nullptr : &m_errors.front());
    }

    // - Copy current View width,height,pixelscale and position (unless useScenePosition)
    // - Start tile-loading for this View.
    void prefetchTiles(const View& view);

    // Return true scene-loading could be completed, false when resources for
    // tile-building and rendering are still pending.
    // Does the finishing touch when everything is available:
    // - Copy Scene camera to View
    // - Update Styles and FontContext to current pixelScale
    // -...
    bool completeView(View& view);

    void cancelTasks();
    void dispose();

    bool isReady() const { return m_state == State::ready; };

    std::shared_ptr<Texture> fetchTexture(const std::string& name, const Url& url,
                                          const TextureOptions& options,
                                          std::unique_ptr<SpriteAtlas> _atlas = nullptr);

    std::shared_ptr<Texture> loadTexture(const std::string& name);

    void loadFont(const std::string& _uri, const std::string& _family,
                  const std::string& _style, const std::string& _weight);

    friend struct SceneLoader;
    friend class Importer;

    const SceneOptions& options() const { return m_options; }

protected:
    Platform& platform() { return m_platform; }
    void pushError(SceneError&& error) { m_errors.push_back(std::move(error)); }
    auto& startPosition() { return m_startPosition; }

private:
    Platform& m_platform;

    SceneOptions m_options;
    std::unique_ptr<Importer> m_importer;

    // Only SceneUpdate errors for now
    std::vector<SceneError> m_errors;

    enum class State {
        initial,
        loading,             // set at start of Scene::load()
        pending_resources,   // set while waiting for (async) resource loading tasks
        pending_completion,  // set end of Scene::load()
        ready,               // set on main thread when Scene::complete() succeeded
        canceled,            // should stop any scene- or tile-loading tasks
        disposed
    } m_state = State::initial;

    // ---------------------------------------------------------------//
    // Loaded Scene Data

    // The root node of the YAML scene configuration
    YAML::Node m_config;

    Camera m_camera;
    glm::dvec3 m_startPosition;

    std::vector<DataLayer> m_layers;
    std::vector<std::shared_ptr<TileSource>> m_tileSources;
    std::vector<std::unique_ptr<Style>> m_styles;

    std::vector<std::unique_ptr<Light>> m_lights;
    std::map<std::string, std::string> m_lightShaderBlocks;

    std::unordered_map<std::string, std::shared_ptr<Texture>> m_textures;

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
    animate m_animated = none;

    // ---------------------------------------------------------------//
    // Runtime Data

    float m_pixelScale = 1.0f;
    float m_time = 0.0;

    std::shared_ptr<FontContext> m_fontContext;
    std::unique_ptr<FeatureSelection> m_featureSelection;
    std::unique_ptr<TileWorker> m_tileWorker;
    std::unique_ptr<TileManager> m_tileManager;
    std::unique_ptr<MarkerManager> m_markerManager;
    std::unique_ptr<LabelManager> m_labelManager;

    struct FontTask {
        FontTask(std::condition_variable& condition, Url url, std::shared_ptr<FontContext> fontContext, FontDescription ft)
            : condition(condition), url(url), ft(ft), fontContext(fontContext) {}
        std::condition_variable& condition;
        Url url;
        FontDescription ft;
        std::shared_ptr<FontContext> fontContext;
        bool done = false;
        UrlCallback cb = nullptr;
        UrlRequestHandle requestHandle = 0;
    };
    struct TextureTask {
        TextureTask(std::condition_variable& condition, Url url, std::shared_ptr<Texture> texture)
            : condition(condition), url(url), texture(texture) {}
        std::condition_variable& condition;
        Url url;
        std::shared_ptr<Texture> texture;
        bool done = false;
        UrlCallback cb = nullptr;
        UrlRequestHandle requestHandle = 0;
    };


    std::mutex m_taskMutex;
    std::condition_variable m_taskCondition;

    std::forward_list<std::shared_ptr<FontTask>> m_pendingFonts;
    std::forward_list<std::shared_ptr<TextureTask>> m_pendingTextures;
};

}
