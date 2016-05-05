#pragma once

#include <memory>
#include <string>
#include <vector>

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

private:

    // Get the sequence of scene names that are designated to be imported into the
    // input scene node by its 'import' fields.
    std::vector<std::string> getScenesToImport(const Node& scene);

    // Get a sequence of scene names ordered such that if scene 'a' imports scene
    // 'b', 'b' will always precede 'a' in the sequence.
    std::vector<std::string> getImportOrder();

    // loads import scenes recursively and saves in m_scenes
    bool loadScene(const std::string& scenePath);

    // loads all the imported scenes and the master scene and returns a unified YAML root node.
    Node importScenes(const std::string& sceneName);

    //void mergeMapFieldsTaskingLast(const std::string& key, Node target, const std::vector<Node>& imports);
    void mergeMapFields(Node& target, const Node& import);

    // import scene to respective root nodes
    fastmap<std::string, Node> m_scenes;

};

}
