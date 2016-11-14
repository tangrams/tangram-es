#pragma once

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <atomic>
#include <condition_variable>

#include "util/url.h"
#include "yaml-cpp/yaml.h"

namespace Tangram {

class Importer {

public:

    using Node = YAML::Node;

    // Loads the main scene with deep merging dependent imported scenes.
    Node applySceneImports(const Url& scenePath, const Url& resourceRoot = Url());

// protected for testing purposes, else could be private
protected:
    virtual std::string getSceneString(const Url& scenePath);

    void processScene(const Url& scenePath, const std::string& sceneString);

    // Get the sequence of scene names that are designated to be imported into the
    // input scene node by its 'import' fields.
    std::vector<Url> getResolvedImportUrls(const Node& scene, const Url& base);

    // loads all the imported scenes and the master scene and returns a unified YAML root node.
    void importScenesRecursive(Node& root, const Url& scenePath, std::vector<Url>& sceneStack);

    void mergeMapFields(Node& target, const Node& import);

    void resolveSceneUrls(Node& root, const Url& base);

private:
    // import scene to respective root nodes and whether imports have been resolved
    std::unordered_map<Url, std::pair<Node, bool>> m_scenes;

    std::vector<Url> m_sceneQueue;
    static std::atomic_uint progressCounter;
    std::mutex sceneMutex;
    std::condition_variable m_condition;

    std::set<std::string> m_globalTextures;

    const unsigned int MAX_SCENE_DOWNLOAD = 4;
};

}
