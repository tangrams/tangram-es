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

class SceneLoader {

    static void loadSources(YAML::Node sources, Scene& scene);
    static void loadFont(YAML::Node fontProps);
    static void loadLights(YAML::Node lights, Scene& scene);
    static void loadCameras(YAML::Node cameras, Scene& scene);
    static void loadLayers(YAML::Node layers, Scene& scene);
    static void loadStyles(YAML::Node styles, Scene& scene);
    static void loadStyleProps(Style* style, YAML::Node styleNode, Scene& scene);
    static void loadTextures(YAML::Node textures, Scene& scene);
    /* loads a texture with default texture properties */
    static void loadTexture(const std::string& url, Scene& scene);
    static void loadMaterial(YAML::Node matNode, Material& material, Scene& scene);
    static void loadShaderConfig(YAML::Node shaders, Style& style, Scene& scene);
    static SceneLayer loadSublayer(YAML::Node layer, const std::string& name, Scene& scene);
    static Filter generateAnyFilter(YAML::Node filter, Scene& scene);
    static Filter generateNoneFilter(YAML::Node filter, Scene& scene);
    static Filter generatePredicate(YAML::Node filter, std::string _key);

    // Style Mixing helper methods
    static YAML::Node mixStyle(const Mixes& mixes);

public:
    SceneLoader() = delete;

    static bool loadScene(const std::string& _sceneString, Scene& _scene);

    static MaterialTexture loadMaterialTexture(YAML::Node matCompNode, Scene& scene);

    // public for testing
    static void parseStyleParams(YAML::Node params, Scene& scene, const std::string& propPrefix,
                                 std::vector<StyleParam>& out);

    static StyleUniforms parseStyleUniforms(const YAML::Node& uniform, Scene& scene);

    // Generic methods to merge properties
    static YAML::Node propMerge(const std::string& propStr, const Mixes& mixes);

    // Methods to merge shader blocks
    static YAML::Node shaderBlockMerge(const Mixes& mixes);

    // Methods to merge shader extensions
    static YAML::Node shaderExtMerge(const Mixes& mixes);
    static Tangram::Filter generateFilter(YAML::Node filter, Scene& scene);
};

}
