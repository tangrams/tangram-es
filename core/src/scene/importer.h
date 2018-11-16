#pragma once

#include "scene/scene.h"
#include "util/url.h"

#include "yaml-cpp/yaml.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace Tangram {

class Platform;

class Importer {

public:

    using Node = YAML::Node;

    // Create an importer to operate on the given scene.
    Importer(std::shared_ptr<Scene> scene);

    // Loads the main scene with deep merging dependent imported scenes.
    Node applySceneImports(Platform& platform);

    static bool isZipArchiveUrl(const Url& url);

    static Url getBaseUrlForZipArchive(const Url& archiveUrl);

    static Url getArchiveUrlForZipEntry(const Url& zipEntryUrl);

    // Traverses the nodes contained in the given root scene node and for all
    // nodes that represent URLs, replaces the contents with that URL resolved
    // against the given base URL.
    static void resolveSceneUrls(Node& root, const Url& base);

protected:

    // Process and store data for an imported scene from a vector of bytes.
    void addSceneData(const Url& sceneUrl, std::vector<char>&& sceneContent);

    // Process and store data for an imported scene from a string of YAML.
    void addSceneYaml(const Url& sceneUrl, const char* sceneYaml, size_t length);

    // Get the sequence of scene names that are designated to be imported into the
    // input scene node by its 'import' fields.
    std::vector<Url> getResolvedImportUrls(const Node& sceneNode, const Url& base);

    // loads all the imported scenes and the master scene and returns a unified YAML root node.
    void importScenesRecursive(Node& root, const Url& sceneUrl, std::vector<Url>& sceneStack);

    void mergeMapFields(Node& target, const Node& import);

    // Importer holds a pointer to the scene it is operating on.
    std::shared_ptr<Scene> m_scene;

    // Imported scenes must be parsed into YAML nodes to find further imports.
    // The parsed scenes are stored in a map with their URLs to be merged once
    // all imports are found and parsed.
    std::unordered_map<Url, Node> m_importedScenes;

    std::vector<Url> m_sceneQueue;
};

}
