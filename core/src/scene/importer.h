#pragma once

#include "util/url.h"

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "yaml-cpp/yaml.h"
#include "scene/scene.h"

namespace Tangram {

class Platform;
class Asset;

class Importer {

public:

    using Node = YAML::Node;

    // Loads the main scene with deep merging dependent imported scenes.
    Node applySceneImports(const std::shared_ptr<Platform>& platform, std::shared_ptr<Scene>& scene);

    void resolveSceneUrls(const std::shared_ptr<Platform>& platform, Scene& scene, Node& root, const Url& base);

    // protected for testing purposes, else could be private
protected:

    void processScene(const std::shared_ptr<Platform>& platform, std::shared_ptr<Scene>& scene,
                      const Url& scenePath, const std::string& sceneString);

    // Get the sequence of scene names that are designated to be imported into the
    // input scene node by its 'import' fields.
    std::vector<Url> getResolvedImportUrls(const std::shared_ptr<Platform>& platform,
                                           std::shared_ptr<Scene>& scene, const Node& sceneNode, const Url& base);

    // loads all the imported scenes and the master scene and returns a unified YAML root node.
    void importScenesRecursive(const std::shared_ptr<Platform>& platform, std::shared_ptr<Scene>& scene,
                               Node& root, const Url& scenePath, std::vector<Url>& sceneStack);

    void mergeMapFields(Node& target, const Node& import);

    // import scene to respective root nodes
    std::unordered_map<Url, Node> m_scenes;

    std::vector<Url> m_sceneQueue;

    const unsigned int MAX_SCENE_DOWNLOAD = 4;
};

}
