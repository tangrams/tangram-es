#pragma once

#include <string>
#include <vector>
#include <memory>
#include <tuple>
#include <unordered_set>
#include <sstream>

#include "yaml-cpp/yaml.h"
#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "glm/vec4.hpp"

#include "util/variant.h"

namespace Tangram {

class Scene;
class TileManager;
class SceneLayer;
class View;
class ShaderProgram;
class Material;
class Style;
struct StyleParam;
struct StaticDrawRule;
struct MaterialTexture;
struct Filter;

// 0: type, 1: values
using StyleUniforms = std::pair<std::string, std::vector<UniformValue>>;

struct SceneLoader {
    using Node = YAML::Node;

    static bool loadScene(const std::string& _sceneString, Scene& _scene);
    static bool loadScene(Node& config, Scene& _scene);

    /*** all public for testing ***/

    static void loadBackground(Node background, Scene& scene);
    static void loadSource(const std::pair<Node, Node>& source, Scene& scene);
    static void loadTexture(const std::pair<Node, Node>& texture, Scene& scene);
    static void loadLayer(const std::pair<Node, Node>& layer, Scene& scene);
    static void loadLight(const std::pair<Node, Node>& light, Scene& scene);
    static void loadCameras(Node cameras, Scene& scene);
    static void loadStyleProps(Style& style, Node styleNode, Scene& scene);
    static void loadMaterial(Node matNode, Material& material, Scene& scene);
    static void loadShaderConfig(Node shaders, Style& style, Scene& scene);
    static SceneLayer loadSublayer(Node layer, const std::string& name, Scene& scene);
    static Filter generateAnyFilter(Node filter, Scene& scene);
    static Filter generateNoneFilter(Node filter, Scene& scene);
    static Filter generatePredicate(Node filter, std::string _key);
    /* loads a texture with default texture properties */
    static void loadTexture(const std::string& url, Scene& scene);

    static MaterialTexture loadMaterialTexture(Node matCompNode, Scene& scene);

    static void parseStyleParams(Node params, Scene& scene, const std::string& propPrefix,
                                 std::vector<StyleParam>& out);
    static void parseTransition(Node params, Scene& scene, std::vector<StyleParam>& out);

    static StyleUniforms parseStyleUniforms(const Node& uniform, Scene& scene);
    static Node mixStyles(const std::vector<Node>& mixes);

    static bool loadStyle(const std::string& styleName, Node styles, Scene& scene,
                          std::unordered_set<std::string>& mixedStyles);

    /* Generate style mixins for a given style node
     * @styleName: styleName to be mixed
     * @styles: YAML::Node for all styles
     * @uniqueStyles: to make sure Mixes returned is a uniqueSet
     */
    static std::vector<Node> getMixins(const Node& styleNode, const Node& styles, Scene& scene,
                                       std::unordered_set<std::string>& mixedStyles);

    // Generic methods to merge properties
    static bool propOr(const std::string& propStr, const std::vector<Node>& mixes);
    static Node propMerge(const std::string& propStr, const std::vector<Node>& mixes);

    // Methods to merge shader blocks
    static Node shaderBlockMerge(const std::vector<Node>& mixes);

    // Methods to merge shader extensions
    static Node shaderExtMerge(const std::vector<Node>& mixes);
    static Filter generateFilter(Node filter, Scene& scene);

    SceneLoader() = delete;

};

}
