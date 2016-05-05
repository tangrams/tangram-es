#include "importer.h"
#include "platform.h"
#include "util/topologicalSort.h"
#include "yaml-cpp/yaml.h"

#include <regex>

using YAML::Node;
using YAML::NodeType;

namespace Tangram {

Node Importer::applySceneImports(const std::string& scenePath) {

    if (loadScene(scenePath)) {
        return importScenes(scenePath);
    }

    return Node();
}

bool Importer::loadScene(const std::string& scenePath) {

    auto sceneString = stringFromFile(setResourceRoot(scenePath.c_str()).c_str(), PathType::resource);
    auto sceneName = scenePath;

    try {
        auto root = YAML::Load(sceneString);
        auto imports = getScenesToImport(root);
        std::regex r("[^//]+$");
        std::smatch match;
        if (std::regex_search(sceneName, match, r)) { sceneName = match[0]; }
        for (const auto& import : imports) {
            // TODO: What happens when parsing fails for an import
            // Relative path of imports with respect to the original
            auto importPath = std::regex_replace(scenePath, r, import);
            loadScene(importPath);
        }
        m_scenes[sceneName] = root;
    }
    catch (YAML::ParserException e) {
        LOGE("Parsing scene config '%s'", e.what());
        return false;
    }
    return true;
}

std::vector<std::string> Importer::getScenesToImport(const Node& scene) {

    std::vector<std::string> sceneNames;

    if (const Node& import = scene["import"]) {
        if (import.IsScalar()) {
            sceneNames.push_back(import.Scalar());
        }
        else if (import.IsSequence()) {
            for (const auto& imp : import) {
                if (imp.IsScalar()) { sceneNames.push_back(imp.Scalar()); }
            }
        }
    }

    return sceneNames;
}

std::vector<std::string> Importer::getImportOrder() {

    std::vector<std::pair<std::string, std::string>> dependencies;

    for (const auto& scene : m_scenes) {
        const auto& name = scene.first.k;
        const auto& sceneRoot = scene.second;
        for (const auto& import : getScenesToImport(sceneRoot)) {
            dependencies.push_back( {import, name} );
        }
    }

    return topologicalSort(dependencies);
}

Node Importer::importScenes(const std::string& sceneName) {

    auto importScenesSorted = getImportOrder();

    Node root = Node();

    for (const auto& import : importScenesSorted) {
        const auto& importRoot = m_scenes[import];
        if (!importRoot.IsMap()) { continue; }
        mergeMapFields(root, importRoot);
    }

    return root;

}

void Importer::mergeMapFields(Node& target, const Node& import) {

    for (const auto& entry : import) {

        const auto &key = entry.first.Scalar();

        if (!target[key]) {
            target[key] = entry.second;
            continue;
        }

        switch(entry.second.Type()) {
            case NodeType::Scalar:
            case NodeType::Sequence:
                target[key] = entry.second;
                break;
            case NodeType::Map: {
                Node newTarget = target[key];
                mergeMapFields(newTarget, entry.second);
                break;
            }
            default:
                break;
        }
    }
}

}
