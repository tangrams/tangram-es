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
#include "style/material.h"
#include "style/polygonStyle.h"
#include "style/polylineStyle.h"
#include "style/textStyle.h"
#include "style/pointStyle.h"
#include "style/rasterStyle.h"
#include "scene/dataLayer.h"
#include "scene/filters.h"
#include "scene/scene.h"
#include "scene/sceneLayer.h"
#include "scene/spriteAtlas.h"
#include "scene/lights.h"
#include "scene/stops.h"
#include "scene/styleMixer.h"
#include "scene/styleParam.h"
#include "util/floatFormatter.h"
#include "util/yamlPath.h"
#include "util/yamlUtil.h"

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

// ':' Delimiter for style params and layer-sublayer naming
static const char DELIMITER = ':';

static const std::string GLOBAL_PREFIX = "global.";

SceneError SceneLoader::applyUpdates(Node& _config, const std::vector<SceneUpdate>& _updates) {

    for (const auto& update : _updates) {
        Node value;

        try {
            value = YAML::Load(update.value);
        } catch (const YAML::ParserException& e) {
            LOGE("Parsing scene update string failed. '%s'", e.what());
            return {update, Error::scene_update_value_yaml_syntax_error};
        }

        if (value) {
            Node node;
            bool pathIsValid = YamlPath(update.path).get(_config, node);
            if (pathIsValid) {
                node = value;
            } else {
                LOGE("Update: %s - %s", update.path.c_str(), update.value.c_str());
                return {update, Error::scene_update_path_not_found};
            }
        }
    }
    return {};
}

void createGlobalRefs(std::vector<std::pair<YamlPath, YamlPath>>& _globalRefs,
                      const Node& _node, YamlPathBuffer& _path) {

    switch(_node.Type()) {
    case NodeType::Scalar: {
        const auto& value = _node.Scalar();
        if (value.length() > 7 && value.compare(0, 7, GLOBAL_PREFIX) == 0) {
            _globalRefs.emplace_back(_path.toYamlPath(),
                                    YamlPath(value.substr(GLOBAL_PREFIX.length())));
        }
    }
        break;
    case NodeType::Sequence: {
        _path.pushSequence();
        for (const auto& entry : _node) {
            createGlobalRefs(_globalRefs, entry, _path);
            _path.increment();
        }
        _path.pop();
    }
        break;
    case NodeType::Map:
        for (const auto& entry : _node) {
            _path.pushMap(&entry.first.Scalar());
            createGlobalRefs(_globalRefs, entry.second, _path);
            _path.pop();
        }
        break;
    default:
        break;
    }
}

void SceneLoader::applyGlobals(Node& _config) {
    const Node& globals = _config["global"];

    // Records the YAML Nodes for which global values have been swapped; keys are
    // nodes that referenced globals, values are nodes of globals themselves.
    std::vector<std::pair<YamlPath, YamlPath>> globalRefs;

    YamlPathBuffer path;
    createGlobalRefs(globalRefs, _config, path);
    if (!globalRefs.empty() && (!globals || !globals.IsMap())) {
        LOGW("Missing global references");
        return;
    }

    for (auto& globalRef : globalRefs) {
        Node target, global;
        bool targetPathIsValid = globalRef.first.get(_config, target);
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

void SceneLoader::applyScene(const Node& _node, Color& backgroundColor,
                             Stops& backgroundStops, Scene::animate& animated) {
    if (!_node) { return; }
    if (!_node.IsMap()) {
        LOGNode("Invalid 'scene' section", _node);
        return;
    }

    if (const Node& background = _node["background"]) {
        if (const Node& colorNode = background["color"]) {
            if (!StyleParam::parseColor(colorNode, backgroundColor)) {

                Stops stopsResult = Stops::Colors(colorNode);
                if (stopsResult.frames.size() > 0) {
                    backgroundStops = stopsResult;
                } else {
                    LOGW("Cannot parse color: %s", Dump(colorNode).c_str());
                }
            }
        }
    }

    if (const Node& animatedNode = _node["animated"]) {
        if (YamlUtil::getBoolOrDefault(animatedNode, false)) {
            // TODO enum class
            animated = Scene::yes;
        } else {
            animated = Scene::no;
        }
    }
}

void SceneLoader::applyCameras(const Node& _config, SceneCamera& _sceneCamera) {

    if (const Node& camera = _config["camera"]) {
        try {
            loadCamera(camera, _sceneCamera);
        }
        catch (const YAML::RepresentationException& e) {
            LOGNode("Parsing camera: '%s'", camera, e.what());
        }

    } else if (const Node& cameras = _config["cameras"]) {
        try {
            loadCameras(cameras, _sceneCamera);
        }
        catch (const YAML::RepresentationException& e) {
            LOGNode("Parsing cameras: '%s'", cameras, e.what());
        }
    }
}

void SceneLoader::loadCameras(const Node& _cameras, SceneCamera& _sceneCamera) {
    // To correctly match the behavior of the webGL library we'll need a place
    // to store multiple view instances.  Since we only have one global view
    // right now, we'll just apply the settings from the first active camera we
    // find.

    for (const auto& entry : _cameras) {
        loadCamera(entry.second, _sceneCamera);
    }
}

void SceneLoader::loadCamera(const Node& _camera, SceneCamera& _sceneCamera) {
    auto& camera = _sceneCamera;

    if (const Node& active = _camera["active"]) {
        if (!YamlUtil::getBoolOrDefault(active, false)) {
            return;
        }
    }

    auto type = _camera["type"].Scalar();
    if (type == "perspective") {
        camera.type = CameraType::perspective;

        // Only one of focal length and FOV is applied;
        // according to docs, focal length takes precedence.
        if (const Node& focal = _camera["focal_length"]) {
            float floatValue;
            if (YamlUtil::getFloat(focal, floatValue)) {
                camera.fieldOfView = View::focalLengthToFieldOfView(floatValue);
            } else if (focal.IsSequence()) {
                camera.fovStops = std::make_shared<Stops>(Stops::Numbers(focal));
                for (auto& f : camera.fovStops->frames) {
                    f.value = View::focalLengthToFieldOfView(f.value.get<float>());
                }
            }
        } else if (const Node& fov = _camera["fov"]) {
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

        if (const Node& vanishing = _camera["vanishing_point"]) {
            if (vanishing.IsSequence() && vanishing.size() >= 2) {
                // Values are pixels, unit strings are ignored.
                camera.vanishingPoint.x = YamlUtil::getFloatOrDefault(vanishing[0], 0.f, true);
                camera.vanishingPoint.y = YamlUtil::getFloatOrDefault(vanishing[1], 0.f, true);
            }
        }
    } else if (type == "isometric") {
        camera.type = CameraType::isometric;

        if (const Node& axis = _camera["axis"]) {
            YamlUtil::parseVec(axis, camera.obliqueAxis);
        }
    } else if (type == "flat") {
        camera.type = CameraType::flat;
    }

    // Default is world origin at 0 zoom
    double x = 0;
    double y = 0;
    float z = 0;

    if (const Node& position = _camera["position"]) {
        x = YamlUtil::getDoubleOrDefault(position[0], x);
        y = YamlUtil::getDoubleOrDefault(position[1], y);
        if (position.size() > 2) {
            z = YamlUtil::getFloatOrDefault(position[2], z);
        }
    }

    if (const Node& zoom = _camera["zoom"]) {
        z = YamlUtil::getFloatOrDefault(zoom, z);
    }

    if (const Node& maxTilt = _camera["max_tilt"]) {
        if (maxTilt.IsSequence()) {
            camera.maxTiltStops = std::make_shared<Stops>(Stops::Numbers(maxTilt));
        } else if (maxTilt.IsScalar()) {
            camera.maxTilt = YamlUtil::getFloatOrDefault(maxTilt, PI);
        }
    }

    auto meters = MapProjection::lngLatToProjectedMeters({x, y});
    camera.startPosition = glm::dvec3{meters.x, meters.y, z};
}

Scene::Lights SceneLoader::applyLights(const Node& _node) {
    Scene::Lights lights;

    if (_node && _node.IsMap()) {
        for (const auto& light : _node) {
            try {
                if (auto sceneLight = loadLight(light)) {
                    lights.push_back(std::move(sceneLight));
                }
            }
            catch (const YAML::RepresentationException& e) {
                LOGNode("Parsing light: '%s'", light, e.what());
            }
        }
    } else if (_node) {
        LOGNode("Invalid 'lights'", _node);
    }

    if (lights.empty()) {
        // Add an ambient light if nothing else is specified
        std::unique_ptr<AmbientLight> amb(new AmbientLight("defaultLight"));
        amb->setAmbientColor({ 1.f, 1.f, 1.f, 1.f });
        lights.push_back(std::move(amb));
    }
    return lights;
}

std::unique_ptr<Light> SceneLoader::loadLight(const std::pair<Node, Node>& _node) {
    const Node& light = _node.second;
    const std::string& name = _node.first.Scalar();
    const std::string& type = light["type"].Scalar();

    if (const Node& visible = light["visible"]) {
        // If 'visible' is false, skip loading this light.
        if (!YamlUtil::getBoolOrDefault(visible, true)) { return nullptr; }
    }

    std::unique_ptr<Light> sceneLight;
    if (type == "ambient") {
        sceneLight = std::make_unique<AmbientLight>(name);

    } else if (type == "directional") {
        auto dLight(std::make_unique<DirectionalLight>(name));

        if (const Node& directionNode = light["direction"]) {
            glm::vec3 direction;
            if (YamlUtil::parseVec<glm::vec3>(directionNode, direction)) {
                dLight->setDirection(direction);
            }
        }
        sceneLight = std::move(dLight);

    } else if (type == "point") {
        auto pLight(std::make_unique<PointLight>(name));

        if (const Node& position = light["position"]) {
            parseLightPosition(position, *pLight);
        }
        if (const Node& radius = light["radius"]) {
            if (radius.size() > 1) {
                pLight->setRadius(YamlUtil::getFloatOrDefault(radius[0], 0.f),
                                  YamlUtil::getFloatOrDefault(radius[1], 0.f));
            } else {
                pLight->setRadius(YamlUtil::getFloatOrDefault(radius, 0.f));
            }
        }
        if (const Node& att = light["attenuation"]) {
            float attenuation;
            if (YamlUtil::getFloat(att, attenuation)) {
                pLight->setAttenuation(attenuation);
            }
        }
        sceneLight = std::move(pLight);

    } else if (type == "spotlight") {
        auto sLight(std::make_unique<SpotLight>(name));

        if (const Node& position = light["position"]) {
            parseLightPosition(position, *sLight);
        }
        if (const Node& directionNode = light["direction"]) {
            glm::vec3 direction;
            if (YamlUtil::parseVec<glm::vec3>(directionNode, direction)) {
                sLight->setDirection(direction);
            }
        }
        if (const Node& radius = light["radius"]) {
            if (radius.size() > 1) {
                sLight->setRadius(YamlUtil::getFloatOrDefault(radius[0], 0.f),
                                  YamlUtil::getFloatOrDefault(radius[1], 0.f));
            } else {
                sLight->setRadius(YamlUtil::getFloatOrDefault(radius, 0.f));
            }
        }
        if (const Node& angle = light["angle"]) {
            sLight->setCutoffAngle(YamlUtil::getFloatOrDefault(angle, 0.f));
        }
        if (const Node& exponent = light["exponent"]) {
            sLight->setCutoffExponent(YamlUtil::getFloatOrDefault(exponent, 0.f));
        }
        sceneLight = std::move(sLight);
    }
    if (const Node& origin = light["origin"]) {
        const std::string& originStr = origin.Scalar();
        if (originStr == "world") {
            sceneLight->setOrigin(LightOrigin::world);
        } else if (originStr == "camera") {
            sceneLight->setOrigin(LightOrigin::camera);
        } else if (originStr == "ground") {
            sceneLight->setOrigin(LightOrigin::ground);
        }
    }
    if (const Node& ambient = light["ambient"]) {
        sceneLight->setAmbientColor(YamlUtil::getColorAsVec4(ambient));
    }
    if (const Node& diffuse = light["diffuse"]) {
        sceneLight->setDiffuseColor(YamlUtil::getColorAsVec4(diffuse));
    }
    if (const Node& specular = light["specular"]) {
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
    return sceneLight;
}

void SceneLoader::parseLightPosition(const Node& _positionNode, PointLight& _light) {
    UnitVec<glm::vec3> positionResult;
    if (StyleParam::parseVec3(_positionNode, UnitSet{Unit::none, Unit::pixel, Unit::meter}, positionResult)) {
        for (auto& unit : positionResult.units) {
            if (unit == Unit::none) {
                unit = Unit::meter;
            }
        }
        _light.setPosition(positionResult);
    } else {
        LOGNode("Invalid light position parameter:", _positionNode);
    }
}

void SceneLoader::applyTextures(const Node& _node, SceneTextures& _textures) {
    if (!_node) { return; }
    if (!_node.IsMap()) {
        LOGNode("Invalid 'textures' section", _node);
        return;
    }

    for (const auto& texture : _node) {
        try { loadTexture(texture, _textures); }
        catch (const YAML::RepresentationException& e) {
            LOGNode("Parsing texture: '%s'", texture, e.what());
        }
    }
}

void SceneLoader::loadTexture(const std::pair<Node, Node>& _node, SceneTextures& _textures) {
    const std::string& name = _node.first.Scalar();
    const Node& textureConfig = _node.second;

    if (!textureConfig.IsMap()) {
        LOGW("Invalid texture node '%s', skipping.", name.c_str());
        return;
    }

    std::string url;
    TextureOptions options;

    if (const Node& urlNode = textureConfig["url"]) {
        if (urlNode.IsScalar()) {
            url = urlNode.Scalar();
        }
    }
    if (url.empty()){
        LOGW("No url specified for texture '%s', skipping.", name.c_str());
        return;
    }

    if (const Node& filtering = textureConfig["filtering"]) {
        if (!parseTexFiltering(filtering, options)) {
            LOGW("Invalid texture filtering: %s", Dump(filtering).c_str());
        }
    }

    if (const Node& density = textureConfig["density"]) {
        options.displayScale = 1.f / YamlUtil::getFloatOrDefault(density, 1.f);
    }

    auto texture = _textures.add(name, Url(url), options);

    if (const Node& sprites = textureConfig["sprites"]) {
        auto atlas = std::make_unique<SpriteAtlas>();
        for (auto it = sprites.begin(); it != sprites.end(); ++it) {

            const Node& sprite = it->second;
            const std::string& spriteName = it->first.Scalar();

            if (sprite) {
                glm::vec4 desc;
                YamlUtil::parseVec<glm::vec4>(sprite, desc);
                glm::vec2 pos = glm::vec2(desc.x, desc.y);
                glm::vec2 size = glm::vec2(desc.z, desc.w);

                atlas->addSpriteNode(spriteName, pos, size);
            }
        }
        texture->setSpriteAtlas(std::move(atlas));
    }

    if (const Node& sizeNode = textureConfig["size"]) {
        glm::vec2 size;
        if (YamlUtil::parseVec<glm::vec2>(sizeNode, size)) {
            texture->resize(size.x, size.y);
            if (texture->spriteAtlas()) {
                texture->spriteAtlas()->updateSpriteNodes(size);
            }
        }
    }
}

bool SceneLoader::parseTexFiltering(const Node& _filteringNode, TextureOptions& _options) {
    if (!_filteringNode.IsScalar()) {
        return false;
    }
    const std::string& filteringString = _filteringNode.Scalar();
    if (filteringString == "linear") {
        _options.minFilter = TextureMinFilter::LINEAR;
        _options.magFilter = TextureMagFilter::LINEAR;
        return true;
    } else if (filteringString == "mipmap") {
        _options.minFilter = TextureMinFilter::LINEAR_MIPMAP_LINEAR;
        _options.generateMipmaps = true;
        return true;
    } else if (filteringString == "nearest") {
        _options.minFilter = TextureMinFilter::NEAREST;
        _options.magFilter = TextureMagFilter::NEAREST;
        return true;
    }
    return false;
}

void SceneLoader::applyFonts(const Node& _node, SceneFonts& _fonts) {
    if (!_node) { return; }
    if (!_node.IsMap()) {
        LOGNode("Invalid 'fonts' section", _node);
        return;
    }

    for (const auto& font : _node) {
        try {
            const std::string& family = font.first.Scalar();

            if (font.second.IsMap()) {
                loadFontDescription(font.second, family, _fonts);
            } else if (font.second.IsSequence()) {
                for (const auto& node : font.second) {
                    loadFontDescription(node, family, _fonts);
                }
            } else {
                // LOG
            }
        }
        catch (const YAML::RepresentationException& e) {
            LOGNode("Parsing font: '%s'", font, e.what());
        }
    }
}

void SceneLoader::loadFontDescription(const Node& _node, const std::string& _family, SceneFonts& _fonts) {
    if (!_node) { return; }
    if (!_node.IsMap()) {
        LOGNode("Invalid 'font' section", _node);
        return;
    }

    std::string style = "regular", weight = "400", uri;
    for (const auto& fontDesc : _node) {
        const std::string& key = fontDesc.first.Scalar();
        if (key == "weight") {
            weight = fontDesc.second.Scalar();
        } else if (key == "style") {
            style = fontDesc.second.Scalar();
        } else if (key == "url") {
            uri = fontDesc.second.Scalar();
        } else if (key == "external") {
            LOGW("external: within fonts: is a no-op in native" \
                 " version of tangram (%s)", _family.c_str());
        }
    }

    if (uri.empty()) {
        LOGW("Empty url: block within fonts: (%s)", _family.c_str());
        return;
    }

    _fonts.add(uri, _family, style, weight);
}

Scene::TileSources SceneLoader::applySources(const Node& _config, const SceneOptions& _options,
                                             Platform& _platform) {

    Scene::TileSources tileSources;

    const Node& sources = _config["sources"];
    if (!sources) {
        LOGW("No source defined in the yaml scene configuration.");
        return {};
    }
    for (const auto& source : sources) {
        std::string srcName = source.first.Scalar();
        try {
            if (auto tileSource = loadSource(source.second, srcName, _options, _platform)) {
                tileSources.push_back(std::move(tileSource));
            }
        }
        catch (const YAML::RepresentationException& e) {
            LOGNode("Invalid source: '%s'", source, e.what());
        }
    }

    auto getTileSource = [&](const std::string& name) -> std::shared_ptr<TileSource> {
        auto it = std::find_if(tileSources.begin(), tileSources.end(),
                           [&](auto& s){ return s->name() == name; });
        if (it != tileSources.end()) { return *it; }
        return nullptr;
    };

    // Add Raster subsources
    for (const auto& source : sources) {
        std::string srcName = source.first.Scalar();
        auto tileSource = getTileSource(srcName);
        if (!tileSource) { continue; }

        if (const Node& rasters = source.second["rasters"]) {
            if (!rasters.IsSequence()) {
                LOGNode("Invalid 'rasters'", rasters);
                continue;
            }
            for (const auto& raster : rasters) {
                if (!raster.IsScalar()) {
                    LOGNode("Invalid 'raster'", raster);
                    continue;
                }
                if (auto rasterSource = getTileSource(raster.Scalar())) {
                    tileSource->addRasterSource(rasterSource);
                } else {
                    LOGNode("Missing raster source", raster);
                }
            }
        }
    }

    // Makes sure to set the data source as a primary tile geometry generation
    // source.  A data source is geometry generating source only when its used
    // within a layer's data block and when the layer is not disabled

    if (const Node& layers = _config["layers"]) {
        for (const auto& member : layers) {
            const auto& layer = member.second;

            if (!layer.IsMap()) { continue; }
            bool enabled = true;

            if (const Node& n = layer["enabled"]) {
                YAML::convert<bool>::decode(n, enabled);
            }
            else if (const Node n = layer["visible"]) {
                YAML::convert<bool>::decode(n, enabled);
            }

            if (!enabled) { continue; }
            const Node& data = layer["data"];
            if (!data || !data.IsMap()) { continue;}
            const Node& data_source = data["source"];
            if (!data_source) { continue;}

            if (!data_source.IsScalar()) {
                LOGNode("Invalid 'source", data);
                continue;
            }
            auto source = data_source.Scalar();
            if (auto dataSource = getTileSource(source)) {
                dataSource->generateGeometry(true);
            } else {
                LOGW("Can't find data source %s for layer %s",
                     source.c_str(), member.first.Scalar().c_str());
            }
         }
     }

    return tileSources;
}

std::shared_ptr<TileSource> SceneLoader::loadSource(const Node& _source, const std::string& _name,
                                                    const SceneOptions& _options, Platform& _platform) {

    std::string type;
    std::string url;
    std::string mbtiles;
    std::vector<std::string> subdomains;

    int32_t minDisplayZoom = -1;
    int32_t maxDisplayZoom = -1;
    int32_t maxZoom = 18;
    int32_t zoomBias = 0;

    if (auto typeNode = _source["type"]) {
        type = typeNode.Scalar();
    }
    if (auto urlNode = _source["url"]) {
        url = urlNode.Scalar();
    }
    if (auto minDisplayZoomNode = _source["min_display_zoom"]) {
        YamlUtil::getInt(minDisplayZoomNode, minDisplayZoom);
    }
    if (auto maxDisplayZoomNode = _source["max_display_zoom"]) {
        YamlUtil::getInt(maxDisplayZoomNode, maxDisplayZoom);
    }
    if (auto maxZoomNode = _source["max_zoom"]) {
        YamlUtil::getInt(maxZoomNode, maxZoom);
    }
    if (auto tileSizeNode = _source["tile_size"]) {
        int tileSize = 0;
        if (YamlUtil::getInt(tileSizeNode, tileSize)) {
            zoomBias = TileSource::zoomBiasFromTileSize(tileSize);
        }
    }

    // Parse and append any URL parameters.
    if (auto urlParamsNode = _source["url_params"]) {
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
                    LOGW("Invalid url_params entry in source '%s', entries" \
                         " should be strings.", _name.c_str());
                }
            }
        } else {
            LOGW("Expected a map of values for url_params in source '%s'.", _name.c_str());
        }
        urlStream << url.substr(baseEnd);
        url = urlStream.str();
    }

    // Apply URL subdomain configuration.
    if (const Node& subDomainNode = _source["url_subdomains"]) {
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
        LOGW("The URL for source '%s' includes the subdomain placeholder '{s}'," \
             " but no subdomains were given.", _name.c_str());
    }
    if (!hasSubdomainPlaceholder && !subdomains.empty()) {
        LOGW("The URL for source '%s' has subdomains specified, but does not" \
             " include the subdomain placeholder '{s}'.", _name.c_str());
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
    if (const Node& tmsNode = _source["tms"]) {
        YamlUtil::getBool(tmsNode, isTms);
    }

    std::unique_ptr<TileSource::DataSource> rawSources;

    if (isMBTilesFile) {
#ifdef TANGRAM_MBTILES_DATASOURCE
        // If we have MBTiles, we know the source is tiled.
        tiled = true;
        // Create an MBTiles data source from the file at the url and add it to the source chain.
        rawSources = std::make_unique<MBTilesDataSource>(_platform, _name, url, "");
#else
        LOGE("MBTiles support is disabled. This source will be ignored: %s", _name.c_str());
        return nullptr;
#endif
    } else if (tiled) {
        auto cacheSize = _options.memoryTileCacheSize;
        if (cacheSize > 0) {
            auto cache = std::make_unique<MemoryCacheDataSource>();
            cache->setCacheSize(cacheSize);
            rawSources = std::move(cache);
        }

        auto s = std::make_unique<NetworkDataSource>(_platform, url, std::move(subdomains), isTms);
        if (rawSources) {
            rawSources->next = std::move(s);
        } else {
            rawSources = std::move(s);
        }
    }

    std::shared_ptr<TileSource> sourcePtr;

    TileSource::ZoomOptions zoomOptions = { minDisplayZoom, maxDisplayZoom, maxZoom, zoomBias };

    if (type == "GeoJSON" && !tiled) {
        bool generateCentroids = false;
        if (auto genLabelCentroidsNode = _source["generate_label_centroids"]) {
            generateCentroids = true;
        }
        sourcePtr = std::make_shared<ClientGeoJsonSource>(_platform, _name, url,
                                                          generateCentroids, zoomOptions);
    } else if (type == "Raster") {
        TextureOptions options;
        if (const Node& filtering = _source["filtering"]) {
            if (!parseTexFiltering(filtering, options)) {
                LOGW("Invalid texture filtering: %s", Dump(filtering).c_str());
            }
        }
        sourcePtr = std::make_shared<RasterSource>(_name, std::move(rawSources), options, zoomOptions);
    } else {
        sourcePtr = std::make_shared<TileSource>(_name, std::move(rawSources), zoomOptions);

        if (type == "GeoJSON") {
            sourcePtr->setFormat(TileSource::Format::GeoJson);
        } else if (type == "TopoJSON") {
            sourcePtr->setFormat(TileSource::Format::TopoJson);
        } else if (type == "MVT") {
            sourcePtr->setFormat(TileSource::Format::Mvt);
        } else {
            LOGE("Source '%s' does not have a valid type. " \
                 "Valid types are 'GeoJSON', 'TopoJSON', and 'MVT'. " \
                 "This source will be ignored.", _name.c_str());
            return nullptr;
        }
    }

    return sourcePtr;
}

Scene::Styles SceneLoader::applyStyles(const Node& _node, SceneTextures& _textures, SceneFunctions& _functions,
                                       SceneStops& _stops, DrawRuleNames& _ruleNames) {

    Scene::Styles styles;
    // Instantiate built-in styles
    styles.emplace_back(new PolygonStyle("polygons"));
    styles.emplace_back(new PolylineStyle("lines"));
    styles.emplace_back(new TextStyle("text", true));
    styles.emplace_back(new PointStyle("points"));
    styles.emplace_back(new RasterStyle("raster"));

    if (!_node) { return styles; }
    if (!_node.IsMap()) {
        LOGNode("Invalid 'styles' section", _node);
        return styles;
    }

    StyleMixer mixer;
    try {
        mixer.mixStyleNodes(_node);
    } catch (const YAML::RepresentationException& e) {
        LOGNode("Mixing styles: '%s'", _node, e.what());
    }

    for (const auto& entry : _node) {
        try {
            auto name = entry.first.Scalar();
            auto styleConfig =  entry.second;


            auto style = loadStyle(name, styleConfig);
            if (!style) { continue; }

            loadStyleProps(styleConfig, *style.get(), _textures);

            const Node& drawNode = styleConfig["draw"];
            if (drawNode) {
                auto params = parseStyleParams(drawNode, _stops, _functions);

                // Note: ruleID and name is immaterial here, as these are only used
                // for rule merging, but style's default styling rules are applied
                // post rule merging for any style parameter which was not assigned
                // during merging step.
                int ruleID = _ruleNames.add(name);
                style->setDefaultDrawRule(std::make_unique<DrawRuleData>(name, ruleID,
                                                                         std::move(params)));
            }

            styles.push_back(std::move(style));
        }
        catch (const YAML::RepresentationException& e) {
            LOGNode("Parsing style: '%s'", entry, e.what());
        }
    }
    return styles;
}

std::unique_ptr<Style> SceneLoader::loadStyle(const std::string& _name, const Node& _styleConfig) {
    const auto& builtIn = Style::builtInStyleNames();

    if (std::find(builtIn.begin(), builtIn.end(), _name) != builtIn.end()) {
        LOGW("Cannot use built-in style name '%s' for new style", _name.c_str());
        return nullptr;
    }

    const Node& baseNode = _styleConfig["base"];
    if (!baseNode) {
        // No base style, this is an abstract style
        return nullptr;
    }

    // Construct style instance using the merged properties
    std::unique_ptr<Style> style;
    const auto& baseStyle = baseNode.Scalar();
    if (baseStyle == "polygons") {
        style = std::make_unique<PolygonStyle>(_name);
    } else if (baseStyle == "lines") {
        style = std::make_unique<PolylineStyle>(_name);
    } else if (baseStyle == "text") {
        style = std::make_unique<TextStyle>(_name, true);
    } else if (baseStyle == "points") {
        style = std::make_unique<PointStyle>(_name);
    } else if (baseStyle == "raster") {
        style = std::make_unique<RasterStyle>(_name);
    } else {
        LOGW("Base style '%s' not recognized, cannot instantiate.", baseStyle.c_str());
        return nullptr;
    }

    if (const Node& rasterNode = _styleConfig["raster"]) {
        const auto& raster = rasterNode.Scalar();
        if (raster == "normal") {
            style->setRasterType(RasterType::normal);
        } else if (raster == "color") {
            style->setRasterType(RasterType::color);
        } else if (raster == "custom") {
            style->setRasterType(RasterType::custom);
        }
    }

    return style;
}

void SceneLoader::loadStyleProps(const Node& _styleNode, Style& _style, SceneTextures& _textures) {
    if (!_styleNode) {
        LOGW("Can not parse style parameters, bad style YAML Node");
        return;
    }

    if (const Node& animatedNode = _styleNode["animated"]) {
        if (!animatedNode.IsScalar()) { LOGW("animated flag should be a scalar"); }
        else {
            bool animate;
            if (YamlUtil::getBool(animatedNode, animate)) {
                _style.setAnimated(animate);
            }
        }
    }

    if (const Node& blendNode = _styleNode["blend"]) {
        const std::string& blendMode = blendNode.Scalar();
        if      (blendMode == "opaque")      { _style.setBlendMode(Blending::opaque); }
        else if (blendMode == "add")         { _style.setBlendMode(Blending::add); }
        else if (blendMode == "multiply")    { _style.setBlendMode(Blending::multiply); }
        else if (blendMode == "overlay")     { _style.setBlendMode(Blending::overlay); }
        else if (blendMode == "inlay")       { _style.setBlendMode(Blending::inlay); }
        else if (blendMode == "translucent") { _style.setBlendMode(Blending::translucent); }
        else { LOGW("Invalid blend mode '%s'", blendMode.c_str()); }
    }

    if (const Node& blendOrderNode = _styleNode["blend_order"]) {
        int blendOrderValue;
        if (YamlUtil::getInt(blendOrderNode, blendOrderValue)) {
            _style.setBlendOrder(blendOrderValue);
        } else {
            LOGE("Integral value expected for blend_order style parameter.\n");
        }
    }

    if (const Node& texcoordsNode = _styleNode["texcoords"]) {
        bool boolValue;
        if (YamlUtil::getBool(texcoordsNode, boolValue)) {
            _style.setTexCoordsGeneration(boolValue);
        }
    }

    if (const Node& dashNode = _styleNode["dash"]) {
        if (auto polylineStyle = dynamic_cast<PolylineStyle*>(&_style)) {
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

    if (const Node& dashBackgroundColor = _styleNode["dash_background_color"]) {
        if (auto polylineStyle = dynamic_cast<PolylineStyle*>(&_style)) {
            glm::vec4 backgroundColor = YamlUtil::getColorAsVec4(dashBackgroundColor);
            polylineStyle->setDashBackgroundColor(backgroundColor);
        }
    }

    if (const Node& shadersNode = _styleNode["shaders"]) {
        loadShaderConfig(shadersNode, _style, _textures);
    }

    if (const Node& lightingNode = _styleNode["lighting"]) {
        const std::string& lighting = lightingNode.Scalar();
        if (lighting == "fragment") { _style.setLightingType(LightingType::fragment); }
        else if (lighting == "vertex") { _style.setLightingType(LightingType::vertex); }
        else if (lighting == "false") { _style.setLightingType(LightingType::none); }
        else if (lighting == "true") { } // use default lighting
        else { LOGW("Unrecognized lighting type '%s'", lighting.c_str()); }
    }

    if (const Node& textureNode = _styleNode["texture"]) {
        if (auto pointStyle = dynamic_cast<PointStyle*>(&_style)) {
            const std::string& textureName = textureNode.Scalar();
            auto styleTexture = _textures.get(textureName);
            if (styleTexture) {
                pointStyle->setDefaultTexture(styleTexture);
            } else {
                LOGW("Undefined texture name %s", textureName.c_str());
            }
        } else if (auto polylineStyle = dynamic_cast<PolylineStyle*>(&_style)) {
            const std::string& textureName = textureNode.Scalar();
            auto texture = _textures.get(textureName);
            if (texture) {
                polylineStyle->setTexture(texture);
                polylineStyle->setTexCoordsGeneration(true);
            }
        }
    }

    if (const Node& materialNode = _styleNode["material"]) {
        loadMaterial(materialNode, _style.getMaterial(), _style, _textures);
    }
}

std::vector<StyleParam> SceneLoader::parseStyleParams(const Node& _params, SceneStops& _stops,
                                                      SceneFunctions& _functions) {
    std::vector<StyleParam> out;
    parseStyleParams(_params, "", _stops, _functions, out);
    return out;
}

void SceneLoader::parseStyleParams(const Node& _params, const std::string& _prefix, SceneStops& _stops,
                                   SceneFunctions& _functions, std::vector<StyleParam>& _out) {

    for (const auto& prop : _params) {
        std::string key;
        if (!_prefix.empty()) {
            key = _prefix + DELIMITER + prop.first.Scalar();
        } else {
            key = prop.first.Scalar();
        }
        const Node& value = prop.second;

        if (key == "transition" || key == "text:transition") {
            parseTransition(prop.second, key, _out);
            continue;
        }

        if (key == "texture") {
            _out.push_back(StyleParam{ StyleParamKey::texture, value });
            continue;
        }

        if (key == "text") {
            // Add StyleParam to signify that icon uses text
            _out.push_back(StyleParam{ StyleParamKey::point_text, "" });
        }

        switch (value.Type()) {
        case NodeType::Scalar: {
            const std::string& val = value.Scalar();

            if (val.compare(0, 8, "function") == 0) {
                StyleParam param(key);
                param.function = _functions.add(val);
                _out.push_back(std::move(param));
            } else {
                _out.push_back(StyleParam{ key, value });
            }
            break;
        }
        case NodeType::Sequence: {
            if (value[0].IsSequence()) {
                auto styleKey = StyleParam::getKey(key);
                if (styleKey != StyleParamKey::none) {

                    if (StyleParam::isColor(styleKey)) {
                        _stops.push_back(Stops::Colors(value));
                        _out.push_back(StyleParam{ styleKey, &_stops.back() });
                    } else if (StyleParam::isSize(styleKey)) {
                        _stops.push_back(Stops::Sizes(value, StyleParam::unitSetForStyleParam(styleKey)));
                        _out.push_back(StyleParam{ styleKey, &_stops.back() });
                    } else if (StyleParam::isWidth(styleKey)) {
                        _stops.push_back(Stops::Widths(value, StyleParam::unitSetForStyleParam(styleKey)));
                        _out.push_back(StyleParam{ styleKey, &_stops.back() });
                    } else if (StyleParam::isOffsets(styleKey)) {
                        _stops.push_back(Stops::Offsets(value, StyleParam::unitSetForStyleParam(styleKey)));
                        _out.push_back(StyleParam{ styleKey, &_stops.back() });
                    } else if (StyleParam::isFontSize(styleKey)) {
                        _stops.push_back(Stops::FontSize(value));
                        _out.push_back(StyleParam{ styleKey, &_stops.back() });
                    } else if (StyleParam::isNumberType(styleKey)) {
                        _stops.push_back(Stops::Numbers(value));
                        _out.push_back(StyleParam{ styleKey, &_stops.back() });
                    }
                } else {
                    LOGW("Unknown style parameter %s", key.c_str());
                }

            } else {
                _out.push_back(StyleParam{ key, value });
            }
            break;
        }
        case NodeType::Map: {
            // NB: Flatten parameter map
            parseStyleParams(value, key, _stops, _functions, _out);

            break;
        }
        case NodeType::Null: {
            // Handles the case, when null style param value is used to unset a merged style param
            _out.emplace_back(StyleParam::getKey(key));
            break;
        }
        default:
            LOGW("Style parameter %s must be a scalar, sequence, or map.", key.c_str());
        }
    }
}

void SceneLoader::parseTransition(const Node& _params, std::string _prefix, std::vector<StyleParam>& _out) {

    // First iterate over the mapping of 'events', we currently recognize 'hide', 'selected', and 'show'.
    for (const auto& event : _params) {
        if (!event.first.IsScalar() || !event.second.IsMap()) {
            LOGW("Can't parse 'transitions' entry, expected a mapping of strings to mappings at: %s",
                 _prefix.c_str());
            continue;
        }

        // Add the event to our key, so it's now 'transition:event'.
        std::string transitionEvent = _prefix + DELIMITER + event.first.Scalar();

        // Iterate over the parameters in the 'event', we currently only recognize 'time'.
        for (const auto& param : event.second) {
            if (!param.first.IsScalar() || !param.second.IsScalar()) {
                LOGW("Expected a mapping of strings to strings or numbers in: %s",
                     transitionEvent.c_str());
                continue;
            }
            // Add the parameter to our key, so it's now 'transition:event:param'.
            std::string transitionEventParam = transitionEvent + DELIMITER + param.first.Scalar();
            // Create a style parameter from the key and value.
            _out.push_back(StyleParam{ transitionEventParam, param.second });
        }
    }
}

void SceneLoader::loadShaderConfig(const Node& _shaders, Style& _style, SceneTextures& _textures) {

    auto& shader = _style.getShaderSource();

    if (const Node& extNode = _shaders["extensions_mixed"]) {
        if (extNode.IsScalar()) {
            shader.addExtensionDeclaration(extNode.Scalar());
        } else if (extNode.IsSequence()) {
            for (const auto& e : extNode) {
                shader.addExtensionDeclaration(e.Scalar());
            }
        }
    }

    if (const Node& definesNode = _shaders["defines"]) {
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

    if (const Node& uniformsNode = _shaders["uniforms"]) {
        for (const auto& uniform : uniformsNode) {
            const std::string& name = uniform.first.Scalar();
            StyleUniform styleUniform;

            if (parseStyleUniforms(uniform.second, styleUniform, _textures)) {
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

                _style.styleUniforms().emplace_back(name, styleUniform.value);
            } else {
                LOGNode("Style uniform parsing failure", uniform.second);
            }
        }
    }

    if (const Node& blocksNode = _shaders["blocks_mixed"]) {
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

bool SceneLoader::parseStyleUniforms(const Node& _value, StyleUniform& _styleUniform,
                                     SceneTextures& _textures) {

    if (_value.IsScalar()) { // float, bool or string (texture)
        double fValue;
        bool bValue;

        if (YamlUtil::getDouble(_value, fValue, false)) {
            _styleUniform.type = "float";
            _styleUniform.value = (float)fValue;
        } else if (YamlUtil::getBool(_value, bValue)) {
            _styleUniform.type = "bool";
            _styleUniform.value = (bool)bValue;
        } else {
            const std::string& strVal = _value.Scalar();
            _styleUniform.type = "sampler2D";
            _styleUniform.value = strVal;

            _textures.get(strVal);
        }
    } else if (_value.IsSequence()) {
        size_t size = _value.size();
        bool parsed = false;
        switch (size) {
            case 2: {
                glm::vec2 vec;
                if (YamlUtil::parseVec<glm::vec2>(_value, vec)) {
                    _styleUniform.value = vec;
                    _styleUniform.type = "vec2";
                    parsed = true;
                }
                break;
            }
            case 3: {
                glm::vec3 vec;
                if (YamlUtil::parseVec(_value, vec)) {
                    _styleUniform.value = vec;
                    _styleUniform.type = "vec3";
                    parsed = true;
                }
                break;
            }
            case 4: {
                glm::vec4 vec;
                if (YamlUtil::parseVec(_value, vec)) {
                    _styleUniform.value = vec;
                    _styleUniform.type = "vec4";
                    parsed = true;
                }
                break;
            }
            default: {
                UniformArray1f uniformArray;
                for (const auto& val : _value) {
                    double fValue;
                    if (YamlUtil::getDouble(val, fValue)) {
                        uniformArray.push_back(fValue);
                    } else {
                        return false;
                    }
                }
                _styleUniform.value = std::move(uniformArray);
                _styleUniform.type = "vec" + std::to_string(size);
                parsed = true;
                break;
            }
        }

        if (!parsed) {
            // array of strings (textures)
            UniformTextureArray textureArrayUniform;
            textureArrayUniform.names.reserve(size);
            _styleUniform.type = "sampler2D";

            for (const auto& strVal : _value) {
                const std::string& textureName = strVal.Scalar();
                textureArrayUniform.names.push_back(textureName);

                _textures.get(textureName);
            }

            _styleUniform.value = std::move(textureArrayUniform);
        }
    } else {
        LOGW("Expected a scalar or sequence value for uniforms");
        return false;
    }
    return true;
}

void SceneLoader::loadMaterial(const Node& _matNode, Material& _material, Style& _style,
                               SceneTextures& _textures) {

    auto parseMaterialVec = [](const Node& prop) -> glm::vec4 {
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
    };

    if (!_matNode.IsMap()) { return; }

    if (const Node& n = _matNode["emission"]) {
        if (n.IsMap()) {
            _material.setEmission(loadMaterialTexture(n, _style, _textures));
        } else {
            _material.setEmission(parseMaterialVec(n));
        }
    }
    if (const Node& n = _matNode["diffuse"]) {
        if (n.IsMap()) {
            _material.setDiffuse(loadMaterialTexture(n, _style, _textures));
        } else {
            _material.setDiffuse(parseMaterialVec(n));
        }
    }
    if (const Node& n = _matNode["ambient"]) {
        if (n.IsMap()) {
            _material.setAmbient(loadMaterialTexture(n, _style, _textures));
        } else {
            _material.setAmbient(parseMaterialVec(n));
        }
    }

    if (const Node& n = _matNode["specular"]) {
        if (n.IsMap()) {
            _material.setSpecular(loadMaterialTexture(n, _style, _textures));
        } else {
            _material.setSpecular(parseMaterialVec(n));
        }
    }

    if (const Node& shininess = _matNode["shininess"]) {
        double value;
        if (YamlUtil::getDouble(shininess, value, false)) {
            _material.setShininess(value);
        }
    }
    _material.setNormal(loadMaterialTexture(_matNode["normal"], _style, _textures));
}

MaterialTexture SceneLoader::loadMaterialTexture(const Node& _matCompNode, Style& _style,
                                                 SceneTextures& _textures) {

    if (!_matCompNode) { return MaterialTexture{}; }

    Node textureNode = _matCompNode["texture"];
    if (!textureNode) {
        LOGNode("Expected a 'texture' parameter", _matCompNode);

        return MaterialTexture{};
    }

    const std::string& name = textureNode.Scalar();

    MaterialTexture matTex;
    matTex.tex = _textures.get(name);

    if (const Node& mappingNode = _matCompNode["mapping"]) {
        const std::string& mapping = mappingNode.Scalar();
        if (mapping == "uv") {
            matTex.mapping = MappingType::uv;

            // Mark the style to generate texture coordinates
            if (!_style.genTexCoords()) {
                LOGW("Style %s has option `texcoords: false` but material %s has uv mapping",
                    _style.getName().c_str(), name.c_str());
                LOGW("Defaulting uvs generation to true for style %s",
                    _style.getName().c_str());
            }

            _style.setTexCoordsGeneration(true);
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

    if (const Node& scaleNode = _matCompNode["scale"]) {
        if (scaleNode.IsSequence() && scaleNode.size() == 2) {
            matTex.scale.x = YamlUtil::getFloatOrDefault(scaleNode[0], matTex.scale.x);
            matTex.scale.y = YamlUtil::getFloatOrDefault(scaleNode[1], matTex.scale.y);
        } else if (scaleNode.IsScalar()) {
            matTex.scale = glm::vec3(YamlUtil::getFloatOrDefault(scaleNode, 1.f));
        } else {
            LOGW("Unrecognized scale parameter in material");
        }
    }

    if (const Node& amountNode = _matCompNode["amount"]) {
        if (amountNode.IsScalar()) {
            matTex.amount = glm::vec3(YamlUtil::getFloatOrDefault(amountNode, 1.f));
        } else if (!YamlUtil::parseVec(amountNode, matTex.amount)) {
            LOGW("Unrecognized amount parameter in material");
        }
    }
    return matTex;
}

std::vector<DataLayer> SceneLoader::applyLayers(const Node& _node,  SceneFunctions& _functions,
                                                SceneStops& _stops, DrawRuleNames& _ruleNames) {
    if (!_node) { return {}; }
    if (!_node.IsMap()) {
        LOGNode("Invalid 'layers' section", _node);
        return {};
    }

    std::vector<DataLayer> dataLayers;
    for (const auto& layer : _node) {
        try {
            const std::string& name = layer.first.Scalar();
            std::string source;
            std::vector<std::string> collections;

            auto sublayer = loadSublayer(layer.second, name, _functions, _stops, _ruleNames);

            if (const Node& data = layer.second["data"]) {

                const Node& data_source = data["source"];
                if (data_source && data_source.IsScalar()) {
                    source = data_source.Scalar();
                }

                if (const Node& data_layer = data["layer"]) {
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
            dataLayers.emplace_back(std::move(sublayer), source, collections);

        }
        catch (const YAML::RepresentationException& e) {
            LOGNode("Parsing layer: '%s'", layer, e.what());
        }
    }
    return dataLayers;
}

SceneLayer SceneLoader::loadSublayer(const Node& _layer, const std::string& _layerName,
                                     SceneFunctions& _functions, SceneStops& _stops,
                                     DrawRuleNames& _ruleNames) {
    std::vector<SceneLayer> sublayers;
    std::vector<DrawRuleData> rules;
    Filter filter;
    bool enabled = true;

    for (const auto& member : _layer) {

        const std::string& key = member.first.Scalar();

        if (key == "data") {
            // Ignored for sublayers
        } else if (key == "draw") {
            // Member is a mapping of draw rules
            for (auto& ruleNode : member.second) {
                auto params = parseStyleParams(ruleNode.second, _stops, _functions);

                auto const& ruleName = ruleNode.first.Scalar();
                int ruleId = _ruleNames.add(ruleName);

                rules.push_back({ ruleName, ruleId, std::move(params) });
            }
        } else if (key == "filter") {
            filter = generateFilter(_functions, member.second);
            if (!filter.isValid()) {
                LOGNode("Invalid 'filter' in layer '%s'", member.second, _layerName.c_str());
                return { _layerName, {}, {}, {}, false };
            }
        } else if (key == "visible") {
            if (!_layer["enabled"].IsDefined()) {
                YAML::convert<bool>::decode(member.second, enabled);
            }
        } else if (key == "enabled") {
            YAML::convert<bool>::decode(member.second, enabled);
        } else {
            // Member is a sublayer
            sublayers.push_back(loadSublayer(member.second, (_layerName + DELIMITER + key),
                                             _functions, _stops, _ruleNames));
        }
    }
    return { _layerName, std::move(filter), rules, std::move(sublayers), enabled };
}

void printFilters(const SceneLayer& layer, int indent){
    LOG("%*s >>> %s\n", indent, "", layer.name().c_str());
    layer.filter().print(indent + 2);

    for (auto& l : layer.sublayers()) {
        printFilters(l, indent + 2);
    }
};

Filter SceneLoader::generateFilter(SceneFunctions& _functions, const Node& _filter) {
    switch (_filter.Type()) {
    case NodeType::Scalar: {

        const std::string& val = _filter.Scalar();
        if (val.compare(0, 8, "function") == 0) {
            return Filter::MatchFunction(_functions.add(val));
        }
        return Filter();
    }
    case NodeType::Sequence: {
        return generateAnyFilter(_functions, _filter);
    }
    case NodeType::Map: {
        std::vector<Filter> filters;
        for (const auto& filtItr : _filter) {
            const std::string& key = filtItr.first.Scalar();
            const Node& node = _filter[key];
            Filter f;
            if (key == "none") {
                f = generateNoneFilter(_functions, node);
            } else if (key == "not") {
                f = generateNoneFilter(_functions, node);
            } else if (key == "any") {
                f = generateAnyFilter(_functions, node);
            } else if (key == "all") {
                f = generateAllFilter(_functions, node);
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

Filter SceneLoader::generateAnyFilter(SceneFunctions& _functions, const Node& _filter) {
    if (_filter.IsSequence()) {
        std::vector<Filter> filters;

        for (const auto& filt : _filter) {
            if (Filter f = generateFilter(_functions, filt)) {
                filters.push_back(std::move(f));
            } else { return Filter(); }
        }
        return Filter::MatchAny(std::move(filters));
    }
    return Filter();
}

Filter SceneLoader::generateAllFilter(SceneFunctions& _functions, const Node& _filter) {
    if (_filter.IsSequence()) {
        std::vector<Filter> filters;

        for (const auto& filt : _filter) {
            if (Filter f = generateFilter(_functions, filt)) {
                filters.push_back(std::move(f));
            } else { return Filter(); }
        }
        return Filter::MatchAll(std::move(filters));
    }
    return Filter();
}

Filter SceneLoader::generateNoneFilter(SceneFunctions& _functions, const Node& _filter) {
    if (_filter.IsSequence()) {
        std::vector<Filter> filters;

        for (const auto& filt : _filter) {
            if (Filter f = generateFilter(_functions, filt)) {
                filters.push_back(std::move(f));
            } else { return Filter(); }
        }
        return Filter::MatchNone(std::move(filters));

    } else if (_filter.IsMap() || _filter.IsScalar()) {
        // 'not' case
        if (Filter f = generateFilter(_functions, _filter)) {
            return Filter::MatchNone({std::move(f)});
        }
    }
    return Filter();
}

Filter SceneLoader::generatePredicate(const Node& _node, std::string _key) {
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

        const Node& max = _node["max"];
        const Node& min = _node["min"];
        if (max && min && max.IsScalar() && min.IsScalar() &&
                (hasMinPixelArea != hasMaxPixelArea)) { return Filter(); }

        return Filter::MatchRange(_key, minVal, maxVal, hasMinPixelArea | hasMaxPixelArea);
    }
    default:
        return Filter();
    }
}

bool SceneLoader::getFilterRangeValue(const Node& _node, double& _val, bool& _hasPixelArea) {
    if (!YamlUtil::getDouble(_node, _val, false)) {
        auto strVal = _node.Scalar();
        auto n = strVal.find("px2");
        if (n == std::string::npos) { return false; }
        try {
            _val = ff::stof(std::string(strVal, 0, n));
            _hasPixelArea = true;
        } catch (std::invalid_argument) { return false; }
    }
    return true;
}


}
