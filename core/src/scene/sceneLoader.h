#pragma once

#include <string>

class Scene;
class TileManager;
class View;

class SceneLoader {

public:

    SceneLoader() {};

    virtual ~SceneLoader() {};

    void loadScene(const std::string& _file, Scene& _scene, TileManager& _tileManager, View& _view);
    
};
