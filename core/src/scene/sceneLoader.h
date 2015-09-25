#pragma once

#include <string>
#include <vector>
#include <memory>
#include <tuple>
#include <unordered_set>

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

// 0: type, 1: values
using StyleUniforms = std::pair<std::string, std::vector<UniformValue>>;

class SceneLoader {

    void loadSources(YAML::Node sources, TileManager& tileManager);
    void loadFont(YAML::Node fontProps);
    void loadLights(YAML::Node lights, Scene& scene);
    void loadCameras(YAML::Node cameras, View& view);
    void loadLayers(YAML::Node layers, Scene& scene, TileManager& tileManager);
    void loadStyles(YAML::Node styles, Scene& scene);
    void loadStyleProps(Style* style, YAML::Node styleNode, Scene& scene);
    void loadTextures(YAML::Node textures, Scene& scene);
    /* loads a texture with default texture properties */
    void loadTexture(const std::string& url, Scene& scene);
    void loadMaterial(YAML::Node matNode, Material& material, Scene& scene);
    MaterialTexture loadMaterialTexture(YAML::Node matCompNode, Scene& scene);
    void loadShaderConfig(YAML::Node shaders, Style& style, Scene& scene);
    SceneLayer loadSublayer(YAML::Node layer, const std::string& name, Scene& scene);
    Filter generateAnyFilter(YAML::Node filter, Scene& scene);
    Filter generateNoneFilter(YAML::Node filter, Scene& scene);
    Filter generatePredicate(YAML::Node filter, std::string _key);

    std::unordered_set<std::string> m_mixedStyles;

public:

    SceneLoader() {};

    virtual ~SceneLoader() {};

    void loadScene(const std::string& _sceneString, Scene& _scene, TileManager& _tileManager, View& _view);

    // public for testing
    std::vector<StyleParam> parseStyleParams(YAML::Node params, Scene& scene, const std::string& propPrefix = "");
    StyleUniforms parseStyleUniforms(const YAML::Node& uniform, Scene& scene);
    YAML::Node mixStyle(const std::vector<YAML::Node>& mixes);

    /* Generate style mixins for a given style node
     * @styleName: styleName to be mixed
     * @styles: YAML::Node for all styles
     * @uniqueStyles: to make sure Mixes returned is a uniqueSet
     */
    std::vector<YAML::Node> recursiveMixins(const std::string& styleName, YAML::Node styles,
            std::unordered_set<std::string>& uniqueStyles);

    // Generic methods to merge properties
    YAML::Node propMerge(const std::string& propStr, const std::vector<YAML::Node>& mixes);

    // Methods to merge shader blocks
    YAML::Node shaderBlockMerge(const std::vector<YAML::Node>& mixes);

    // Methods to merge shader extensions
    YAML::Node shaderExtMerge(const std::vector<YAML::Node>& mixes);
    Filter generateFilter(YAML::Node filter, Scene& scene);
};

}
