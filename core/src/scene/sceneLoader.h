#pragma once

#include "gl/uniform.h"

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

namespace Tangram {

class Scene;
class TileManager;
class SceneLayer;
class View;
class ShaderProgram;
class Material;
class Style;
struct StyleParam;
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
    static void loadMaterial(Node matNode, Material& material, Scene& scene, Style& style);
    static void loadShaderConfig(Node shaders, Style& style, Scene& scene);
    static SceneLayer loadSublayer(Node layer, const std::string& name, Scene& scene);
    static Filter generateFilter(Node filter, Scene& scene);
    static Filter generateAnyFilter(Node filter, Scene& scene);
    static Filter generateNoneFilter(Node filter, Scene& scene);
    static Filter generatePredicate(Node filter, std::string _key);
    /* loads a texture with default texture properties */
    static void loadTexture(const std::string& url, Scene& scene);

    static MaterialTexture loadMaterialTexture(Node matCompNode, Scene& scene, Style& style);

    static void parseStyleParams(Node params, Scene& scene, const std::string& propPrefix,
                                 std::vector<StyleParam>& out);
    static void parseTransition(Node params, Scene& scene, std::vector<StyleParam>& out);

    static StyleUniforms parseStyleUniforms(const Node& uniform, Scene& scene);

    static bool loadStyle(const std::string& styleName, Node config, Scene& scene);


    SceneLoader() = delete;

};

}
