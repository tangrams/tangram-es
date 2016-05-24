#include <vector>
#include "platform.h"
#include "scene.h"
#include "sceneLoader.h"
#include "lights.h"
#include "data/clientGeoJsonSource.h"
#include "data/geoJsonSource.h"
#include "data/mvtSource.h"
#include "data/topoJsonSource.h"
#include "data/rasterSource.h"
#include "gl/shaderProgram.h"
#include "style/material.h"
#include "style/polygonStyle.h"
#include "style/polylineStyle.h"
#include "style/textStyle.h"
#include "style/debugStyle.h"
#include "style/debugTextStyle.h"
#include "style/pointStyle.h"
#include "style/rasterStyle.h"
#include "scene/dataLayer.h"
#include "scene/filters.h"
#include "scene/sceneLayer.h"
#include "scene/spriteAtlas.h"
#include "scene/stops.h"
#include "scene/styleMixer.h"
#include "scene/styleParam.h"
#include "util/yamlHelper.h"
#include "view/view.h"

#include "csscolorparser.hpp"

#include <algorithm>
#include <iterator>
#include <unordered_map>

using YAML::Node;
using YAML::NodeType;
using YAML::BadConversion;

#define LOGNode(fmt, node, ...) LOGW(fmt ":\n'%s'\n", ## __VA_ARGS__, Dump(node).c_str())

namespace Tangram {

const std::string DELIMITER = ":";
// TODO: make this configurable: 16MB default in-memory DataSource cache:
constexpr size_t CACHE_SIZE = 16 * (1024 * 1024);

bool SceneLoader::loadScene(const std::string& _sceneString, Scene& _scene) {

    Node& root = _scene.config();

    if (loadConfig(_sceneString, root)) {
        applyConfig(root, _scene);
        return true;
    }
    return false;
}

bool SceneLoader::loadConfig(const std::string& _sceneString, Node& root) {

    try { root = YAML::Load(_sceneString); }
    catch (YAML::ParserException e) {
        LOGE("Parsing scene config '%s'", e.what());
        return false;
    }
    return true;
}

void SceneLoader::applyUpdates(Node& root, const std::vector<Scene::Update>& updates) {

    for (const auto& update : updates) {

        auto node = root;
        std::string key;
        for (auto it = update.keys.begin(); it != update.keys.end(); ++it) {
            key = *it;
            if (it + 1 == update.keys.end()) { break; } // Stop before last key.
            node.reset(node[key]); // Node safely becomes invalid is key is not present.
        }

        if (node) {
            try {
                node[key] = YAML::Load(update.value);
            } catch (YAML::ParserException e) {
                LOGW("Cannot update scene - value invalid: %s", e.what());
            }
        } else {
            std::string path;
            for (const auto& k : update.keys) { path += "." + k; }
            LOGW("Cannot update scene - key not found: %s", path.c_str());
        }
    }
}

void printFilters(const SceneLayer& layer, int indent){
    LOG("%*s >>> %s\n", indent, "", layer.name().c_str());
    layer.filter().print(indent + 2);

    for (auto& l : layer.sublayers()) {
        printFilters(l, indent + 2);
    }
};

void SceneLoader::applyGlobalProperties(Node& node, Scene& scene) {
    switch(node.Type()) {
    case NodeType::Scalar:
        {
            std::string key = node.Scalar();
            if (key.compare(0, 7, "global.") == 0) {
                key.replace(0, 7, "");
                std::replace(key.begin(), key.end(), '.', DELIMITER[0]);
                node = scene.globals()[key];
            }
        }
        break;
    case NodeType::Sequence:
        for (auto n : node) {
            applyGlobalProperties(n, scene);
        }
        break;
    case NodeType::Map:
        for (auto n : node) {
            applyGlobalProperties(n.second, scene);
        }
        break;
    default:
        break;
    }
}

void SceneLoader::parseGlobals(const Node& node, Scene& scene, const std::string& key) {
    switch (node.Type()) {
    case NodeType::Scalar:
    case NodeType::Sequence:
        scene.globals()[key] = node;
        break;
    case NodeType::Map:
        if (key.size() > 0) {
            scene.globals()[key] = node;
        }
        for (const auto& g : node) {
            std::string value = g.first.Scalar();
            Node global = node[value];
            std::string mapKey = (key.size() == 0) ? value : (key + DELIMITER + value);
            parseGlobals(global, scene, mapKey);
        }
    default:
        break;
    }
}

bool SceneLoader::applyConfig(Node& config, Scene& _scene) {

    // Instantiate built-in styles
    _scene.styles().emplace_back(new PolygonStyle("polygons"));
    _scene.styles().emplace_back(new PolylineStyle("lines"));
    _scene.styles().emplace_back(new DebugTextStyle("debugtext", true));
    _scene.styles().emplace_back(new TextStyle("text", _scene.fontContext(), true));
    _scene.styles().emplace_back(new DebugStyle("debug"));
    _scene.styles().emplace_back(new PointStyle("points", _scene.fontContext()));
    _scene.styles().emplace_back(new RasterStyle("raster"));

    if (Node globals = config["global"]) {
        parseGlobals(globals, _scene);
        applyGlobalProperties(config, _scene);
    }


    if (Node sources = config["sources"]) {
        for (const auto& source : sources) {
            std::string srcName = source.first.Scalar();
            try { loadSource(srcName, source.second, sources, _scene); }
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

    if (Node styles = config["styles"]) {
        StyleMixer mixer;
        try {
            mixer.mixStyleNodes(styles);
        } catch (YAML::RepresentationException e) {
            LOGNode("Mixing styles: '%s'", styles, e.what());
        }
        for (const auto& entry : styles) {
            try {
                auto name = entry.first.Scalar();
                auto config = entry.second;
                loadStyle(name, config, _scene);
            }
            catch (YAML::RepresentationException e) {
                LOGNode("Parsing style: '%s'", entry, e.what());
            }
        }
    }

    // Styles that are opaque must be ordered first in the scene so that
    // they are rendered 'under' styles that require blending
    std::sort(_scene.styles().begin(), _scene.styles().end(), Style::compare);

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
    }

    if (_scene.lights().empty()) {
        // Add an ambient light if nothing else is specified
        std::unique_ptr<AmbientLight> amb(new AmbientLight("defaultLight"));
        amb->setAmbientColor({ 1.f, 1.f, 1.f, 1.f });
        _scene.lights().push_back(std::move(amb));
    }

    if (Node camera = config["camera"]) {
        try { loadCamera(camera, _scene); }
        catch (YAML::RepresentationException e) {
            LOGNode("Parsing camera: '%s'", camera, e.what());
        }

    } else if (Node cameras = config["cameras"]) {
        try { loadCameras(cameras, _scene); }
        catch (YAML::RepresentationException e) {
            LOGNode("Parsing cameras: '%s'", cameras, e.what());
        }
    }

    loadBackground(config["scene"]["background"], _scene);

    Node animated = config["scene"]["animated"];
    if (animated) {
        _scene.animated(animated.as<bool>());
    }

    for (auto& style : _scene.styles()) {
        style->build(_scene);
    }

    return true;
}

void SceneLoader::loadShaderConfig(Node shaders, Style& style, Scene& scene) {

    if (!shaders) { return; }

    auto& shader = *(style.getShaderProgram());

    if (Node extNode = shaders["extensions_mixed"]) {
        if (extNode.IsScalar()) {
            shader.addSourceBlock("extensions", ShaderProgram::getExtensionDeclaration(extNode.Scalar()));
        } else if (extNode.IsSequence()) {
            for (const auto& e : extNode) {
                shader.addSourceBlock("extensions", ShaderProgram::getExtensionDeclaration(e.Scalar()));
            }
        }
    }
    //shader.addSourceBlock("defines", "#define " + name + " " + value);
    shaders["defines"]["STYLE"] = style.getName();

    if (Node definesNode = shaders["defines"]) {
        for (const auto& define : definesNode) {
            const std::string& name = define.first.Scalar();

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
                const std::string& value = define.second.Scalar();
                shader.addSourceBlock("defines", "#define " + name + " " + value);
            }
        }
    }

    if (Node uniformsNode = shaders["uniforms"]) {
        for (const auto& uniform : uniformsNode) {
            const std::string& name = uniform.first.Scalar();
            StyleUniform styleUniform;

            if (parseStyleUniforms(uniform.second, scene, styleUniform)) {
                if (styleUniform.value.is<UniformArray1f>()) {
                    UniformArray1f& array = styleUniform.value.get<UniformArray1f>();
                    shader.addSourceBlock("uniforms", "uniform float " + name +
                        "[" + std::to_string(array.size()) + "];");
                } else if(styleUniform.value.is<UniformTextureArray>()) {
                    UniformTextureArray& textureArray = styleUniform.value.get<UniformTextureArray>();
                    shader.addSourceBlock("uniforms", "uniform " + styleUniform.type + " " + name +
                        "[" + std::to_string(textureArray.names.size()) + "];");
                } else {
                    shader.addSourceBlock("uniforms", "uniform " + styleUniform.type + " " + name + ";");
                }

                style.styleUniforms().emplace_back(name, styleUniform.value);
            } else {
                LOGNode("Style uniform parsing failure '%s'", uniform.second);
            }
        }
    }

    if (Node blocksNode = shaders["blocks_mixed"]) {
        for (const auto& block : blocksNode) {
            const auto& name = block.first.Scalar();
            const auto& value = block.second;
            if (value.IsSequence()){
                for (const auto& it : value) {
                    shader.addSourceBlock(name, it.Scalar(), false);
                }
            } else if (value.IsScalar()) {
                shader.addSourceBlock(name, value.Scalar(), false);
            }
        }
    }
}

glm::vec4 parseMaterialVec(const Node& prop) {

    switch (prop.Type()) {
    case NodeType::Sequence:
        return parseVec<glm::vec4>(prop);
    case NodeType::Scalar: {
        double value;
        if (getDouble(prop, value)) {
            return glm::vec4(value, value, value, 1.0);
        } else {
            return getColorAsVec4(prop);
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

void SceneLoader::loadMaterial(Node matNode, Material& material, Scene& scene, Style& style) {
    if (!matNode.IsMap()) { return; }

    if (Node n = matNode["emission"]) {
        if (n.IsMap()) {
            material.setEmission(loadMaterialTexture(n, scene, style));
        } else {
            material.setEmission(parseMaterialVec(n));
        }
    }
    if (Node n = matNode["diffuse"]) {
        if (n.IsMap()) {
            material.setDiffuse(loadMaterialTexture(n, scene, style));
        } else {
            material.setDiffuse(parseMaterialVec(n));
        }
    }
    if (Node n = matNode["ambient"]) {
        if (n.IsMap()) {
            material.setAmbient(loadMaterialTexture(n, scene, style));
        } else {
            material.setAmbient(parseMaterialVec(n));
        }
    }

    if (Node n = matNode["specular"]) {
        if (n.IsMap()) {
            material.setSpecular(loadMaterialTexture(n, scene, style));
        } else {
            material.setSpecular(parseMaterialVec(n));
        }
    }

    if (Node shininess = matNode["shininess"]) {
        double value;
        if (getDouble(shininess, value, "shininess")) {
            material.setShininess(value);
        }
    }

    material.setNormal(loadMaterialTexture(matNode["normal"], scene, style));
}

MaterialTexture SceneLoader::loadMaterialTexture(Node matCompNode, Scene& scene, Style& style) {

    if (!matCompNode) { return MaterialTexture{}; }

    Node textureNode = matCompNode["texture"];
    if (!textureNode) {
        LOGNode("Expected a 'texture' parameter", matCompNode);

        return MaterialTexture{};
    }

    const std::string& name = textureNode.Scalar();

    MaterialTexture matTex;
    matTex.tex = scene.textures()[name];

    if (!matTex.tex) { matTex.tex = std::make_shared<Texture>(name); }

    if (Node mappingNode = matCompNode["mapping"]) {
        const std::string& mapping = mappingNode.Scalar();
        if (mapping == "uv") {
            matTex.mapping = MappingType::uv;

            // Mark the style to generate texture coordinates
            if (!style.genTexCoords()) {
                LOGW("Style %s has option `texcoords: false` but material %s has uv mapping",
                    style.getName().c_str(), name.c_str());
                LOGW("Defaulting uvs generation to true for style %s",
                    style.getName().c_str());
            }

            style.setTexCoordsGeneration(true);
        } else if (mapping == "spheremap") {
            matTex.mapping = MappingType::spheremap;
        } else if (mapping == "planar") {
            matTex.mapping = MappingType::planar;
        } else if (mapping == "triplanar") {
            matTex.mapping = MappingType::triplanar;
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

bool SceneLoader::loadTexture(const std::string& url, Scene& scene) {
    TextureOptions options = {GL_RGBA, GL_RGBA, {GL_LINEAR, GL_LINEAR}, {GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE}};

    unsigned int size = 0;
    unsigned char* blob = bytesFromFile(url.c_str(), PathType::resource, &size);

    if (!blob) {
        LOGE("Can't load texture resource at url %s", url.c_str());
        return false;
    }

    std::shared_ptr<Texture> texture(new Texture(blob, size, options, false));

    free(blob);

    scene.textures().emplace(url, texture);

    return true;
}


bool SceneLoader::extractTexFiltering(Node& filtering, TextureFiltering& filter) {
    const std::string& textureFiltering = filtering.Scalar();
    if (textureFiltering == "linear") {
        filter.min = filter.mag = GL_LINEAR;
        return false;
    } else if (textureFiltering == "mipmap") {
        filter.min = GL_LINEAR_MIPMAP_LINEAR;
        return true;
    } else if (textureFiltering == "nearest") {
        filter.min = filter.mag = GL_NEAREST;
        return false;
    } else {
        return false;
    }
}

void SceneLoader::loadTexture(const std::pair<Node, Node>& node, Scene& scene) {

    const std::string& name = node.first.Scalar();
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
        if (extractTexFiltering(filtering, options.filtering)) {
            generateMipmaps = true;
        }
    }

    std::shared_ptr<Texture> texture(new Texture(file, options, generateMipmaps));

    if (Node sprites = textureConfig["sprites"]) {
        std::shared_ptr<SpriteAtlas> atlas(new SpriteAtlas(texture, file));

        for (auto it = sprites.begin(); it != sprites.end(); ++it) {

            const Node sprite = it->second;
            const std::string& spriteName = it->first.Scalar();

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
        const std::string& blendMode = blendNode.Scalar();
        if      (blendMode == "none")     { style.setBlendMode(Blending::none); }
        else if (blendMode == "add")      { style.setBlendMode(Blending::add); }
        else if (blendMode == "multiply") { style.setBlendMode(Blending::multiply); }
        else if (blendMode == "overlay")  { style.setBlendMode(Blending::overlay); }
        else if (blendMode == "inlay")    { style.setBlendMode(Blending::inlay); }
        else { LOGW("Invalid blend mode '%s'", blendMode.c_str()); }
    }

    if (Node blendOrderNode = styleNode["blend_order"]) {
        try {
            auto blendOrder = blendOrderNode.as<int>();
            style.setBlendOrder(blendOrder);
        } catch (const BadConversion& e) {
            LOGE("Integral value expected for blend_order style parameter.\n");
        }
    }

    if (Node texcoordsNode = styleNode["texcoords"]) {
        style.setTexCoordsGeneration(texcoordsNode.as<bool>());
    }

    if (Node shadersNode = styleNode["shaders"]) {
        loadShaderConfig(shadersNode, style, scene);
    }

    if (Node materialNode = styleNode["material"]) {
        loadMaterial(materialNode, *(style.getMaterial()), scene, style);
    }

    if (Node lightingNode = styleNode["lighting"]) {
        const std::string& lighting = lightingNode.Scalar();
        if (lighting == "fragment") { style.setLightingType(LightingType::fragment); }
        else if (lighting == "vertex") { style.setLightingType(LightingType::vertex); }
        else if (lighting == "false") { style.setLightingType(LightingType::none); }
        else if (lighting == "true") { } // use default lighting
        else { LOGW("Unrecognized lighting type '%s'", lighting.c_str()); }
    }

    if (Node textureNode = styleNode["texture"]) {
        if (auto pointStyle = dynamic_cast<PointStyle*>(&style)) {
            const std::string& textureName = textureNode.Scalar();
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

bool SceneLoader::loadStyle(const std::string& name, Node config, Scene& scene) {

    const auto& builtIn = Style::builtInStyleNames();

    if (std::find(builtIn.begin(), builtIn.end(), name) != builtIn.end()) {
        LOGW("Cannot use built-in style name '%s' for new style", name.c_str());
        return false;
    }

    Node baseNode = config["base"];
    if (!baseNode) {
        // No base style, this is an abstract style
        return true;
    }

    // Construct style instance using the merged properties
    std::unique_ptr<Style> style;
    auto baseStyle = baseNode.Scalar();
    if (baseStyle == "polygons") {
        style = std::make_unique<PolygonStyle>(name);
    } else if (baseStyle == "lines") {
        style = std::make_unique<PolylineStyle>(name);
    } else if (baseStyle == "text") {
        style = std::make_unique<TextStyle>(name, scene.fontContext(), true);
    } else if (baseStyle == "points") {
        style = std::make_unique<PointStyle>(name, scene.fontContext());
    } else if (baseStyle == "raster") {
        style = std::make_unique<RasterStyle>(name);
    } else {
        LOGW("Base style '%s' not recognized, cannot instantiate.", baseStyle.c_str());
        return false;
    }

    Node rasterNode = config["raster"];
    if (rasterNode) {
        const auto& raster = rasterNode.Scalar();
        if (raster == "normal") {
            style->setRasterType(RasterType::normal);
        } else if (raster == "color") {
            style->setRasterType(RasterType::color);
        } else if (raster == "custom") {
            style->setRasterType(RasterType::custom);
        }
    }

    loadStyleProps(*style.get(), config, scene);

    scene.styles().push_back(std::move(style));

    return true;
}

void SceneLoader::loadSource(const std::string& name, const Node& source, const Node& sources, Scene& _scene) {
    if (_scene.getDataSource(name)) {
        LOGW("Duplicate DataSource: %s", name.c_str());
        return;
    }

    std::string type = source["type"].Scalar();
    std::string url = source["url"].Scalar();
    int32_t maxZoom = 18;

    if (auto maxZoomNode = source["max_zoom"]) {
        maxZoom = maxZoomNode.as<int32_t>(maxZoom);
    }

    // Parse and append any URL parameters.
    if (auto urlParamsNode = source["url_params"]) {
        std::stringstream urlStream;
        // Transform our current URL from "base[?query][#hash]" into "base?params[query][#hash]".
        auto hashStart = std::min(url.find_first_of("#"), url.size());
        auto queryStart = std::min(url.find_first_of("?"), url.size());
        auto baseEnd = std::min(hashStart, queryStart + 1);
        urlStream << url.substr(0, baseEnd);
        if (queryStart == url.size()) {
            urlStream << "?";
        }
        if (urlParamsNode.IsMap()) {
            for (const auto& entry : urlParamsNode) {
                if (entry.first.IsScalar() && entry.second.IsScalar()) {
                    urlStream << entry.first.Scalar() << "=" << entry.second.Scalar() << "&";
                } else {
                    LOGW("Invalid url_params entry in source '%s', entries should be strings.", name.c_str());
                }
            }
        } else {
            LOGW("Expected a map of values for url_params in source '%s'.", name.c_str());
        }
        urlStream << url.substr(baseEnd);
        url = urlStream.str();
    }

    // distinguish tiled and non-tiled sources by url
    bool tiled = url.find("{x}") != std::string::npos &&
        url.find("{y}") != std::string::npos &&
        url.find("{z}") != std::string::npos;

    std::shared_ptr<DataSource> sourcePtr;

    if (type == "GeoJSON") {
        if (tiled) {
            sourcePtr = std::shared_ptr<DataSource>(new GeoJsonSource(name, url, maxZoom));
        } else {
            sourcePtr = std::shared_ptr<DataSource>(new ClientGeoJsonSource(name, url, maxZoom));
        }
    } else if (type == "TopoJSON") {
        sourcePtr = std::shared_ptr<DataSource>(new TopoJsonSource(name, url, maxZoom));
    } else if (type == "MVT") {
        sourcePtr = std::shared_ptr<DataSource>(new MVTSource(name, url, maxZoom));
    } else if (type == "Raster") {
        TextureOptions options = {GL_RGBA, GL_RGBA, {GL_LINEAR, GL_LINEAR}, {GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE} };
        bool generateMipmaps = false;
        if (Node filtering = source["filtering"]) {
            if (extractTexFiltering(filtering, options.filtering)) {
                generateMipmaps = true;
            }
        }
        sourcePtr = std::shared_ptr<DataSource>(new RasterSource(name, url, maxZoom, options, generateMipmaps));
    } else {
        LOGW("Unrecognized data source type '%s', skipping", type.c_str());
    }

    if (sourcePtr) {
        sourcePtr->setCacheSize(CACHE_SIZE);
        _scene.dataSources().push_back(sourcePtr);
    }

    if (auto rasters = source["rasters"]) {
        loadSourceRasters(sourcePtr, source["rasters"], sources, _scene);
    }

}

void SceneLoader::loadSourceRasters(std::shared_ptr<DataSource> &source, Node rasterNode, const Node& sources,
                                    Scene& scene) {
    if (rasterNode.IsSequence()) {
        for (const auto& raster : rasterNode) {
            std::string srcName = raster.Scalar();
            try {
                loadSource(srcName, sources[srcName], sources, scene);
            } catch (YAML::RepresentationException e) {
                LOGNode("Parsing sources: '%s'", sources[srcName], e.what());
                return;
            }
            source->rasterSources().push_back(scene.getDataSource(srcName));
        }
    }
}

void SceneLoader::parseLightPosition(Node position, PointLight& light) {
    if (position.IsSequence()) {
        UnitVec<glm::vec3> lightPos;
        std::string positionSequence;

        // Evaluate sequence separated by ',' to parse with parseVec3
        for (auto n : position) {
            positionSequence += n.Scalar() + ",";
        }

        StyleParam::parseVec3(positionSequence, {Unit::meter, Unit::pixel}, lightPos);
        light.setPosition(lightPos);
    } else {
        LOGNode("Wrong light position parameter %s", position);
    }
}

void SceneLoader::loadLight(const std::pair<Node, Node>& node, Scene& scene) {

    const Node light = node.second;
    const std::string& name = node.first.Scalar();
    const std::string& type = light["type"].Scalar();

    if (Node visible = light["visible"]) {
        // If 'visible' is false, skip loading this light.
        if (!visible.as<bool>(true)) { return; }
    }

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
            parseLightPosition(position, *pLight);
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
            parseLightPosition(position, *sLight);
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
        const std::string& originStr = origin.Scalar();
        if (originStr == "world") {
            sceneLight->setOrigin(LightOrigin::world);
        } else if (originStr == "camera") {
            sceneLight->setOrigin(LightOrigin::camera);
        } else if (originStr == "ground") {
            sceneLight->setOrigin(LightOrigin::ground);
        }
    }
    if (Node ambient = light["ambient"]) {
        sceneLight->setAmbientColor(getColorAsVec4(ambient));
    }
    if (Node diffuse = light["diffuse"]) {
        sceneLight->setDiffuseColor(getColorAsVec4(diffuse));
    }
    if (Node specular = light["specular"]) {
        sceneLight->setSpecularColor(getColorAsVec4(specular));
    }

    // Verify that light position parameters are consistent with the origin type
    if (sceneLight->getType() == LightType::point || sceneLight->getType() == LightType::spot) {
        auto pLight = static_cast<PointLight&>(*sceneLight);
        auto lightPosition = pLight.getPosition();
        LightOrigin origin = pLight.getOrigin();

        if (origin == LightOrigin::world) {
            if (lightPosition.units[0] == Unit::pixel || lightPosition.units[1] == Unit::pixel) {
                LOGW("Light position with attachment %s may not be used with unit of type %s",
                    lightOriginString(origin).c_str(), unitString(Unit::pixel).c_str());
                LOGW("Long/Lat expected in meters");
            }
        }
    }

    scene.lights().push_back(std::move(sceneLight));
}

void SceneLoader::loadCamera(const Node& camera, Scene& scene) {

    auto& view = scene.view();

    if (Node active = camera["active"]) {
        if (!active.as<bool>()) {
            return;
        }
    }

    auto type = camera["type"].Scalar();
    if (type == "perspective") {

        view->setCameraType(CameraType::perspective);

        // Only one of focal length and FOV is applied;
        // according to docs, focal length takes precedence.
        if (Node focal = camera["focal_length"]) {
            if (focal.IsScalar()) {
                float length = focal.as<float>(view->getFocalLength());
                view->setFocalLength(length);
            } else if (focal.IsSequence()) {
                auto stops = std::make_shared<Stops>(Stops::Numbers(focal));
                view->setFocalLengthStops(stops);
            }
        } else if (Node fov = camera["fov"]) {
            if (fov.IsScalar()) {
                float degrees = fov.as<float>(view->getFieldOfView() * RAD_TO_DEG);
                view->setFieldOfView(degrees * DEG_TO_RAD);
            } else if (fov.IsSequence()) {
                auto stops = std::make_shared<Stops>(Stops::Numbers(fov));
                for (auto& f : stops->frames) { f.value = f.value.get<float>() * DEG_TO_RAD; }
                view->setFieldOfViewStops(stops);
            }
        }

        if (Node vanishing = camera["vanishing_point"]) {
            if (vanishing.IsSequence() && vanishing.size() >= 2) {
                // Values are pixels, unit strings are ignored.
                float x = std::stof(vanishing[0].Scalar());
                float y = std::stof(vanishing[1].Scalar());
                view->setVanishingPoint(x, y);
            }
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

    scene.startPosition = glm::dvec2(x, y);
    scene.startZoom = z;
}

void SceneLoader::loadCameras(Node _cameras, Scene& _scene) {

    // To correctly match the behavior of the webGL library we'll need a place
    // to store multiple view instances.  Since we only have one global view
    // right now, we'll just apply the settings from the first active camera we
    // find.

    for (const auto& entry : _cameras) {
        loadCamera(entry.second, _scene);
    }
}

Filter SceneLoader::generateFilter(Node _filter, Scene& scene) {

    if (!_filter) {  return Filter(); }

    std::vector<Filter> filters;

    switch (_filter.Type()) {
    case NodeType::Scalar: {

        const std::string& val = _filter.Scalar();

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
            const std::string& key = filtItr.first.Scalar();
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
    if (filters.size() == 1) { return std::move(filters.front()); }
    return (Filter::MatchAll(std::move(filters)));
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
        double number;
        if (getDouble(_node, number)) {
            return Filter::MatchEquality(_key, { Value(number) });
        }
        bool existence;
        if (getBool(_node, existence)) {
            return Filter::MatchExistence(_key, existence);
        }
        const std::string& value = _node.Scalar();
        return Filter::MatchEquality(_key, { Value(std::move(value)) });
    }
    case NodeType::Sequence: {
        std::vector<Value> values;
        for (const auto& valItr : _node) {
            double number;
            if (getDouble(valItr, number)) {
                values.emplace_back(number);
            } else {
                const std::string& value = valItr.Scalar();
                values.emplace_back(std::move(value));
            }
        }
        return Filter::MatchEquality(_key, std::move(values));
    }
    case NodeType::Map: {
        double minVal = -std::numeric_limits<double>::infinity();
        double maxVal = std::numeric_limits<double>::infinity();

        for (const auto& valItr : _node) {
            if (valItr.first.Scalar() == "min") {

                if (!getDouble(valItr.second, minVal, "min")) {
                    LOGNode("Invalid  'filter'", _node);
                    return Filter();
                }
            } else if (valItr.first.Scalar() == "max") {

                if (!getDouble(valItr.second, maxVal, "max")) {
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
    } else if (_filter.IsMap() || _filter.IsScalar()) { // not case
        filters.emplace_back(generateFilter(_filter, scene));
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
            key = prefix + DELIMITER + prop.first.Scalar();
        } else {
            key = prop.first.Scalar();
        }
        if (key == "transition") {
            parseTransition(prop.second, scene, out);
            continue;
        }

        if (key == "text") {
            // Add StyleParam to signify that icon uses text
            out.push_back(StyleParam{ StyleParamKey::point_text, "" });
        }

        Node value = prop.second;

        switch (value.Type()) {
        case NodeType::Scalar: {
            const std::string& val = value.Scalar();

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
                        std::vector<Unit> allowedUnits;
                        StyleParam::unitsForStyleParam(styleKey, allowedUnits);
                        scene.stops().push_back(Stops::Widths(value, *scene.mapProjection(), allowedUnits));
                        out.push_back(StyleParam{ styleKey, &(scene.stops().back()) });
                    } else if (StyleParam::isOffsets(styleKey)) {
                        std::vector<Unit> allowedUnits;
                        StyleParam::unitsForStyleParam(styleKey, allowedUnits);
                        scene.stops().push_back(Stops::Offsets(value, allowedUnits));
                        out.push_back(StyleParam{ styleKey, &(scene.stops().back()) });
                    } else if (StyleParam::isFontSize(styleKey)) {
                        scene.stops().push_back(Stops::FontSize(value));
                        out.push_back(StyleParam{ styleKey, &(scene.stops().back()) });
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

bool SceneLoader::parseStyleUniforms(const Node& value, Scene& scene, StyleUniform& styleUniform) {
    if (value.IsScalar()) { // float, bool or string (texture)
        double fValue;
        bool bValue;

        if (getDouble(value, fValue)) {
            styleUniform.type = "float";
            styleUniform.value = (float)fValue;
        } else if (getBool(value, bValue)) {
            styleUniform.type = "bool";
            styleUniform.value = (bool)bValue;
        } else {
            const std::string& strVal = value.Scalar();
            styleUniform.type = "sampler2D";
            std::shared_ptr<Texture> texture;
            scene.texture(strVal, texture);

            if (!texture && !loadTexture(strVal, scene)) {
                LOGW("Can't load texture with name %s", strVal.c_str());
                return false;
            }

            styleUniform.value = strVal;
        }
    } else if (value.IsSequence()) {
        int size = value.size();
        try {
            switch (size) {
                case 2:
                    styleUniform.value = parseVec<glm::vec2>(value);
                    break;
                case 3:
                    styleUniform.value = parseVec<glm::vec3>(value);
                    break;
                case 4:
                    styleUniform.value = parseVec<glm::vec4>(value);
                    break;
                default:
                    UniformArray1f uniformArray;
                    for (const auto& val : value) {
                        double fValue;
                        if (getDouble(val, fValue)) {
                            uniformArray.push_back(fValue);
                        } else {
                            return false;
                        }
                    }
                    styleUniform.value = std::move(uniformArray);
                    break;
            }

            styleUniform.type = "vec" + std::to_string(size);
        } catch (const BadConversion& e) { // array of strings (textures)
            UniformTextureArray textureArrayUniform;
            textureArrayUniform.names.reserve(size);
            styleUniform.type = "sampler2D";

            for (const auto& strVal : value) {
                const std::string& textureName = strVal.Scalar();
                textureArrayUniform.names.push_back(textureName);
                std::shared_ptr<Texture> texture;
                scene.texture(textureName, texture);

                if (!texture && !loadTexture(textureName, scene)) {
                    LOGW("Can't load texture with name %s", textureName.c_str());
                    return false;
                }
            }

            styleUniform.value = std::move(textureArrayUniform);
        }
    } else {
        LOGW("Expected a scalar or sequence value for uniforms");
        return false;
    }

    return true;
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
                    prefixedKey = prefix + DELIMITER + key;
                    break;
                case YAML::NodeType::Scalar:
                    prefixedKey = prefix + DELIMITER + prop.first.as<std::string>();
                    break;
                default:
                    LOGW("Expected a scalar or sequence value for transition");
                    continue;
                    break;
            }

            for (auto child : prop.second) {
                auto childKey = prefixedKey + DELIMITER + child.first.as<std::string>();
                out.push_back(StyleParam{ childKey, child.second.as<std::string>() });
            }
        }
    }
}

SceneLayer SceneLoader::loadSublayer(Node layer, const std::string& layerName, Scene& scene) {

    std::vector<SceneLayer> sublayers;
    std::vector<DrawRuleData> rules;
    Filter filter;
    bool visible = true;

    for (const auto& member : layer) {

        const std::string& key = member.first.Scalar();

        if (key == "data") {
            // Ignored for sublayers
        } else if (key == "draw") {
            // Member is a mapping of draw rules
            for (auto& ruleNode : member.second) {

                std::vector<StyleParam> params;
                parseStyleParams(ruleNode.second, scene, "", params);

                const std::string& ruleName = ruleNode.first.Scalar();
                int ruleId = scene.addIdForName(ruleName);

                rules.push_back({ ruleName, ruleId, std::move(params) });
            }
        } else if (key == "filter") {
            filter = generateFilter(member.second, scene);
        } else if (key == "properties") {
            // TODO: ignored for now
        } else if (key == "visible") {
            getBool(member.second, visible, "visible");
        } else {
            // Member is a sublayer
            sublayers.push_back(loadSublayer(member.second, (layerName + DELIMITER + key), scene));
        }
    }

    return { layerName, std::move(filter), rules, std::move(sublayers), visible };
}

void SceneLoader::loadLayer(const std::pair<Node, Node>& layer, Scene& scene) {

    const std::string& name = layer.first.Scalar();

    std::string source;
    std::vector<std::string> collections;

    if (Node data = layer.second["data"]) {
        if (Node data_source = data["source"]) {
            if (data_source.IsScalar()) {
                source = data_source.Scalar();
                auto dataSource = scene.getDataSource(source);
                if (dataSource) {
                    dataSource->generateGeometry(true);
                } else {
                    LOGW("Can't find data source %s for layer %s", source.c_str(), name.c_str());
                }
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

    scene.layers().push_back({ std::move(sublayer), source, collections });
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
