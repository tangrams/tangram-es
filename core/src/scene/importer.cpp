#include "importer.h"
#include "platform.h"
#include "scene/sceneLoader.h"
#include "util/y2j.h"
#include "log.h"

#include <regex>

namespace Tangram {

std::atomic_uint Importer::progressCounter(0);

JsonDocument Importer::applySceneImports(const Url& scenePath, const Url& resourceRoot) {

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

    JsonDocument root;

    LOGD("Processing scene import Stack:");
    std::vector<Url> sceneStack;
    importScenesRecursive(root, rootScenePath, sceneStack);

    return root;
}

void Importer::processScene(const Url& scenePath, const std::string &sceneString) {

    LOGD("Process: '%s'", scenePath.string().c_str());

    // Don't load imports twice
    if (m_scenes.find(scenePath) != m_scenes.end()) {
        return;
    }

    const char* errorMessage = nullptr;
    size_t errorLine = 0;
    JsonDocument document = yamlParseBytes(sceneString.data(), sceneString.length(), &errorMessage, &errorLine);

    if (!errorMessage) {

        for (const auto& import : getResolvedImportUrls(Node(&document), scenePath)) {
            m_sceneQueue.push_back(import);
            m_condition.notify_all();
        }

        m_scenes[scenePath].Swap(document);

    } else {
        LOGE("Parsing scene config at line %d: '%s'", errorLine, errorMessage);
    }
}

bool nodeIsPotentialUrl(const Node& node) {
    // Check that the node is scalar and not null.
    if (!node.isString()) { return false; }

    // Check that the node does not contain a 'global' reference.
    if (strncmp(node.getString(), "global.", 7) == 0) { return false; }

    return true;
}

bool nodeIsTextureUrl(const Node& node, const Node& textures) {

    if (!nodeIsPotentialUrl(node)) { return false; }

    // Check that the node is not a number or a boolean.
    if (!node.isString()) { return false; }

    // Check that the node does not name a scene texture.
    if (textures[node.getString()]) { return false; }

    return true;
}

void Importer::resolveSceneUrls(JsonDocument& doc, const Url& base) {

    Node root(&doc);

    // Resolve global texture URLs.

    Node textures = root["textures"];

    if (textures) {
        for (auto texture : textures.getMapping()) {
            if (Node textureUrlNode = texture.value["url"]) {
                if (nodeIsPotentialUrl(textureUrlNode)) {
                    auto& urlString = Url(textureUrlNode.getString()).resolved(base).string();
                    textureUrlNode.getValue()->SetString(urlString, doc.GetAllocator());
                }
            }
        }
    }

    // Resolve inline texture URLs.

    if (Node styles = root["styles"]) {

        for (auto& entry : styles.getMapping()) {

            Node style = entry.value;
            if (!style.isMapping()) { continue; }

            //style->texture
            if (Node texture = style["texture"]) {
                if (nodeIsTextureUrl(texture, textures)) {
                    auto& urlString = Url(texture.getString()).resolved(base).string();
                    texture.getValue()->SetString(urlString, doc.GetAllocator());
                }
            }

            //style->material->texture
            if (Node material = style["material"]) {
                if (!material.isMapping()) { continue; }
                for (auto& prop : {"emission", "ambient", "diffuse", "specular", "normal"}) {
                    if (Node propNode = material[prop]) {
                        if (!propNode.isMapping()) { continue; }
                        if (Node matTexture = propNode["texture"]) {
                            if (nodeIsTextureUrl(matTexture, textures)) {
                                auto& urlString = Url(matTexture.getString()).resolved(base).string();
                                matTexture.getValue()->SetString(urlString, doc.GetAllocator());
                            }
                        }
                    }
                }
            }

            //style->shader->uniforms->texture
            if (Node shaders = style["shaders"]) {
                if (!shaders.isMapping()) { continue; }
                if (Node uniforms = shaders["uniforms"]) {
                    for (auto& uniformEntry : uniforms.getMapping()) {
                        Node uniformValue = uniformEntry.value;
                        if (nodeIsTextureUrl(uniformValue, textures)) {
                            auto& urlString = Url(uniformValue.getString()).resolved(base).string();
                            uniformValue.getValue()->SetString(urlString, doc.GetAllocator());
                        } else if (uniformValue.isSequence()) {
                            for (Node u : uniformValue) {
                                if (nodeIsTextureUrl(u, textures)) {
                                    auto& urlString = Url(u.getString()).resolved(base).string();
                                    u.getValue()->SetString(urlString, doc.GetAllocator());
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
        for (auto& source : sources.getMapping()) {
            if (!source.value.isMapping()) { continue; }
            if (Node sourceUrl = source.value["url"]) {
                if (nodeIsPotentialUrl(sourceUrl)) {
                    auto& urlString = Url(sourceUrl.getString()).resolved(base).string();
                    sourceUrl.getValue()->SetString(urlString, doc.GetAllocator());
                }
            }
        }
    }

    // Resolve font URLs.

    if (Node fonts = root["fonts"]) {
        if (fonts.isMapping()) {
            for (const auto& font : fonts.getMapping()) {
                if (font.value.isMapping()) {
                    Node urlNode = font.value["url"];
                    if (nodeIsPotentialUrl(urlNode)) {
                        auto& urlString = Url(urlNode.getString()).resolved(base).string();
                        urlNode.getValue()->SetString(urlString, doc.GetAllocator());
                    }
                } else if (font.value.isSequence()) {
                    for (auto& fontNode : font.value.getSequence()) {
                        Node urlNode = fontNode["url"];
                        if (nodeIsPotentialUrl(urlNode)) {
                            auto& urlString = Url(urlNode.getString()).resolved(base).string();
                            urlNode.getValue()->SetString(urlString, doc.GetAllocator());
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

std::vector<Url> Importer::getResolvedImportUrls(const Node& scene, const Url& base) {

    std::vector<Url> scenePaths;

    if (const Node& import = scene["import"]) {
        if (import.isString()) {
            scenePaths.push_back(Url(import.getString()).resolved(base));
        } else if (import.isSequence()) {
            for (const auto& path : import.getSequence()) {
                if (path.isString()) {
                    scenePaths.push_back(Url(path.getString()).resolved(base));
                }
            }
        }
    }

    return scenePaths;
}

void Importer::importScenesRecursive(JsonDocument& root, const Url& scenePath, std::vector<Url>& sceneStack) {

    LOGD("Starting importing Scene: %s", scenePath.string().c_str());

    for (const auto& s : sceneStack) {
        if (scenePath == s) {
            LOGE("%s will cause a cyclic import. Stopping this scene from being imported", scenePath.string().c_str());
            return;
        }
    }

    sceneStack.push_back(scenePath);

    auto& sceneDocument = m_scenes[scenePath];
    Node sceneNode(&sceneDocument);

    if (!sceneNode.isMapping()) { return; }

    auto imports = getResolvedImportUrls(sceneNode, scenePath);

    // Don't want to merge imports, so remove them here.
    sceneDocument.EraseMember("import");

    for (const auto& url : imports) {

        importScenesRecursive(root, url, sceneStack);

    }

    sceneStack.pop_back();

    mergeMapFields(root, root, sceneDocument);

    resolveSceneUrls(root, scenePath);
}

void Importer::mergeMapFields(JsonDocument& doc, JsonValue& target, const JsonValue& import) {

    for (const auto& entry : import.GetObject()) {

        const char* key = entry.name.GetString();
        const auto& source = entry.value;

        auto destIt = target.FindMember(key);
        if (destIt == target.MemberEnd()) {
            target.AddMember(entry.name, entry.value, doc.GetAllocator());
            continue;
        }

        JsonValue& dest = destIt->value;

        if (dest.GetType() != source.GetType()) {
            LOGN("Merging different node types: '%s'\n'%d'\n<==\n'%d'",
                 key, dest.GetType(), source.GetType());
        }

        if (dest.IsObject()) {
            if (source.IsObject()) {
                mergeMapFields(doc, dest, source);
            } else {
                dest = source;
            }
        } else {
            dest = source;
        }
    }
}

}
