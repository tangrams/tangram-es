#pragma once

#include "gl/uniform.h"
#include "map.h"
#include "scene/scene.h"

#include <memory>
#include <string>
#include <tuple>
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

    static bool loadScene(std::shared_ptr<Scene> scene);

    static bool applyUpdates(Scene& scene, const std::vector<SceneUpdate>& updates);
    static void applyGlobals(Scene& scene);

    static void applyScene(Scene& scene);
    static void loadBackground(Scene& scene, const Node& background);

    static void applyCameras(Scene& scene);
    static void loadCameras(Scene& scene, const Node& cameras);
    static void loadCamera(Scene& scene, const Node& camera);

    static void applyLights(Scene& scene);
    static void loadLight(Scene& scene, const std::pair<Node, Node>& light);
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
    static void parseStyleParams(Scene& scene, const Node& params, const std::string& propPrefix,
                                 std::vector<StyleParam>& out);
    static void parseTransition(Scene& scene, const Node& params, std::string prefix,
                                std::vector<StyleParam>& out);
    static void loadShaderConfig(Scene& scene, const Node& shaders, Style& style);
    static bool parseStyleUniforms(Scene& scene, const Node& value, StyleUniform& styleUniform);
    static void loadMaterial(Scene& scene, const Node& matNode, Material& material, Style& style);
    static MaterialTexture loadMaterialTexture(Scene& scene, const Node& matCompNode, Style& style);

    static void applyLayers(Scene& scene);
    static void loadLayer(Scene& scene, const std::pair<Node, Node>& layer);
    static SceneLayer loadSublayer(Scene& scene, const Node& layer, const std::string& name);
    static Filter generateFilter(Scene& scene, const Node& filter);
    static Filter generateAnyFilter(Scene& scene, const Node& filter);
    static Filter generateAllFilter(Scene& scene, const Node& filter);
    static Filter generateNoneFilter(Scene& scene, const Node& filter);
    static Filter generatePredicate(const Node& filter, std::string key);
    static bool getFilterRangeValue(const Node& node, double& val, bool& hasPixelArea);

    SceneLoader() = delete;

};

}
