#pragma once

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

#include "util/fastmap.h"

namespace YAML {
    class Node;
}

namespace Tangram {

class Importer {

public:

    using Node = YAML::Node;

    // Loads the main scene with deep merging dependentent imported scenes.
    Node applySceneImports(const std::string& scenePath);

// protected for testing purposes, else could be private
protected:
    virtual std::string getSceneString(const std::string& scenePath);
    bool loadScene(const std::string& scenePath);

    // Get the sequence of scene names that are designated to be imported into the
    // input scene node by its 'import' fields.
    std::vector<std::string> getScenesToImport(const Node& scene);

    // Get a sequence of scene names ordered such that if scene 'a' imports scene
    // 'b', 'b' will always precede 'a' in the sequence.
    std::vector<std::string> getImportOrder(const std::string& baseScene);


    // loads all the imported scenes and the master scene and returns a unified YAML root node.
    Node importScenes(const std::string& sceneName);

    //void mergeMapFieldsTaskingLast(const std::string& key, Node target, const std::vector<Node>& imports);
    void mergeMapFields(Node& target, const Node& import);

    std::string getFilename(const std::string& scenePath);
    void normalizeSceneTextures(Node& root, const std::string& parentPath);
    void setNormalizedTexture(Node& texture, const std::vector<std::string>& names,
            const std::string& parentPath);
    void normalizeSceneImports(Node& root, const std::string& parentPath);
    std::string normalizePath(const std::string& path, const std::string& parentPath);

private:
    // import scene to respective root nodes
    fastmap<std::string, Node> m_scenes;
    std::unordered_set<std::string> m_globalTextures;
    std::unordered_map<std::string, std::string> m_textureNames;

};

}
