#pragma once

#include <string>
#include <vector>
#include <memory>
#include <tuple>

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
struct DrawRule;
struct MaterialTexture;
struct Filter;

using Mixes = std::vector<YAML::Node>;
// 0: type, 1: values
using StyleUniforms = std::pair<std::string, std::vector<UniformValue>>;

struct SceneLoader {
    using Node = YAML::Node;

    static bool loadScene(const std::string& _sceneString, Scene& _scene);

    /*** public for testing ***/

    static void loadSource(const std::pair<Node, Node>& source, Scene& scene);
    static void loadTexture(const std::pair<Node, Node>& texture, Scene& scene);
    static void loadStyle(const std::pair<Node, Node>& style, Node styles, Scene& scene);
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

    // Style Mixing helper methods
    static Node mixStyle(const Mixes& mixes);

    static MaterialTexture loadMaterialTexture(Node matCompNode, Scene& scene);

    static void parseStyleParams(Node params, Scene& scene, const std::string& propPrefix,
                                 std::vector<StyleParam>& out);

    static StyleUniforms parseStyleUniforms(const YAML::Node& uniform, Scene& scene);

    // Generic methods to merge properties
    static Node propMerge(const std::string& propStr, const Mixes& mixes);

    // Methods to merge shader blocks
    static Node shaderBlockMerge(const Mixes& mixes);

    // Methods to merge shader extensions
    static Node shaderExtMerge(const Mixes& mixes);
    static Tangram::Filter generateFilter(Node filter, Scene& scene);

    SceneLoader() = delete;
};

}
