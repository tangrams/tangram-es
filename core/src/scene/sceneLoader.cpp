#include "sceneLoader.h"

#include "scene.h"
#include "tileManager.h"
#include "view.h"
#include "lights.h"

#include "yaml-cpp/yaml.h"

using namespace YAML;

void loadLights(Node lights, Scene& scene);

void SceneLoader::loadScene(const std::string& _file, Scene& _scene, TileManager& _tileManager, View& _view) {

    Node config = YAML::LoadFile(_file);
    
    loadLights(config["lights"], _scene);
    
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
            if (light["radius"]) {
                Node rad = light["radius"];
                if (rad.size() > 1) {
                    pLightPtr->setRadius(rad[0].as<float>(), rad[1].as<float>());
                } else {
                    pLightPtr->setRadius(rad.as<float>());
                }
            }
            if (light["attenuation"]) {
                pLightPtr->setAttenuation(light["attenuation"].as<float>());
            }
            lightPtr = std::unique_ptr<Light>(pLightPtr);

        } else if (type == "spotlight") {

            SpotLight* sLightPtr = new SpotLight(name);
            if (light["position"]) {
                sLightPtr->setPosition(parseVec3(light["position"]));
            }
            if (light["direction"]) {
                sLightPtr->setDirection(parseVec3(light["direction"]));
            }
            if (light["radius"]) {
                Node rad = light["radius"];
                if (rad.size() > 1) {
                    sLightPtr->setRadius(rad[0].as<float>(), rad[1].as<float>());
                } else {
                    sLightPtr->setRadius(rad.as<float>());
                }
            }
            if (light["angle"]) {
                sLightPtr->setCutoffAngle(light["angle"].as<float>());
            }
            if (light["exponent"]) {
                sLightPtr->setCutoffExponent(light["exponent"].as<float>());
            }
            
            lightPtr = std::unique_ptr<Light>(sLightPtr);

        }
        
        if (light["origin"]) {
            const std::string origin = light["origin"].as<std::string>();
            if (origin == "world") {
                lightPtr->setOrigin(LightOrigin::WORLD);
            } else if (origin == "camera") {
                lightPtr->setOrigin(LightOrigin::CAMERA);
            } else if (origin == "ground") {
                lightPtr->setOrigin(LightOrigin::GROUND);
            }
        }

        if (light["ambient"]) {
            lightPtr->setAmbientColor(parseVec4(light["ambient"]));
        }

        if (light["diffuse"]) {
            lightPtr->setDiffuseColor(parseVec4(light["diffuse"]));
        }

        if (light["specular"]) {
            lightPtr->setSpecularColor(parseVec4(light["specular"]));
        }

        scene.addLight(std::move(lightPtr));

    }

}
