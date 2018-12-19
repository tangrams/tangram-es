#pragma once

#include "map.h"
#include "platform.h"
#include "stops.h"
#include "sceneOptions.h"
#include "text/fontContext.h" // For FontDescription
#include "tile/tileManager.h"
#include "util/color.h"
#include "util/url.h"
#include "util/yamlPath.h"
#include "view/view.h"

#include <atomic>
#include <forward_list>
#include <functional>
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
class MarkerManager;
class Platform;
class SceneLayer;
class SelectionQuery;
class Style;
class Texture;
class TileSource;
struct SceneLoader;

struct SceneCamera : public Camera {
    glm::dvec3 startPosition;
};

struct SceneFunctions : public std::vector<std::string> {
    int add(const std::string& _function);
};

using SceneStops = std::list<Stops>;

struct DrawRuleNames : std::vector<std::string> {
    int add(const std::string& _name) {
        int id = getId(_name);
        if (id < 0) {
            push_back(_name);
            return size() - 1;
        }
        return id;
    }
    int getId(const std::string& _name) const {
        auto it = std::find(begin(), end(), _name);
        if (it == end()) {
            return -1;
        }
        return it - begin();
    }
};

struct SceneTextures {
    struct Task {
        Task(Url url, std::shared_ptr<Texture> texture) : url(url), texture(texture) {}
        bool started = false;
        bool done = false;
        Url url;
        std::shared_ptr<Texture> texture;
        UrlRequestHandle requestHandle = 0;
    };

    std::unordered_map<std::string, std::shared_ptr<Texture>> textures;

    std::forward_list<std::shared_ptr<Task>> tasks;

    std::shared_ptr<Texture> add(const std::string& name, const Url& url,
                                 const TextureOptions& options);

    std::shared_ptr<Texture> get(const std::string& name);
};

struct SceneFonts {
    struct Task {
        Task(Url url, FontDescription ft) : url(url), ft(ft) {}
        bool started = false;
        bool done = false;
        Url url;
        FontDescription ft;
        UrlRequestHandle requestHandle = 0;
        UrlResponse response;
    };
    std::forward_list<std::shared_ptr<Task>> tasks;

    void add(const std::string& _uri, const std::string& _family,
             const std::string& _style, const std::string& _weight);
};

class Scene {
public:
    enum animate { yes, no, none };

    Scene(Platform& platform, SceneOptions&& = SceneOptions{""});
    ~Scene();

    Scene(const Scene& _other) = delete;
    Scene(Scene&& _other) = delete;

    /// Load the whole Scene
    bool load();

    auto& tileSources() const { return m_tileSources; }
    auto& featureSelection() const { return m_featureSelection; }
    auto& fontContext() const { return m_fontContext; }

    const auto& config() const { return m_config; }
    const auto& functions() const { return m_jsFunctions; }
    const auto& layers() const { return m_layers; }
    const auto& lightBlocks() const { return m_lightShaderBlocks; }
    const auto& lights() const { return m_lights; }
    const auto& options() const { return m_options; }
    const auto& styles() const { return m_styles; }
    const auto& textures() const { return m_textures.textures; }

    std::shared_ptr<TileSource> getTileSource(int32_t id) const;
    std::shared_ptr<Texture> getTexture(const std::string& name) const;

    float time() const { return m_time; }
    animate animated() const { return m_animated; }

    float pixelScale() const { return m_pixelScale; }
    void setPixelScale(float _scale);

    /// Update TileManager, Labels and Markers for current View, returns:
    /// - hasLoadingTiles
    /// - labelsNeedUpdate
    /// - markersNeedUpdate
    std::tuple<bool,bool,bool> update(const View& _view, float _dt);

    void renderBeginFrame(RenderState& _rs);
    bool render(RenderState& _rs, View& _view);
    void renderSelection(RenderState& _rs, View& _view,
                         FrameBuffer& _selectionBuffer,
                         std::vector<SelectionQuery>& _selectionQueries);

    Color backgroundColor(int _zoom) const;

    /// Used for FrameInfo debug
    TileManager* tileManager() const { return m_tileManager.get(); }

    MarkerManager* markerManager() const { return m_markerManager.get(); }

    const SceneError* errors() const {
        return (m_errors.empty() ? nullptr : &m_errors.front());
    }

    /// When Scene is async loading this function can be run from
    /// main-thread to start loading tiles for the current view.
    /// - Copy current View width,height,pixelscale and position
    ///   (unless options.useScenePosition is set)
    /// - Start tile-loading for this View.
    void prefetchTiles(const View& view);

    /// Returns true when scene-loading could be completed,
    /// false when resources for tile-building and rendering
    /// are still pending.
    /// Does the finishing touch when everything is available:
    /// - Sets Scene camera to View
    /// - Update Styles and FontContext to current pixelScale
    /// - Sets platform continuousRendering mode
    bool completeScene(View& view);

    /// Cancel scene loading and all TileManager tasks
    void cancelTasks();

    /// Cancel all scene tasks and wait for TileWorker thread to join
    void dispose();

    /// Returns true when scene finished loading and completeScene() suceeded.
    bool isReady() const { return m_state == State::ready; };
    bool isPendingCompletion() const { return m_state == State::pending_completion; };

    /// Scene ID
    const int32_t id;

    friend struct SceneLoader;
    friend class Importer;
    friend class Map;

    using Lights = std::vector<std::unique_ptr<Light>>;
    using LightShaderBlocks = std::map<std::string, std::string>;
    using TileSources = std::vector<std::shared_ptr<TileSource>>;
    using Styles = std::vector<std::unique_ptr<Style>>;
    using Layers = std::vector<DataLayer>;

protected:

    Platform& platform() { return m_platform; }
    void pushError(SceneError&& error) { m_errors.push_back(std::move(error)); }

    Platform& m_platform;

    SceneOptions m_options;
    std::unique_ptr<Importer> m_importer;

    /// Only SceneUpdate errors for now
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

    /// ---------------------------------------------------------------///
    /// Loaded Scene Data
    /// The root node of the YAML scene configuration
    YAML::Node m_config;

    SceneCamera m_camera;

    Layers m_layers;
    TileSources m_tileSources;
    Styles m_styles;

    Lights m_lights;
    LightShaderBlocks m_lightShaderBlocks;

    void runTextureTasks();
    SceneTextures m_textures;

    void runFontTasks();
    SceneFonts m_fonts;

    /// Container of all strings used in styling rules; these need to be
    /// copied and compared frequently when applying styling, so rules use
    /// integer indices into this container to represent strings
    DrawRuleNames m_names;

    SceneFunctions m_jsFunctions;
    SceneStops m_stops;

    Color m_background;
    Stops m_backgroundStops;
    animate m_animated = none;

    /// ---------------------------------------------------------------///
    /// Runtime Data
    float m_pixelScale = 1.0f;
    float m_time = 0.0;

    /// Set true when all resources for TileBuilder are available
    bool m_readyToBuildTiles = false;

    std::unique_ptr<FontContext> m_fontContext;
    std::unique_ptr<FeatureSelection> m_featureSelection;
    std::unique_ptr<TileWorker> m_tileWorker;
    std::unique_ptr<TileManager> m_tileManager;
    std::unique_ptr<MarkerManager> m_markerManager;
    std::unique_ptr<LabelManager> m_labelManager;

    std::mutex m_taskMutex;
    std::condition_variable m_taskCondition;
};

}
