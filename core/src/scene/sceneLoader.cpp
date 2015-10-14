#include <vector>
#include "platform.h"
#include "scene.h"
#include "sceneLoader.h"
#include "lights.h"
#include "data/clientGeoJsonSource.h"
#include "data/geoJsonSource.h"
#include "data/mvtSource.h"
#include "gl/shaderProgram.h"
#include "style/material.h"
#include "style/polygonStyle.h"
#include "style/polylineStyle.h"
#include "style/textStyle.h"
#include "style/debugStyle.h"
#include "style/debugTextStyle.h"
#include "style/pointStyle.h"
#include "scene/dataLayer.h"
#include "scene/filters.h"
#include "scene/sceneLayer.h"
#include "scene/spriteAtlas.h"
#include "scene/stops.h"
#include "util/yamlHelper.h"
#include "view/view.h"

#include "yaml-cpp/yaml.h"

#include <algorithm>
#include <iterator>
#include <unordered_map>

using YAML::Node;
using YAML::NodeType;
using YAML::BadConversion;

#define LOGNode(fmt, node, ...) LOGW(fmt ":\n'%s'\n", ## __VA_ARGS__, Dump(node).c_str())

namespace Tangram {

// TODO: make this configurable: 16MB default in-memory DataSource cache:
constexpr size_t CACHE_SIZE = 16 * (1024 * 1024);


bool SceneLoader::loadScene(const std::string& _sceneString, Scene& _scene) {

    Node config;

    try { config = YAML::Load(_sceneString); }
    catch (YAML::ParserException e) {
        LOGE("Parsing scene config '%s'", e.what());
        return false;
    }
    return loadScene(config, _scene);
}

bool SceneLoader::loadScene(Node& config, Scene& _scene) {

    // Instantiate built-in styles
    _scene.styles().emplace_back(new PolygonStyle("polygons"));
    _scene.styles().emplace_back(new PolylineStyle("lines"));
    _scene.styles().emplace_back(new TextStyle("text", _scene.fontContext(), true, false));
    _scene.styles().emplace_back(new DebugTextStyle(_scene.fontContext(), 0, "debugtext", 30.0f, true, false));
    _scene.styles().emplace_back(new DebugStyle("debug"));
    _scene.styles().emplace_back(new PointStyle("points"));

    if (Node sources = config["sources"]) {
        for (const auto& source : sources) {
            try { loadSource(source, _scene); }
            catch (YAML::RepresentationException e) {
                LOGNode("Parsing sources: '%s'", source, e.what());
            }
        }
    } else {
        LOGW("No source defined in the yaml scene configuration.");
    }

    if (Node textures = config["textures"]) {
        for (const auto& texture : textures) {
            try { loadTexture(texture, _scene); }
            catch (YAML::RepresentationException e) {
                LOGNode("Parsing texture: '%s'", texture, e.what());
            }
        }
    }

    std::unordered_set<std::string> mixedStyles;

    if (Node styles = config["styles"]) {
        for (const auto& style : styles) {
            try {
                std::string styleName = style.first.as<std::string>();
                loadStyle(styleName, styles, _scene, mixedStyles);
            }
            catch (YAML::RepresentationException e) {
                LOGNode("Parsing style: '%s'", style, e.what());
            }
        }
    }

    // Styles that are opaque must be ordered first in the scene so that
    // they are rendered 'under' styles that require blending
    std::sort(_scene.styles().begin(), _scene.styles().end(),
              [](std::unique_ptr<Style>& a, std::unique_ptr<Style>& b) {
                  if (a->blendMode() != b->blendMode()) {
                      return a->blendMode() == Blending::none;
                  }
                  // Just for consistent ordering
                  // anytime the scene is loaded
                  return a->getName() < b->getName();
              });

    // Post style sorting set their respective IDs=>vector indices
    // These indices are used for style geometry lookup in tiles
    auto& styles = _scene.styles();
    for(uint32_t i = 0; i < styles.size(); i++) {
        styles[i]->setID(i);
    }

    if (Node layers = config["layers"]) {
        for (const auto& layer : layers) {
            try { loadLayer(layer, _scene); }
            catch (YAML::RepresentationException e) {
                LOGNode("Parsing layer: '%s'", layer, e.what());
            }
        }
    }

    if (Node lights = config["lights"]) {
        for (const auto& light : lights) {
            try { loadLight(light, _scene); }
            catch (YAML::RepresentationException e) {
                LOGNode("Parsing light: '%s'", light, e.what());
            }
        }
    } else {
        // Add an ambient light if nothing else is specified
        std::unique_ptr<AmbientLight> amb(new AmbientLight("defaultLight"));
        amb->setAmbientColor({ 1.f, 1.f, 1.f, 1.f });
        _scene.lights().push_back(std::move(amb));
    }

    if (Node cameras = config["cameras"]) {
        try { loadCameras(cameras, _scene); }
        catch (YAML::RepresentationException e) {
            LOGNode("Parsing cameras: '%s'", cameras, e.what());
        }
    }

    loadBackground(config["scene"]["background"], _scene);

    for (auto& style : _scene.styles()) {
        style->build(_scene.lights());
    }

    return true;
}

void SceneLoader::loadShaderConfig(Node shaders, Style& style, Scene& scene) {

    if (!shaders) { return; }

    auto& shader = *(style.getShaderProgram());

    if (Node extensionsNode = shaders["extensions"]) {
        switch (extensionsNode.Type()) {
        case NodeType::Sequence:
            for (const auto& extNode : extensionsNode) {
                auto extName = extNode.as<std::string>();
                std::ostringstream ext;
                ext << "#if defined(GL_ES) == 0 || defined(GL_" << extName << ")\n";
                ext << "    #extension GL_" << extName << " : enable\n";
                ext << "    #define TANGRAM_EXTENSION_" << extName << '\n';
                ext << "#endif\n";

                shader.addSourceBlock("extensions", ext.str());
            }
            break;
        default:
            LOGNode("Invalid 'extensions'", extensionsNode);
        }
    }
    //shader.addSourceBlock("defines", "#define " + name + " " + value);
    shaders["defines"]["STYLE"] = style.getName();

    if (Node definesNode = shaders["defines"]) {
        for (const auto& define : definesNode) {
            std::string name = define.first.as<std::string>();

            // undefine any previous definitions
            shader.addSourceBlock("defines", "#undef " + name);

            bool bValue;

            if (getBool(define.second, bValue)) {
                // specifying a define to be 'true' means that it is simply
                // defined and has no value
                if (bValue) {
                    shader.addSourceBlock("defines", "#define " + name);
                }
            } else {
                std::string value = define.second.as<std::string>();
                shader.addSourceBlock("defines", "#define " + name + " " + value);
            }
        }
    }

    if (Node uniformsNode = shaders["uniforms"]) {
        for (const auto& uniform : uniformsNode) {
            auto name = uniform.first.as<std::string>();
            auto uniforms = parseStyleUniforms(uniform.second, scene);
            auto& type = uniforms.first;
            auto& uniformValues = uniforms.second;
            int size = uniformValues.size();
            if (size == 1) {
                shader.addSourceBlock("uniforms", "uniform " + type + " " + name + ";");
                style.styleUniforms().emplace_back(name, uniformValues[0]);
            } else {
                shader.addSourceBlock("uniforms", "uniform " + type + " " + name +
                                                        "[" + std::to_string(size) + "];");
                for (int i = 0; i < size; i++) {
                    style.styleUniforms().emplace_back(name + "[" + std::to_string(i) + "]", uniformValues[i]);
                }
            }
        }
    }

    if (Node blocksNode = shaders["blocks"]) {
        for (const auto& block : blocksNode) {
            auto& name = block.first.as<std::string>();
            if (block.second.IsSequence()){
                for (auto & n : block.second) {
                    auto& value = n.as<std::string>();
                    shader.addSourceBlock(name, value, false);

                }
            } else {
                LOGNode("Invalid merged shader %s 'block'", block, name.c_str());
            }
        }
    }
}

glm::vec4 parseMaterialVec(const Node& prop) {

    switch (prop.Type()) {
    case NodeType::Sequence:
        return parseVec<glm::vec4>(prop);
    case NodeType::Scalar: {
        float value;
        if (getFloat(prop, value)) {
            return glm::vec4(value, value, value, 1.0);
        } else {
            LOGNode("Invalid 'material'", prop);
            // TODO: css color parser and hex_values
        }
        break;
    }
    case NodeType::Map:
        // Handled as texture
        break;
    default:
        LOGNode("Invalid 'material'", prop);
        break;
    }
    return glm::vec4(0.0);
}

void SceneLoader::loadMaterial(Node matNode, Material& material, Scene& scene) {
    if (!matNode.IsMap()) { return; }

    if (Node n = matNode["emission"]) {
        if (n.IsMap()) {
            material.setEmission(loadMaterialTexture(n, scene));
        } else {
            material.setEmission(parseMaterialVec(n));
        }
    }
    if (Node n = matNode["diffuse"]) {
        if (n.IsMap()) {
            material.setDiffuse(loadMaterialTexture(n, scene));
        } else {
            material.setDiffuse(parseMaterialVec(n));
        }
    }
    if (Node n = matNode["ambient"]) {
        if (n.IsMap()) {
            material.setAmbient(loadMaterialTexture(n, scene));
        } else {
            material.setAmbient(parseMaterialVec(n));
        }
    }

    if (Node n = matNode["specular"]) {
        if (n.IsMap()) {
            material.setSpecular(loadMaterialTexture(n, scene));
        } else {
            material.setSpecular(parseMaterialVec(n));
        }
    }

    if (Node shininess = matNode["shininess"]) {
        float value;
        if (getFloat(shininess, value, "shininess")) {
            material.setShininess(value);
        }
    }

    material.setNormal(loadMaterialTexture(matNode["normal"], scene));
}

MaterialTexture SceneLoader::loadMaterialTexture(Node matCompNode, Scene& scene) {

    if (!matCompNode) { return MaterialTexture{}; }

    Node textureNode = matCompNode["texture"];
    if (!textureNode) {
        LOGNode("Expected a 'texture' parameter", matCompNode);

        return MaterialTexture{};
    }

    std::string name = textureNode.as<std::string>();

    MaterialTexture matTex;
    matTex.tex = scene.textures()[name];

    if (!matTex.tex) { matTex.tex = std::make_shared<Texture>(name); }

    if (Node mappingNode = matCompNode["mapping"]) {
        std::string mapping = mappingNode.as<std::string>();
        if (mapping == "uv") {
            matTex.mapping = MappingType::uv;
        } else if (mapping == "spheremap") {
            matTex.mapping = MappingType::spheremap;
        } else if (mapping == "planar") {
            // TODO
            LOGW("Planar texture mapping not yet implemented");
        } else if (mapping == "triplanar") {
            // TODO
            LOGW("Triplanar texture mapping not yet implemented");
        } else {
            LOGW("Unrecognized texture mapping '%s'", mapping.c_str());
        }
    }

    if (Node scaleNode = matCompNode["scale"]) {
        if (scaleNode.IsSequence() && scaleNode.size() == 2) {
            matTex.scale = { scaleNode[0].as<float>(), scaleNode[1].as<float>(), 1.f };
        } else if (scaleNode.IsScalar()) {
            matTex.scale = glm::vec3(scaleNode.as<float>());
        } else {
            LOGW("Unrecognized scale parameter in material");
        }
    }

    if (Node amountNode = matCompNode["amount"]) {
        if (amountNode.IsSequence() && amountNode.size() == 3) {
            matTex.amount = { amountNode[0].as<float>(),
                              amountNode[1].as<float>(),
                              amountNode[2].as<float>() };
        } else if (amountNode.IsScalar()) {
            matTex.amount = glm::vec3(amountNode.as<float>());
        } else {
            LOGW("Unrecognized amount parameter in material");
        }
    }

    return matTex;
}

void SceneLoader::loadTexture(const std::string& url, Scene& scene) {
    TextureOptions options = {GL_RGBA, GL_RGBA, {GL_LINEAR, GL_LINEAR}, {GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE} };
    std::shared_ptr<Texture> texture(new Texture(url, options, false));
    scene.textures().emplace(url, texture);
}

void SceneLoader::loadTexture(const std::pair<Node, Node>& node, Scene& scene) {

    std::string name = node.first.as<std::string>();
    Node textureConfig = node.second;

    std::string file;
    TextureOptions options = {GL_RGBA, GL_RGBA, {GL_LINEAR, GL_LINEAR}, {GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE} };

    if (Node url = textureConfig["url"]) {
        file = url.as<std::string>();
    } else {
        LOGW("No url specified for texture '%s', skipping.", name.c_str());
        return;
    }

    bool generateMipmaps = false;

    if (Node filtering = textureConfig["filtering"]) {
        std::string f = filtering.as<std::string>();
        if (f == "linear") { options.m_filtering = { GL_LINEAR, GL_LINEAR }; }
        else if (f == "mipmap") {
            options.m_filtering = { GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR };
            generateMipmaps = true;
        } else if (f == "nearest") { options.m_filtering = { GL_NEAREST, GL_NEAREST }; }
    }

    std::shared_ptr<Texture> texture(new Texture(file, options, generateMipmaps));

    if (Node sprites = textureConfig["sprites"]) {
        std::shared_ptr<SpriteAtlas> atlas(new SpriteAtlas(texture, file));

        for (auto it = sprites.begin(); it != sprites.end(); ++it) {

            const Node sprite = it->second;
            std::string spriteName = it->first.as<std::string>();

            if (sprite) {
                glm::vec4 desc = parseVec<glm::vec4>(sprite);
                glm::vec2 pos = glm::vec2(desc.x, desc.y);
                glm::vec2 size = glm::vec2(desc.z, desc.w);

                atlas->addSpriteNode(spriteName, pos, size);
            }
        }
        scene.spriteAtlases()[name] = atlas;
    }
    scene.textures().emplace(name, texture);
}

void SceneLoader::loadStyleProps(Style& style, Node styleNode, Scene& scene) {

    if (!styleNode) {
        LOGW("Can not parse style parameters, bad style YAML Node");
        return;
    }

    if (Node animatedNode = styleNode["animated"]) {
        LOGW("'animated' property will be set but not yet implemented in styles"); // TODO
        if (!animatedNode.IsScalar()) { LOGW("animated flag should be a scalar"); }
        else {
            bool animate;
            if (getBool(animatedNode, animate, "animated")) {
                style.setAnimated(animate);
            }
        }
    }

    if (Node blendNode = styleNode["blend"]) {
        auto str = blendNode.as<std::string>();
        if      (str == "none")     { style.setBlendMode(Blending::none); }
        else if (str == "add")      { style.setBlendMode(Blending::add); }
        else if (str == "multiply") { style.setBlendMode(Blending::multiply); }
        else if (str == "overlay")  { style.setBlendMode(Blending::overlay); }
        else if (str == "inlay")    { style.setBlendMode(Blending::inlay); }
        else { LOGW("Invalid blend mode '%s'", str.c_str()); }
    }

    if (Node texcoordsNode = styleNode["texcoords"]) {
        LOGW("'texcoords' style parameter is currently ignored");

        if (texcoordsNode.as<bool>()) { } // TODO
        else { } // TODO
    }

    if (Node shadersNode = styleNode["shaders"]) {
        loadShaderConfig(shadersNode, style, scene);
    }

    if (Node materialNode = styleNode["material"]) {
        loadMaterial(materialNode, *(style.getMaterial()), scene);
    }

    if (Node lightingNode = styleNode["lighting"]) {
        auto lighting = lightingNode.as<std::string>();
        if (lighting == "fragment") { style.setLightingType(LightingType::fragment); }
        else if (lighting == "vertex") { style.setLightingType(LightingType::vertex); }
        else if (lighting == "false") { style.setLightingType(LightingType::none); }
        else if (lighting == "true") { } // use default lighting
        else { LOGW("Unrecognized lighting type '%s'", lighting.c_str()); }
    }

    if (Node textureNode = styleNode["texture"]) {

        if (auto pointStyle = dynamic_cast<PointStyle*>(&style)) {

            auto textureName = textureNode.as<std::string>();
            auto atlases = scene.spriteAtlases();
            auto atlasIt = atlases.find(textureName);
            if (atlasIt != atlases.end()) {
                pointStyle->setSpriteAtlas(atlasIt->second);
            } else {
                auto textures = scene.textures();
                auto texIt = textures.find(textureName);
                if (texIt != textures.end()) {
                    pointStyle->setTexture(texIt->second);
                } else {
                    LOGW("Undefined texture name %s", textureName.c_str());
                }
            }
        }
    }

    if (Node urlNode = styleNode["url"]) {
        // TODO
        LOGW("Loading style from URL not yet implemented");
    }
}

bool SceneLoader::propOr(const std::string& propStr, const std::vector<Node>& mixes) {

    for (const auto& mixNode : mixes) {
        if (!mixNode.IsMap()) { continue; }

        Node node = mixNode[propStr];
        if (node && node.IsScalar()) {
            bool bValue;
            if (getBool(node, bValue, propStr.c_str()) && bValue) {
                return true;
            }
        }
    }
    return false;
}

Node SceneLoader::propMerge(const std::string& propName, const std::vector<Node>& mixes) {
    Node result;

    if (propName == "extensions" || propName == "blocks") {
         // handled by explicit methods
        return result;
    }

    std::unordered_map<std::string, std::vector<Node>> mapMixes;

    for (const auto& mixNode : mixes) {
        if (!mixNode.IsMap()) { continue; }

        Node propValue = mixNode[propName];
        if (!propValue) { continue; }

        switch (propValue.Type()) {
        case NodeType::Scalar:
        case NodeType::Sequence:
            // Overwrite Property
            result = Clone(propValue);
            break;

        case NodeType::Map:
            // Reset previous scalar/sequence node
            result.reset();
            for (const auto& tag : propValue) {
                // Deep Merge for all Map Props
                const std::string& tagName = tag.first.Scalar();
                mapMixes[tagName].push_back(propValue);
            }
            break;
        default:
            LOGNode("Cannot merge property '%s'", mixNode, propName.c_str());
            break;
        }
    }

    if (result.IsScalar() || result.IsSequence()) {
        mapMixes.clear();
    }

    // Recursively merge map nodes from this propStr node
    for (auto& it : mapMixes) {
        if (Node n = propMerge(it.first, it.second)) {
            result[it.first] = n;
        }
    }

    return result;
}

Node SceneLoader::shaderBlockMerge(const std::vector<Node>& styles) {

    Node result;

    for (const auto& style : styles) {
        if (!style.IsMap()) {
            LOGNode("Expected map for 'style'", style);
            continue;
        }

        Node shaderNode = style["shaders"];
        if (!shaderNode) { continue; }
        if (!shaderNode.IsMap()) {
            LOGNode("Expected map for 'shader'", shaderNode);
            continue;
        }

        Node blocks = shaderNode["blocks"];
        if (!blocks) { continue; }
        if (!blocks.IsMap()) {
            LOGNode("Expected map for 'blocks'", shaderNode);
            continue;
        }

        for (const auto& block : blocks) {
            const auto& blockName = block.first.Scalar();

            if (block.second.IsSequence()) {
                for (auto& n : block.second)
                    result[blockName].push_back(n);

            } else if (block.second.IsScalar()) {
                result[blockName].push_back(block.second);
            }
        }
    }

    return result;
}

Node SceneLoader::shaderExtMerge(const std::vector<Node>& styles) {

    Node result;
    std::unordered_set<std::string> uniqueList;

    for (const auto& style : styles) {
        if (!style.IsMap()) {
            LOGNode("Expected map for 'style'", style);
            continue;
        }

        Node shaderNode = style["shaders"];
        if (!shaderNode) { continue; }
        if (!shaderNode.IsMap()) {
            LOGNode("Expected map for 'shader'", shaderNode);
            continue;
        }
        Node extNode = shaderNode["extensions"];
        if (!extNode) { continue; }

        switch(extNode.Type()) {
        case NodeType::Scalar: {
            auto val = extNode.as<std::string>();
            if (uniqueList.find(val) == uniqueList.end()) {
                result.push_back(extNode);
                uniqueList.insert(val);
            }
            break;
        }
        case NodeType::Sequence: {
            for (const auto& n : extNode) {
                auto val = n.as<std::string>();
                if (uniqueList.find(val) == uniqueList.end()) {
                    result.push_back(n);
                    uniqueList.insert(val);
                }
            }
            break;
        }
        default:
            LOGNode("Expected scalar or sequence value for 'extensions' node", shaderNode);
        }
    }

    return result;
}

Node SceneLoader::mixStyles(const std::vector<Node>& styles) {

    Node result;

    for (auto& property: {"animated", "texcoords"}) {
        if (propOr(property, styles)) {
            result[property] = true;
        }
    }

    for (auto& property : {"base", "lighting", "texture", "blend", "material", "shaders"}) {
        Node node = propMerge(property, styles);
        if (!node.IsNull()) {
            result[property] = node;
        }
    }

    Node shaderNode = result["shaders"];

    Node shaderExtNode = shaderExtMerge(styles);
    if (!shaderExtNode.IsNull()) {
        shaderNode["extensions"] = shaderExtNode;
    }

    Node shaderBlocksNode = shaderBlockMerge(styles);
    if (!shaderBlocksNode.IsNull()) {
        shaderNode["blocks"] = shaderBlocksNode;
    }

    return result;
}

std::vector<Node> SceneLoader::getMixins(const Node& styleNode, const Node& styles, Scene& scene,
                                         std::unordered_set<std::string>& mixedStyles) {

    Node mixNode = styleNode["mix"];
    std::vector<Node> mixes;

    if (!mixNode) {
        return {};
    }

    std::unordered_set<std::string> uniqueStyles;
    Node mixNodes;

    if (mixNode.IsScalar()) {
        // NB: Would be nice if scalar node had a single item iterator.
        mixes.reserve(2);
        mixNodes.push_back(mixNode);
    } else if (mixNode.IsSequence()) {
        mixes.reserve(mixNode.size() + 1);
        mixNodes = mixNode;
    } else {
        // LOGW("Expected scalar or sequence value as 'mix' parameter: %s.",
        //      styleName.c_str());
        return {};
    }

    for (const auto& mixNode : mixNodes) {
        auto& mixName = mixNode.Scalar();

        if (mixedStyles.find(mixName) == mixedStyles.end()) {
            // Recursively process the mix style first
            if (!loadStyle(mixName, styles, scene, mixedStyles)) {
                continue;
            }

        } else if (uniqueStyles.find(mixName) != uniqueStyles.end()) {
            // This mix has already been added to the current style
            continue;
        }
        uniqueStyles.insert(mixName);

        if (Node mixStyle = styles[mixName]) {
            mixes.push_back(mixStyle);
        }
    }

    return mixes;
}

bool SceneLoader::loadStyle(const std::string& styleName, Node styles, Scene& scene,
                            std::unordered_set<std::string>& mixedStyles) {

    static const auto builtIn = {
        "polygons", "lines", "points", "text", "debug", "debugtext"
    };

    if (std::find(builtIn.begin(), builtIn.end(), styleName) != builtIn.end()) {
        LOGW("Cannot use built-in style name '%s' for new style", styleName.c_str());
        return false;
    }

    Node styleNode = styles[styleName];
    if (!styleNode) {
        LOGW("Style name '%s' is not defined", styleName.c_str());
        return false;
    }

    if (mixedStyles.find(styleName) != mixedStyles.end()) {
        return false;  // This style is already added
    }

    // Add here to not allow loops in addMixinNode recursion!
    mixedStyles.insert(styleName);

    std::vector<Node> mixes = getMixins(styleNode, styles, scene, mixedStyles);

    // Finally through our self into the mix!
    mixes.push_back(styleNode);

    // Update styleNode with mixed style (also for future uses)
    styleNode = mixStyles(mixes);

    Node baseNode = styleNode["base"];
    if (!baseNode) {
        // No base style, this is an abstract styleNode
        return true;
    }

    // Construct style instance using the merged properties
    std::unique_ptr<Style> style;
    std::string baseStyle = baseNode.as<std::string>();
    if (baseStyle == "polygons") {
        style = std::make_unique<PolygonStyle>(styleName);
    } else if (baseStyle == "lines") {
        style = std::make_unique<PolylineStyle>(styleName);
    } else if (baseStyle == "text") {
        style = std::make_unique<TextStyle>(styleName, scene.fontContext(), true, false);
    } else if (baseStyle == "points") {
        style = std::make_unique<PointStyle>(styleName);
    } else {
        LOGW("Base style '%s' not recognized, cannot instantiate.", baseStyle.c_str());
        return false;
    }

    loadStyleProps(*style.get(), styleNode, scene);

    scene.styles().push_back(std::move(style));

    return true;
}

void SceneLoader::loadSource(const std::pair<Node, Node>& src, Scene& _scene) {

    const Node source = src.second;
    std::string name = src.first.as<std::string>();
    std::string type = source["type"].as<std::string>();
    std::string url = source["url"].as<std::string>();

    // distinguish tiled and non-tiled sources by url
    bool tiled = url.find("{x}") != std::string::npos &&
        url.find("{y}") != std::string::npos &&
        url.find("{z}") != std::string::npos;

    std::shared_ptr<DataSource> sourcePtr;

    if (type == "GeoJSON") {
        if (tiled) {
            sourcePtr = std::shared_ptr<DataSource>(new GeoJsonSource(name, url));
        } else {
            sourcePtr = std::shared_ptr<DataSource>(new ClientGeoJsonSource(name, url));
        }
    } else if (type == "TopoJSON") {
        LOGW("TopoJSON data sources not yet implemented"); // TODO
    } else if (type == "MVT") {
        sourcePtr = std::shared_ptr<DataSource>(new MVTSource(name, url));
    } else {
        LOGW("Unrecognized data source type '%s', skipping", type.c_str());
    }

    if (sourcePtr) {
        sourcePtr->setCacheSize(CACHE_SIZE);
        _scene.dataSources().push_back(sourcePtr);
    }
}

void SceneLoader::loadLight(const std::pair<Node, Node>& node, Scene& scene) {

    const Node light = node.second;
    const std::string name = node.first.Scalar();
    const std::string type = light["type"].as<std::string>();

    std::unique_ptr<Light> sceneLight;

    if (type == "ambient") {
        sceneLight = std::make_unique<AmbientLight>(name);

    } else if (type == "directional") {
        auto dLight(std::make_unique<DirectionalLight>(name));

        if (Node direction = light["direction"]) {
            dLight->setDirection(parseVec<glm::vec3>(direction));
        }
        sceneLight = std::move(dLight);

    } else if (type == "point") {
        auto pLight(std::make_unique<PointLight>(name));

        if (Node position = light["position"]) {
            pLight->setPosition(parseVec<glm::vec3>(position));
        }
        if (Node radius = light["radius"]) {
            if (radius.size() > 1) {
                pLight->setRadius(radius[0].as<float>(), radius[1].as<float>());
            } else {
                pLight->setRadius(radius.as<float>());
            }
        }
        if (Node att = light["attenuation"]) {
            pLight->setAttenuation(att.as<float>());
        }
        sceneLight = std::move(pLight);

    } else if (type == "spotlight") {
        auto sLight(std::make_unique<SpotLight>(name));

        if (Node position = light["position"]) {
            sLight->setPosition(parseVec<glm::vec3>(position));
        }
        if (Node direction = light["direction"]) {
            sLight->setDirection(parseVec<glm::vec3>(direction));
        }
        if (Node radius = light["radius"]) {
            if (radius.size() > 1) {
                sLight->setRadius(radius[0].as<float>(), radius[1].as<float>());
            } else {
                sLight->setRadius(radius.as<float>());
            }
        }
        if (Node angle = light["angle"]) {
            sLight->setCutoffAngle(angle.as<float>());
        }
        if (Node exponent = light["exponent"]) {
            sLight->setCutoffExponent(exponent.as<float>());
        }
        sceneLight = std::move(sLight);
    }
    if (Node origin = light["origin"]) {
        const std::string originStr = origin.as<std::string>();
        if (originStr == "world") {
            sceneLight->setOrigin(LightOrigin::world);
        } else if (originStr == "camera") {
            sceneLight->setOrigin(LightOrigin::camera);
        } else if (originStr == "ground") {
            sceneLight->setOrigin(LightOrigin::ground);
        }
    }
    if (Node ambient = light["ambient"]) {
        sceneLight->setAmbientColor(parseVec<glm::vec4>(ambient));
    }
    if (Node diffuse = light["diffuse"]) {
        sceneLight->setDiffuseColor(parseVec<glm::vec4>(diffuse));
    }
    if (Node specular = light["specular"]) {
        sceneLight->setSpecularColor(parseVec<glm::vec4>(specular));
    }

    scene.lights().push_back(std::move(sceneLight));
}

void SceneLoader::loadCameras(Node _cameras, Scene& _scene) {

    // To correctly match the behavior of the webGL library we'll need a place
    // to store multiple view instances.  Since we only have one global view
    // right now, we'll just apply the settings from the first active camera we
    // find.

    auto& view = _scene.view();

    for (const auto& entry : _cameras) {

        const Node camera = entry.second;

        if (Node active = camera["active"]) {
            if (!active.as<bool>()) {
                continue;
            }
        }

        auto type = camera["type"].Scalar();
        if (type == "perspective") {

            view->setCameraType(CameraType::perspective);

            if (Node fov = camera["fov"]) {
                // TODO
            }

            if (Node focal = camera["focal_length"]) {
                // TODO
            }

            if (Node vanishing = camera["vanishing_point"]) {
                // TODO
            }
        } else if (type == "isometric") {

            view->setCameraType(CameraType::isometric);

            if (Node axis = camera["axis"]) {
                view->setObliqueAxis(axis[0].as<float>(), axis[1].as<float>());
            }
        } else if (type == "flat") {

            view->setCameraType(CameraType::flat);

        }

        // Default is world origin at 0 zoom
        double x = 0;
        double y = 0;
        float z = 0;

        if (Node position = camera["position"]) {
            x = position[0].as<double>();
            y = position[1].as<double>();
            if (position.size() > 2) {
                z = position[2].as<float>();
            }
        }

        if (Node zoom = camera["zoom"]) {
            z = zoom.as<float>();
        }

        _scene.startPosition = glm::dvec2(x, y);
        _scene.startZoom = z;

    }
}

Filter SceneLoader::generateFilter(Node _filter, Scene& scene) {

    if (!_filter) {  return Filter(); }

    std::vector<Filter> filters;

    switch (_filter.Type()) {
    case NodeType::Scalar: {

        auto& val = _filter.as<std::string>();

        if (val.compare(0, 8, "function") == 0) {
            scene.functions().push_back(val);
            return Filter::MatchFunction(scene.functions().size()-1);
        }
        break;
    }
    case NodeType::Sequence: {
        for (const auto& filtItr : _filter) {
            filters.push_back(generateFilter(filtItr, scene));
        }
        break;
    }
    case NodeType::Map: {
        for (const auto& filtItr : _filter) {
            std::string key = filtItr.first.as<std::string>();
            Node node = _filter[key];

            if (key == "none") {
                filters.push_back(generateNoneFilter(node, scene));
            } else if (key == "not") {
                filters.push_back(generateNoneFilter(node, scene));
            } else if (key == "any") {
                filters.push_back(generateAnyFilter(node, scene));
            } else if (key == "all") {
                filters.push_back(generateFilter(node, scene));
            } else {
                filters.push_back(generatePredicate(node, key));
            }
        }
        break;
    }
    default:
        // logMsg
        break;
    }

    if (filters.size() == 0) { return Filter(); }
    if (filters.size() == 1) { return filters.front(); }
    return (Filter::MatchAll(filters));
}

Filter SceneLoader::generatePredicate(Node _node, std::string _key) {

    switch (_node.Type()) {
    case NodeType::Scalar: {
        if (_node.Tag() == "tag:yaml.org,2002:str") {
            // Node was explicitly tagged with '!!str' or the canonical tag
            // 'tag:yaml.org,2002:str' yaml-cpp normalizes the tag value to the
            // canonical form
            return Filter::MatchEquality(_key, { Value(_node.as<std::string>()) });
        }
        float number;
        if (getFloat(_node, number)) {
            return Filter::MatchEquality(_key, { Value(number) });
        }
        bool existence;
        if (getBool(_node, existence)) {
            return Filter::MatchExistence(_key, existence);
        }
        std::string value = _node.as<std::string>();
        return Filter::MatchEquality(_key, { Value(std::move(value)) });
    }
    case NodeType::Sequence: {
        std::vector<Value> values;
        for (const auto& valItr : _node) {
            float number;
            if (getFloat(valItr, number)) {
                values.emplace_back(number);
            } else {
                std::string value = valItr.as<std::string>();
                values.emplace_back(std::move(value));
            }
        }
        return Filter::MatchEquality(_key, std::move(values));
    }
    case NodeType::Map: {
        float minVal = -std::numeric_limits<float>::infinity();
        float maxVal = std::numeric_limits<float>::infinity();

        for (const auto& valItr : _node) {
            if (valItr.first.Scalar() == "min") {

                if (!getFloat(valItr.second, minVal, "min")) {
                    LOGNode("Invalid  'filter'", _node);
                    return Filter();
                }
            } else if (valItr.first.Scalar() == "max") {

                if (!getFloat(valItr.second, maxVal, "max")) {
                    LOGNode("Invalid  'filter'", _node);
                    return Filter();
                }
            } else {
                LOGNode("Invalid  'filter'", _node);
                return Filter();
            }
        }
        return Filter::MatchRange(_key, minVal, maxVal);
    }
    default:
        LOGNode("Invalid 'filter'", _node);
        return Filter();
    }
}

Filter SceneLoader::generateAnyFilter(Node _filter, Scene& scene) {
    std::vector<Filter> filters;

    if (!_filter.IsSequence()) {
        LOGW("Invalid filter. 'Any' expects a list.");
        return Filter();
    }
    for (const auto& filt : _filter) {
        filters.emplace_back(generateFilter(filt, scene));
    }
    return Filter::MatchAny(std::move(filters));
}

Filter SceneLoader::generateNoneFilter(Node _filter, Scene& scene) {

    std::vector<Filter> filters;

    if (_filter.IsSequence()) {
        for (const auto& filt : _filter) {
            filters.emplace_back(generateFilter(filt, scene));
        }
    } else if (_filter.IsMap()) { // not case
        for (const auto& filt : _filter) {
            std::string keyFilter = filt.first.as<std::string>();
            filters.emplace_back(generatePredicate(_filter[keyFilter], keyFilter));
        }
    } else {
        LOGW("Invalid filter. 'None' expects a list or an object.");
        return Filter();
    }

    return Filter::MatchNone(std::move(filters));
}

void SceneLoader::parseStyleParams(Node params, Scene& scene, const std::string& prefix,
                                   std::vector<StyleParam>& out) {

    for (const auto& prop : params) {
        std::string key;
        if (!prefix.empty()) {
            key = prefix + ":" + prop.first.Scalar();
        } else {
            key = prop.first.as<std::string>();
        }
        if (key == "transition") {
            parseTransition(prop.second, scene, out);
            continue;
        }

        Node value = prop.second;

        switch (value.Type()) {
        case NodeType::Scalar: {
            auto& val = value.as<std::string>();

            if (val.compare(0, 8, "function") == 0) {
                StyleParam param(key, "");
                param.function = scene.functions().size();
                scene.functions().push_back(val);
                out.push_back(std::move(param));
            } else {
                out.push_back(StyleParam{ key, val });
            }
            break;
        }
        case NodeType::Sequence: {
            if (value[0].IsSequence()) {
                auto styleKey = StyleParam::getKey(key);
                if (styleKey != StyleParamKey::none) {

                    if (StyleParam::isColor(styleKey)) {
                        scene.stops().push_back(Stops::Colors(value));
                        out.push_back(StyleParam{ styleKey, &(scene.stops().back()) });

                    } else if (StyleParam::isWidth(styleKey)) {
                        scene.stops().push_back(Stops::Widths(value, *scene.mapProjection()));
                        out.push_back(StyleParam{ styleKey, &(scene.stops().back()) });

                    } else {
                        // TODO other stops
                    }
                } else {
                    LOGW("Unknown style parameter %s", key.c_str());
                }

            } else {
                out.push_back(StyleParam{ key, parseSequence(value) });
            }
            break;
        }
        case NodeType::Map: {
            // NB: Flatten parameter map
            parseStyleParams(value, scene, key, out);
            break;
        }
        default:
            LOGW("Style parameter %s must be a scalar, sequence, or map.", key.c_str());
        }
    }
}

StyleUniforms SceneLoader::parseStyleUniforms(const Node& value, Scene& scene) {
    std::string type = "";
    std::vector<UniformValue> uniformValues;
    if (value.IsScalar()) { // float, bool or string (texture)
        float fValue;
        bool bValue;
        if (getFloat(value, fValue)) {
            type = "float";
            uniformValues.push_back(fValue);

        } else if (getBool(value, bValue)) {
            type = "bool";
            uniformValues.push_back(bValue);

        } else {
            auto strVal = value.as<std::string>();
            type = "sampler2D";
            auto texItr = scene.textures().find(strVal);
            if (texItr == scene.textures().end()) {
                loadTexture(strVal, scene);
            }
            uniformValues.push_back(strVal);
        }

    } else if (value.IsSequence()) {
        int size = value.size();
        try {
            type = "vec" + std::to_string(size);
            UniformValue uVal;
            switch (size) {
                case 2:
                    uVal = parseVec<glm::vec2>(value);
                    break;
                case 3:
                    uVal = parseVec<glm::vec3>(value);
                    break;
                case 4:
                    uVal = parseVec<glm::vec4>(value);
                    break;
                default:
                    break;
                    // TODO: Handle numeric arrays past 4 elements
            }
            uniformValues.push_back(uVal);
        } catch (const BadConversion& e) { // array of strings (textures)
            uniformValues.reserve(size);
            type = "sampler2D";
            for (const auto& strVal : value) {
                auto textureName = strVal.as<std::string>();
                uniformValues.push_back(textureName);
                auto texItr = scene.textures().find(textureName);
                if (texItr == scene.textures().end()) {
                    loadTexture(textureName, scene);
                }
            }
        }
    } else {
        LOGW("Expected a scalar or sequence value for uniforms");
    }
    return std::make_pair(type, std::move(uniformValues));
}

void SceneLoader::parseTransition(Node params, Scene& scene, std::vector<StyleParam>& out) {

    static const std::string prefix = "transition";

    for (const auto& prop : params) {
        if (!prop.first) {
            continue;
        }
        auto keys = prop.first.as<std::vector<std::string>>();

        for (const auto& key : keys) {
            std::string prefixedKey;
            switch (prop.first.Type()) {
                case YAML::NodeType::Sequence:
                    prefixedKey = prefix + ":" + key;
                    break;
                case YAML::NodeType::Scalar:
                    prefixedKey = prefix + ":" + prop.first.as<std::string>();
                    break;
                default:
                    LOGW("Expected a scalar or sequence value for transition");
                    continue;
                    break;
            }

            for (auto child : prop.second) {
                auto childKey = prefixedKey + ":" + child.first.as<std::string>();
                out.push_back(StyleParam{ childKey, child.second.as<std::string>() });
            }
        }
    }
}

SceneLayer SceneLoader::loadSublayer(Node layer, const std::string& name, Scene& scene) {

    std::vector<SceneLayer> sublayers;
    std::vector<StaticDrawRule> rules;
    Filter filter;

    for (const auto& member : layer) {

        const std::string key = member.first.as<std::string>();

        if (key == "data") {
            // Ignored for sublayers
        } else if (key == "draw") {
            // Member is a mapping of draw rules
            for (auto& ruleNode : member.second) {
                auto name = ruleNode.first.as<std::string>();
                auto explicitStyle = ruleNode.second["style"];
                auto style = explicitStyle
                    ? explicitStyle.as<std::string>()
                    : name;

                int styleId = scene.getStyleId(style);
                if (styleId < 0) {
                    LOGE("TODO Invalid style reference! %s", style.c_str());
                    continue;
                }
                std::vector<StyleParam> params;
                parseStyleParams(ruleNode.second, scene, "", params);
                int nameId = scene.addStyleNameId(name);
                rules.push_back({ name, nameId, std::move(params) });
            }
        } else if (key == "filter") {
            filter = generateFilter(member.second, scene);
        } else if (key == "properties") {
            // TODO: ignored for now
        } else if (key == "visible") {
            // TODO: ignored for now
        } else {
            // Member is a sublayer
            sublayers.push_back(loadSublayer(member.second, key, scene));
        }
    }

    return { name, filter, rules, sublayers };
}

void SceneLoader::loadLayer(const std::pair<Node, Node>& layer, Scene& scene) {

    std::string name = layer.first.as<std::string>();

    std::string source;
    std::vector<std::string> collections;

    if (Node data = layer.second["data"]) {
        if (Node data_source = data["source"]) {
            if (data_source.IsScalar()) {
                source = data_source.Scalar();
            }
        }

        if (Node data_layer = data["layer"]) {
            if (data_layer.IsScalar()) {
                collections.push_back(data_layer.Scalar());
            } else if (data_layer.IsSequence()) {
                collections = data_layer.as<std::vector<std::string>>();
            }
        }
    }

    if (collections.empty()) {
        collections.push_back(name);
    }

    auto sublayer = loadSublayer(layer.second, name, scene);

    scene.layers().push_back({ sublayer, source, collections });
}

void SceneLoader::loadBackground(Node background, Scene& scene) {

    if (!background) { return; }

    if (Node colorNode = background["color"]) {
        std::string str;
        if (colorNode.IsScalar()) {
            str = colorNode.Scalar();
        } else if (colorNode.IsSequence()) {
            str = parseSequence(colorNode);
        }
        scene.background().abgr = StyleParam::parseColor(str);
    }
}

}
