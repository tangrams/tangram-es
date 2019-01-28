#include "scene/sceneLoader.h"

#include "data/clientGeoJsonSource.h"
#include "data/memoryCacheDataSource.h"
#include "data/mbtilesDataSource.h"
#include "data/networkDataSource.h"
#include "data/rasterSource.h"
#include "data/tileSource.h"
#include "gl/shaderSource.h"
#include "gl/texture.h"
#include "log.h"
#include "platform.h"
#include "style/debugStyle.h"
#include "style/debugTextStyle.h"
#include "style/material.h"
#include "style/polygonStyle.h"
#include "style/polylineStyle.h"
#include "style/textStyle.h"
#include "style/pointStyle.h"
#include "style/rasterStyle.h"
#include "scene/dataLayer.h"
#include "scene/filters.h"
#include "scene/importer.h"
#include "scene/scene.h"
#include "scene/sceneLayer.h"
#include "scene/spriteAtlas.h"
#include "scene/lights.h"
#include "scene/stops.h"
#include "scene/styleMixer.h"
#include "scene/styleParam.h"
#include "util/base64.h"
#include "util/floatFormatter.h"
#include "util/yamlPath.h"
#include "util/yamlUtil.h"
#include "view/view.h"

#include "csscolorparser.hpp"
#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "glm/vec4.hpp"

#include <algorithm>
#include <cassert>
#include <iterator>
#include <regex>
#include <vector>

using YAML::Node;
using YAML::NodeType;
using YAML::BadConversion;

#define LOGNode(fmt, node, ...) LOGW(fmt ":\n'%s'\n", ## __VA_ARGS__, Dump(node).c_str())

namespace Tangram {

// TODO: make this configurable: 16MB default in-memory DataSource cache:
constexpr size_t CACHE_SIZE = 16 * (1024 * 1024);

static const std::string GLOBAL_PREFIX = "global.";

bool SceneLoader::loadScene(Platform& _platform, std::shared_ptr<Scene> _scene,
                            const std::vector<SceneUpdate>& _updates) {

    Importer sceneImporter(_scene);

    _scene->config() = sceneImporter.applySceneImports(_platform);

    if (!_scene->config()) {
        return false;
    }

    if (!applyUpdates(_platform, *_scene, _updates)) {
        LOGW("Scene updates failed when loading scene");
        return false;
    }

    // Load font resources
    _scene->fontContext()->loadFonts();

    applyConfig(_platform, _scene);

    return true;
}

bool SceneLoader::applyUpdates(Platform& platform, Scene& scene,
                               const std::vector<SceneUpdate>& updates) {
    auto& root = scene.config();

    for (const auto& update : updates) {
        Node value;

        try {
            value = YAML::Load(update.value);
        } catch (const YAML::ParserException& e) {
            LOGE("Parsing scene update string failed. '%s'", e.what());
            scene.errors.push_back({update, Error::scene_update_value_yaml_syntax_error});
            return false;
        }

        if (value) {
            Node node;
            bool pathIsValid = YamlPath(update.path).get(root, node);
            if (pathIsValid) {
                node = value;
            } else {
                scene.errors.push_back({update, Error::scene_update_path_not_found});
                return false;
            }
        }
    }

    Importer::resolveSceneUrls(root, scene.url());

    return true;
}

void printFilters(const SceneLayer& layer, int indent){
    LOG("%*s >>> %s\n", indent, "", layer.name().c_str());
    layer.filter().print(indent + 2);

    for (auto& l : layer.sublayers()) {
        printFilters(l, indent + 2);
    }
};

void createGlobalRefs(const Node& node, Scene& scene, YamlPathBuffer& path) {
    switch(node.Type()) {
    case NodeType::Scalar: {
            const auto& value = node.Scalar();
            if (value.length() > 7 && value.compare(0, 7, GLOBAL_PREFIX) == 0) {
                scene.globalRefs().emplace_back(path.toYamlPath(),
                                                YamlPath(value.substr(GLOBAL_PREFIX.length())));
            }
        }
        break;
    case NodeType::Sequence: {
            path.pushSequence();
            for (const auto& entry : node) {
                createGlobalRefs(entry, scene, path);
                path.increment();
            }
            path.pop();
        }
        break;
    case NodeType::Map:
        for (const auto& entry : node) {
            path.pushMap(&entry.first.Scalar());
            createGlobalRefs(entry.second, scene, path);
            path.pop();
        }
        break;
    default:
        break;
    }
}

void SceneLoader::applyGlobals(Node root, Scene& scene) {

    YamlPathBuffer path;
    createGlobalRefs(root, scene, path);
    const auto& globals = root["global"];
    if (!scene.globalRefs().empty() && !globals.IsMap()) {
        LOGW("Missing global references");
    }

    for (auto& globalRef : scene.globalRefs()) {
        Node target, global;
        bool targetPathIsValid = globalRef.first.get(root, target);
        bool globalPathIsValid = globalRef.second.get(globals, global);
        if (targetPathIsValid && globalPathIsValid && target.IsDefined() && global.IsDefined()) {
            target = global;
        } else {
            LOGW("Global reference is undefined: %s <= %s",
                 globalRef.first.codedPath.c_str(),
                 globalRef.second.codedPath.c_str());
        }
    }
}

bool SceneLoader::applyConfig(Platform& _platform, const std::shared_ptr<Scene>& _scene) {

    Node& config = _scene->config();

    // Instantiate built-in styles
    _scene->styles().emplace_back(new PolygonStyle("polygons"));
    _scene->styles().emplace_back(new PolylineStyle("lines"));
    _scene->styles().emplace_back(new DebugTextStyle("debugtext", _scene->fontContext(), true));
    _scene->styles().emplace_back(new TextStyle("text", _scene->fontContext(), true));
    _scene->styles().emplace_back(new DebugStyle("debug"));
    _scene->styles().emplace_back(new PointStyle("points", _scene->fontContext()));
    _scene->styles().emplace_back(new RasterStyle("raster"));

    if (config["global"]) {
        applyGlobals(config, *_scene);
    }


    if (Node sources = config["sources"]) {
        for (const auto& source : sources) {
            std::string srcName = source.first.Scalar();
            try { loadSource(_platform, srcName, source.second, sources, _scene); }
            catch (YAML::RepresentationException e) {
                LOGNode("Parsing sources: '%s'", source, e.what());
            }
        }
    } else {
        LOGW("No source defined in the yaml scene configuration.");
    }

    if (Node textures = config["textures"]) {
        for (const auto& texture : textures) {
            try { loadTexture(_platform, texture, _scene); }
            catch (YAML::RepresentationException e) {
                LOGNode("Parsing texture: '%s'", texture, e.what());
            }
        }
    }

    if (Node fonts = config["fonts"]) {
        if (fonts.IsMap()) {
            for (const auto& font : fonts) {
                try { loadFont(_platform, font, _scene); }
                catch (YAML::RepresentationException e) {
                    LOGNode("Parsing font: '%s'", font, e.what());
                }
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
                loadStyle(_platform, name, config, _scene);
            }
            catch (YAML::RepresentationException e) {
                LOGNode("Parsing style: '%s'", entry, e.what());
            }
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

        if (auto pointStyle = dynamic_cast<PointStyle*>(styles[i].get())) {
            pointStyle->setTextures(_scene->textures());
        }
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

    if (_scene->lights().empty()) {
        // Add an ambient light if nothing else is specified
        std::unique_ptr<AmbientLight> amb(new AmbientLight("defaultLight"));
        amb->setAmbientColor({ 1.f, 1.f, 1.f, 1.f });
        _scene->lights().push_back(std::move(amb));
    }

    _scene->lightBlocks() = Light::assembleLights(_scene->lights());

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
        _scene->animated(YamlUtil::getBoolOrDefault(animated, false));
    }

    for (auto& style : _scene->styles()) {
        style->build(*_scene);
    }

    return true;
}

void SceneLoader::loadShaderConfig(Platform& platform, Node shaders, Style& style,
                                   const std::shared_ptr<Scene>& scene) {

    if (!shaders) { return; }

    auto& shader = style.getShaderSource();

    if (Node extNode = shaders["extensions_mixed"]) {
        if (extNode.IsScalar()) {
            shader.addExtensionDeclaration(extNode.Scalar());
        } else if (extNode.IsSequence()) {
            for (const auto& e : extNode) {
                shader.addExtensionDeclaration(e.Scalar());
            }
        }
    }

    if (Node definesNode = shaders["defines"]) {
        for (const auto& define : definesNode) {
            const std::string& name = define.first.Scalar();

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

            if (YamlUtil::getBool(define.second, bValue)) {
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

            if (parseStyleUniforms(platform, uniform.second, scene, styleUniform)) {
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
    case NodeType::Sequence: {
        glm::vec4 vec;
        if (YamlUtil::parseVec<glm::vec4>(prop, vec)) {
            return vec;
        }
        break;
    }
    case NodeType::Scalar: {
        double value;
        if (YamlUtil::getDouble(prop, value, false)) {
            return glm::vec4(value, value, value, 1.0);
        } else {
            return YamlUtil::getColorAsVec4(prop);
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

void SceneLoader::loadMaterial(Platform& platform, Node matNode, Material& material,
                               const std::shared_ptr<Scene>& scene, Style& style) {

    if (!matNode.IsMap()) { return; }

    if (Node n = matNode["emission"]) {
        if (n.IsMap()) {
            material.setEmission(loadMaterialTexture(platform, n, scene, style));
        } else {
            material.setEmission(parseMaterialVec(n));
        }
    }
    if (Node n = matNode["diffuse"]) {
        if (n.IsMap()) {
            material.setDiffuse(loadMaterialTexture(platform, n, scene, style));
        } else {
            material.setDiffuse(parseMaterialVec(n));
        }
    }
    if (Node n = matNode["ambient"]) {
        if (n.IsMap()) {
            material.setAmbient(loadMaterialTexture(platform, n, scene, style));
        } else {
            material.setAmbient(parseMaterialVec(n));
        }
    }

    if (Node n = matNode["specular"]) {
        if (n.IsMap()) {
            material.setSpecular(loadMaterialTexture(platform, n, scene, style));
        } else {
            material.setSpecular(parseMaterialVec(n));
        }
    }

    if (Node shininess = matNode["shininess"]) {
        double value;
        if (YamlUtil::getDouble(shininess, value, false)) {
            material.setShininess(value);
        }
    }

    material.setNormal(loadMaterialTexture(platform, matNode["normal"], scene, style));
}

MaterialTexture SceneLoader::loadMaterialTexture(Platform& platform, Node matCompNode,
                                                 const std::shared_ptr<Scene>& scene, Style& style) {

    if (!matCompNode) { return MaterialTexture{}; }

    Node textureNode = matCompNode["texture"];
    if (!textureNode) {
        LOGNode("Expected a 'texture' parameter", matCompNode);

        return MaterialTexture{};
    }

    const std::string& name = textureNode.Scalar();

    MaterialTexture matTex;
    matTex.tex = getOrLoadTexture(platform, name, scene);

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
            matTex.scale.x = YamlUtil::getFloatOrDefault(scaleNode[0], matTex.scale.x);
            matTex.scale.y = YamlUtil::getFloatOrDefault(scaleNode[1], matTex.scale.y);
        } else if (scaleNode.IsScalar()) {
            matTex.scale = glm::vec3(YamlUtil::getFloatOrDefault(scaleNode, 1.f));
        } else {
            LOGW("Unrecognized scale parameter in material");
        }
    }

    if (Node amountNode = matCompNode["amount"]) {
        if (amountNode.IsScalar()) {
            matTex.amount = glm::vec3(YamlUtil::getFloatOrDefault(amountNode, 1.f));
        } else if (!YamlUtil::parseVec(amountNode, matTex.amount)) {
            LOGW("Unrecognized amount parameter in material");
        }
    }

    return matTex;
}

bool SceneLoader::parseTexFiltering(Node& filteringNode, TextureOptions& options) {
    if (!filteringNode.IsScalar()) {
        return false;
    }
    const std::string& filteringString = filteringNode.Scalar();
    if (filteringString == "linear") {
        options.minFilter = TextureMinFilter::LINEAR;
        options.magFilter = TextureMagFilter::LINEAR;
        return true;
    } else if (filteringString == "mipmap") {
        options.minFilter = TextureMinFilter::LINEAR_MIPMAP_LINEAR;
        options.generateMipmaps = true;
        return true;
    } else if (filteringString == "nearest") {
        options.minFilter = TextureMinFilter::NEAREST;
        options.magFilter = TextureMagFilter::NEAREST;
        return true;
    }
    return false;
}


std::shared_ptr<Texture> SceneLoader::fetchTexture(Platform& platform,
                                                   const std::string& name, const std::string& urlString,
                                                   const TextureOptions& options,
                                                   const std::shared_ptr<Scene>& scene,
                                                   std::unique_ptr<SpriteAtlas> _atlas) {
    std::shared_ptr<Texture> texture;

    Url url(urlString);

    if (url.hasBase64Data() && url.mediaType() == "image/png") {
        auto data = url.data();

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
        texture = std::make_shared<Texture>(options);

        if (!texture->loadImageFromMemory(blob.data(), blob.size())) {
            LOGE("Invalid Base64 texture");
        }
    } else {
        texture = std::make_shared<Texture>(options);
        texture->setSpriteAtlas(std::move(_atlas));

        scene->pendingTextures++;
        scene->startUrlRequest(platform, url, [&, url, scene, texture](UrlResponse&& response) {
                if (response.error) {
                    LOGE("Error retrieving URL '%s': %s", url.string().c_str(), response.error);
                } else {
                    if (texture) {
                        auto data = reinterpret_cast<const uint8_t*>(response.content.data());
                        auto length = response.content.size();
                        if (!texture->loadImageFromMemory(data, length)) {
                            LOGE("Invalid texture data from URL '%s'", url.string().c_str());
                        }
                        if (texture->spriteAtlas()) {
                            texture->spriteAtlas()->updateSpriteNodes({texture->width(), texture->height()});
                        }
                    }
                }
                scene->pendingTextures--;
                if (scene->pendingTextures == 0) {
                    platform.requestRender();
                }
            });
    }

    return texture;
}

std::shared_ptr<Texture> SceneLoader::getOrLoadTexture(Platform& platform,
                                                       const std::string& name, const std::shared_ptr<Scene>& scene) {

    auto entry = scene->textures().find(name);
    if (entry != scene->textures().end()) {
        return entry->second;
    }

    // If texture could not be found by name then interpret name as URL
    TextureOptions options;
    auto texture = fetchTexture(platform, name, name, options, scene);

    scene->textures().emplace(name, texture);

    return texture;
}

void SceneLoader::loadTexture(Platform& platform, const std::pair<Node, Node>& node,
                              const std::shared_ptr<Scene>& scene) {

    const std::string& name = node.first.Scalar();
    const Node& textureConfig = node.second;

    if (!textureConfig.IsMap()) {
        LOGW("Invalid texture node '%s', skipping.", name.c_str());
        return;
    }

    std::string url;
    TextureOptions options;

    if (Node urlNode = textureConfig["url"]) {
        if (urlNode.IsScalar()) {
            url = urlNode.Scalar();
        }
    }
    if (url.empty()){
        LOGW("No url specified for texture '%s', skipping.", name.c_str());
        return;
    }

    if (Node filtering = textureConfig["filtering"]) {
        if (!parseTexFiltering(filtering, options)) {
            LOGW("Invalid texture filtering: %s", Dump(filtering).c_str());
        }
    }

    if (Node density = textureConfig["density"]) {
        options.displayScale = 1.f / YamlUtil::getFloatOrDefault(density, 1.f);
    }

    std::unique_ptr<SpriteAtlas> atlas;
    if (Node sprites = textureConfig["sprites"]) {
        atlas = std::make_unique<SpriteAtlas>();

        for (auto it = sprites.begin(); it != sprites.end(); ++it) {

            const Node sprite = it->second;
            const std::string& spriteName = it->first.Scalar();

            if (sprite) {
                glm::vec4 desc;
                YamlUtil::parseVec<glm::vec4>(sprite, desc);
                glm::vec2 pos = glm::vec2(desc.x, desc.y);
                glm::vec2 size = glm::vec2(desc.z, desc.w);

                atlas->addSpriteNode(spriteName, pos, size);
            }
        }
    }
    auto texture = fetchTexture(platform, name, url, options, scene, std::move(atlas));

    scene->textures().emplace(name, texture);
}

void loadFontDescription(Platform& platform, const Node& node,
                         const std::string& family, const std::shared_ptr<Scene>& scene) {
    if (!node.IsMap()) {
        LOGW("");
        return;
    }
    std::string style = "regular", weight = "400", uri;

    for (const auto& fontDesc : node) {
        const std::string& key = fontDesc.first.Scalar();
        if (key == "weight") {
            weight = fontDesc.second.Scalar();
        } else if (key == "style") {
            style = fontDesc.second.Scalar();
        } else if (key == "url") {
            uri = fontDesc.second.Scalar();
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

    FontDescription _ft(familyNormalized, styleNormalized, weight, uri);

    Url url(uri);

    // Load font file.
    scene->pendingFonts++;
    scene->startUrlRequest(platform, url, [_ft, scene](UrlResponse&& response) {
        if (response.error) {
            LOGE("Error retrieving font '%s' at %s: ", _ft.alias.c_str(), _ft.uri.c_str(), response.error);
        } else {
            scene->fontContext()->addFont(_ft, alfons::InputSource(std::move(response.content)));
        }
        scene->pendingFonts--;
    });
}

void SceneLoader::loadFont(Platform& platform, const std::pair<Node, Node>& font,
                           const std::shared_ptr<Scene>& scene) {
    const std::string& family = font.first.Scalar();

    if (font.second.IsMap()) {
        loadFontDescription(platform, font.second, family, scene);
    } else if (font.second.IsSequence()) {
        for (const auto& node : font.second) {
            loadFontDescription(platform, node, family, scene);
        }
    }
}

void SceneLoader::loadStyleProps(Platform& platform, Style& style, Node styleNode,
                                 const std::shared_ptr<Scene>& scene) {

    if (!styleNode) {
        LOGW("Can not parse style parameters, bad style YAML Node");
        return;
    }

    if (Node animatedNode = styleNode["animated"]) {
        if (!animatedNode.IsScalar()) { LOGW("animated flag should be a scalar"); }
        else {
            bool animate;
            if (YamlUtil::getBool(animatedNode, animate)) {
                style.setAnimated(animate);
            }
        }
    }

    if (Node blendNode = styleNode["blend"]) {
        const std::string& blendMode = blendNode.Scalar();
        if      (blendMode == "opaque")      { style.setBlendMode(Blending::opaque); }
        else if (blendMode == "add")         { style.setBlendMode(Blending::add); }
        else if (blendMode == "multiply")    { style.setBlendMode(Blending::multiply); }
        else if (blendMode == "overlay")     { style.setBlendMode(Blending::overlay); }
        else if (blendMode == "inlay")       { style.setBlendMode(Blending::inlay); }
        else if (blendMode == "translucent") { style.setBlendMode(Blending::translucent); }
        else { LOGW("Invalid blend mode '%s'", blendMode.c_str()); }
    }

    if (Node blendOrderNode = styleNode["blend_order"]) {
        int blendOrderValue;
        if (YamlUtil::getInt(blendOrderNode, blendOrderValue)) {
            style.setBlendOrder(blendOrderValue);
        } else {
            LOGE("Integral value expected for blend_order style parameter.\n");
        }
    }

    if (Node texcoordsNode = styleNode["texcoords"]) {
        bool boolValue;
        if (YamlUtil::getBool(texcoordsNode, boolValue)) {
            style.setTexCoordsGeneration(boolValue);
        }
    }

    if (Node dashNode = styleNode["dash"]) {
        if (auto polylineStyle = dynamic_cast<PolylineStyle*>(&style)) {
            if (dashNode.IsSequence()) {
                std::vector<float> dashValues;
                dashValues.reserve(dashNode.size());
                for (const auto& dashValue : dashNode) {
                    float floatValue;
                    if (YamlUtil::getFloat(dashValue, floatValue)) {
                        dashValues.push_back(floatValue);
                    }
                }
                polylineStyle->setDashArray(dashValues);
                polylineStyle->setTexCoordsGeneration(true);
            }
        }
    }

    if (Node dashBackgroundColor = styleNode["dash_background_color"]) {
        if (auto polylineStyle = dynamic_cast<PolylineStyle*>(&style)) {
            glm::vec4 backgroundColor = YamlUtil::getColorAsVec4(dashBackgroundColor);
            polylineStyle->setDashBackgroundColor(backgroundColor);
        }
    }

    if (Node shadersNode = styleNode["shaders"]) {
        loadShaderConfig(platform, shadersNode, style, scene);
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
            auto styleTexture = scene->getTexture(textureName);
            if (styleTexture) {
                pointStyle->setDefaultTexture(styleTexture);
            } else {
                LOGW("Undefined texture name %s", textureName.c_str());
            }
        } else if (auto polylineStyle = dynamic_cast<PolylineStyle*>(&style)) {
            const std::string& textureName = textureNode.Scalar();
            auto texture = scene->getTexture(textureName);
            if (texture) {
                polylineStyle->setTexture(texture);
                polylineStyle->setTexCoordsGeneration(true);
            }
        }
    }

    if (Node materialNode = styleNode["material"]) {
        loadMaterial(platform, materialNode, style.getMaterial(), scene, style);
    }

    if (const Node& drawNode = styleNode["draw"]) {
        std::vector<StyleParam> params;
        int ruleID = scene->addIdForName(style.getName());
        parseStyleParams(drawNode, scene, "", params);
        /*Note:  ruleID and name is immaterial here, as these are only used for rule merging, but
         * style's default styling rules are applied post rule merging for any style parameter which
         * was not assigned during merging step.
         */
        auto rule = std::make_unique<DrawRuleData>(style.getName(), ruleID, std::move(params));
        style.setDefaultDrawRule(std::move(rule));
    }

}

bool SceneLoader::loadStyle(Platform& platform, const std::string& name,
                            Node config, const std::shared_ptr<Scene>& scene) {

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
        const auto& raster = rasterNode.Scalar();
        if (raster == "normal") {
            style->setRasterType(RasterType::normal);
        } else if (raster == "color") {
            style->setRasterType(RasterType::color);
        } else if (raster == "custom") {
            style->setRasterType(RasterType::custom);
        }
    }

    loadStyleProps(platform, *style.get(), config, scene);

    scene->styles().push_back(std::move(style));

    return true;
}

void SceneLoader::loadSource(Platform& platform, const std::string& name,
                             const Node& source, const Node& sources, const std::shared_ptr<Scene>& _scene) {
    if (_scene->getTileSource(name)) {
        LOGW("Duplicate TileSource: %s", name.c_str());
        return;
    }

    std::string type;
    std::string url;
    std::string mbtiles;
    std::vector<std::string> subdomains;

    int32_t minDisplayZoom = -1;
    int32_t maxDisplayZoom = -1;
    int32_t maxZoom = 18;
    int32_t zoomBias = 0;
    bool generateCentroids = false;

    if (auto typeNode = source["type"]) {
        type = typeNode.Scalar();
    }
    if (auto urlNode = source["url"]) {
        url = urlNode.Scalar();
    }
    if (auto minDisplayZoomNode = source["min_display_zoom"]) {
        YamlUtil::getInt(minDisplayZoomNode, minDisplayZoom);
    }
    if (auto maxDisplayZoomNode = source["max_display_zoom"]) {
        YamlUtil::getInt(maxDisplayZoomNode, maxDisplayZoom);
    }
    if (auto maxZoomNode = source["max_zoom"]) {
        YamlUtil::getInt(maxZoomNode, maxZoom);
    }
    if (auto tileSizeNode = source["tile_size"]) {
        int tileSize = 0;
        if (YamlUtil::getInt(tileSizeNode, tileSize)) {
            zoomBias = TileSource::zoomBiasFromTileSize(tileSize);
        }
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

    // Apply URL subdomain configuration.
    if (Node subDomainNode = source["url_subdomains"]) {
        if (subDomainNode.IsSequence()) {
            for (const auto& domain : subDomainNode) {
                if (domain.IsScalar()) {
                    subdomains.push_back(domain.Scalar());
                }
            }
        }
    }

    // Check whether the URL template and subdomains make sense together, and warn if not.
    bool hasSubdomainPlaceholder = (url.find("{s}") != std::string::npos);
    if (hasSubdomainPlaceholder && subdomains.empty()) {
        LOGW("The URL for source '%s' includes the subdomain placeholder '{s}', but no subdomains were given.", name.c_str());
    }
    if (!hasSubdomainPlaceholder && !subdomains.empty()) {
        LOGW("The URL for source '%s' has subdomains specified, but does not include the subdomain placeholder '{s}'.", name.c_str());
    }

    // distinguish tiled and non-tiled sources by url
    bool tiled = url.size() > 0 &&
        url.find("{x}") != std::string::npos &&
        url.find("{y}") != std::string::npos &&
        url.find("{z}") != std::string::npos;

    bool isMBTilesFile = false;
    {
        const char* extStr = ".mbtiles";
        const size_t extLength = strlen(extStr);
        const size_t urlLength = url.length();
        isMBTilesFile = urlLength > extLength && (url.compare(urlLength - extLength, extLength, extStr) == 0);
    }

    bool isTms = false;
    if (auto tmsNode = source["tms"]) {
        YamlUtil::getBool(tmsNode, isTms);
    }

    auto rawSources = std::make_unique<MemoryCacheDataSource>();
    rawSources->setCacheSize(CACHE_SIZE);

    if (isMBTilesFile) {
#ifdef TANGRAM_MBTILES_DATASOURCE
        // If we have MBTiles, we know the source is tiled.
        tiled = true;
        // Create an MBTiles data source from the file at the url and add it to the source chain.
        rawSources->setNext(std::make_unique<MBTilesDataSource>(platform, name, url, ""));
#else
        LOGE("MBTiles support is disabled. This source will be ignored: %s", name.c_str());
        return;
#endif
    } else if (tiled) {
        rawSources->setNext(std::make_unique<NetworkDataSource>(platform, url, std::move(subdomains), isTms));
    }

    std::shared_ptr<TileSource> sourcePtr;

    TileSource::ZoomOptions zoomOptions = { minDisplayZoom, maxDisplayZoom, maxZoom, zoomBias };

    if (type == "GeoJSON" && !tiled) {
        if (auto genLabelCentroidsNode = source["generate_label_centroids"]) {
            generateCentroids = true;
        }
        sourcePtr = std::make_shared<ClientGeoJsonSource>(platform, name, url, generateCentroids,
                                                          zoomOptions);
    } else if (type == "Raster") {
        TextureOptions options;
        if (Node filtering = source["filtering"]) {
            if (!parseTexFiltering(filtering, options)) {
                LOGW("Invalid texture filtering: %s", Dump(filtering).c_str());
            }
        }

        sourcePtr = std::make_shared<RasterSource>(name, std::move(rawSources), options, zoomOptions);
    } else {
        sourcePtr = std::make_shared<TileSource>(name, std::move(rawSources), zoomOptions);

        if (type == "GeoJSON") {
            sourcePtr->setFormat(TileSource::Format::GeoJson);
        } else if (type == "TopoJSON") {
            sourcePtr->setFormat(TileSource::Format::TopoJson);
        } else if (type == "MVT") {
            sourcePtr->setFormat(TileSource::Format::Mvt);
        } else {
            LOGE("Source '%s' does not have a valid type. Valid types are 'GeoJSON', 'TopoJSON', and 'MVT'. " \
                "This source will be ignored.", name.c_str());
            return;
        }
    }

    _scene->tileSources().push_back(sourcePtr);

    if (auto rasters = source["rasters"]) {
        loadSourceRasters(platform, sourcePtr, source["rasters"], sources, _scene);
    }

}

void SceneLoader::loadSourceRasters(Platform& platform, std::shared_ptr<TileSource> &source,
                                    Node rasterNode, const Node& sources, const std::shared_ptr<Scene>& scene) {
    if (rasterNode.IsSequence()) {
        for (const auto& raster : rasterNode) {
            std::string srcName = raster.Scalar();
            try {
                loadSource(platform, srcName, sources[srcName], sources, scene);
            } catch (YAML::RepresentationException e) {
                LOGNode("Parsing sources: '%s'", sources[srcName], e.what());
                return;
            }
            source->addRasterSource(scene->getTileSource(srcName));
        }
    }
}

void SceneLoader::parseLightPosition(Node positionNode, PointLight& light) {
    UnitVec<glm::vec3> positionResult;
    if (StyleParam::parseVec3(positionNode, UnitSet{Unit::none, Unit::pixel, Unit::meter}, positionResult)) {
        for (auto& unit : positionResult.units) {
            if (unit == Unit::none) {
                unit = Unit::meter;
            }
        }
        light.setPosition(positionResult);
    } else {
        LOGNode("Invalid light position parameter:", positionNode);
    }
}

void SceneLoader::loadLight(const std::pair<Node, Node>& node, const std::shared_ptr<Scene>& scene) {

    const Node light = node.second;
    const std::string& name = node.first.Scalar();
    const std::string& type = light["type"].Scalar();

    if (Node visible = light["visible"]) {
        // If 'visible' is false, skip loading this light.
        if (!YamlUtil::getBoolOrDefault(visible, true)) { return; }
    }

    std::unique_ptr<Light> sceneLight;

    if (type == "ambient") {
        sceneLight = std::make_unique<AmbientLight>(name);

    } else if (type == "directional") {
        auto dLight(std::make_unique<DirectionalLight>(name));

        if (Node directionNode = light["direction"]) {
            glm::vec3 direction;
            if (YamlUtil::parseVec<glm::vec3>(directionNode, direction)) {
                dLight->setDirection(direction);
            }
        }
        sceneLight = std::move(dLight);

    } else if (type == "point") {
        auto pLight(std::make_unique<PointLight>(name));

        if (Node position = light["position"]) {
            parseLightPosition(position, *pLight);
        }
        if (Node radius = light["radius"]) {
            if (radius.size() > 1) {
                pLight->setRadius(YamlUtil::getFloatOrDefault(radius[0], 0.f), YamlUtil::getFloatOrDefault(radius[1], 0.f));
            } else {
                pLight->setRadius(YamlUtil::getFloatOrDefault(radius, 0.f));
            }
        }
        if (Node att = light["attenuation"]) {
            float attenuation;
            if (YamlUtil::getFloat(att, attenuation)) {
                pLight->setAttenuation(attenuation);
            }
        }
        sceneLight = std::move(pLight);

    } else if (type == "spotlight") {
        auto sLight(std::make_unique<SpotLight>(name));

        if (Node position = light["position"]) {
            parseLightPosition(position, *sLight);
        }
        if (Node directionNode = light["direction"]) {
            glm::vec3 direction;
            if (YamlUtil::parseVec<glm::vec3>(directionNode, direction)) {
                sLight->setDirection(direction);
            }
        }
        if (Node radius = light["radius"]) {
            if (radius.size() > 1) {
                sLight->setRadius(YamlUtil::getFloatOrDefault(radius[0], 0.f), YamlUtil::getFloatOrDefault(radius[1], 0.f));
            } else {
                sLight->setRadius(YamlUtil::getFloatOrDefault(radius, 0.f));
            }
        }
        if (Node angle = light["angle"]) {
            sLight->setCutoffAngle(YamlUtil::getFloatOrDefault(angle, 0.f));
        }
        if (Node exponent = light["exponent"]) {
            sLight->setCutoffExponent(YamlUtil::getFloatOrDefault(exponent, 0.f));
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
        sceneLight->setAmbientColor(YamlUtil::getColorAsVec4(ambient));
    }
    if (Node diffuse = light["diffuse"]) {
        sceneLight->setDiffuseColor(YamlUtil::getColorAsVec4(diffuse));
    }
    if (Node specular = light["specular"]) {
        sceneLight->setSpecularColor(YamlUtil::getColorAsVec4(specular));
    }

    // Verify that light position parameters are consistent with the origin type
    if (sceneLight->getType() == LightType::point || sceneLight->getType() == LightType::spot) {
        auto pLight = static_cast<PointLight&>(*sceneLight);
        auto lightPosition = pLight.getPosition();
        LightOrigin origin = pLight.getOrigin();

        if (origin == LightOrigin::world) {
            if (lightPosition.units[0] == Unit::pixel || lightPosition.units[1] == Unit::pixel) {
                LOGW("Light position with attachment %s may not be used with unit of type %s",
                    lightOriginString(origin).c_str(), unitToString(Unit::pixel).c_str());
                LOGW("Long/Lat expected in meters");
            }
        }
    }

    scene->lights().push_back(std::move(sceneLight));
}

void SceneLoader::loadCamera(const Node& _camera, const std::shared_ptr<Scene>& _scene) {

    auto& camera = _scene->camera();

    if (Node active = _camera["active"]) {
        if (!YamlUtil::getBoolOrDefault(active, false)) {
            return;
        }
    }

    auto type = _camera["type"].Scalar();
    if (type == "perspective") {
        camera.type = CameraType::perspective;

        // Only one of focal length and FOV is applied;
        // according to docs, focal length takes precedence.
        if (Node focal = _camera["focal_length"]) {
            float floatValue;
            if (YamlUtil::getFloat(focal, floatValue)) {
                camera.fieldOfView = View::focalLengthToFieldOfView(floatValue);
            } else if (focal.IsSequence()) {
                camera.fovStops = std::make_shared<Stops>(Stops::Numbers(focal));
                for (auto& f : camera.fovStops->frames) {
                    f.value = View::focalLengthToFieldOfView(f.value.get<float>());
                }
            }
        } else if (Node fov = _camera["fov"]) {
            if (fov.IsScalar()) {
                double degrees = YamlUtil::getDoubleOrDefault(fov, camera.fieldOfView * RAD_TO_DEG);
                camera.fieldOfView = degrees * DEG_TO_RAD;
            } else if (fov.IsSequence()) {
                camera.fovStops = std::make_shared<Stops>(Stops::Numbers(fov));
                for (auto& f : camera.fovStops->frames) {
                    f.value = float(f.value.get<float>() * DEG_TO_RAD);
                }
            }
        }

        if (Node vanishing = _camera["vanishing_point"]) {
            if (vanishing.IsSequence() && vanishing.size() >= 2) {
                // Values are pixels, unit strings are ignored.
                camera.vanishingPoint.x = YamlUtil::getFloatOrDefault(vanishing[0], 0.f, true);
                camera.vanishingPoint.y = YamlUtil::getFloatOrDefault(vanishing[1], 0.f, true);
            }
        }
    } else if (type == "isometric") {
        camera.type = CameraType::isometric;

        if (Node axis = _camera["axis"]) {
            YamlUtil::parseVec(axis, camera.obliqueAxis);
        }
    } else if (type == "flat") {
        camera.type = CameraType::flat;
    }

    // Default is world origin at 0 zoom
    double x = 0;
    double y = 0;
    float z = 0;

    if (Node position = _camera["position"]) {
        x = YamlUtil::getDoubleOrDefault(position[0], x);
        y = YamlUtil::getDoubleOrDefault(position[1], y);
        if (position.size() > 2) {
            z = YamlUtil::getFloatOrDefault(position[2], z);
        }
    }

    if (Node zoom = _camera["zoom"]) {
        z = YamlUtil::getFloatOrDefault(zoom, z);
    }

    if (Node maxTilt = _camera["max_tilt"]) {
        if (maxTilt.IsSequence()) {
            camera.maxTiltStops = std::make_shared<Stops>(Stops::Numbers(maxTilt));
        } else if (maxTilt.IsScalar()) {
            camera.maxTilt = YamlUtil::getFloatOrDefault(maxTilt, PI);
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

    for (const auto& entry : _cameras) {
        loadCamera(entry.second, _scene);
    }
}

Filter SceneLoader::generateFilter(Node _filter, Scene& scene) {

    switch (_filter.Type()) {
    case NodeType::Scalar: {

        const std::string& val = _filter.Scalar();
        if (val.compare(0, 8, "function") == 0) {
            return Filter::MatchFunction(scene.addJsFunction(val));
        }
        return Filter();
    }
    case NodeType::Sequence: {
        return generateAnyFilter(_filter, scene);
    }
    case NodeType::Map: {
        std::vector<Filter> filters;
        for (const auto& filtItr : _filter) {
            const std::string& key = filtItr.first.Scalar();
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
    }
    default:
        return Filter();
    }
}

Filter SceneLoader::generatePredicate(Node _node, std::string _key) {

    switch (_node.Type()) {
    case NodeType::Scalar: {
        if (_node.Tag() == "tag:yaml.org,2002:str") {
            // Node was explicitly tagged with '!!str' or the canonical tag
            // 'tag:yaml.org,2002:str' yaml-cpp normalizes the tag value to the
            // canonical form
            return Filter::MatchEquality(_key, { Value(_node.Scalar()) });
        }
        double number;
        if (YamlUtil::getDouble(_node, number, false)) {
            return Filter::MatchEquality(_key, { Value(number) });
        }
        bool existence;
        if (YamlUtil::getBool(_node, existence)) {
            return Filter::MatchExistence(_key, existence);
        }
        const std::string& value = _node.Scalar();
        return Filter::MatchEquality(_key, { Value(std::move(value)) });
    }
    case NodeType::Sequence: {
        std::vector<Value> values;
        for (const auto& valItr : _node) {
            double number;
            if (YamlUtil::getDouble(valItr, number, false)) {
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
        bool hasMinPixelArea = false;
        bool hasMaxPixelArea = false;

        for (const auto& n : _node) {
            if (n.first.Scalar() == "min") {
                if(!getFilterRangeValue(n.second, minVal, hasMinPixelArea)) {
                    return Filter();
                }
            } else if (n.first.Scalar() == "max") {
                if (!getFilterRangeValue(n.second, maxVal, hasMaxPixelArea)) {
                    return Filter();
                }
            }
        }

        if (_node["max"].IsScalar() && _node["min"].IsScalar() &&
                (hasMinPixelArea != hasMaxPixelArea)) { return Filter(); }

        return Filter::MatchRange(_key, minVal, maxVal, hasMinPixelArea | hasMaxPixelArea);
    }
    default:
        return Filter();
    }
}

bool SceneLoader::getFilterRangeValue(const Node& node, double& val, bool& hasPixelArea) {
    if (!YamlUtil::getDouble(node, val, false)) {
        auto strVal = node.Scalar();
        auto n = strVal.find("px2");
        if (n == std::string::npos) { return false; }
        try {
            val = ff::stof(std::string(strVal, 0, n));
            hasPixelArea = true;
        } catch (std::invalid_argument) { return false; }
    }
    return true;
}

Filter SceneLoader::generateAnyFilter(Node _filter, Scene& scene) {

    if (_filter.IsSequence()) {
        std::vector<Filter> filters;

        for (const auto& filt : _filter) {
            if (Filter f = generateFilter(filt, scene)) {
                filters.push_back(std::move(f));
            } else { return Filter(); }
        }
        return Filter::MatchAny(std::move(filters));
    }
    return Filter();
}

Filter SceneLoader::generateAllFilter(Node _filter, Scene& scene) {

    if (_filter.IsSequence()) {
        std::vector<Filter> filters;

        for (const auto& filt : _filter) {
            if (Filter f = generateFilter(filt, scene)) {
                filters.push_back(std::move(f));
            } else { return Filter(); }
        }
        return Filter::MatchAll(std::move(filters));
    }
    return Filter();
}

Filter SceneLoader::generateNoneFilter(Node _filter, Scene& scene) {

    if (_filter.IsSequence()) {
        std::vector<Filter> filters;

        for (const auto& filt : _filter) {
            if (Filter f = generateFilter(filt, scene)) {
                filters.push_back(std::move(f));
            } else { return Filter(); }
        }
        return Filter::MatchNone(std::move(filters));

    } else if (_filter.IsMap() || _filter.IsScalar()) {
        // 'not' case
        if (Filter f = generateFilter(_filter, scene)) {
            return Filter::MatchNone({std::move(f)});
        }
    }
    return Filter();
}

void SceneLoader::parseStyleParams(Node params, const std::shared_ptr<Scene>& scene,
                                   const std::string& prefix, std::vector<StyleParam>& out) {

    for (const auto& prop : params) {

        std::string key;
        if (!prefix.empty()) {
            key = prefix + DELIMITER + prop.first.Scalar();
        } else {
            key = prop.first.Scalar();
        }
        Node value = prop.second;

        if (key == "transition" || key == "text:transition") {
            parseTransition(prop.second, scene, key, out);
            continue;
        }

        if (key == "texture") {
            out.push_back(StyleParam{ StyleParamKey::texture, value });
            continue;
        }

        if (key == "text") {
            // Add StyleParam to signify that icon uses text
            out.push_back(StyleParam{ StyleParamKey::point_text, "" });
        }

        switch (value.Type()) {
        case NodeType::Scalar: {
            const std::string& val = value.Scalar();

            if (val.compare(0, 8, "function") == 0) {
                StyleParam param(key);
                param.function = scene->addJsFunction(val);
                out.push_back(std::move(param));
            } else {
                out.push_back(StyleParam{ key, value });
            }
            break;
        }
        case NodeType::Sequence: {
            if (value[0].IsSequence()) {
                auto styleKey = StyleParam::getKey(key);
                if (styleKey != StyleParamKey::none) {

                    if (StyleParam::isColor(styleKey)) {
                        scene->stops().push_back(Stops::Colors(value));
                        out.push_back(StyleParam{ styleKey, &(scene->stops().back()) });
                    } else if (StyleParam::isSize(styleKey)) {
                        scene->stops().push_back(Stops::Sizes(value, StyleParam::unitSetForStyleParam(styleKey)));
                        out.push_back(StyleParam{ styleKey, &(scene->stops().back()) });
                    } else if (StyleParam::isWidth(styleKey)) {
                        scene->stops().push_back(Stops::Widths(value, StyleParam::unitSetForStyleParam(styleKey)));
                        out.push_back(StyleParam{ styleKey, &(scene->stops().back()) });
                    } else if (StyleParam::isOffsets(styleKey)) {
                        scene->stops().push_back(Stops::Offsets(value, StyleParam::unitSetForStyleParam(styleKey)));
                        out.push_back(StyleParam{ styleKey, &(scene->stops().back()) });
                    } else if (StyleParam::isFontSize(styleKey)) {
                        scene->stops().push_back(Stops::FontSize(value));
                        out.push_back(StyleParam{ styleKey, &(scene->stops().back()) });
                    } else if (StyleParam::isNumberType(styleKey)) {
                        scene->stops().push_back(Stops::Numbers(value));
                        out.push_back(StyleParam{ styleKey, &(scene->stops().back()) });
                    }
                } else {
                    LOGW("Unknown style parameter %s", key.c_str());
                }

            } else {
                out.push_back(StyleParam{ key, value });
            }
            break;
        }
        case NodeType::Map: {
            // NB: Flatten parameter map
            parseStyleParams(value, scene, key, out);

            break;
        }
        case NodeType::Null: {
            // Handles the case, when null style param value is used to unset a merged style param
            out.emplace_back(StyleParam::getKey(key));
            break;
        }
        default:
            LOGW("Style parameter %s must be a scalar, sequence, or map.", key.c_str());
        }
    }
}

bool SceneLoader::parseStyleUniforms(Platform& platform, const Node& value,
                                     const std::shared_ptr<Scene>& scene, StyleUniform& styleUniform) {
    if (value.IsScalar()) { // float, bool or string (texture)
        double fValue;
        bool bValue;

        if (YamlUtil::getDouble(value, fValue, false)) {
            styleUniform.type = "float";
            styleUniform.value = (float)fValue;
        } else if (YamlUtil::getBool(value, bValue)) {
            styleUniform.type = "bool";
            styleUniform.value = (bool)bValue;
        } else {
            const std::string& strVal = value.Scalar();
            styleUniform.type = "sampler2D";
            styleUniform.value = strVal;

            getOrLoadTexture(platform, strVal, scene);
        }
    } else if (value.IsSequence()) {
        size_t size = value.size();
        bool parsed = false;
        switch (size) {
            case 2: {
                glm::vec2 vec;
                if (YamlUtil::parseVec<glm::vec2>(value, vec)) {
                    styleUniform.value = vec;
                    styleUniform.type = "vec2";
                    parsed = true;
                }
                break;
            }
            case 3: {
                glm::vec3 vec;
                if (YamlUtil::parseVec(value, vec)) {
                    styleUniform.value = vec;
                    styleUniform.type = "vec3";
                    parsed = true;
                }
                break;
            }
            case 4: {
                glm::vec4 vec;
                if (YamlUtil::parseVec(value, vec)) {
                    styleUniform.value = vec;
                    styleUniform.type = "vec4";
                    parsed = true;
                }
                break;
            }
            default: {
                UniformArray1f uniformArray;
                for (const auto& val : value) {
                    double fValue;
                    if (YamlUtil::getDouble(val, fValue)) {
                        uniformArray.push_back(fValue);
                    } else {
                        return false;
                    }
                }
                styleUniform.value = std::move(uniformArray);
                styleUniform.type = "vec" + std::to_string(size);
                parsed = true;
                break;
            }
        }

        if (!parsed) {
            // array of strings (textures)
            UniformTextureArray textureArrayUniform;
            textureArrayUniform.names.reserve(size);
            styleUniform.type = "sampler2D";

            for (const auto& strVal : value) {
                const std::string& textureName = strVal.Scalar();
                textureArrayUniform.names.push_back(textureName);

                getOrLoadTexture(platform, textureName, scene);
            }

            styleUniform.value = std::move(textureArrayUniform);
        }
    } else {
        LOGW("Expected a scalar or sequence value for uniforms");
        return false;
    }

    return true;
}

void SceneLoader::parseTransition(Node params, const std::shared_ptr<Scene>& scene, std::string _prefix,
                                  std::vector<StyleParam>& out) {

    // First iterate over the mapping of 'events', we currently recognize 'hide', 'selected', and 'show'.
    for (const auto& event : params) {
        if (!event.first.IsScalar() || !event.second.IsMap()) {
            LOGW("Can't parse 'transitions' entry, expected a mapping of strings to mappings at: %s", _prefix.c_str());
            continue;
        }

        // Add the event to our key, so it's now 'transition:event'.
        std::string transitionEvent = _prefix + DELIMITER + event.first.Scalar();

        // Iterate over the parameters in the 'event', we currently only recognize 'time'.
        for (const auto& param : event.second) {
            if (!param.first.IsScalar() || !param.second.IsScalar()) {
                LOGW("Expected a mapping of strings to strings or numbers in: %s", transitionEvent.c_str());
                continue;
            }
            // Add the parameter to our key, so it's now 'transition:event:param'.
            std::string transitionEventParam = transitionEvent + DELIMITER + param.first.Scalar();
            // Create a style parameter from the key and value.
            out.push_back(StyleParam{ transitionEventParam, param.second });
        }
    }
}

SceneLayer SceneLoader::loadSublayer(const Node& layer, const std::string& layerName,
                                     const std::shared_ptr<Scene>& scene) {

    std::vector<SceneLayer> sublayers;
    std::vector<DrawRuleData> rules;
    Filter filter;
    bool enabled = true;

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
                int ruleId = scene->addIdForName(ruleName);

                rules.push_back({ ruleName, ruleId, std::move(params) });
            }
        } else if (key == "filter") {
            filter = generateFilter(member.second, *scene);
            if (!filter.isValid()) {
                LOGNode("Invalid 'filter' in layer '%s'", member.second, layerName.c_str());
                return { layerName, {}, {}, {}, false };
            }
        } else if (key == "visible") {
            if (!layer["enabled"].IsDefined()) {
                YAML::convert<bool>::decode(member.second, enabled);
            }
        } else if (key == "enabled") {
            YAML::convert<bool>::decode(member.second, enabled);
        } else {
            // Member is a sublayer
            sublayers.push_back(loadSublayer(member.second, (layerName + DELIMITER + key), scene));
        }
    }

    return { layerName, std::move(filter), rules, std::move(sublayers), enabled };
}

void SceneLoader::loadLayer(const std::pair<Node, Node>& layer, const std::shared_ptr<Scene>& scene) {

    const std::string& name = layer.first.Scalar();

    std::string source;
    std::vector<std::string> collections;

    auto sublayer = loadSublayer(layer.second, name, scene);

    if (Node data = layer.second["data"]) {
        if (Node data_source = data["source"]) {
            if (data_source.IsScalar()) {
                source = data_source.Scalar();
                auto dataSource = scene->getTileSource(source);
                // Makes sure to set the data source as a primary tile geometry generation source.
                // A data source is geometry generating source only when its used within a layer's data block
                // and when the layer is not disabled
                if (dataSource && sublayer.enabled()) {
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
                collections.reserve(data_layer.size());
                for (const auto& entry : data_layer) {
                    if (entry.IsScalar()) {
                        collections.push_back(entry.Scalar());
                    }
                }
            }
        }
    }

    if (collections.empty()) {
        collections.push_back(name);
    }


    scene->layers().push_back({ std::move(sublayer), source, collections });
}

void SceneLoader::loadBackground(Node background, const std::shared_ptr<Scene>& scene) {

    if (!background) { return; }

    if (Node colorNode = background["color"]) {
        Color colorResult;
        if (StyleParam::parseColor(colorNode, colorResult)) {
            scene->background() = colorResult;
        } else {
            Stops stopsResult = Stops::Colors(colorNode);
            if (stopsResult.frames.size() > 0) {
                scene->backgroundStops() = stopsResult;
            } else {
                LOGW("Cannot parse color: %s", Dump(colorNode).c_str());
            }
        }
    }
}

}
