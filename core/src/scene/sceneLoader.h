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

    void loadSources(YAML::Node sources, Scene& scene);
    void loadFont(YAML::Node fontProps);
    void loadLights(YAML::Node lights, Scene& scene);
    void loadCameras(YAML::Node cameras, Scene& scene);
    void loadLayers(YAML::Node layers, Scene& scene);
    void loadStyles(YAML::Node styles, Scene& scene);
    void loadStyleProps(Style* style, YAML::Node styleNode, Scene& scene);
    void loadTextures(YAML::Node textures, Scene& scene);
    /* loads a texture with default texture properties */
    void loadTexture(const std::string& url, Scene& scene);
    void loadMaterial(YAML::Node matNode, Material& material, Scene& scene);
    void loadShaderConfig(YAML::Node shaders, Style& style, Scene& scene);
    SceneLayer loadSublayer(YAML::Node layer, const std::string& name, Scene& scene);
    Filter generateAnyFilter(YAML::Node filter, Scene& scene);
    Filter generateNoneFilter(YAML::Node filter, Scene& scene);
    Filter generatePredicate(YAML::Node filter, std::string _key);

    // Style Mixing helper methods
    YAML::Node mixStyle(const Mixes& mixes);

public:

    SceneLoader() {};

    virtual ~SceneLoader() {};

    bool loadScene(const std::string& _sceneString, Scene& _scene);

    static MaterialTexture loadMaterialTexture(YAML::Node matCompNode, Scene& scene);

    // public for testing
    std::vector<StyleParam> parseStyleParams(YAML::Node params, Scene& scene, const std::string& propPrefix = "");
    StyleUniforms parseStyleUniforms(const YAML::Node& uniform, Scene& scene);

    // Generic methods to merge properties
    YAML::Node propMerge(const std::string& propStr, const Mixes& mixes);

    // Methods to merge shader blocks
    YAML::Node shaderBlockMerge(const Mixes& mixes);

    // Methods to merge shader extensions
    YAML::Node shaderExtMerge(const Mixes& mixes);
    Tangram::Filter generateFilter(YAML::Node filter, Scene& scene);
};

}
