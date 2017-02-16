#pragma once

#include "gl/uniform.h"
#include "scene/scene.h"
#include "tangram.h"

#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>

#include "yaml-cpp/yaml.h"
#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "glm/vec4.hpp"

namespace Tangram {

class Material;
class PointLight;
class SceneLayer;
class ShaderProgram;
class Style;
class TileManager;
class TileSource;
class View;
struct Filter;
struct MaterialTexture;
struct StyleParam;
struct TextureFiltering;
struct TextureOptions;

// 0: type, 1: values
struct StyleUniform {
    std::string type;
    UniformValue value;
};

struct SceneLoader {
    using Node = YAML::Node;

    static bool loadScene(const std::shared_ptr<Platform>& _platform, std::shared_ptr<Scene> _scene, const std::vector<SceneUpdate>& updates = {});
    static bool applyConfig(const std::shared_ptr<Platform>& platform, const std::shared_ptr<Scene>& scene);
    static void applyUpdates(Scene& scene, const std::vector<SceneUpdate>& updates);
    static void applyGlobals(Node root, Scene& scene);

    /*** all public for testing ***/

    static void loadBackground(Node background, const std::shared_ptr<Scene>& scene);
    static void loadSource(const std::shared_ptr<Platform>& platform, const std::string& name,
                           const Node& source, const Node& sources, const std::shared_ptr<Scene>& scene);
    static void loadSourceRasters(const std::shared_ptr<Platform>& platform, std::shared_ptr<TileSource>& source, Node rasterNode,
                                  const Node& sources, const std::shared_ptr<Scene>& scene);
    static void loadTexture(const std::shared_ptr<Platform>& platform, const std::pair<Node, Node>& texture, const std::shared_ptr<Scene>& scene);
    static void loadLayer(const std::pair<Node, Node>& layer, const std::shared_ptr<Scene>& scene);
    static void loadLight(const std::pair<Node, Node>& light, const std::shared_ptr<Scene>& scene);
    static void loadCameras(Node cameras, const std::shared_ptr<Scene>& scene);
    static void loadCamera(const Node& camera, const std::shared_ptr<Scene>& scene);
    static void loadStyleProps(const std::shared_ptr<Platform>& platform, Style& style, Node styleNode, const std::shared_ptr<Scene>& scene);
    static void loadMaterial(const std::shared_ptr<Platform>& platform, Node matNode, Material& material, const std::shared_ptr<Scene>& scene, Style& style);
    static void loadShaderConfig(const std::shared_ptr<Platform>& platform, Node shaders, Style& style, const std::shared_ptr<Scene>& scene);
    static void loadFont(const std::shared_ptr<Platform>& platform, const std::pair<Node, Node>& font, const std::shared_ptr<Scene>& scene);
    static SceneLayer loadSublayer(Node layer, const std::string& name, const std::shared_ptr<Scene>& scene);
    static Filter generateFilter(Node filter, Scene& scene);
    static Filter generateAnyFilter(Node filter, Scene& scene);
    static Filter generateAllFilter(Node filter, Scene& scene);
    static Filter generateNoneFilter(Node filter, Scene& scene);
    static Filter generatePredicate(Node filter, std::string _key);
    static bool getFilterRangeValue(const Node& node, double& val, bool& hasPixelArea);
    /* loads a texture with default texture properties */
    static bool loadTexture(const std::shared_ptr<Platform>& platform, const std::string& url, const std::shared_ptr<Scene>& scene);
    static std::shared_ptr<Texture> fetchTexture(const std::shared_ptr<Platform>& platform, const std::string& name, const std::string& url,
            const TextureOptions& options, bool generateMipmaps, const std::shared_ptr<Scene>& scene);
    static bool extractTexFiltering(Node& filtering, TextureFiltering& filter);

    /*
     * Sprite nodes are created using a default 1x1 black texture when sprite atlas is requested over the network.
     * Once a sprite atlas has been fetched, sprite nodes need to be updated according to the width/height of the
     * fetched sprite atlas.
     */
    static void updateSpriteNodes(const std::string& texName,
            std::shared_ptr<Texture>& texture, const std::shared_ptr<Scene>& scene);

    static MaterialTexture loadMaterialTexture(const std::shared_ptr<Platform>& platform, Node matCompNode,
                                               const std::shared_ptr<Scene>& scene, Style& style);

    static void parseStyleParams(Node params, const std::shared_ptr<Scene>& scene, const std::string& propPrefix,
                                 std::vector<StyleParam>& out);
    static void parseTransition(Node params, const std::shared_ptr<Scene>& scene, std::string _prefix, std::vector<StyleParam>& out);

    static bool parseStyleUniforms(const std::shared_ptr<Platform>& platform, const Node& value,
                                   const std::shared_ptr<Scene>& scene, StyleUniform& styleUniform);

    static void parseLightPosition(Node position, PointLight& light);

    static bool loadStyle(const std::shared_ptr<Platform>& platform, const std::string& styleName,
                          Node config, const std::shared_ptr<Scene>& scene);

    static std::mutex m_textureMutex;
    SceneLoader() = delete;

};

}
