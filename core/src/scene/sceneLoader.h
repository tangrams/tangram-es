#pragma once

#include <string>

class Scene;
class TileManager;
class View;

/* Forward Declaration of yaml-cpp node type */
namespace YAML {
    class Node;
}

namespace Tangram {
    struct Filter;
}

class SceneLoader {

    void loadSources(YAML::Node sources, TileManager& tileManager);
    void loadLights(YAML::Node lights, Scene& scene);
    void loadCameras(YAML::Node cameras, View& view);
    void loadLayers(YAML::Node layers, Scene& scene, TileManager& tileManager);
    Tangram::Filter* generateAnyFilter(YAML::Node filter);
    Tangram::Filter* generateNoneFilter(YAML::Node filter);
    Tangram::Filter* generatePredicate(YAML::Node filter, std::string _key);


public:

    SceneLoader() {};

    virtual ~SceneLoader() {};

    void loadScene(const std::string& _file, Scene& _scene, TileManager& _tileManager, View& _view);

    Tangram::Filter* generateFilter(YAML::Node filter);
};
