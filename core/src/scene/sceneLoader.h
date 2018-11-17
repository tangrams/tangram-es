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

    static bool loadScene(std::shared_ptr<Scene> _scene);

    static bool applyUpdates(Scene& scene, const std::vector<SceneUpdate>& updates);
    static void applyGlobals(Scene& scene);

    static void applyScene(Scene& scene);
    static void loadBackground(const Node& background, Scene& scene);

    static void applyCameras(Scene& scene);
    static void loadCameras(const Node& cameras, Scene& scene);
    static void loadCamera(const Node& camera, Scene& scene);

    static void applyLights(Scene& scene);
    static void loadLight(const std::pair<Node, Node>& light, Scene& scene);
    static void parseLightPosition(const Node& positionNode, PointLight& light);

    static void applyTextures(Scene& scene);
    static void loadTexture(const std::pair<Node, Node>& texture, Scene& scene);
    static bool parseTexFiltering(const Node& filteringNode, TextureOptions& options);

    static void applyFonts(Scene& scene);
    static void loadFontDescription(const Node& node, const std::string& family, Scene& scene);

    static void applySources(Scene& scene);
    static void loadSource(const std::string& name, const Node& source, Scene& scene);
    static void loadSourceRasters(TileSource& source, const Node& rasterNode, Scene& scene);

    static void applyStyles(Scene& scene);
    static bool loadStyle(const std::string& styleName, const Node& config, Scene& scene);
    static void loadStyleProps(Style& style, const Node& styleNode, Scene& scene);
    static void parseStyleParams(const Node& params, Scene& scene, const std::string& propPrefix,
                                 std::vector<StyleParam>& out);
    static void parseTransition(const Node& params, Scene& scene, std::string _prefix,
                                std::vector<StyleParam>& out);
    static void loadShaderConfig(const Node& shaders, Style& style, Scene& scene);
    static bool parseStyleUniforms(const Node& value, Scene& scene, StyleUniform& styleUniform);
    static void loadMaterial(const Node& matNode, Material& material, Scene& scene, Style& style);
    static MaterialTexture loadMaterialTexture(const Node& matCompNode, Scene& scene, Style& style);

    static void applyLayers(Scene& scene);
    static void loadLayer(const std::pair<Node, Node>& layer, Scene& scene);
    static SceneLayer loadSublayer(const Node& layer, const std::string& name, Scene& scene);
    static Filter generateFilter(const Node& filter, Scene& scene);
    static Filter generateAnyFilter(const Node& filter, Scene& scene);
    static Filter generateAllFilter(const Node& filter, Scene& scene);
    static Filter generateNoneFilter(const Node& filter, Scene& scene);
    static Filter generatePredicate(const Node& filter, std::string _key);
    static bool getFilterRangeValue(const Node& node, double& val, bool& hasPixelArea);

    SceneLoader() = delete;

};

}
