#include <vector>
#include "platform.h"
#include "scene.h"
#include "sceneLoader.h"
#include "lights.h"
#include "clientGeoJsonSource.h"
#include "geoJsonSource.h"
#include "material.h"
#include "mvtSource.h"
#include "polygonStyle.h"
#include "polylineStyle.h"
#include "textStyle.h"
#include "debugStyle.h"
#include "debugTextStyle.h"
#include "spriteStyle.h"
#include "filters.h"
#include "sceneLayer.h"
#include "scene/dataLayer.h"
#include "util/yamlHelper.h"

#include "yaml-cpp/yaml.h"

#include <algorithm>
#include <unordered_set>
#include <unordered_map>

using YAML::Node;
using YAML::NodeType;
using YAML::BadConversion;

#define LOGE(fmt, ...) do { logMsg("SceneLoader/Error: " fmt "\n", ## __VA_ARGS__); } while(0)
#define LOGW(fmt, ...) do { logMsg("SceneLoader/Warn: " fmt "\n", ## __VA_ARGS__); } while(0)
#define LOGN(fmt, node) do { logMsg("SceneLoader/Warn: " fmt ":\n'%s'\n", Dump(node).c_str()); } while(0)

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

    // Instantiate built-in styles
    _scene.styles().emplace_back(new PolygonStyle("polygons"));
    _scene.styles().emplace_back(new PolylineStyle("lines"));
    _scene.styles().emplace_back(new TextStyle("text", true, true));
    _scene.styles().emplace_back(new DebugTextStyle(0, "debugtext", 30.0f, true, false));
    _scene.styles().emplace_back(new DebugStyle("debug"));
    _scene.styles().emplace_back(new SpriteStyle("sprites"));

    if (Node sources = config["sources"]) {
        for (const auto& source : sources) {
            try { loadSource(source, _scene); }
            catch (YAML::RepresentationException e) {
                LOGE("Parsing source: '%s' in:\n%s",
                     e.what(), Dump(source).c_str());
            }
        }
    } else {
        LOGW("No source defined in the yaml scene configuration.");
    }

    if (Node textures = config["textures"]) {
        for (const auto& texture : textures) {
            try { loadTexture(texture, _scene); }
            catch (YAML::RepresentationException e) {
                LOGE("Parsing texture: '%s' in:\n%s",
                     e.what(), Dump(texture).c_str());
            }
        }
    }

    if (Node styles = config["styles"]) {
        for (const auto& style : styles) {
            try { loadStyle(style, styles, _scene); }
            catch (YAML::RepresentationException e) {
                LOGE("Parsing style: '%s' in:\n%s",
                     e.what(), Dump(style).c_str());
            }
        }
    }

    if (Node layers = config["layers"]) {
        for (const auto& layer : layers) {
            try { loadLayer(layer, _scene); }
            catch (YAML::RepresentationException e) {
                LOGE("Parsing layer: '%s' in:\n%s",
                     e.what(), Dump(layer).c_str());
            }
        }
    }

    if (Node lights = config["lights"]) {
        for (const auto& light : lights) {
            try { loadLight(light, _scene); }
            catch (YAML::RepresentationException e) {
                LOGE("Parsing light: '%s' in:\n%s",
                     e.what(), Dump(light).c_str());
            }
        }
    } else {
        // Add an ambient light if nothing else is specified
        std::unique_ptr<AmbientLight> amb(new AmbientLight("defaultLight"));
        amb->setAmbientColor({ .5f, .5f, .5f, 1.f });
        _scene.lights().push_back(std::move(amb));
    }

    if (Node cameras = config["cameras"]) {
        try { loadCameras(cameras, _scene); }
        catch (YAML::RepresentationException e) {
            LOGE("Parsing cameras: '%s' in:\n%s",
                   e.what(), Dump(cameras).c_str());
        }
    }

    for (auto& style : _scene.styles()) {
        style->build(_scene.lights());
    }

    // Styles that are opaque must be ordered first in the scene so that
    // they are rendered 'under' styles that require blending
    std::sort(_scene.styles().begin(), _scene.styles().end(),
              [](std::unique_ptr<Style>& a, std::unique_ptr<Style>& b) {
                  return a->blendMode() == Blending::none;
              });

    return true;
}

void SceneLoader::loadShaderConfig(Node shaders, Style& style, Scene& scene) {

    if (!shaders) { return; }

    auto& shader = *(style.getShaderProgram());

    if (Node extensionsNode = shaders["extensions"]) {

        std::vector<std::string> extensions;
        switch (extensionsNode.Type()) {
        case NodeType::Scalar:
            extensions.push_back(extensionsNode.as<std::string>());
            break;
        case NodeType::Sequence:
            extensions.reserve(extensionsNode.size());
            for (const auto& extNode : extensionsNode) {
                extensions.push_back(extNode.as<std::string>());
            }
            break;
        default:
            LOGN("Invalid 'extensions'", extensionsNode);
        }

        for (const auto& extName : extensions) {
            char buffer[1000]; //sufficient space
            sprintf(buffer, "#ifdef %s\n  #extension %s : enable\n  #define TANGRAM_EXTENSION_%s\n#endif",
                    extName.c_str(), extName.c_str(), extName.c_str());
            shader.addSourceBlock("extensions", std::string(buffer));
        }
    }

    if (Node definesNode = shaders["defines"]) {
        for (const auto& define : definesNode) {
            std::string name = define.first.as<std::string>();
            std::string value = define.second.as<std::string>();
            if (value == "true") {
                // specifying a define to be 'true' means that it is simply
                // defined and has no value
                value = "";
            } else if (value == "false") {
                // specifying a define to be 'false' means that the define will
                // not be defined at all
                continue;
            }
            shader.addSourceBlock("defines", "#define " + name + " " + value);
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
            std::string name = block.first.as<std::string>();
            std::string value = block.second.as<std::string>();
            //logMsg("add block '%s - %s'\n",name.c_str(), value.c_str());

            shader.addSourceBlock(name, value); // TODO: Warn on unrecognized injection points
        }
    }
}

glm::vec4 parseMaterialVec(const Node& prop) {

    switch (prop.Type()) {
    case NodeType::Sequence:
        return parseVec<glm::vec4>(prop);
    case NodeType::Scalar:
        try {
            float value = prop.as<float>();
            return glm::vec4(value, value, value, 1.0);
        } catch (const BadConversion& e) {
            LOGN("Invalid 'material'", prop);
            // TODO: css color parser and hex_values
        }
        break;
    case NodeType::Map:
        // Handled as texture
        break;
    default:
        LOGN("Invalid 'material'", prop);
        break;
    }
    return glm::vec4(0.0);
}

void SceneLoader::loadMaterial(Node matNode, Material& material, Scene& scene) {

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
        try { material.setShininess(shininess.as<float>()); }
        catch(const BadConversion& e) {
            LOGN("Expected float value for 'shininess'", matNode);
        }
    }

    material.setNormal(loadMaterialTexture(matNode["normal"], scene));
}

MaterialTexture SceneLoader::loadMaterialTexture(Node matCompNode, Scene& scene) {

    if (!matCompNode) { return MaterialTexture{}; }

    Node textureNode = matCompNode["texture"];
    if (!textureNode) {
        LOGN("Expected a 'texture' parameter", matCompNode);

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
            options.m_filtering = { GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR_MIPMAP_LINEAR };
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

void SceneLoader::loadStyleProps(Style& style, YAML::Node styleNode, Scene& scene) {

    if (!styleNode) {
        LOGW("Can not parse style parameters, bad style YAML Node");
        return;
    }

    if (Node animatedNode = styleNode["animated"]) {
        LOGW("'animated' property will be set but not yet implemented in styles"); // TODO
        if (!animatedNode.IsScalar()) { LOGW("animated flag should be a scalar"); }
        else {
            try {
                style.setAnimated(animatedNode.as<bool>());
            } catch(const BadConversion& e) {
                LOGW("Expected a boolean value in 'animated' property. Using default (false).");
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

        if (auto spriteStyle = dynamic_cast<SpriteStyle*>(&style)) {

            auto textureName = textureNode.as<std::string>();
            auto atlases = scene.spriteAtlases();
            auto it = atlases.find(textureName);
            if (it != atlases.end()) {
                spriteStyle->setSpriteAtlas(it->second);
            } else {
                LOGW("Undefined texture name %s", textureName.c_str());
            }
        }
    }

    if (Node urlNode = styleNode["url"]) {
        // TODO
        LOGW("Loading style from URL not yet implemented");
    }
}

Node SceneLoader::propMerge(const std::string& propName, const Mixes& mixes) {

    Node result;

    if (propName == "extensions" || propName == "blocks") {
         // handled by explicit methods
        return result;
    }

    std::vector<std::string> mapTags;
    std::unordered_map<std::string, Mixes> mapMixes;

    for (const auto& mixNode : mixes) {
        Node propValue = mixNode[propName];
        if (!propValue) { continue; }

        switch (propValue.Type()) {
        case NodeType::Scalar:
            if (propValue.as<std::string>() == "true") {
                result = propValue;
                // NB: Boolean are or'ed so we can leave the loop
                // when at least one value is 'true'.
                goto endLoop;
            } else  {
                // Overwrite Properties
                result = propValue;
            }
            break;

        case NodeType::Sequence:
            // Overwrite Properties
            result = propValue;
            break;

        case NodeType::Map:
            // Reset previous scalar/sequence node
            result.reset();
            for (const auto& tag : propValue) {
                auto tagName = tag.first.as<std::string>();
                // Deep Merge for all Map Props
                if (mapMixes.find(tagName) == mapMixes.end()) {
                    mapTags.push_back(tagName);
                }
                mapMixes[tagName].push_back(propValue);
            }
            break;
        default:
            LOGN("Cannot merge property", propValue);
            break;
        }
    }

endLoop:
    if (result.IsScalar() || result.IsSequence()) {
        mapMixes.clear();
        mapTags.clear();
    }

    // Recursively merge map nodes from this propStr node
    for (auto& mapTag : mapTags) {
        Node n = propMerge(mapTag, mapMixes[mapTag]);
        result[mapTag] = n;
    }

    return result;
}

Node SceneLoader::shaderBlockMerge(const Mixes& mixes) {

    Node node;
    for (const auto& mixNode : mixes) {
        Node shaderNode = mixNode["shaders"];
        if (!shaderNode) { continue; }
        if (!shaderNode.IsMap()) {
            LOGN("Expected map for 'shader'", shaderNode);
            continue;
        }
        Node blocks = shaderNode["blocks"];
        if (!blocks) { continue; }
        if (!blocks.IsMap()) {
            LOGN("Expected map for 'blocks'", shaderNode);
            continue;
        }

        for (const auto& block : blocks) {
            std::string blockName = block.first.as<std::string>();
            std::string value = block.second.as<std::string>();
            if (node[blockName]) {
                node[blockName] = node[blockName].as<std::string>() + "\n" + value;
            } else {
                node[blockName] = value;
            }
        }
    }
    return node;
}

Node SceneLoader::shaderExtMerge(const Mixes& mixes) {

    Node node;
    std::unordered_set<std::string> uniqueList;

    for (const auto& mixNode : mixes) {
        Node shaderNode = mixNode["shaders"];
        if (!shaderNode) { continue; }
        if (!shaderNode.IsMap()) {
            LOGN("Expected map for 'shader'", shaderNode);
            continue;
        }
        Node extNode = shaderNode["extensions"];
        if (!extNode) { continue; }

        switch(extNode.Type()) {
        case NodeType::Scalar: {
            auto val = extNode.as<std::string>();
            if (uniqueList.find(val) == uniqueList.end()) {
                node.push_back(extNode);
                uniqueList.insert(val);
            }
            break;
        }
        case NodeType::Sequence: {
            for (const auto& n : extNode) {
                auto val = n.as<std::string>();
                if (uniqueList.find(val) == uniqueList.end()) {
                    node.push_back(n);
                    uniqueList.insert(val);
                }
            }
            break;
        }
        default:
            LOGN("Expected scalar or sequence value for 'extensions' node", node);
        }
    }

    return node;
}

Node SceneLoader::mixStyle(const Mixes& mixes) {

    Node styleNode;

    for (auto& property : { "animated", "texcoords", "base", "lighting",
                            "texture", "blend", "material", "shaders" }) {
        Node node = propMerge(property, mixes);
        if (!node.IsNull()) { styleNode[property] = node; }
    }

    Node shaderNode = styleNode["shaders"];

    Node shaderExtNode = shaderExtMerge(mixes);
    if (!shaderExtNode.IsNull()) {
        shaderNode["extensions"] = shaderExtNode;
    }

    Node shaderBlocksNode = shaderBlockMerge(mixes);
    if (!shaderBlocksNode.IsNull()) {
        shaderNode["blocks"] = shaderBlocksNode;
    }

    return styleNode;
}

void SceneLoader::loadStyle(const std::pair<Node, Node>& styleIt, Node styles, Scene& scene) {

    std::string styleName = styleIt.first.as<std::string>();
    Node styleNode = styleIt.second;

    bool validName = true;
    for (auto& builtIn : { "polygons", "lines", "points", "text", "debug", "debugtext" }) {
        if (styleName == builtIn) { validName = false; }
    }
    if (!validName) {
        LOGW("Cannot use built-in style name '%s' for new style", styleName.c_str());
        return;
    }

    Mixes mixes;
    if (Node mixNode = styleNode["mix"]) {
        if (mixNode.IsScalar()) {
            mixes.reserve(2);
            mixes.push_back(styles[mixNode.as<std::string>()]);
        } else if (mixNode.IsSequence()) {
            mixes.reserve(mixNode.size() + 1);
            for (const auto& mixStyleNode : mixNode) {
                mixes.push_back(styles[mixStyleNode.as<std::string>()]);
            }
        } else {
            LOGW("Parsing mix param for style: '%s'. Expected scalar or sequence value",
                 styleName.c_str());
        }
    }
    mixes.push_back(styles[styleName]);

    Node mixedStyleNode = mixStyle(mixes);

    // Construct style instance using the merged properties
    if (Node baseNode = mixedStyleNode["base"]) {
        std::unique_ptr<Style> style;

        std::string baseString = baseNode.as<std::string>();
        if (baseString == "polygons") {
            style = std::make_unique<PolygonStyle>(styleName);
        } else if (baseString == "lines") {
            style = std::make_unique<PolylineStyle>(styleName);
        } else if (baseString == "text") {
            style = std::make_unique<TextStyle>(styleName, true, true);
        } else if (baseString == "points") {
            style = std::make_unique<SpriteStyle>(styleName);
        } else {
            LOGW("Base style '%s' not recognized, cannot instantiate.", baseString.c_str());
            return;
        }

        loadStyleProps(*style.get(), mixedStyleNode, scene);
        scene.styles().push_back(std::move(style));
    } else {
        // No baseNode, this is an abstract styleNode
        //
        // TODO check why do all the merging above then?
        // when the mixedStyleNode is lost now
        return;
    }
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

    if (type == "GeoJSONTiles") {
        if (tiled) {
            sourcePtr = std::shared_ptr<DataSource>(new GeoJsonSource(name, url));
        } else {
            sourcePtr = std::shared_ptr<DataSource>(new ClientGeoJsonSource(name, url));
        }
    } else if (type == "TopoJSONTiles") {
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

    for (const auto& cam : _cameras) {

        const Node camera = cam.second;

        const std::string type = camera["type"].as<std::string>();
        if (type == "perspective") {
            // The default camera
            Node fov = camera["fov"];
            if (fov) {
                // TODO
            }

            Node focal = camera["focal_length"];
            if (focal) {
                // TODO
            }

            Node vanishing = camera["vanishing_point"];
            if (vanishing) {
                // TODO
            }
        } else if (type == "isometric") {
            // TODO
            Node axis = camera["axis"];
            if (axis) {
                // TODO
            }
        } else if (type == "flat") {
            // TODO
        }

        double x = -74.00976419448854;
        double y = 40.70532700869127;
        float z = 16;

        Node position = camera["position"];
        if (position) {
            x = position[0].as<double>();
            y = position[1].as<double>();
            if (position.size() > 2) {
                z = position[2].as<float>();
            }
        }

        Node zoom = camera["zoom"];
        if (zoom) {
            z = zoom.as<float>();
        }

        _scene.startPosition = glm::dvec2(x, y);
        _scene.startZoom = z;

        Node active = camera["active"];
        if (active) {
            break;
        }
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
    case NodeType::Scalar:
        if (_node.Tag() == "tag:yaml.org,2002:str") {
            // Node was explicitly tagged with '!!str' or the canonical tag
            // 'tag:yaml.org,2002:str' yaml-cpp normalizes the tag value to the
            // canonical form
            return Filter::MatchEquality(_key, { Value(_node.as<std::string>()) });
        }
        try {
            return Filter::MatchEquality(_key, { Value(_node.as<float>()) });
        } catch (const BadConversion& e) {
            std::string value = _node.as<std::string>();
            if (value == "true") {
                return Filter::MatchExistence(_key, true);
            } else if (value == "false") {
                return Filter::MatchExistence(_key, false);
            } else {
                return Filter::MatchEquality(_key, { Value(value) });
            }
        }
    case NodeType::Sequence: {
        std::vector<Value> values;
        for (const auto& valItr : _node) {
            try {
                values.emplace_back(valItr.as<float>());
            } catch(const BadConversion& e) {
                std::string value = valItr.as<std::string>();
                values.emplace_back(value);
            }
        }
        return Filter::MatchEquality(_key, std::move(values));
    }
    case NodeType::Map: {
        float minVal = -std::numeric_limits<float>::infinity();
        float maxVal = std::numeric_limits<float>::infinity();

        for (const auto& valItr : _node) {
            if (valItr.first.as<std::string>() == "min") {
                try {
                    minVal = valItr.second.as<float>();
                } catch (const BadConversion& e) {
                    LOGN("Invalid  'filter', expect a float value type\n", _node);
                    return Filter();
                }
            } else if (valItr.first.as<std::string>() == "max") {
                try {
                    maxVal = valItr.second.as<float>();
                } catch (const BadConversion& e) {
                    LOGN("Invalid  'filter', expect a float value type", _node);
                    return Filter();
                }
            } else {
                LOGN("Invalid  'filter'", _node);
                return Filter();
            }
        }
        return Filter::MatchRange(_key, minVal, maxVal);
    }
    default:
        LOGN("Invalid 'filter'", _node);
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
            key = prop.first.Scalar();
        }

        Node value = prop.second;

        switch (value.Type()) {
        case NodeType::Scalar: {
            auto& val = value.Scalar();

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

                    scene.stops().push_back(Stops(value, StyleParam::isColor(styleKey)));

                    out.push_back(StyleParam{ styleKey, &(scene.stops().back()) });
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
    if (value.IsScalar()) { //float, int (bool), string (texture)
        UniformValue uVal;
        try {
            uVal = value.as<float>();
            type = "float";
        } catch (const BadConversion& e) {
            try {
                uVal = value.as<bool>();
                type = "bool";
            } catch (const BadConversion& e) {
                auto strVal = value.as<std::string>();
                type = "sampler2D";
                uVal = strVal;
                auto texItr = scene.textures().find(strVal);
                if (texItr == scene.textures().end()) {
                    loadTexture(strVal, scene);
                }
            }
        }
        uniformValues.push_back(uVal);
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

SceneLayer SceneLoader::loadSublayer(Node layer, const std::string& name, Scene& scene) {

    std::vector<SceneLayer> sublayers;
    std::vector<DrawRule> rules;
    Filter filter;

    for (const auto& member : layer) {

        const std::string key = member.first.as<std::string>();

        if (key == "data") {
            // Ignored for sublayers
        } else if (key == "draw") {
            // Member is a mapping of draw rules
            for (auto& ruleNode : member.second) {

                auto explicitStyle = ruleNode.second["style"];
                auto style = explicitStyle
                    ? explicitStyle.as<std::string>()
                    : ruleNode.first.as<std::string>();

                std::vector<StyleParam> params;
                parseStyleParams(ruleNode.second, scene, "", params);
                rules.push_back({ style, std::move(params) });
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

    Node data = layer.second["data"];
    Node data_layer = data["layer"];
    Node data_source = data["source"];

    std::string collection = data_layer ? data_layer.as<std::string>() : name;
    std::string source = data_source ? data_source.as<std::string>() : "";

    auto sublayer = loadSublayer(layer.second, name, scene);

    scene.layers().push_back({ sublayer, source, collection });
}

}
