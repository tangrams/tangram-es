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
#include "scene/importer.h"
#include "scene/sceneLayer.h"
#include "scene/spriteAtlas.h"
#include "scene/stops.h"
#include "scene/styleMixer.h"
#include "scene/styleParam.h"
#include "util/base64.h"
#include "util/y2j.h"
#include "util/yamlHelper.h"
#include "view/view.h"
#include "log.h"

#include "csscolorparser.hpp"

#include <vector>
#include <algorithm>
#include <iterator>
#include <regex>

#define LOGNode(fmt, node, ...) // LOGW(fmt ":\n'%s'\n", ## __VA_ARGS__, Dump(node).c_str())

namespace Tangram {

const std::string DELIMITER = ":";
// TODO: make this configurable: 16MB default in-memory DataSource cache:
constexpr size_t CACHE_SIZE = 16 * (1024 * 1024);

static const std::string GLOBAL_PREFIX = "global.";

std::mutex SceneLoader::m_textureMutex;

bool SceneLoader::loadScene(std::shared_ptr<Scene> _scene, const std::vector<SceneUpdate>& _updates) {

    Importer sceneImporter;

    _scene->config() = sceneImporter.applySceneImports(_scene->path(), _scene->resourceRoot());

    if (_scene->config()) {

        applyUpdates(*_scene, _updates);

        // Load font resources
        _scene->fontContext()->loadFonts();

        applyConfig(_scene);

        return true;
    }
    return false;
}

void SceneLoader::applyUpdates(Scene& scene, const std::vector<SceneUpdate>& updates) {
    auto& root = scene.config();
    for (const auto& update : updates) {
        const char* errorMessage = nullptr;
        size_t errorLine = 0;
        const auto& value = update.value;
        JsonDocument document = yamlParseBytes(update.value.data(), update.value.size(), &errorMessage, &errorLine);

        if (!errorMessage) {
            // FIXME: Set node at update path to update value.
        } else {
            LOGE("While parsing update value: %s\nEncountered error: %s\n", value.data(), errorMessage);
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

void createGlobalRefsRecursive(Node node, Scene& scene, YamlPath path) {
    if (node.isSequence()) {
        int i = 0;
        for (const auto& entry : node.getSequence()) {
            createGlobalRefsRecursive(entry, scene, path.add(i++));
        }
    } else if (node.isMapping()) {
        for (const auto& entry : node.getMapping()) {
            createGlobalRefsRecursive(entry.value, scene, path.add(entry.name.getString()));
        }
    } else if (node.isString()){
        const char* value = node.getString();
        if (strncmp(value, "global.", 7) == 0) {
            scene.globalRefs().emplace_back(path, YamlPath(value));
        }
    }
}

void SceneLoader::applyGlobals(Node root, Scene& scene) {

    createGlobalRefsRecursive(root, scene, YamlPath());

    for (auto& globalRef : scene.globalRefs()) {
        auto target = globalRef.first.get(root);
        auto global = globalRef.second.get(root);
        target = global;
    }
}

bool SceneLoader::applyConfig(const std::shared_ptr<Scene>& _scene) {

    Node& config = _scene->config();

    // Instantiate built-in styles
    _scene->styles().emplace_back(new PolygonStyle("polygons"));
    _scene->styles().emplace_back(new PolylineStyle("lines"));
    _scene->styles().emplace_back(new DebugTextStyle("debugtext", true));
    _scene->styles().emplace_back(new TextStyle("text", _scene->fontContext(), true));
    _scene->styles().emplace_back(new DebugStyle("debug"));
    _scene->styles().emplace_back(new PointStyle("points", _scene->fontContext()));
    _scene->styles().emplace_back(new RasterStyle("raster"));

    if (config["global"]) {
        applyGlobals(config, *_scene);
    }


    if (Node sources = config["sources"]) {
        for (const auto& source : sources.getMapping()) {
            const char* name = source.name.getString();
            loadSource(name, source.value, sources, _scene);
        }
    } else {
        LOGW("No source defined in the yaml scene configuration.");
    }

    if (Node textures = config["textures"]) {
        for (const auto& texture : textures.getMapping()) {
            loadTexture(texture, _scene);
        }
    }

    if (Node fonts = config["fonts"]) {
        for (const auto& font : fonts.getMapping()) {
            loadFont(font, _scene);
        }
    }

    if (Node styles = config["styles"]) {
        StyleMixer mixer;
        mixer.mixStyleNodes(styles);
        for (const auto& entry : styles.getMapping()) {
            auto name = entry.name.getString();
            auto config = entry.value;
            loadStyle(name, config, _scene);
        }
    }

    // Styles that are opaque must be ordered first in the scene so that
    // they are rendered 'under' styles that require blending
    std::sort(_scene->styles().begin(), _scene->styles().end(), Style::compare);

    // Post style sorting set their respective IDs=>vector indices
    // These indices are used for style geometry lookup in tiles
    auto& styles = _scene->styles();
    for(uint32_t i = 0; i < styles.size(); i++) {
        styles[i]->setID(i);
    }

    if (Node layers = config["layers"]) {
        for (const auto& layer : layers.getMapping()) {
            loadLayer(layer, _scene);
        }
    }

    if (Node lights = config["lights"]) {
        for (const auto& light : lights.getMapping()) {
            loadLight(light, _scene);
        }
    }

    if (_scene->lights().empty()) {
        // Add an ambient light if nothing else is specified
        std::unique_ptr<AmbientLight> amb(new AmbientLight("defaultLight"));
        amb->setAmbientColor({ 1.f, 1.f, 1.f, 1.f });
        _scene->lights().push_back(std::move(amb));
    }

    if (Node camera = config["camera"]) {
        loadCamera(camera, _scene);
    } else if (Node cameras = config["cameras"]) {
        loadCameras(cameras, _scene);
    }

    loadBackground(config["scene"]["background"], _scene);

    Node animated = config["scene"]["animated"];
    if (animated && animated.isBool()) {
        _scene->animated(animated.getBool());
    }

    for (auto& style : _scene->styles()) {
        style->build(*_scene);
    }

    return true;
}

void SceneLoader::loadShaderConfig(Node shaders, Style& style, const std::shared_ptr<Scene>& scene) {

    if (!shaders) { return; }

    auto& shader = *(style.getShaderProgram());

    if (Node extNode = shaders["extensions_mixed"]) {
        if (extNode.isString()) {
            shader.addSourceBlock("extensions", ShaderProgram::getExtensionDeclaration(extNode.getString()));
        } else if (extNode.isSequence()) {
            for (const auto& e : extNode.getSequence()) {
                if (e.isString()) {
                    shader.addSourceBlock("extensions", ShaderProgram::getExtensionDeclaration(e.getString()));
                }
            }
        }
    }
    //shader.addSourceBlock("defines", "#define " + name + " " + value);
    // shaders["defines"]["STYLE"] = style.getName();

    if (Node definesNode = shaders["defines"]) {
        for (const auto& define : definesNode.getMapping()) {
            const std::string name = define.name.getString();

            // undefine any previous definitions
            {
                auto pos = name.find('(');
                if (pos == std::string::npos) {
                    shader.addSourceBlock("defines", "#undef " + name);
                } else {
                    shader.addSourceBlock("defines", "#undef " + name.substr(0, pos));
                }
            }
            bool bValue;

            if (define.value.isTrue()) {
                // specifying a define to be 'true' means that it is simply
                // defined and has no value
                shader.addSourceBlock("defines", "#define " + name);
            } else if (define.value.isString()){
                const std::string value = define.value.getString();
                shader.addSourceBlock("defines", "#define " + name + " " + value);
            }
        }
    }

    if (Node uniformsNode = shaders["uniforms"]) {
        for (const auto& uniform : uniformsNode.getMapping()) {
            const std::string name = uniform.name.getString();
            StyleUniform styleUniform;

            if (parseStyleUniforms(uniform.value, scene, styleUniform)) {
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
                LOGNode("Style uniform parsing failure", uniform.second);
            }
        }
    }

    if (Node blocksNode = shaders["blocks_mixed"]) {
        for (const auto& block : blocksNode.getMapping()) {
            const char* name = block.name.getString();
            auto& value = block.value;
            if (value.isSequence()){
                for (const auto& it : value.getSequence()) {
                    if (it.isString()) {
                        shader.addSourceBlock(name, it.getString(), false);
                    }
                }
            } else if (value.isString()) {
                shader.addSourceBlock(name, value.getString(), false);
            }
        }
    }
}

glm::vec4 parseMaterialVec(const Node& prop) {
    if (prop.isSequence()) {
        // return parseVec<glm::vec4>(prop);
        return glm::vec4(); // FIXME
    }
    if (prop.isNumber()) {
        float value = static_cast<float>(prop.getDouble());
        return glm::vec4(value, value, value, 1.);
    }
    if (prop.isString()) {
        return getColorAsVec4(prop);
    }
    if (!prop.isMapping()) {
        LOGNode("Invalid 'material'", prop);
    }
    return glm::vec4();
}

void SceneLoader::loadMaterial(Node matNode, Material& material, const std::shared_ptr<Scene>& scene, Style& style) {
    if (!matNode.isMapping()) { return; }

    if (Node n = matNode["emission"]) {
        if (n.isMapping()) {
            material.setEmission(loadMaterialTexture(n, scene, style));
        } else {
            material.setEmission(parseMaterialVec(n));
        }
    }
    if (Node n = matNode["diffuse"]) {
        if (n.isMapping()) {
            material.setDiffuse(loadMaterialTexture(n, scene, style));
        } else {
            material.setDiffuse(parseMaterialVec(n));
        }
    }
    if (Node n = matNode["ambient"]) {
        if (n.isMapping()) {
            material.setAmbient(loadMaterialTexture(n, scene, style));
        } else {
            material.setAmbient(parseMaterialVec(n));
        }
    }

    if (Node n = matNode["specular"]) {
        if (n.isMapping()) {
            material.setSpecular(loadMaterialTexture(n, scene, style));
        } else {
            material.setSpecular(parseMaterialVec(n));
        }
    }

    if (Node shininess = matNode["shininess"]) {
        if (shininess.isNumber()) {
            material.setShininess(shininess.getFloat());
        }
    }

    material.setNormal(loadMaterialTexture(matNode["normal"], scene, style));
}

MaterialTexture SceneLoader::loadMaterialTexture(Node matCompNode, const std::shared_ptr<Scene>& scene, Style& style) {

    if (!matCompNode) { return MaterialTexture{}; }

    Node textureNode = matCompNode["texture"];
    if (!textureNode) {
        LOGNode("Expected a 'texture' parameter", matCompNode);

        return MaterialTexture{};
    }

    const std::string& name = textureNode.getString();

    MaterialTexture matTex;
    {
        std::lock_guard<std::mutex> lock(m_textureMutex);
        matTex.tex = scene->textures()[name];
    }

    if (!matTex.tex) {
        // Load inline material  textures
        if (!loadTexture(name, scene)) {
            LOGW("Not able to load material texture: %s", name.c_str());
            return MaterialTexture();
        }
    }

    Node mappingNode = matCompNode["mapping"];
    if (mappingNode && mappingNode.isString()) {
        const std::string& mapping = mappingNode.getString();
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
        if (scaleNode.isSequence() && scaleNode.getSequenceCount() == 2) {
            matTex.scale = { scaleNode[0].getDoubleOr(0.), scaleNode[1].getDoubleOr(0.), 1.f };
        } else if (scaleNode.isNumber()) {
            matTex.scale = glm::vec3(scaleNode.getDouble());
        } else {
            LOGW("Unrecognized scale parameter in material");
        }
    }

    if (Node amountNode = matCompNode["amount"]) {
        if (amountNode.isSequence() && amountNode.getSequenceCount() == 3) {
            matTex.amount = { amountNode[0].getDoubleOr(0.),
                              amountNode[1].getDoubleOr(0.),
                              amountNode[2].getDoubleOr(0.) };
        } else if (amountNode.isNumber()) {
            matTex.amount = glm::vec3(amountNode.getDouble());
        } else {
            LOGW("Unrecognized amount parameter in material");
        }
    }

    return matTex;
}

bool SceneLoader::extractTexFiltering(Node& filtering, TextureFiltering& filter) {
    const std::string& textureFiltering = filtering.getStringOr(nullptr);
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

void SceneLoader::updateSpriteNodes(const std::string& texName,
        std::shared_ptr<Texture>& texture, const std::shared_ptr<Scene>& scene) {
    auto& spriteAtlases = scene->spriteAtlases();
    if (spriteAtlases.find(texName) != spriteAtlases.end()) {
        auto& spriteAtlas = spriteAtlases[texName];
        spriteAtlas->updateSpriteNodes(texture);
    }
}

std::shared_ptr<Texture> SceneLoader::fetchTexture(const std::string& name, const std::string& url,
        const TextureOptions& options, bool generateMipmaps, const std::shared_ptr<Scene>& scene) {

    std::shared_ptr<Texture> texture;

    std::regex r("^(http|https):/");
    std::smatch match;
    // TODO: generalize using URI handlers
    if (std::regex_search(url, match, r)) {
        scene->pendingTextures++;
        startUrlRequest(url, [=](std::vector<char>&& rawData) {
                auto ptr = (unsigned char*)(rawData.data());
                size_t dataSize = rawData.size();
                std::lock_guard<std::mutex> lock(m_textureMutex);
                auto texture = scene->getTexture(name);
                if (texture) {
                    if (!texture->loadImageFromMemory(ptr, dataSize)) {
                        LOGE("Invalid texture data '%s'", url.c_str());
                    }

                    updateSpriteNodes(name, texture, scene);
                    scene->pendingTextures--;
                    if (scene->pendingTextures == 0) {
                        requestRender();
                    }
                }
            });
        texture = std::make_shared<Texture>(nullptr, 0, options, generateMipmaps);
    } else {

        if (url.substr(0, 22) == "data:image/png;base64,") {
            // Skip data: prefix
            auto data = url.substr(22);

            std::vector<unsigned char> blob;

            try {
                blob = Base64::decode(data);
            } catch(std::runtime_error e) {
                LOGE("Can't decode Base64 texture '%s'", e.what());
            }

            if (blob.empty()) {
                LOGE("Can't decode Base64 texture");
                return nullptr;
            }
            texture = std::make_shared<Texture>(0, 0, options, generateMipmaps);

            if (!texture->loadImageFromMemory(blob.data(), blob.size())) {
                LOGE("Invalid Base64 texture");
            }

        } else {
            size_t size = 0;
            unsigned char* blob = bytesFromFile(url.c_str(), size);

            if (!blob) {
                LOGE("Can't load texture resource at url '%s'", url.c_str());
                return nullptr;
            }
            texture = std::make_shared<Texture>(0, 0, options, generateMipmaps);

            if (!texture->loadImageFromMemory(blob, size)) {
                LOGE("Invalid texture data '%s'", url.c_str());
            }
            free(blob);
        }
    }

    return texture;
}

bool SceneLoader::loadTexture(const std::string& url, const std::shared_ptr<Scene>& scene) {
    TextureOptions options = {GL_RGBA, GL_RGBA, {GL_LINEAR, GL_LINEAR}, {GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE}};

    auto texture = fetchTexture(url, url, options, false, scene);
    if (texture) {
        std::lock_guard<std::mutex> lock(m_textureMutex);
        scene->textures().emplace(url, texture);
        return true;
    }

    return false;
}

void SceneLoader::loadTexture(const Node::MappingMember& node, const std::shared_ptr<Scene>& scene) {

    const std::string& name = node.name.getString();
    Node textureConfig = node.value;

    std::string file;
    TextureOptions options = {GL_RGBA, GL_RGBA, {GL_LINEAR, GL_LINEAR}, {GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE} };

    if (Node url = textureConfig["url"]) {
        file = url.getStringOr(nullptr);
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

    auto texture = fetchTexture(name, file, options, generateMipmaps, scene);
    if (!texture) { return; }

    std::lock_guard<std::mutex> lock(m_textureMutex);
    if (Node sprites = textureConfig["sprites"]) {
        std::shared_ptr<SpriteAtlas> atlas(new SpriteAtlas(texture));

        for (auto& it : sprites.getMapping()) {

            const Node sprite = it.value;
            const std::string& spriteName = it.name.getString();

            if (sprite) {
                glm::vec4 desc = glm::vec4(); // FIXME: parseVec<glm::vec4>(sprite);
                glm::vec2 pos = glm::vec2(desc.x, desc.y);
                glm::vec2 size = glm::vec2(desc.z, desc.w);

                atlas->addSpriteNode(spriteName, pos, size);
            }
        }
        scene->spriteAtlases()[name] = atlas;
    }
    scene->textures().emplace(name, texture);
}

void loadFontDescription(const Node& node, const std::string& family, const std::shared_ptr<Scene>& scene) {
    if (!node.isMapping()) {
        LOGW(""); // FIXME: wth?
        return;
    }
    std::string style = "normal", weight = "400", uri;

    for (const auto& fontDesc : node.getMapping()) {
        const std::string& key = fontDesc.name.getString();
        if (key == "weight") {
            weight = fontDesc.value.getStringOr(nullptr);
        } else if (key == "style") {
            style = fontDesc.value.getStringOr(nullptr);
        } else if (key == "url") {
            uri = fontDesc.value.getStringOr(nullptr);
        } else if (key == "external") {
            LOGW("external: within fonts: is a no-op in native version of tangram (%s)", family.c_str());
        }
    }

    if (uri.empty()) {
        LOGW("Empty url: block within fonts: (%s)", family.c_str());
        return;
    }

    std::string familyNormalized, styleNormalized;

    familyNormalized.resize(family.size());
    styleNormalized.resize(style.size());

    std::transform(family.begin(), family.end(), familyNormalized.begin(), ::tolower);
    std::transform(style.begin(), style.end(), styleNormalized.begin(), ::tolower);

    // Download/Load the font and add it to the context
    FontDescription _ft(familyNormalized, styleNormalized, weight, uri);

    std::regex regex("^(http|https):/");
    std::smatch match;

    if (std::regex_search(uri, match, regex)) {
        // Load remote
        scene->pendingFonts++;
        startUrlRequest(_ft.uri, [_ft, scene](std::vector<char>&& rawData) {
            if (rawData.size() == 0) {
                LOGE("Bad URL request for font %s at URL %s", _ft.alias.c_str(), _ft.uri.c_str());
            } else {
                unsigned char* data = (unsigned char*)malloc(rawData.size());
                std::memcpy(data, rawData.data(), rawData.size());
                auto source = std::make_shared<alfons::InputSource>(data, rawData.size());
                scene->fontContext()->addFont(_ft, source);
            }
            scene->pendingFonts--;
        });
    } else {
        // Load from local storage
        size_t dataSize = 0;

        if (unsigned char* data = bytesFromFile(_ft.uri.c_str(), dataSize)) {
            auto source = std::make_shared<alfons::InputSource>(data, dataSize);

            LOGN("Add local font %s (%s)", _ft.uri.c_str(), _ft.bundleAlias.c_str());
            scene->fontContext()->addFont(_ft, source);
        } else {
            LOGW("Local font at path %s can't be found (%s)", _ft.uri.c_str(), _ft.bundleAlias.c_str());
        }
    }
}

void SceneLoader::loadFont(const Node::MappingMember& font, const std::shared_ptr<Scene>& scene) {
    const std::string& family = font.name.getString();

    if (font.value.isMapping()) {
        loadFontDescription(font.value, family, scene);
    } else if (font.value.isSequence()) {
        for (const auto& node : font.value.getSequence()) {
            loadFontDescription(node, family, scene);
        }
    }
}

void SceneLoader::loadStyleProps(Style& style, Node styleNode, const std::shared_ptr<Scene>& scene) {

    if (!styleNode) {
        LOGW("Can not parse style parameters, bad style YAML Node");
        return;
    }

    if (Node animatedNode = styleNode["animated"]) {
        if (animatedNode.isBool()) {
            style.setAnimated(animatedNode.getBool());
        } else {
            LOGW("Expected a boolean value for 'animated' style parameter.");
        }
    }

    if (Node blendNode = styleNode["blend"]) {
        const std::string& blendMode = blendNode.getStringOr(nullptr);
        if      (blendMode == "opaque")   { style.setBlendMode(Blending::opaque); }
        else if (blendMode == "add")      { style.setBlendMode(Blending::add); }
        else if (blendMode == "multiply") { style.setBlendMode(Blending::multiply); }
        else if (blendMode == "overlay")  { style.setBlendMode(Blending::overlay); }
        else if (blendMode == "inlay")    { style.setBlendMode(Blending::inlay); }
        else { LOGW("Invalid blend mode '%s'", blendMode.c_str()); }
    }

    if (Node blendOrderNode = styleNode["blend_order"]) {
        if (blendOrderNode.isInt()) {
            style.setBlendOrder(static_cast<int>(blendOrderNode.getInt()));
        } else {
            LOGW("Expected an integer value for 'blend_order' style parameter.");
        }
    }

    if (Node texcoordsNode = styleNode["texcoords"]) {
        if (texcoordsNode.isBool()) {
            style.setTexCoordsGeneration(texcoordsNode.getBool());
        } else {
            LOGW("Expected a boolean value for 'texcoords' style parameter.");
        }

    }

    if (Node dashNode = styleNode["dash"]) {
        if (auto polylineStyle = dynamic_cast<PolylineStyle*>(&style)) {
            if (dashNode.isSequence()) {
                std::vector<int> dashValues;
                for (auto& it : dashNode.getSequence()) {
                    if (it.isInt()) { dashValues.push_back(static_cast<int>(it.getInt())); }
                }
                polylineStyle->setDashArray(dashValues);
                polylineStyle->setTexCoordsGeneration(true);
            }
        }
    }

    if (Node dashBackgroundColor = styleNode["dash_background_color"]) {
        if (auto polylineStyle = dynamic_cast<PolylineStyle*>(&style)) {
            glm::vec4 backgroundColor = getColorAsVec4(dashBackgroundColor);
            polylineStyle->setDashBackgroundColor(backgroundColor);
        }
    }

    if (Node shadersNode = styleNode["shaders"]) {
        loadShaderConfig(shadersNode, style, scene);
    }

    if (Node lightingNode = styleNode["lighting"]) {
        const std::string& lighting = lightingNode.getStringOr(nullptr);
        if (lighting == "fragment") { style.setLightingType(LightingType::fragment); }
        else if (lighting == "vertex") { style.setLightingType(LightingType::vertex); }
        else if (lighting == "false") { style.setLightingType(LightingType::none); }
        else if (lighting == "true") { } // use default lighting
        else { LOGW("Unrecognized lighting type '%s'", lighting.c_str()); }
    }

    if (Node textureNode = styleNode["texture"]) {
        std::lock_guard<std::mutex> lock(m_textureMutex);
        if (auto pointStyle = dynamic_cast<PointStyle*>(&style)) {
            const std::string& textureName = textureNode.getStringOr(nullptr);
            auto& atlases = scene->spriteAtlases();
            auto atlasIt = atlases.find(textureName);
            auto styleTexture = scene->getTexture(textureName);
            if (atlasIt != atlases.end()) {
                pointStyle->setSpriteAtlas(atlasIt->second);
            } else if (styleTexture){
                pointStyle->setTexture(styleTexture);
            } else {
                LOGW("Undefined texture name %s", textureName.c_str());
            }
        } else if (auto polylineStyle = dynamic_cast<PolylineStyle*>(&style)) {
            const std::string& textureName = textureNode.getStringOr(nullptr);
            auto texture = scene->getTexture(textureName);
            if (texture) {
                polylineStyle->setTexture(texture);
                polylineStyle->setTexCoordsGeneration(true);
            }
        }
    }

    if (Node materialNode = styleNode["material"]) {
        loadMaterial(materialNode, *(style.getMaterial()), scene, style);
    }

}

bool SceneLoader::loadStyle(const std::string& name, Node config, const std::shared_ptr<Scene>& scene) {

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
    const std::string& baseStyle = baseNode.getStringOr(nullptr);
    if (baseStyle == "polygons") {
        style = std::make_unique<PolygonStyle>(name);
    } else if (baseStyle == "lines") {
        style = std::make_unique<PolylineStyle>(name);
    } else if (baseStyle == "text") {
        style = std::make_unique<TextStyle>(name, scene->fontContext(), true);
    } else if (baseStyle == "points") {
        style = std::make_unique<PointStyle>(name, scene->fontContext());
    } else if (baseStyle == "raster") {
        style = std::make_unique<RasterStyle>(name);
    } else {
        LOGW("Base style '%s' not recognized, cannot instantiate.", baseStyle.c_str());
        return false;
    }

    Node rasterNode = config["raster"];
    if (rasterNode) {
        const std::string& raster = rasterNode.getStringOr(nullptr);
        if (raster == "normal") {
            style->setRasterType(RasterType::normal);
        } else if (raster == "color") {
            style->setRasterType(RasterType::color);
        } else if (raster == "custom") {
            style->setRasterType(RasterType::custom);
        }
    }

    loadStyleProps(*style.get(), config, scene);

    scene->styles().push_back(std::move(style));

    return true;
}

void SceneLoader::loadSource(const std::string& name, const Node& source, const Node& sources, const std::shared_ptr<Scene>& _scene) {
    if (_scene->getDataSource(name)) {
        LOGW("Duplicate DataSource: %s", name.c_str());
        return;
    }

    std::string type = source["type"].getStringOr(nullptr);
    std::string url = source["url"].getStringOr(nullptr);
    int32_t minDisplayZoom = -1;
    int32_t maxDisplayZoom = -1;
    int32_t maxZoom = 18;

    if (auto minDisplayZoomNode = source["min_display_zoom"]) {
        minDisplayZoom = minDisplayZoomNode.getIntOr(minDisplayZoom);
    }
    if (auto maxDisplayZoomNode = source["max_display_zoom"]) {
        maxDisplayZoom = maxDisplayZoomNode.getIntOr(maxDisplayZoom);
    }
    if (auto maxZoomNode = source["max_zoom"]) {
        maxZoom = maxZoomNode.getIntOr(maxZoom);
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
        if (urlParamsNode.isMapping()) {
            for (const auto& entry : urlParamsNode.getMapping()) {
                if (entry.value.isString()) {
                    urlStream << entry.name.getString() << "=" << entry.value.getString() << "&";
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
            sourcePtr = std::shared_ptr<DataSource>(new GeoJsonSource(name, url, minDisplayZoom, maxDisplayZoom, maxZoom));
        } else {
            sourcePtr = std::shared_ptr<DataSource>(new ClientGeoJsonSource(name, url, minDisplayZoom, maxDisplayZoom, maxZoom));
        }
    } else if (type == "TopoJSON") {
        sourcePtr = std::shared_ptr<DataSource>(new TopoJsonSource(name, url, minDisplayZoom, maxDisplayZoom, maxZoom));
    } else if (type == "MVT") {
        sourcePtr = std::shared_ptr<DataSource>(new MVTSource(name, url, minDisplayZoom, maxDisplayZoom, maxZoom));
    } else if (type == "Raster") {
        TextureOptions options = {GL_RGBA, GL_RGBA, {GL_LINEAR, GL_LINEAR}, {GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE} };
        bool generateMipmaps = false;
        if (Node filtering = source["filtering"]) {
            if (extractTexFiltering(filtering, options.filtering)) {
                generateMipmaps = true;
            }
        }
        sourcePtr = std::shared_ptr<DataSource>(new RasterSource(name, url, minDisplayZoom, maxDisplayZoom, maxZoom, options, generateMipmaps));
    } else {
        LOGW("Unrecognized data source type '%s', skipping", type.c_str());
    }

    if (sourcePtr) {
        sourcePtr->setCacheSize(CACHE_SIZE);
        _scene->dataSources().push_back(sourcePtr);
    }

    if (auto rasters = source["rasters"]) {
        loadSourceRasters(sourcePtr, source["rasters"], sources, _scene);
    }

}

void SceneLoader::loadSourceRasters(std::shared_ptr<DataSource> &source, Node rasterNode, const Node& sources,
                                    const std::shared_ptr<Scene>& scene) {
    if (rasterNode.isSequence()) {
        for (const auto& raster : rasterNode.getSequence()) {
            std::string srcName = raster.getStringOr(nullptr);
            loadSource(srcName, sources[srcName], sources, scene);
            source->addRasterSource(scene->getDataSource(srcName));
        }
    }
}

void SceneLoader::parseLightPosition(Node position, PointLight& light) {
    if (position.isSequence()) {
        UnitVec<glm::vec3> lightPos;
        std::string positionSequence;

        // Evaluate sequence separated by ',' to parse with parseVec3
        for (auto n : position.getSequence()) {
            if (n.isString()) {
                positionSequence += n.getString();
            } else if (n.isNumber()) {
                positionSequence += std::to_string(n.getDouble());
            }
            positionSequence += ",";
        }

        StyleParam::parseVec3(positionSequence, {Unit::meter, Unit::pixel}, lightPos);
        light.setPosition(lightPos);
    } else {
        LOGNode("Wrong light position parameter", position);
    }
}

void SceneLoader::loadLight(const Node::MappingMember& node, const std::shared_ptr<Scene>& scene) {

    const Node light = node.value;
    const std::string& name = node.name.getString();

    std::string type;
    if (Node typeNode = light["type"]) {
        type = typeNode.getStringOr(nullptr);
    }

    if (Node visible = light["visible"]) {
        // If 'visible' is false, skip loading this light.
        if (visible.isFalse()) { return; }
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
            if (radius.getSequenceCount() > 1) {
                pLight->setRadius(radius[0].getFloatOr(0.f), radius[1].getFloatOr(0.f));
            } else {
                pLight->setRadius(radius.getFloatOr(0.f));
            }
        }
        if (Node att = light["attenuation"]) {
            pLight->setAttenuation(att.getFloatOr(0.f));
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
            if (radius.getSequenceCount() > 1) {
                sLight->setRadius(radius[0].getFloatOr(0.f), radius[1].getFloatOr(0.f));
            } else {
                sLight->setRadius(radius.getFloatOr(0.f));
            }
        }
        if (Node angle = light["angle"]) {
            sLight->setCutoffAngle(angle.getFloatOr(0.f));
        }
        if (Node exponent = light["exponent"]) {
            sLight->setCutoffExponent(exponent.getFloatOr(0.f));
        }
        sceneLight = std::move(sLight);
    }
    if (Node origin = light["origin"]) {
        const std::string& originStr = origin.getStringOr(nullptr);
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

    scene->lights().push_back(std::move(sceneLight));
}

void SceneLoader::loadCamera(const Node& _camera, const std::shared_ptr<Scene>& _scene) {

    auto& camera = _scene->camera();

    if (Node active = _camera["active"]) {
        if (!active.isFalse()) {
            return;
        }
    }

    std::string type = _camera["type"].getStringOr(nullptr);
    if (type == "perspective") {
        camera.type = CameraType::perspective;

        // Only one of focal length and FOV is applied;
        // according to docs, focal length takes precedence.
        if (Node focal = _camera["focal_length"]) {
            if (focal.isNumber()) {
                float length = focal.getFloat();
                camera.fieldOfView = View::focalLengthToFieldOfView(length);
            } else if (focal.isSequence()) {
                camera.fovStops = std::make_shared<Stops>(Stops::Numbers(focal));
                for (auto& f : camera.fovStops->frames) {
                    f.value = View::focalLengthToFieldOfView(f.value.get<float>());
                }
            }
        } else if (Node fov = _camera["fov"]) {
            if (fov.isNumber()) {
                float degrees = fov.getFloatOr(camera.fieldOfView * RAD_TO_DEG);
                camera.fieldOfView = degrees * DEG_TO_RAD;

            } else if (fov.isSequence()) {
                camera.fovStops = std::make_shared<Stops>(Stops::Numbers(fov));
                for (auto& f : camera.fovStops->frames) {
                    f.value = float(f.value.get<float>() * DEG_TO_RAD);
                }
            }
        }

        if (Node vanishing = _camera["vanishing_point"]) {
            if (vanishing.isSequence() && vanishing.getSequenceCount() >= 2) {
                // Values are pixels, unit strings are ignored.
                float x = vanishing[0].isString() ? std::stof(vanishing[0].getString()) : vanishing[0].getFloatOr(0.f);
                float y = vanishing[1].isString() ? std::stof(vanishing[1].getString()) : vanishing[1].getFloatOr(0.f);
                camera.vanishingPoint = { x, y };
            }
        }
    } else if (type == "isometric") {
        camera.type = CameraType::isometric;

        if (Node axis = _camera["axis"]) {
            if (axis.isSequence() && axis.getSequenceCount()) {
                camera.obliqueAxis = { axis[0].getFloatOr(0.f), axis[1].getFloatOr(0.f) };
            }
        }
    } else if (type == "flat") {
        camera.type = CameraType::flat;
    }

    // Default is world origin at 0 zoom
    double x = 0;
    double y = 0;
    float z = 0;

    if (Node position = _camera["position"]) {
        if (position.isSequence() && position.getSequenceCount() >= 2) {
            x = position[0].getDoubleOr(0.);
            y = position[1].getDoubleOr(0.);
            if (position.getSequenceCount() > 2) {
                z = position[2].getFloatOr(0.f);
            }
        }
    }

    if (Node zoom = _camera["zoom"]) {
        z = zoom.getFloatOr(0.f);
    }

    if (Node maxTilt = _camera["max_tilt"]) {
        if (maxTilt.isSequence()) {
            camera.maxTiltStops = std::make_shared<Stops>(Stops::Numbers(maxTilt));
        } else {
            camera.maxTilt = maxTilt.getFloatOr(PI);
        }
    }

    _scene->startPosition = glm::dvec2(x, y);
    _scene->startZoom = z;
}

void SceneLoader::loadCameras(Node _cameras, const std::shared_ptr<Scene>& _scene) {

    // To correctly match the behavior of the webGL library we'll need a place
    // to store multiple view instances.  Since we only have one global view
    // right now, we'll just apply the settings from the first active camera we
    // find.

    for (const auto& entry : _cameras.getMapping()) {
        loadCamera(entry.value, _scene);
    }
}

Filter SceneLoader::generateFilter(Node _filter, Scene& scene) {

    if (_filter.isSequence()){
        return generateAnyFilter(_filter, scene);
    } else if (_filter.isMapping()) {
        std::vector<Filter> filters;
        for (const auto& filtItr : _filter.getMapping()) {
            const std::string& key = filtItr.name.getString();
            Node node = _filter[key];
            Filter f;
            if (key == "none") {
                f = generateNoneFilter(node, scene);
            } else if (key == "not") {
                f = generateNoneFilter(node, scene);
            } else if (key == "any") {
                f = generateAnyFilter(node, scene);
            } else if (key == "all") {
                f = generateAllFilter(node, scene);
            } else {
                f = generatePredicate(node, key);
            }

            if (f.isValid()) { filters.push_back(std::move(f)); }
        }

        if (!filters.empty()) {
            if (filters.size() == 1) { return filters.front(); }

            return Filter::MatchAll(std::move(filters));
        }
        return Filter();
    } else {
        const std::string& val = _filter.getStringOr(nullptr);
        if (val.compare(0, 8, "function") == 0) {
            return Filter::MatchFunction(scene.addJsFunction(val));
        }
        return Filter();
    }
}

Filter SceneLoader::generatePredicate(Node _node, std::string _key) {

    if (_node.isSequence()) {
        std::vector<Value> values;
        for (const auto& value : _node.getSequence()) {
            if (value.isNumber()) {
                values.emplace_back(value.getDouble());
            } else {
                const std::string& str = value.getStringOr(nullptr);
                values.emplace_back(std::move(str));
            }
        }
        return Filter::MatchEquality(_key, std::move(values));
    } else if (_node.isMapping()) {
        float minVal = -std::numeric_limits<float>::infinity();
        float maxVal = std::numeric_limits<float>::infinity();
        if (Node min = _node["min"]) {
            minVal = min.getFloatOr(minVal);
        }
        if (Node max = _node["max"]) {
            maxVal = max.getFloatOr(maxVal);
        }
        return Filter::MatchRange(_key, minVal, maxVal);
    } else {
        if (_node.isString()) {
            std::string value = _node.getString();
            return Filter::MatchEquality(_key, { Value(value) });
        }
        if (_node.isNumber()) {
            return Filter::MatchEquality(_key, { Value(_node.getDouble()) });
        }
        if (_node.isBool()) {
            return Filter::MatchExistence(_key, _node.getBool());
        }
        return Filter();
    }
}

Filter SceneLoader::generateAnyFilter(Node _filter, Scene& scene) {

    if (_filter.isSequence()) {
        std::vector<Filter> filters;

        for (const auto& filt : _filter.getSequence()) {
            if (Filter f = generateFilter(filt, scene)) {
                filters.push_back(std::move(f));
            } else { return Filter(); }
        }
        return Filter::MatchAny(std::move(filters));
    }
    return Filter();
}

Filter SceneLoader::generateAllFilter(Node _filter, Scene& scene) {

    if (_filter.isSequence()) {
        std::vector<Filter> filters;

        for (const auto& filt : _filter.getSequence()) {
            if (Filter f = generateFilter(filt, scene)) {
                filters.push_back(std::move(f));
            } else { return Filter(); }
        }
        return Filter::MatchAll(std::move(filters));
    }
    return Filter();
}

Filter SceneLoader::generateNoneFilter(Node _filter, Scene& scene) {

    if (_filter.isSequence()) {
        std::vector<Filter> filters;

        for (const auto& filt : _filter.getSequence()) {
            if (Filter f = generateFilter(filt, scene)) {
                filters.push_back(std::move(f));
            } else { return Filter(); }
        }
        return Filter::MatchNone(std::move(filters));

    } else {
        // 'not' case
        if (Filter f = generateFilter(_filter, scene)) {
            return Filter::MatchNone({std::move(f)});
        }
    }
    return Filter();
}

void SceneLoader::parseStyleParams(Node params, const std::shared_ptr<Scene>& scene, const std::string& prefix,
                                   std::vector<StyleParam>& out) {

    for (const auto& prop : params) {

        std::string key;
        if (!prefix.empty()) {
            key = prefix + DELIMITER + prop.first.Scalar();
        } else {
            key = prop.first.Scalar();
        }
        if (key == "transition" || key == "text:transition") {
            parseTransition(prop.second, scene, key, out);
            continue;
        }

        if (key == "text") {
            // Add StyleParam to signify that icon uses text
            out.push_back(StyleParam{ StyleParamKey::point_text, "" });
        }

        Node value = prop.second;

        if (value.isSequence()) {
            if (value[0].isSequence()) {
                auto styleKey = StyleParam::getKey(key);
                if (styleKey != StyleParamKey::none) {

                    if (StyleParam::isColor(styleKey)) {

                        scene->stops().push_back(Stops::Colors(value));
                        out.push_back(StyleParam{ styleKey, &(scene->stops().back()) });

                    } else if (StyleParam::isSize(styleKey)) {
                        scene->stops().push_back(Stops::Sizes(value, StyleParam::unitsForStyleParam(styleKey)));
                        out.push_back(StyleParam{ styleKey, &(scene->stops().back()) });

                    } else if (StyleParam::isWidth(styleKey)) {
                        scene->stops().push_back(Stops::Widths(value, *scene->mapProjection(),
                                                              StyleParam::unitsForStyleParam(styleKey)));

                        out.push_back(StyleParam{ styleKey, &(scene->stops().back()) });

                    } else if (StyleParam::isOffsets(styleKey)) {
                        scene->stops().push_back(Stops::Offsets(value, StyleParam::unitsForStyleParam(styleKey)));
                        out.push_back(StyleParam{ styleKey, &(scene->stops().back()) });

                    } else if (StyleParam::isFontSize(styleKey)) {
                        scene->stops().push_back(Stops::FontSize(value));
                        out.push_back(StyleParam{ styleKey, &(scene->stops().back()) });
                    }
                } else {
                    LOGW("Unknown style parameter %s", key.c_str());
                }

            } else {
                // TODO optimize for color values
                out.push_back(StyleParam{ key, parseSequence(value) });
            }
        } else if (value.isMapping()) {
            // NB: Flatten parameter map
            parseStyleParams(value, scene, key, out);
        } else {
            std::string str;
            if (value.isString()) {
              str = value.getString();
            } else if (value.isBool()) {
              str = std::to_string(value.getBool());
            } else if (value.isNumber()) {
              str = std::to_string(value.getDouble());
            }
            if (str.compare(0, 8, "function") == 0) {
              StyleParam param(key, "");
              param.function = scene->addJsFunction(str);
              out.push_back(std::move(param));
            } else {
              out.push_back(StyleParam{ key, str });
            }
        }
    }
}

bool SceneLoader::parseStyleUniforms(const Node& value, const std::shared_ptr<Scene>& scene, StyleUniform& styleUniform) {
    if (value.isSequence()) {
        size_t size = value.getSequenceCount();
        if (size > 0 && value[0].isString()) {
            UniformTextureArray uta;
            uta.names.reserve(size);
            styleUniform.type = "sampler2D";
            for (const auto& val : value.getSequence()) {
                std::string name = val.getStringOr(nullptr);
                uta.names.push_back(name);
                if (scene) {
                    auto texture = scene->getTexture(name);
                    if (!texture && !loadTexture(name, scene)) {
                        LOGW("Can't load texture with name '%s'", name.c_str());
                        return false;
                    }
                }
            }
            styleUniform.value = uta;
        } else {
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
              for (const auto& val : value.getSequence()) {
                if (val.isNumber()) {
                  uniformArray.push_back(val.getFloat());
                } else {
                  return false;
                }
              }
              styleUniform.value = std::move(uniformArray);
              break;
            }
            styleUniform.type = "vec" + std::to_string(size);
        }

    } else { // float, bool or string (texture)
        if (value.isNumber()) {
          styleUniform.type = "float";
          styleUniform.value = value.getFloat();
        } else if (value.isBool()) {
          styleUniform.type = "bool";
          styleUniform.value = value.getBool();
        } else if (value.isString()) {
          const std::string& strVal = value.getString();
          styleUniform.type = "sampler2D";

          if (scene) {
            std::shared_ptr<Texture> texture = scene->getTexture(strVal);

            if (!texture && !loadTexture(strVal, scene)) {
              LOGW("Can't load texture with name '%s'", strVal.c_str());
              return false;
            }
          }

          styleUniform.value = strVal;
        }
    }

    return true;
}

void SceneLoader::parseTransition(Node params, const std::shared_ptr<Scene>& scene, std::string _prefix, std::vector<StyleParam>& out) {

    for (const auto& prop : params.getMapping()) {
        std::string prefixedKey = _prefix + DELIMITER + prop.name.getString();
        for (auto child : prop.value.getMapping()) {
          auto childKey = prefixedKey + DELIMITER + child.name.getString();
          std::string childValue;
          if (child.value.isString()) {
              childValue = child.value.getString();
          } else if (child.value.isNumber()) {
              childValue = std::to_string(child.value.getDouble());
          }
          out.push_back(StyleParam{ childKey, childValue });
        }
    }
}

SceneLayer SceneLoader::loadSublayer(Node layer, const std::string& layerName, const std::shared_ptr<Scene>& scene) {

    std::vector<SceneLayer> sublayers;
    std::vector<DrawRuleData> rules;
    Filter filter;
    bool visible = true;

    for (const auto& member : layer.getMapping()) {

        const std::string& key = member.name.getString();

        if (key == "data") {
            // Ignored for sublayers
        } else if (key == "draw") {
            // Member is a mapping of draw rules
            for (auto& ruleNode : member.value.getMapping()) {

                std::vector<StyleParam> params;
                parseStyleParams(ruleNode.value, scene, "", params);

                const std::string& ruleName = ruleNode.name.getString();
                int ruleId = scene->addIdForName(ruleName);

                rules.push_back({ ruleName, ruleId, std::move(params) });
            }
        } else if (key == "filter") {
            filter = generateFilter(member.value, *scene);
            if (!filter.isValid()) {
                LOGNode("Invalid 'filter' in layer '%s'", member.second, layerName.c_str());
                return { layerName, {}, {}, {}, false };
            }
        } else if (key == "properties") {
            // TODO: ignored for now
        } else if (key == "visible") {
            visible = !member.value.isFalse();
        } else {
            // Member is a sublayer
            sublayers.push_back(loadSublayer(member.value, (layerName + DELIMITER + key), scene));
        }
    }

    return { layerName, std::move(filter), rules, std::move(sublayers), visible };
}

void SceneLoader::loadLayer(const Node::MappingMember& layer, const std::shared_ptr<Scene>& scene) {

    const std::string& name = layer.name.getString();

    std::string source;
    std::vector<std::string> collections;

    if (Node data = layer.value["data"]) {
        if (Node data_source = data["source"]) {
            if (data_source.isString()) {
                source = data_source.getString();
                auto dataSource = scene->getDataSource(source);
                if (dataSource) {
                    dataSource->generateGeometry(true);
                } else {
                    LOGW("Can't find data source %s for layer %s", source.c_str(), name.c_str());
                }
            }
        }

        if (Node data_layer = data["layer"]) {
            if (data_layer.isString()) {
                collections.push_back(data_layer.getString());
            } else if (data_layer.isSequence()) {
                for (const auto& entry : data_layer.getSequence()) {
                    collections.push_back(entry.getStringOr(nullptr));
                }
            }
        }
    }

    if (collections.empty()) {
        collections.push_back(name);
    }

    auto sublayer = loadSublayer(layer.value, name, scene);

    scene->layers().push_back({ std::move(sublayer), source, collections });
}

void SceneLoader::loadBackground(Node background, const std::shared_ptr<Scene>& scene) {

    if (!background) { return; }

    if (Node colorNode = background["color"]) {
        std::string str;
        if (colorNode.isString()) {
            str = colorNode.getString();
        } else if (colorNode.isSequence()) {
            str = parseSequence(colorNode);
        }
        scene->background().abgr = StyleParam::parseColor(str);
    }
}

}
