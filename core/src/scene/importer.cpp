#include "importer.h"
#include "platform.h"
#include "scene/sceneLoader.h"
#include "yaml-cpp/yaml.h"
#include "log.h"

#include <regex>

using YAML::Node;
using YAML::NodeType;

namespace Tangram {

std::atomic_uint Importer::progressCounter(0);

Node Importer::applySceneImports(const Url& scenePath, const Url& resourceRoot) {

    Url path;
    Url rootScenePath = scenePath.resolved(resourceRoot);

    m_sceneQueue.push_back(rootScenePath);

    while (true) {
        {
            std::unique_lock<std::mutex> lock(sceneMutex);

            m_condition.wait(lock, [&, this]{
                    if (m_sceneQueue.empty()) {
                        // Not busy at all?
                        if (progressCounter == 0) { return true; }
                    } else {
                        // More work and not completely busy?
                        if (progressCounter < MAX_SCENE_DOWNLOAD) { return true; }
                    }

                    return false;
                });


            if (m_sceneQueue.empty()) {
                if (progressCounter == 0) {
                    break;
                }
                continue;
            }

            path = m_sceneQueue.back();
            m_sceneQueue.pop_back();

            if (m_scenes.find(path) != m_scenes.end()) { continue; }
        }

        if (path.hasHttpScheme()) {
            progressCounter++;
            startUrlRequest(path.string(), [&, path](std::vector<char>&& rawData) {
                if (!rawData.empty()) {
                    std::unique_lock<std::mutex> lock(sceneMutex);
                    processScene(path, std::string(rawData.data(), rawData.size()));
                }
                progressCounter--;
                m_condition.notify_all();
            });
        } else {
            std::unique_lock<std::mutex> lock(sceneMutex);
            processScene(path, getSceneString(path));
        }
    }

    Node root = Node();

    LOGD("Processing scene import Stack:");
    std::vector<Url> sceneStack;
    std::unordered_set<std::string> globalTextures;
    importScenes(root, rootScenePath, sceneStack, globalTextures);

    return root;
}

void Importer::processScene(const Url& scenePath, const std::string &sceneString) {

    LOGD("Process: '%s'", scenePath.string().c_str());

    // Don't load imports twice
    if (m_scenes.find(scenePath) != m_scenes.end()) {
        return;
    }

    try {
        auto sceneNode = YAML::Load(sceneString);

        resolveSceneUrls(sceneNode, scenePath);

        m_scenes[scenePath] = sceneNode;

        for (const auto& import : getScenesToImport(sceneNode)) {
            m_sceneQueue.push_back(import);
            m_condition.notify_all();
        }
    } catch (YAML::ParserException e) {
        LOGE("Parsing scene config '%s'", e.what());
    }
}

void Importer::setResolvedTextureUrl(const std::unordered_set<std::string>& globalTextures,
                                     Node& textureNode, const Url& base) {

    if (!textureNode.IsScalar()) { return; }

    const auto& name = textureNode.Scalar();

    // If texture name matches a global texture then don't resolve it.
    if (globalTextures.find(name) != globalTextures.end()) { return; }

    // Assign resolved texture path to yaml node.
    textureNode = Url(name).resolved(base).string();
}

// Helper function; returns true if a node is a scalar that is not a null, bool, or number.
bool isStringNode(const Node& node) {
    bool booleanValue = false;
    double numberValue = 0.;
    if (node.IsNull() || !node.IsScalar()) { return false; }
    if (YAML::convert<bool>::decode(node, booleanValue)) { return false; }
    if (YAML::convert<double>::decode(node, numberValue)) { return false; }
    return true;
}

void Importer::resolveSceneUrls(Node& root, const Url& base) {

    // Resolve import URLs.

    if (Node import = root["import"]) {
        if (import.IsScalar()) {
            import = Url(import.Scalar()).resolved(base).string();
        } else if (import.IsSequence()) {
            for (Node path : import) {
                if (path.IsScalar()) {
                    path = Url(path.Scalar()).resolved(base).string();
                }
            }
        }
    }

    // Resolve data source URLs.

    if (Node sources = root["sources"]) {
        for (auto source : sources) {
            if (Node sourceUrl = source.second["url"]) {
                sourceUrl = Url(sourceUrl.Scalar()).resolved(base).string();
            }
        }
    }

    // Resolve font URLs.

    if (Node fonts = root["fonts"]) {
        if (fonts.IsMap()) {
            for (const auto& font : fonts) {
                if (font.second.IsMap()) {
                    auto urlNode = font.second["url"];
                    if (urlNode.IsScalar()) {
                        urlNode = Url(urlNode.Scalar()).resolved(base).string();
                    }
                } else if (font.second.IsSequence()) {
                    for (auto& fontNode : font.second) {
                        auto urlNode = fontNode["url"];
                        if (urlNode.IsScalar()) {
                            urlNode = Url(urlNode.Scalar()).resolved(base).string();
                        }
                    }
                }
            }
        }
    }
}

std::string Importer::getSceneString(const Url& scenePath) {
    return stringFromFile(scenePath.string().c_str());
}

std::vector<Url> Importer::getScenesToImport(const Node& scene) {

    std::vector<Url> scenePaths;

    if (const Node& import = scene["import"]) {
        if (import.IsScalar()) {
            scenePaths.push_back(Url(import.Scalar()));
        } else if (import.IsSequence()) {
            for (const auto& path : import) {
                if (path.IsScalar()) {
                    scenePaths.push_back(Url(path.Scalar()));
                }
            }
        }
    }

    return scenePaths;
}

void Importer::importScenes(Node& root, const Url& scenePath, std::vector<Url>& sceneStack,
                            std::unordered_set<std::string>& globalTextures) {

    LOGD("Starting importing Scene: %s", scenePath.string().c_str());

    for (const auto& s : sceneStack) {
        if (scenePath == s) {
            LOGE("%s will cause a cyclic import. Stopping this scene from being imported", scenePath.string().c_str());
            return;
        }
    }

    sceneStack.push_back(scenePath);

    auto sceneNode = m_scenes[scenePath];

    if (sceneNode.IsNull()) { return; }
    if (!sceneNode.IsMap()) { return; }

    // Resolve global texture URLs.
    Url base(scenePath);
    if (Node texturesNode = sceneNode["textures"]) {
        for (auto texture : texturesNode) {
            if (Node textureUrlNode = texture.second["url"]) {
                if (textureUrlNode.IsScalar()) {
                    textureUrlNode = Url(textureUrlNode.Scalar()).resolved(base).string();
                }
                globalTextures.insert(texture.first.Scalar());
            }
        }
    }

    auto imports = getScenesToImport(sceneNode);

    for (const auto& importScene : imports) {
        std::unordered_set<std::string> textures;

        importScenes(root, importScene, sceneStack, textures);

        for (auto& t : textures) { globalTextures.insert(t); }
    }

    // Resolve inline texture URLs after m_globalTextures are determined.
    if (Node styles = sceneNode["styles"]) {

        for (auto entry : styles) {

            Node style = entry.second;
            //style->texture
            if (Node texture = style["texture"]) {
                setResolvedTextureUrl(globalTextures, texture, base);
            }

            //style->material->texture
            if (Node material = style["material"]) {
                for (auto& prop : {"emission", "ambient", "diffuse", "specular", "normal"}) {
                    if (Node propNode = material[prop]) {
                        if (Node matTexture = propNode["texture"]) {
                            setResolvedTextureUrl(globalTextures, matTexture, base);
                        }
                    }
                }
            }

            //style->shader->uniforms->texture
            if (Node shaders = style["shaders"]) {
                if (Node uniforms = shaders["uniforms"]) {
                    for (auto uniformEntry : uniforms) {
                        Node uniformValue = uniformEntry.second;
                        if (isStringNode(uniformValue)) {
                            setResolvedTextureUrl(globalTextures, uniformValue, base);
                        } else if (uniformValue.IsSequence()) {
                            for (auto u : uniformValue) {
                                if (isStringNode(u)) {
                                    setResolvedTextureUrl(globalTextures, u, base);
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    sceneStack.pop_back();

    mergeMapFields(root, sceneNode);
}

void Importer::mergeMapFields(Node& target, const Node& import) {

    for (const auto& entry : import) {

        const auto &key = entry.first.Scalar();

        // do not merge ignore property
        if (key == "import") { continue; }

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
