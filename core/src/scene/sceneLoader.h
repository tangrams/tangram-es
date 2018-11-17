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
class ShaderProgram;
class SpriteAtlas;
class Style;
class Texture;
class TileManager;
class TileSource;
class View;
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

    static void applySources(Scene& scene);
    static void loadSource(const std::string& name, const Node& source, const Node& sources, Scene& scene);
    static void loadSourceRasters(std::shared_ptr<TileSource>& source, const Node& rasterNode, const Node& sources, Scene& scene);

    static void applyStyles(std::shared_ptr<Scene>& scene);
    static bool loadStyle(const std::string& styleName, const Node& config, std::shared_ptr<Scene>& scene);
    static void loadStyleProps(Style& style, const Node& styleNode, std::shared_ptr<Scene>& scene);
    static void parseStyleParams(const Node& params, Scene& scene, const std::string& propPrefix, std::vector<StyleParam>& out);
    static void parseTransition(const Node& params, Scene& scene, std::string _prefix, std::vector<StyleParam>& out);
    static void loadMaterial(const Node& matNode, Material& material, std::shared_ptr<Scene>& scene, Style& style);
    static void loadShaderConfig(const Node& shaders, Style& style, std::shared_ptr<Scene>& scene);
    static bool parseStyleUniforms(const Node& value, std::shared_ptr<Scene>& scene, StyleUniform& styleUniform);
    static MaterialTexture loadMaterialTexture(const Node& matCompNode, std::shared_ptr<Scene>& scene, Style& style);

    static void applyTextures(std::shared_ptr<Scene>& scene);
    static void loadTexture(const std::pair<Node, Node>& texture, std::shared_ptr<Scene>& scene);

    static void applyFonts(std::shared_ptr<Scene>& scene);
    static void loadFont(const std::pair<Node, Node>& font, std::shared_ptr<Scene>& scene);
    static void loadFontDescription(const Node& node, const std::string& family, std::shared_ptr<Scene>& scene);

    static void applyLayers(Scene& scene);
    static void loadLayer(const std::pair<Node, Node>& layer, Scene& scene);
    static SceneLayer loadSublayer(const Node& layer, const std::string& name, Scene& scene);

    static Filter generateFilter(const Node& filter, Scene& scene);
    static Filter generateAnyFilter(const Node& filter, Scene& scene);
    static Filter generateAllFilter(const Node& filter, Scene& scene);
    static Filter generateNoneFilter(const Node& filter, Scene& scene);
    static Filter generatePredicate(const Node& filter, std::string _key);
    static bool getFilterRangeValue(const Node& node, double& val, bool& hasPixelArea);

    static bool parseTexFiltering(const Node& filteringNode, TextureOptions& options);

    SceneLoader() = delete;

};

}
