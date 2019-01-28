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

    static bool loadScene(Platform& _platform, std::shared_ptr<Scene> _scene,
                          const std::vector<SceneUpdate>& updates = {});

    static bool applyConfig(Platform& platform, const std::shared_ptr<Scene>& scene);
    static bool applyUpdates(Platform& platform, Scene& scene,
                             const std::vector<SceneUpdate>& updates);
    static void applyGlobals(Node root, Scene& scene);

    /*** all public for testing ***/

    static void loadBackground(Node background, const std::shared_ptr<Scene>& scene);
    static void loadSource(Platform& platform, const std::string& name,
                           const Node& source, const Node& sources, const std::shared_ptr<Scene>& scene);
    static void loadSourceRasters(Platform& platform, std::shared_ptr<TileSource>& source, Node rasterNode,
                                  const Node& sources, const std::shared_ptr<Scene>& scene);
    static void loadTexture(Platform& platform, const std::pair<Node, Node>& texture, const std::shared_ptr<Scene>& scene);
    static void loadLayer(const std::pair<Node, Node>& layer, const std::shared_ptr<Scene>& scene);
    static void loadLight(const std::pair<Node, Node>& light, const std::shared_ptr<Scene>& scene);
    static void loadCameras(Node cameras, const std::shared_ptr<Scene>& scene);
    static void loadCamera(const Node& camera, const std::shared_ptr<Scene>& scene);
    static void loadStyleProps(Platform& platform, Style& style, Node styleNode, const std::shared_ptr<Scene>& scene);
    static void loadMaterial(Platform& platform, Node matNode, Material& material, const std::shared_ptr<Scene>& scene, Style& style);
    static void loadShaderConfig(Platform& platform, Node shaders, Style& style, const std::shared_ptr<Scene>& scene);
    static void loadFont(Platform& platform, const std::pair<Node, Node>& font, const std::shared_ptr<Scene>& scene);
    static SceneLayer loadSublayer(const Node& layer, const std::string& name, const std::shared_ptr<Scene>& scene);
    static Filter generateFilter(Node filter, Scene& scene);
    static Filter generateAnyFilter(Node filter, Scene& scene);
    static Filter generateAllFilter(Node filter, Scene& scene);
    static Filter generateNoneFilter(Node filter, Scene& scene);
    static Filter generatePredicate(Node filter, std::string _key);
    static bool getFilterRangeValue(const Node& node, double& val, bool& hasPixelArea);
    /* loads a texture with default texture properties */
    static std::shared_ptr<Texture> getOrLoadTexture(Platform& platform, const std::string& url, const std::shared_ptr<Scene>& scene);
    static std::shared_ptr<Texture> fetchTexture(Platform& platform, const std::string& name, const std::string& url,
                                                 const TextureOptions& options, const std::shared_ptr<Scene>& scene,
                                                 std::unique_ptr<SpriteAtlas> _atlas = nullptr);

    static bool parseTexFiltering(Node& filteringNode, TextureOptions& options);

    static MaterialTexture loadMaterialTexture(Platform& platform, Node matCompNode,
                                               const std::shared_ptr<Scene>& scene, Style& style);

    static void parseStyleParams(Node params, const std::shared_ptr<Scene>& scene, const std::string& propPrefix,
                                 std::vector<StyleParam>& out);
    static void parseTransition(Node params, const std::shared_ptr<Scene>& scene, std::string _prefix, std::vector<StyleParam>& out);

    static bool parseStyleUniforms(Platform& platform, const Node& value,
                                   const std::shared_ptr<Scene>& scene, StyleUniform& styleUniform);

    static void parseLightPosition(Node positionNode, PointLight& light);

    static bool loadStyle(Platform& platform, const std::string& styleName,
                          Node config, const std::shared_ptr<Scene>& scene);

    SceneLoader() = delete;

};

}
