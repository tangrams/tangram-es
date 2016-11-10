#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <atomic>
#include <condition_variable>

#include "util/fastmap.h"

namespace YAML {
    class Node;
}

namespace Tangram {

class Importer {

public:

    using Node = YAML::Node;

    // Loads the main scene with deep merging dependentent imported scenes.
    Node applySceneImports(const std::string& scenePath, const std::string& resourceRoot = "");

// protected for testing purposes, else could be private
protected:
    virtual std::string getSceneString(const std::string& scenePath);
    void processScene(const std::string& scenePath, const std::string& sceneString);

    // Get the sequence of scene names that are designated to be imported into the
    // input scene node by its 'import' fields.
    std::vector<std::string> getScenesToImport(const Node& scene);

    // loads all the imported scenes and the master scene and returns a unified YAML root node.
    void importScenes(Node& root, const std::string& sceneName, std::vector<std::string>& sceneStack);

    //void mergeMapFieldsTaskingLast(const std::string& key, Node target, const std::vector<Node>& imports);
    void mergeMapFields(Node& target, const Node& import);

    void normalizeSceneTextures(Node& root, const std::string& parentPath);
    void setNormalizedTexture(Node& texture, const std::vector<std::string>& names,
            const std::string& parentPath);
    void normalizeSceneImports(Node& root, const std::string& parentPath);
    void normalizeSceneTileSources(Node& root, const std::string& parentPath);
    void normalizeFonts(Node& root, const std::string& parentPath);
    std::string normalizePath(const std::string& path, const std::string& parentPath);

private:
    // import scene to respective root nodes
    std::map<std::string, Node> m_scenes;
    std::unordered_set<std::string> m_globalTextures;
    std::unordered_map<std::string, std::string> m_textureNames;

    std::vector<std::string> m_sceneQueue;
    static std::atomic_uint progressCounter;
    std::mutex sceneMutex;
    std::condition_variable m_condition;

    const unsigned int MAX_SCENE_DOWNLOAD = 4;
};

}
