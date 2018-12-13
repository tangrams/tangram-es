#pragma once

#include "gl/uniform.h"
#include "map.h"
#include "scene/scene.h"

#include <string>
#include <vector>

#include "yaml-cpp/yaml.h"

namespace Tangram {

class Material;
class PointLight;
class SceneLayer;
class Style;
class Texture;
class TileSource;
struct Filter;
struct MaterialTexture;
struct StyleParam;
struct TextureOptions;

// 0: type, 1: values
struct StyleUniform {
    std::string type;
    UniformValue value;
};

struct SceneLoader {
    using Node = YAML::Node;

    static SceneError applyUpdates(Node& config, const std::vector<SceneUpdate>& updates);
    static void applyGlobals(Node& config);

    static void applyScene(const Node& _config, Color& background, Stops& backgroundStops,
                           Scene::animate& animated);

    static void applyCameras(const Node& config, SceneCamera& camera);
    static void loadCameras(const Node& camerasNode, SceneCamera& camera);
    static void loadCamera(const Node& cameraNode, SceneCamera& camera);

    static void applyLights(const Node& config, Scene::Lights& lights);
    static std::unique_ptr<Light> loadLight(const std::pair<Node, Node>& light);
    static void parseLightPosition(const Node& positionNode, PointLight& light);

    static void applyTextures(Scene& scene);
    static void loadTexture(Scene& scene, const std::pair<Node, Node>& texture);
    static bool parseTexFiltering(const Node& filteringNode, TextureOptions& options);

    static void applyFonts(Scene& scene);
    static void loadFontDescription(Scene& scene, const Node& node, const std::string& family);

    static void applySources(Scene& scene);
    static void loadSource(Scene& scene, const std::string& name, const Node& source);
    static void loadSourceRasters(Scene& scene, TileSource& source, const Node& rasterNode);

    static void applyStyles(Scene& scene);
    static bool loadStyle(Scene& scene, const std::string& styleName, const Node& config);
    static void loadStyleProps(Scene& scene, Style& style, const Node& styleNode);
    static void parseStyleParams(SceneStops& _stops, SceneFunctions& _functions,
                                 const Node& params, const std::string& propPrefix,
                                 std::vector<StyleParam>& out);
    static void parseTransition(const Node& params, std::string prefix, std::vector<StyleParam>& out);
    static void loadShaderConfig(Scene& scene, const Node& shaders, Style& style);
    static bool parseStyleUniforms(Scene& scene, const Node& value, StyleUniform& styleUniform);
    static void loadMaterial(Scene& scene, const Node& matNode, Material& material, Style& style);
    static MaterialTexture loadMaterialTexture(Scene& scene, const Node& matCompNode, Style& style);

    static void applyLayers(Scene& scene);
    static void loadLayer(Scene& scene, const std::pair<Node, Node>& layer);
    static SceneLayer loadSublayer(Scene& scene, const Node& layer, const std::string& name);
    static Filter generateFilter(SceneFunctions& functions, const Node& filter);
    static Filter generateAnyFilter(SceneFunctions& functions, const Node& filter);
    static Filter generateAllFilter(SceneFunctions& functions, const Node& filter);
    static Filter generateNoneFilter(SceneFunctions& functions, const Node& filter);
    static Filter generatePredicate(const Node& filter, std::string key);
    static bool getFilterRangeValue(const Node& node, double& val, bool& hasPixelArea);

    SceneLoader() = delete;

};

}
