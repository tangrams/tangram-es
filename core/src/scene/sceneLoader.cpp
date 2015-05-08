#include "sceneLoader.h"

#include "scene.h"
#include "tileManager.h"
#include "view.h"
#include "lights.h"
#include "geoJsonSource.h"
#include "protobufSource.h"

#include "yaml-cpp/yaml.h"

using namespace YAML;

void loadSources(Node sources, TileManager& tileManager);
void loadLights(Node lights, Scene& scene);

void SceneLoader::loadScene(const std::string& _file, Scene& _scene, TileManager& _tileManager, View& _view) {

    Node config = YAML::LoadFile(_file);
    
    loadSources(config["sources"], _tileManager);
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

void loadSources(Node sources, TileManager& tileManager) {
    
    for (auto it = sources.begin(); it != sources.end(); ++it) {
        
        const Node source = it->second;
        std::string type = source["type"].as<std::string>();
        std::string url = source["url"].as<std::string>();
        
        std::unique_ptr<DataSource> sourcePtr;
        
        if (type == "GeoJSONTiles") {
            sourcePtr = std::unique_ptr<DataSource>(new GeoJsonSource());
        } else if (type == "TopoJSONTiles") {
            // TODO
        } else if (type == "MVT") {
            sourcePtr = std::unique_ptr<DataSource>(new ProtobufSource());
        }
        
        if (sourcePtr) {
            sourcePtr->setUrlTemplate(url);
            tileManager.addDataSource(std::move(sourcePtr));
        }
    }
    
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
                lightPtr->setOrigin(LightOrigin::WORLD);
            } else if (originStr == "camera") {
                lightPtr->setOrigin(LightOrigin::CAMERA);
            } else if (originStr == "ground") {
                lightPtr->setOrigin(LightOrigin::GROUND);
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

        scene.addLight(std::move(lightPtr));

    }

}
