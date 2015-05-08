#include "sceneLoader.h"

#include "scene.h"
#include "tileManager.h"
#include "view.h"
#include "lights.h"

#include "yaml-cpp/yaml.h"
#include "glm/vec2.hpp"

using namespace YAML;

void loadCameras(Node cameras, View& view);

void loadLights(Node lights, Scene& scene);

void SceneLoader::loadScene(const std::string& _file, Scene& _scene, TileManager& _tileManager, View& _view) {

    Node config = YAML::LoadFile(_file);

    loadCameras(config["cameras"], _view);
    loadLights(config["lights"], _scene);
    
}

void loadCameras(Node cameras, View& view) {

    // Load defaults
    glm::dvec2 target = view.getMapProjection().LonLatToMeters(glm::dvec2(-74.00796, 40.70361));
    view.setPosition(target.x, target.y);

    if (!cameras) { return; }

    Node camera = cameras[0]; // We only support one camera for now

}

glm::vec4 parseVec4(const Node& node) {
    glm::vec4 vec;
    int i = 0;
    for (auto it = node.begin(); it != node.end(); ++it) {
        if (i < 4) {
            vec[i++] = it->as<float>();
        } else {
            break;
        }
    }
    return vec;
}

glm::vec3 parseVec3(const Node& node) {
    // for now I'll just parse 4-element color nodes
    glm::vec3 vec;
    int i = 0;
    for (auto it = node.begin(); it != node.end(); ++it) {
        if (i < 3) {
            vec[i++] = it->as<float>();
        } else {
            break;
        }
    }
    return vec;
}

void loadLights(Node lights, Scene& scene) {

    if (!lights) { 
        
        // Add an ambient light if nothing else is specified
        std::unique_ptr<AmbientLight> amb(new AmbientLight("defaultLight"));
        amb->setAmbientColor({ .5f, .5f, .5f, 1.f });
        scene.addLight(std::move(amb));

        return; 
    }

    for (auto it = lights.begin(); it != lights.end(); ++it) {

        const Node light = it->second;
        const std::string name = it->first.Scalar();
        const std::string type = light["type"].as<std::string>();

        std::unique_ptr<Light> lightPtr;
        if (type == "ambient") {
            lightPtr = std::unique_ptr<Light>(new AmbientLight(name));
        } else if (type == "directional") {
            DirectionalLight* dLightPtr = new DirectionalLight(name);
            if (light["direction"]) {
                dLightPtr->setDirection(parseVec3(light["direction"]));
            }
            lightPtr = std::unique_ptr<Light>(dLightPtr);
        } else if (type == "point") {
            PointLight* pLightPtr = new PointLight(name);
            if (light["position"]) {
                pLightPtr->setPosition(parseVec3(light["position"]));
            }
            lightPtr = std::unique_ptr<Light>(pLightPtr);

        } else if (type == "spotlight") {
            SpotLight* sLightPtr = new SpotLight(name);
            if (light["position"]) {
                sLightPtr->setPosition(parseVec3(light["position"]));
            }
            lightPtr = std::unique_ptr<Light>(sLightPtr);
        }

        if (light["ambient"]) {
            glm::vec4 color = parseVec4(light["ambient"]);
            lightPtr->setAmbientColor(color);
        }

        if (light["diffuse"]) {
            glm::vec4 color = parseVec4(light["diffuse"]);
            lightPtr->setDiffuseColor(color);
        }

        if (light["specular"]) {
            glm::vec4 color = parseVec4(light["specular"]);
            lightPtr->setSpecularColor(color);
        }

        scene.addLight(std::move(lightPtr));

    }

}
