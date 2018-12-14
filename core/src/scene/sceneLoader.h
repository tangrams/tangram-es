#pragma once

#include "gl/uniform.h"
#include "map.h"
#include "scene/scene.h"

#include <string>
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

    static SceneError applyUpdates(Node& config, const std::vector<SceneUpdate>& updates);


    /// Global
    static void applyGlobals(Node& config);


    /// Scene
    static void applyScene(const Node& sceneNode, Color& background, Stops& backgroundStops,

                           Scene::animate& animated);
    /// Cameras
    static void applyCameras(const Node& config, SceneCamera& camera);
    static void loadCameras(const Node& camerasNode, SceneCamera& camera);
    static void loadCamera(const Node& cameraNode, SceneCamera& camera);


    /// Lights
    static Scene::Lights applyLights(const Node& lightsNode);
    static std::unique_ptr<Light> loadLight(const std::pair<Node, Node>& light);
    static void parseLightPosition(const Node& positionNode, PointLight& light);


    /// Textures
    static void applyTextures(const Node& texturesNode, SceneTextures& textures);
    static void loadTexture(const std::pair<Node, Node>& texture, SceneTextures& textures);
    static bool parseTexFiltering(const Node& filteringNode, TextureOptions& options);


    /// Fonts
    static void applyFonts(const Node& fontsNode, SceneFonts& fonts);
    static void loadFontDescription(const Node& font, const std::string& family, SceneFonts& fonts);


    /// Sources
    static Scene::TileSources applySources(const Node& config, const SceneOptions& options, Platform& platform);

    static std::shared_ptr<TileSource> loadSource(const Node& source, const std::string& name,
                                                  const SceneOptions& options, Platform& platform);

    /// Styles
    static Scene::Styles applyStyles(const Node& stylesNode, SceneTextures& textures, SceneFunctions& functions,
                                     SceneStops& stops, DrawRuleNames& ruleNames);

    static std::unique_ptr<Style> loadStyle(const std::string& styleName, const Node& styleConfig);
    static void loadStyleProps(const Node& styleConfig, Style& style, SceneTextures& textures);

    /// - StyleParams
    static std::vector<StyleParam> parseStyleParams(const Node& params, SceneStops& stops,
                                                    SceneFunctions& functions);
    static void parseStyleParams(const Node& _params, const std::string& _prefix, SceneStops& _stops,
                                 SceneFunctions& _functions, std::vector<StyleParam>& _out);

    static void parseTransition(const Node& params, std::string prefix, std::vector<StyleParam>& out);

    /// - Shader
    static void loadShaderConfig(const Node& shaders, Style& style, SceneTextures& textures);
    static bool parseStyleUniforms(const Node& value, StyleUniform& styleUniform, SceneTextures& textures);
    static void loadMaterial(const Node& matNode, Material& material, Style& style, SceneTextures& textures);
    static MaterialTexture loadMaterialTexture(const Node& matCompNode, Style& style, SceneTextures& textures);


    /// Layers
    static Scene::Layers applyLayers(const Node& layersNode, SceneFunctions& functions, SceneStops& stops,
                                     DrawRuleNames& ruleNames);

    static void loadLayer(const std::pair<Node, Node>& layer, SceneFunctions& functions, SceneStops& stops,
                          DrawRuleNames& ruleNames);

    static SceneLayer loadSublayer(const Node& layer, const std::string& name, SceneFunctions& functions,
                                   SceneStops& stops, DrawRuleNames& ruleNames);
    /// - Filter
    static Filter generateFilter(SceneFunctions& functions, const Node& filter);
    static Filter generateAnyFilter(SceneFunctions& functions, const Node& filter);
    static Filter generateAllFilter(SceneFunctions& functions, const Node& filter);
    static Filter generateNoneFilter(SceneFunctions& functions, const Node& filter);
    static Filter generatePredicate(const Node& filter, std::string key);
    static bool getFilterRangeValue(const Node& node, double& val, bool& hasPixelArea);

    SceneLoader() = delete;

};

}
