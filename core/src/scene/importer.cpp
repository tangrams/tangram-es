#include "scene/importer.h"

#include "log.h"
#include "platform.h"
#include "scene/sceneLoader.h"

#include "yaml-cpp/yaml.h"
#include <cassert>

using YAML::Node;
using YAML::NodeType;

namespace Tangram {

std::atomic_uint Importer::progressCounter(0);

Node Importer::applySceneImports(const std::shared_ptr<Platform>& platform, const Url& scenePath,
        const Url& resourceRoot) {

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
            platform->startUrlRequest(path.string(), [&, path](std::vector<char>&& rawData) {
                if (!rawData.empty()) {
                    std::unique_lock<std::mutex> lock(sceneMutex);
                    processScene(path, std::string(rawData.data(), rawData.size()));
                }
                progressCounter--;
                m_condition.notify_all();
            });
        } else {
            std::unique_lock<std::mutex> lock(sceneMutex);
            processScene(path, getSceneString(platform, path));
        }
    }

    Node root = Node();

    LOGD("Processing scene import Stack:");
    std::vector<Url> sceneStack;
    importScenesRecursive(platform, root, rootScenePath, sceneStack);

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

        m_scenes[scenePath] = sceneNode;

        for (const auto& import : getResolvedImportUrls(sceneNode, scenePath)) {
            m_sceneQueue.push_back(import);
            m_condition.notify_all();
        }
    } catch (YAML::ParserException e) {
        LOGE("Parsing scene config '%s'", e.what());
    }
}

bool nodeIsPotentialUrl(const Node& node) {
    // Check that the node is scalar and not null.
    if (!node || !node.IsScalar()) { return false; }

    // Check that the node does not contain a 'global' reference.
    if (node.Scalar().compare(0, 7, "global.") == 0) { return false; }

    return true;
}

bool nodeIsTextureUrl(const Node& node, const Node& textures) {

    if (!nodeIsPotentialUrl(node)) { return false; }

    // Check that the node is not a number or a boolean.
    bool booleanValue = false;
    double numberValue = 0.;
    if (YAML::convert<bool>::decode(node, booleanValue)) { return false; }
    if (YAML::convert<double>::decode(node, numberValue)) { return false; }

    // Check that the node does not name a scene texture.
    if (textures[node.Scalar()]) { return false; }

    return true;
}

void Importer::resolveSceneUrls(const std::shared_ptr<Platform>& platform, Node& root,
        const Url& base) {

    // Resolve global texture URLs.

    Node textures = root["textures"];

    if (textures) {
        for (auto texture : textures) {
            if (Node textureUrlNode = texture.second["url"]) {
                if (nodeIsPotentialUrl(textureUrlNode)) {
                    textureUrlNode = Url(textureUrlNode.Scalar()).resolved(base).string();
                }
            }
        }
    }

    // Resolve inline texture URLs.

    if (Node styles = root["styles"]) {

        for (auto entry : styles) {

            Node style = entry.second;
            if (!style.IsMap()) { continue; }

            //style->texture
            if (Node texture = style["texture"]) {
                if (nodeIsTextureUrl(texture, textures)) {
                    texture = Url(texture.Scalar()).resolved(base).string();
                }
            }

            //style->material->texture
            if (Node material = style["material"]) {
                if (!material.IsMap()) { continue; }
                for (auto& prop : {"emission", "ambient", "diffuse", "specular", "normal"}) {
                    if (Node propNode = material[prop]) {
                        if (!propNode.IsMap()) { continue; }
                        if (Node matTexture = propNode["texture"]) {
                            if (nodeIsTextureUrl(matTexture, textures)) {
                                matTexture = Url(matTexture.Scalar()).resolved(base).string();
                            }
                        }
                    }
                }
            }

            //style->shader->uniforms->texture
            if (Node shaders = style["shaders"]) {
                if (!shaders.IsMap()) { continue; }
                if (Node uniforms = shaders["uniforms"]) {
                    for (auto uniformEntry : uniforms) {
                        Node uniformValue = uniformEntry.second;
                        if (nodeIsTextureUrl(uniformValue, textures)) {
                            uniformValue = Url(uniformValue.Scalar()).resolved(base).string();
                        } else if (uniformValue.IsSequence()) {
                            for (Node u : uniformValue) {
                                if (nodeIsTextureUrl(u, textures)) {
                                    u = Url(u.Scalar()).resolved(base).string();
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // Resolve data source URLs.

    if (Node sources = root["sources"]) {
        for (auto source : sources) {
            if (!source.second.IsMap()) { continue; }
            if (Node sourceUrl = source.second["url"]) {
                if (nodeIsPotentialUrl(sourceUrl)) {
                    auto resolvedUrl = Url(sourceUrl.Scalar()).resolved(base);
                    sourceUrl = (resolvedUrl.isAbsolute()) ?
                            resolvedUrl.string() : platform->resolveAssetPath(resolvedUrl.string());
                }
            }
        }
    }

    // Resolve font URLs.

    if (Node fonts = root["fonts"]) {
        if (fonts.IsMap()) {
            for (const auto& font : fonts) {
                if (font.second.IsMap()) {
                    auto urlNode = font.second["url"];
                    if (nodeIsPotentialUrl(urlNode)) {
                        urlNode = Url(urlNode.Scalar()).resolved(base).string();
                    }
                } else if (font.second.IsSequence()) {
                    for (auto& fontNode : font.second) {
                        auto urlNode = fontNode["url"];
                        if (nodeIsPotentialUrl(urlNode)) {
                            urlNode = Url(urlNode.Scalar()).resolved(base).string();
                        }
                    }
                }
            }
        }
    }
}

std::string Importer::getSceneString(const std::shared_ptr<Platform>& platform,
        const Url& scenePath) {
    return platform->stringFromFile(scenePath.string().c_str());
}

std::vector<Url> Importer::getResolvedImportUrls(const Node& scene, const Url& base) {

    std::vector<Url> scenePaths;

    if (const Node& import = scene["import"]) {
        if (import.IsScalar()) {
            scenePaths.push_back(Url(import.Scalar()).resolved(base));
        } else if (import.IsSequence()) {
            for (const auto& path : import) {
                if (path.IsScalar()) {
                    scenePaths.push_back(Url(path.Scalar()).resolved(base));
                }
            }
        }
    }

    return scenePaths;
}

void Importer::importScenesRecursive(const std::shared_ptr<Platform>& platform, Node& root,
        const Url& scenePath, std::vector<Url>& sceneStack) {

    LOGD("Starting importing Scene: %s", scenePath.string().c_str());

    for (const auto& s : sceneStack) {
        if (scenePath == s) {
            LOGE("%s will cause a cyclic import. Stopping this scene from being imported",
                    scenePath.string().c_str());
            return;
        }
    }

    sceneStack.push_back(scenePath);

    auto sceneNode = m_scenes[scenePath];

    if (sceneNode.IsNull()) { return; }
    if (!sceneNode.IsMap()) { return; }

    auto imports = getResolvedImportUrls(sceneNode, scenePath);

    // Don't want to merge imports, so remove them here.
    sceneNode.remove("import");

    for (const auto& url : imports) {

        importScenesRecursive(platform, root, url, sceneStack);

    }

    sceneStack.pop_back();

    mergeMapFields(root, sceneNode);

    resolveSceneUrls(platform, root, scenePath);
}

void Importer::mergeMapFields(Node& target, const Node& import) {

    for (const auto& entry : import) {

        const auto& key = entry.first.Scalar();
        const auto& source = entry.second;
        auto dest = target[key];

        if (!dest) {
            dest = source;
            continue;
        }

        if (dest.Type() != source.Type()) {
            LOGN("Merging different node types: '%s'\n'%s'\n<==\n'%s'",
                 key.c_str(), Dump(dest).c_str(), Dump(source).c_str());
        }

        switch(dest.Type()) {
            case NodeType::Null:
            case NodeType::Scalar:
            case NodeType::Sequence:
                dest = source;
                break;

            case NodeType::Map: {
                auto newTarget = dest;
                if (source.IsMap()) {
                    mergeMapFields(newTarget, source);
                } else {
                    dest = source;
                }
                break;
            }
            default:
                // NodeType::Undefined is handled above by checking (!dest).
                // All types are handled, so this should never be reached.
                assert(false);
                break;
        }
    }
}

}
