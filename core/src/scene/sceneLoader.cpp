#include <vector>
#include "platform.h"
#include "scene.h"
#include "sceneLoader.h"
#include "tileManager.h"
#include "view.h"
#include "lights.h"
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
#include "text/fontContext.h"

#include "yaml-cpp/yaml.h"

#include <algorithm>

using YAML::Node;
using YAML::BadConversion;

namespace Tangram {

// TODO: make this configurable: 16MB default in-memory DataSource cache:
constexpr size_t CACHE_SIZE = 16 * (1024 * 1024);

void SceneLoader::loadScene(const std::string& _sceneString, Scene& _scene, TileManager& _tileManager, View& _view) {

    try {
        Node config = YAML::Load(_sceneString);

        loadSources(config["sources"], _tileManager);
        loadTextures(config["textures"], _scene);
        loadStyles(config["styles"], _scene);
        loadLayers(config["layers"], _scene, _tileManager);
        loadCameras(config["cameras"], _view);
        loadLights(config["lights"], _scene);

        for (auto& style : _scene.styles()) {
            style->build(_scene.lights());
        }

        // Styles that are opaque must be ordered first in the scene so that they are rendered 'under' styles that require blending
        std::sort(_scene.styles().begin(), _scene.styles().end(), [](std::unique_ptr<Style>& a, std::unique_ptr<Style>& b) {
                return a->isOpaque();
            });
    } catch (YAML::ParserException e) {
        logMsg("Error: Parsing scene config '%s'\n", e.what());
    } catch (YAML::RepresentationException e) {
        logMsg("Error: Parsing scene config '%s'\n", e.what());
    }
}

std::string parseSequence(const Node& node) {
    std::stringstream sstream;
    for (const auto& val : node) {
        try {
            sstream << val.as<float>() << ",";
        } catch (const BadConversion& e) {
            logMsg("Error: Float value was expected for styleParam sequence value\n");
        }
    }
    return sstream.str();
}

glm::vec4 parseVec4(const Node& node) {
    glm::vec4 vec;
    int i = 0;
    for (const auto& nodeVal : node) {
        if (i < 4) {
            vec[i++] = nodeVal.as<float>();
        } else {
            break;
        }
    }
    return vec;
}

glm::vec3 parseVec3(const Node& node) {
    glm::vec3 vec;
    int i = 0;
    for (const auto& nodeVal : node) {
        if (i < 3) {
            vec[i++] = nodeVal.as<float>();
        } else {
            break;
        }
    }
    return vec;
}

void SceneLoader::loadShaderConfig(YAML::Node shaders, ShaderProgram& shader) {

    if (!shaders) {
        return;
    }

    Node definesNode = shaders["defines"];
    if (definesNode) {
        for (const auto& define : definesNode) {
            std::string name = define.first.as<std::string>();
            std::string value = define.second.as<std::string>();
            if (value == "true") {
                value = ""; // specifying a define to be 'true' means that it is simply defined and has no value
            } else if (value == "false") {
                continue; // specifying a define to be 'false' means that the define will not be defined at all
            }
            shader.addSourceBlock("defines", "#define " + name + " " + value);
        }
    }

    Node uniformsNode = shaders["uniforms"];
    if (uniformsNode) {
        for (const auto& uniform : uniformsNode) {
            std::string name = uniform.first.as<std::string>();
            std::string value = uniform.first.as<std::string>();
            shader.addSourceBlock("uniforms", "uniform " + name + " = " + value + ";");
        }
    }

    Node blocksNode = shaders["blocks"];
    if (blocksNode) {
        for (const auto& block : blocksNode) {
            std::string name = block.first.as<std::string>();
            std::string value = block.second.as<std::string>();
            shader.addSourceBlock(name, value); // TODO: Warn on unrecognized injection points
        }
    }

}

void SceneLoader::loadMaterial(YAML::Node matNode, Material& material, Scene& scene) {

    Node diffuse = matNode["diffuse"];
    if (diffuse) {
        if (diffuse.IsMap()) {
            material.setDiffuse(loadMaterialTexture(diffuse, scene));
        } else if (diffuse.IsSequence()) {
            material.setDiffuse(parseVec4(diffuse));
        } else {
            try {
                float difValue = diffuse.as<float>();
                material.setDiffuse(glm::vec4(difValue, difValue, difValue, 1.0));
            } catch (const BadConversion& e) {
                // TODO: css color parser and hex_values
            }
        }
    }

    Node ambient = matNode["ambient"];
    if (ambient) {
        if (ambient.IsMap()) {
            material.setAmbient(loadMaterialTexture(ambient, scene));
        } else if (ambient.IsSequence()) {
            material.setAmbient(parseVec4(ambient));
        } else {
            try {
                float ambientValue = ambient.as<float>();
                material.setAmbient(glm::vec4(ambientValue, ambientValue, ambientValue, 1.0));
            } catch (const BadConversion& e) {
                // TODO: css color parser and hex_values
            }
        }
    }

    Node specular = matNode["specular"];
    if (specular) {
        if (specular.IsMap()) {
            material.setSpecular(loadMaterialTexture(specular, scene));
        } else if (specular.IsSequence()) {
            material.setSpecular(parseVec4(specular));
        } else {
            try {
                float specValue = specular.as<float>();
                material.setSpecular(glm::vec4(specValue, specValue, specValue, 1.0));
            } catch (const BadConversion& e) {
                // TODO: css color parser and hex_values
            }
        }
    }

    Node shininess = matNode["shininess"];

    if (shininess) {
        try {
            material.setShininess(shininess.as<float>());
        } catch(const BadConversion& e) {
            logMsg("Error: float value expected for shininess material parameter");
        }
    }

    Node normal = matNode["normal"];
    if (normal) {
        material.setNormal(loadMaterialTexture(normal, scene));
    }

}

MaterialTexture SceneLoader::loadMaterialTexture(YAML::Node matCompNode, Scene& scene) {

    MaterialTexture matTex;

    Node textureNode = matCompNode["texture"];
    Node mappingNode = matCompNode["mapping"];
    Node scaleNode = matCompNode["scale"];
    Node amountNode = matCompNode["amount"];

    if (!textureNode) {
        logMsg("WARNING: Expected a \"texture\" parameter; Material may be incorrect.\n");
        return matTex;
    }

    std::string name = textureNode.as<std::string>();

    auto& tex = scene.textures()[name];

    if (!tex) {
        tex = std::make_shared<Texture>(name);
    }

    matTex.tex = tex;

    if (!matTex.tex) { matTex.tex.reset(new Texture(name)); }

    if (mappingNode) {
        std::string mapping = mappingNode.as<std::string>();
        if (mapping == "uv") { matTex.mapping = MappingType::uv; }
        else if (mapping == "planar") { logMsg("WARNING: planar texture mapping not yet implemented\n"); } // TODO
        else if (mapping == "triplanar") { logMsg("WARNING: triplanar texture mapping not yet implemented\n"); } // TODO
        else if (mapping == "spheremap") { matTex.mapping = MappingType::spheremap; }
        else { logMsg("WARNING: unrecognized texture mapping \"%s\"\n", mapping.c_str()); }
    }

    if (scaleNode) {
        if (scaleNode.IsSequence() && scaleNode.size() == 2) {
            matTex.scale = { scaleNode[0].as<float>(), scaleNode[1].as<float>(), 1.f };
        } else if (scaleNode.IsScalar()) {
            matTex.scale = glm::vec3(scaleNode.as<float>());
        } else {
            logMsg("WARNING: unrecognized scale parameter in material\n");
        }
    }

    if (amountNode) {
        if (amountNode.IsSequence() && amountNode.size() == 3) {
            matTex.amount = { amountNode[0].as<float>(), amountNode[1].as<float>(), amountNode[2].as<float>() };
        } else if (amountNode.IsScalar()) {
            matTex.amount = glm::vec3(amountNode.as<float>());
        } else {
            logMsg("WARNING: unrecognized amount parameter in material\n");
        }
    }

    return matTex;
}

void SceneLoader::loadTextures(YAML::Node textures, Scene& scene) {

    if (!textures) {
        return;
    }

    for (const auto& textureNode : textures) {

        std::string name = textureNode.first.as<std::string>();
        Node textureConfig = textureNode.second;

        std::string file;
        TextureOptions options = {GL_RGBA, GL_RGBA, {GL_LINEAR, GL_LINEAR}, {GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE} };

        Node url = textureConfig["url"];
        if (url) {
            file = url.as<std::string>();
        } else {
            logMsg("WARNING: No url specified for texture \"%s\", skipping.\n", name.c_str());
            continue;
        }

        Node filtering = textureConfig["filtering"];
        if (filtering) {
            std::string f = filtering.as<std::string>();
            if (f == "linear") { options.m_filtering = { GL_LINEAR, GL_LINEAR }; }
            else if (f == "mipmap") { options.m_filtering = { GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR_MIPMAP_LINEAR }; }
            else if (f == "nearest") { options.m_filtering = { GL_NEAREST, GL_NEAREST }; }
        }

        Node sprites = textureConfig["sprites"];
        if (sprites) { logMsg("WARNING: sprite mapping not yet implemented\n"); } // TODO

        scene.textures().emplace(name, std::make_shared<Texture>(file, options));
    }
}

void SceneLoader::loadStyles(YAML::Node styles, Scene& scene) {

    auto fontCtx = FontContext::GetInstance(); // To add font for debugTextStyle
    fontCtx->addFont("FiraSans", "Medium", "");
    // Instantiate built-in styles
    scene.styles().emplace_back(new PolygonStyle("polygons"));
    scene.styles().emplace_back(new PolylineStyle("lines"));
    scene.styles().emplace_back(new TextStyle("text", true, true));
    scene.styles().emplace_back(new DebugTextStyle("FiraSans_Medium_", "debugtext", 30.0f, true));
    scene.styles().emplace_back(new DebugStyle("debug"));
    scene.styles().emplace_back(new SpriteStyle("sprites"));

    if (!styles) {
        return;
    }

    for (const auto& styleIt : styles) {

        Style* style = nullptr;

        std::string styleName = styleIt.first.as<std::string>();
        Node styleNode = styleIt.second;

        bool validName = true;
        for (auto builtIn : { "polygons", "lines", "points", "text", "debug", "debugtext" }) {
            if (styleName == builtIn) { validName = false; }
        }
        if (!validName) {
            logMsg("WARNING: cannot use built-in style name \"%s\" for new style\n", styleName.c_str());
            continue;
        }

        Node baseNode = styleNode["base"];
        if (baseNode) {
            std::string baseString = baseNode.as<std::string>();
            if (baseString == "lines") { style = new PolylineStyle(styleName); }
            else if (baseString == "text") { style = new TextStyle(styleName, true, true); }
            else if (baseString == "sprites") { logMsg("WARNING: sprite base styles not yet implemented\n"); } // TODO
            else { logMsg("WARNING: base style \"%s\" not recognized, defaulting to polygons\n", baseString.c_str()); }
        }

        if (style == nullptr) { style = new PolygonStyle(styleName); }

        Node animatedNode = styleNode["animated"];
        if (animatedNode) { logMsg("WARNING: animated styles not yet implemented\n"); } // TODO

        Node blendNode = styleNode["blend"];
        if (blendNode) {

            logMsg("WARNING: blending modes not yet implemented\n");

            std::string blend = blendNode.as<std::string>();
            if (blend == "add") { } // TODO
            else if (blend == "multiply") { } // TODO

        }

        Node texcoordsNode = styleNode["texcoords"];
        if (texcoordsNode) {

            logMsg("WARNING: texcoords style parameter is currently ignored\n");

            if (texcoordsNode.as<bool>()) { } // TODO
            else { } // TODO

        }

        Node shadersNode = styleNode["shaders"];
        if (shadersNode) {
            loadShaderConfig(shadersNode, *(style->getShaderProgram()));
        }

        Node materialNode = styleNode["material"];
        if (materialNode) {
            loadMaterial(materialNode, *(style->getMaterial()), scene);
        }

        Node lightingNode = styleNode["lighting"];
        if (lightingNode) {
            std::string lighting = lightingNode.as<std::string>();
            if (lighting == "fragment") { style->setLightingType(LightingType::fragment); }
            else if (lighting == "vertex") { style->setLightingType(LightingType::vertex); }
            else if (lighting == "false") { style->setLightingType(LightingType::none); }
            else if (lighting == "true") { } // use default lighting
            else { logMsg("WARNING: unrecognized lighting type \"%s\"\n", lighting.c_str()); }
        }

        Node urlNode = styleNode["url"];
        if (urlNode) { logMsg("WARNING: loading style from URL not yet implemented\n"); } // TODO

        scene.styles().push_back(std::unique_ptr<Style>(style));

    }

}

void SceneLoader::loadSources(Node sources, TileManager& tileManager) {

    if (!sources) {
        logMsg("Warning: No source defined in the yaml scene configuration.\n");
        return;
    }

    for (const auto& src : sources) {

        const Node source = src.second;
        std::string name = src.first.as<std::string>();
        std::string type = source["type"].as<std::string>();
        std::string url = source["url"].as<std::string>();

        std::shared_ptr<DataSource> sourcePtr;

        if (type == "GeoJSONTiles") {
            sourcePtr = std::shared_ptr<DataSource>(new GeoJsonSource(name, url));
            sourcePtr->setCacheSize(CACHE_SIZE);
        } else if (type == "TopoJSONTiles") {
            logMsg("WARNING: TopoJSON data sources not yet implemented\n"); // TODO
        } else if (type == "MVT") {
            sourcePtr = std::shared_ptr<DataSource>(new MVTSource(name, url));
            sourcePtr->setCacheSize(CACHE_SIZE);
        } else {
            logMsg("WARNING: unrecognized data source type \"%s\", skipping\n", type.c_str());
        }

        if (sourcePtr) {
            tileManager.addDataSource(std::move(sourcePtr));
        }
    }

}

void SceneLoader::loadLights(Node lights, Scene& scene) {

    if (!lights) {

        // Add an ambient light if nothing else is specified
        std::unique_ptr<AmbientLight> amb(new AmbientLight("defaultLight"));
        amb->setAmbientColor({ .5f, .5f, .5f, 1.f });
        scene.lights().push_back(std::move(amb));

        return;
    }

    for (const auto& lt : lights) {

        const Node light = lt.second;
        const std::string name = lt.first.Scalar();
        const std::string type = light["type"].as<std::string>();

        std::unique_ptr<Light> lightPtr;

        if (type == "ambient") {

            lightPtr = std::unique_ptr<Light>(new AmbientLight(name));

        } else if (type == "directional") {

            DirectionalLight* dLightPtr = new DirectionalLight(name);
            Node direction = light["direction"];
            if (direction) {
                dLightPtr->setDirection(parseVec3(direction));
            }
            lightPtr = std::unique_ptr<Light>(dLightPtr);

        } else if (type == "point") {

            PointLight* pLightPtr = new PointLight(name);
            Node position = light["position"];
            if (position) {
                pLightPtr->setPosition(parseVec3(position));
            }
            Node radius = light["radius"];
            if (radius) {
                if (radius.size() > 1) {
                    pLightPtr->setRadius(radius[0].as<float>(), radius[1].as<float>());
                } else {
                    pLightPtr->setRadius(radius.as<float>());
                }
            }
            Node att = light["attenuation"];
            if (att) {
                pLightPtr->setAttenuation(att.as<float>());
            }
            lightPtr = std::unique_ptr<Light>(pLightPtr);

        } else if (type == "spotlight") {

            SpotLight* sLightPtr = new SpotLight(name);
            Node position = light["position"];
            if (position) {
                sLightPtr->setPosition(parseVec3(position));
            }
            Node direction = light["direction"];
            if (direction) {
                sLightPtr->setDirection(parseVec3(direction));
            }
            Node radius = light["radius"];
            if (radius) {
                if (radius.size() > 1) {
                    sLightPtr->setRadius(radius[0].as<float>(), radius[1].as<float>());
                } else {
                    sLightPtr->setRadius(radius.as<float>());
                }
            }
            Node angle = light["angle"];
            if (angle) {
                sLightPtr->setCutoffAngle(angle.as<float>());
            }
            Node exponent = light["exponent"];
            if (exponent) {
                sLightPtr->setCutoffExponent(exponent.as<float>());
            }

            lightPtr = std::unique_ptr<Light>(sLightPtr);

        }

        Node origin = light["origin"];
        if (origin) {
            const std::string originStr = origin.as<std::string>();
            if (originStr == "world") {
                lightPtr->setOrigin(LightOrigin::world);
            } else if (originStr == "camera") {
                lightPtr->setOrigin(LightOrigin::camera);
            } else if (originStr == "ground") {
                lightPtr->setOrigin(LightOrigin::ground);
            }
        }

        Node ambient = light["ambient"];
        if (ambient) {
            lightPtr->setAmbientColor(parseVec4(ambient));
        }

        Node diffuse = light["diffuse"];
        if (diffuse) {
            lightPtr->setDiffuseColor(parseVec4(diffuse));
        }

        Node specular = light["specular"];
        if (specular) {
            lightPtr->setSpecularColor(parseVec4(specular));
        }

        scene.lights().push_back(std::move(lightPtr));

    }

}

void SceneLoader::loadCameras(Node cameras, View& view) {

    // To correctly match the behavior of the webGL library we'll need a place to store multiple view instances.
    // Since we only have one global view right now, we'll just apply the settings from the first active camera we find.

    for (const auto& cam : cameras) {

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
        glm::dvec2 projPos = view.getMapProjection().LonLatToMeters(glm::dvec2(x, y));
        view.setPosition(projPos.x, projPos.y);

        Node zoom = camera["zoom"];
        if (zoom) {
            z = zoom.as<float>();
        }
        view.setZoom(z);

        Node active = camera["active"];
        if (active) {
            break;
        }

    }

}

Filter SceneLoader::generateFilter(YAML::Node _filter) {

    if (!_filter) {
        return Filter();
    }

    std::vector<Filter> filters;

    for (const auto& filtItr : _filter) {
        Filter filter;

        if (_filter.IsSequence()) {

            filter = generateFilter(filtItr);

        } else if (filtItr.first.as<std::string>() == "none") {

            filter = generateNoneFilter(_filter["none"]);

        } else if (filtItr.first.as<std::string>() == "not") {

            filter = generateNoneFilter(_filter["not"]);

        } else if (filtItr.first.as<std::string>() == "any") {

            filter = generateAnyFilter(_filter["any"]);

        } else if (filtItr.first.as<std::string>() == "all") {

            filter = generateFilter(_filter["all"]);

        } else {

            std::string key = filtItr.first.as<std::string>();
            filter = generatePredicate(_filter[key], key);

        }

        filters.push_back(filter);

    }

    if (filters.size() == 1) {
        return filters.front();
    } else if (filters.size() > 0) {
        return (Filter(Operators::all, filters));
    } else {
        return Filter();
    }

}

Filter SceneLoader::generatePredicate(YAML::Node _node, std::string _key) {

    if (_node.IsScalar()) {
        if (_node.Tag() == "tag:yaml.org,2002:str") {
            // Node was explicitly tagged with '!!str' or the canonical tag 'tag:yaml.org,2002:str'
            // yaml-cpp normalizes the tag value to the canonical form
            return Filter(_key, { Value(_node.as<std::string>()) });
        }
        try {
            return Filter(_key, { Value(_node.as<float>()) });
        } catch (const BadConversion& e) {
            std::string value = _node.as<std::string>();
            if (value == "true") {
                return Filter(_key, true);
            } else if (value == "false") {
                return Filter(_key, false);
            } else {
                return Filter(_key, { Value(value) });
            }
        }
    } else if (_node.IsSequence()) {
        std::vector<Value> values;
        for (const auto& valItr : _node) {
            try {
                values.emplace_back(valItr.as<float>());
            } catch(const BadConversion& e) {
                std::string value = valItr.as<std::string>();
                values.emplace_back(value);
            }
        }
        return Filter(_key, std::move(values));
    } else if (_node.IsMap()) {
        float minVal = -std::numeric_limits<float>::infinity();
        float maxVal = std::numeric_limits<float>::infinity();

        for (const auto& valItr : _node) {
            if (valItr.first.as<std::string>() == "min") {
                try {
                    minVal = valItr.second.as<float>();
                } catch (const BadConversion& e) {
                    logMsg("Error: Badly formed filter.\tExpect a float value type, string found.\n");
                    return Filter();
                }
            } else if (valItr.first.as<std::string>() == "max") {
                try {
                    maxVal = valItr.second.as<float>();
                } catch (const BadConversion& e) {
                    logMsg("Error: Badly formed filter.\tExpect a float value type, string found.\n");
                    return Filter();
                }
            } else {
                logMsg("Error: Badly formed Filter\n");
                return Filter();
            }
        }
        return Filter(_key, minVal, maxVal);

    } else {
        logMsg("Error: Badly formed Filter\n");
        return Filter();
    }

}

Filter SceneLoader::generateAnyFilter(YAML::Node _filter) {
    std::vector<Filter> filters;

    if (!_filter.IsSequence()) {
        logMsg("Error: Badly formed filter. \"Any\" expects a list.\n");
        return Filter();
    }
    for (const auto& filt : _filter) {
        filters.emplace_back(generateFilter(filt));
    }
    return Filter(Operators::any, std::move(filters));
}

Filter SceneLoader::generateNoneFilter(YAML::Node _filter) {

    std::vector<Filter> filters;

    if (_filter.IsSequence()) {
        for (const auto& filt : _filter) {
            filters.emplace_back(generateFilter(filt));
        }
    } else if (_filter.IsMap()) { // not case
        for (const auto& filt : _filter) {
            std::string keyFilter = filt.first.as<std::string>();
            filters.emplace_back(generatePredicate(_filter[keyFilter], keyFilter));
        }
    } else {
        logMsg("Error: Badly formed filter. \"None\" expects a list or an object.\n");
        return Filter();
    }

    return Filter(Operators::none, std::move(filters));
}

std::vector<StyleParam> SceneLoader::parseStyleParams(Node params, const std::string& prefix) {

    std::vector<StyleParam> out;

    for (const auto& prop : params) {

        // Load Fonts if prop is font
        if (prop.first.as<std::string>() == "font") {
            loadFont(prop.second);
        }

        std::string key = (prefix.empty() ? "" : (prefix + ":")) + prop.first.as<std::string>();

        Node value = prop.second;

        switch (value.Type()) {
            case YAML::NodeType::Scalar:   out.push_back({ key, value.as<std::string>() }); break;
            case YAML::NodeType::Sequence: out.push_back({ key, parseSequence(value) }); break;
            case YAML::NodeType::Map: {
                auto subparams = parseStyleParams(value, key);
                out.insert(out.end(), subparams.begin(), subparams.end());
                break;
            }
            default: logMsg("ERROR: Style parameter %s must be a scalar, sequence, or map.\n", key.c_str());
        }
    }

    return out;

}

void SceneLoader::loadFont(YAML::Node fontProps) {

    std::string name = "";
    std::string weight = "";
    std::string face = "";
    auto fontCtx = FontContext::GetInstance();

    auto nameNode = fontProps["name"];
    if (nameNode) { name = nameNode.as<std::string>(); }

    auto weightNode = fontProps["weight"];
    if (weightNode) { weight = weightNode.as<std::string>(); }

    auto faceNode = fontProps["face"];
    if (faceNode) { face = faceNode.as<std::string>(); }

    fontCtx->addFont(std::move(name), std::move(weight), std::move(face));

}

SceneLayer SceneLoader::loadSublayer(YAML::Node layer, const std::string& name, Scene& scene) {

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
                auto style = explicitStyle ? explicitStyle.as<std::string>() : ruleNode.first.as<std::string>();
                auto params = parseStyleParams(ruleNode.second);
                rules.push_back({ style, params });

            }
        } else if (key == "filter") {
            filter = generateFilter(member.second);
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

void SceneLoader::loadLayers(Node layers, Scene& scene, TileManager& tileManager) {

    if (!layers) {
        return;
    }

    for (const auto& layer : layers) {

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

}
