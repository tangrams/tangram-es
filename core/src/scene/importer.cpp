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

bool isZipUrl(const Url& url) {
    return (Url::getPathExtension(url.path()) == "zip");
}

Node Importer::applySceneImports(const std::shared_ptr<Platform>& platform,
        std::shared_ptr<Scene>& scene) {

    const Url& scenePath = scene->path();
    const Url& resourceRoot = scene->resourceRoot();

    Url path;
    Url rootScenePath = scenePath.resolved(resourceRoot);

    // Asset fills the m_path of the yaml asset with the yaml file in the zip bundle
    createSceneAsset(platform, scene, rootScenePath, Url(""), Url(""));

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

        bool isZipped = isZipUrl(path);
        auto& asset = scene->sceneAssets()[path.string()];
        // An asset at this path must have been created by now.
        assert(asset);

        if (path.hasHttpScheme() && !asset->zipHandle()) {
            progressCounter++;
            platform->startUrlRequest(path.string(), [&, isZipped, path](std::vector<char>&& rawData) {
                if (!rawData.empty()) {
                    std::unique_lock<std::mutex> lock(sceneMutex);
                    auto& asset = scene->sceneAssets()[path.string()];
                    if (isZipped) {
                        asset->buildZipHandle(rawData);
                        processScene(platform, scene, path, asset->readStringFromAsset(platform));
                    } else {
                        processScene(platform, scene, path, std::string(rawData.data(), rawData.size()));
                    }
                }
                progressCounter--;
                m_condition.notify_all();
            });
        } else {
            std::unique_lock<std::mutex> lock(sceneMutex);
            processScene(platform, scene, path, getSceneString(platform, path, asset));
        }
    }

    Node root = Node();

    LOGD("Processing scene import Stack:");
    std::vector<Url> sceneStack;
    importScenesRecursive(platform, scene, root, rootScenePath, sceneStack);

    return root;
}

void Importer::processScene(const std::shared_ptr<Platform>& platform, std::shared_ptr<Scene>& scene,
        const Url& scenePath, const std::string &sceneString) {

    LOGD("Process: '%s'", scenePath.string().c_str());

    // Don't load imports twice
    if (m_scenes.find(scenePath) != m_scenes.end()) {
        return;
    }

    try {
        auto sceneNode = YAML::Load(sceneString);

        m_scenes[scenePath] = sceneNode;

        for (const auto& import : getResolvedImportUrls(platform, scene, sceneNode, scenePath)) {
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

void  Importer::createSceneAsset(const std::shared_ptr<Platform>& platform, std::shared_ptr<Scene>& scene,
        const Url& resolvedUrl, const Url& relativeUrl, const Url& base) {

    auto& sceneAssets = scene->sceneAssets();
    auto& resolvedStr = resolvedUrl.string();
    auto& baseStr = base.string();
    auto relativeStr = relativeUrl.string();

    if (sceneAssets.find(resolvedStr) != sceneAssets.end()) { return; }

    // needed to find apt file inside zipBundle
    if (*relativeStr.begin() == '/') { relativeStr.erase(relativeStr.begin()); }

    if (isZipUrl(resolvedUrl)) {
        if (relativeUrl.hasHttpScheme() || (resolvedUrl.hasHttpScheme() && base.isEmpty())) {
            // Data to be fetched later (and zipHandle created) in network callback
            sceneAssets[resolvedStr] = std::make_unique<Asset>(resolvedStr, relativeStr);

        } else if (relativeUrl.isAbsolute() || base.isEmpty()) {
            sceneAssets[resolvedStr] = std::make_unique<Asset>(resolvedStr, relativeStr, nullptr,
                                                               platform->bytesFromFile(resolvedStr.c_str()));
        } else {
            auto& parentAsset = sceneAssets[baseStr];
            // Parent asset (for base Str) must have been created by now
            assert(parentAsset);
            sceneAssets[resolvedStr] = std::make_unique<Asset>(resolvedStr, relativeStr, nullptr,
                                                               parentAsset->readBytesFromAsset(platform, relativeStr));
        }
    } else {
        auto& parentAsset = sceneAssets[baseStr];

        if (relativeUrl.isAbsolute() || (parentAsset && !parentAsset->zipHandle())) {
            // Make sure to first check for cases when the asset does not belong within a zipBundle
            sceneAssets[resolvedStr] = std::make_unique<Asset>(resolvedStr, relativeStr);
        } else if (parentAsset && parentAsset->zipHandle()) {
            // Asset is in zip bundle
            sceneAssets[resolvedStr] = std::make_unique<Asset>(resolvedStr, relativeStr, parentAsset->zipHandle());
        } else {
            sceneAssets[resolvedStr] = std::make_unique<Asset>(resolvedStr, relativeStr);
        }
    }
}

void Importer::resolveSceneUrls(const std::shared_ptr<Platform>& platform,
        std::shared_ptr<Scene>& scene, Node& root, const Url& base) {

    // Resolve global texture URLs.
    std::string relativeUrl = "";

    Node textures = root["textures"];

    if (textures) {
        for (auto texture : textures) {
            if (Node textureUrlNode = texture.second["url"]) {
                if (nodeIsPotentialUrl(textureUrlNode)) {
                    relativeUrl = textureUrlNode.Scalar();
                    textureUrlNode = Url(textureUrlNode.Scalar()).resolved(base).string();
                    createSceneAsset(platform, scene, textureUrlNode.Scalar(), relativeUrl, base);
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
                    relativeUrl = texture.Scalar();
                    texture = Url(texture.Scalar()).resolved(base).string();
                    createSceneAsset(platform, scene, texture.Scalar(), relativeUrl, base);
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
                                relativeUrl = matTexture.Scalar();
                                matTexture = Url(matTexture.Scalar()).resolved(base).string();
                                createSceneAsset(platform, scene, matTexture.Scalar(), relativeUrl, base);
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
                            relativeUrl = uniformValue.Scalar();
                            uniformValue = Url(uniformValue.Scalar()).resolved(base).string();
                            createSceneAsset(platform, scene, uniformValue.Scalar(), relativeUrl, base);
                        } else if (uniformValue.IsSequence()) {
                            for (Node u : uniformValue) {
                                if (nodeIsTextureUrl(u, textures)) {
                                    relativeUrl = u.Scalar();
                                    u = Url(u.Scalar()).resolved(base).string();
                                    createSceneAsset(platform, scene, u.Scalar(), relativeUrl, base);
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // Resolve data source URLs.

    // TODO: create assets for sources
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
                        relativeUrl = urlNode.Scalar();
                        urlNode = Url(urlNode.Scalar()).resolved(base).string();
                        createSceneAsset(platform, scene, urlNode.Scalar(), relativeUrl, base);
                    }
                } else if (font.second.IsSequence()) {
                    for (auto& fontNode : font.second) {
                        auto urlNode = fontNode["url"];
                        if (nodeIsPotentialUrl(urlNode)) {
                            relativeUrl = urlNode.Scalar();
                            urlNode = Url(urlNode.Scalar()).resolved(base).string();
                            createSceneAsset(platform, scene, urlNode.Scalar(), relativeUrl, base);
                        }
                    }
                }
            }
        }
    }
}

std::string Importer::getSceneString(const std::shared_ptr<Platform>& platform,
        const Url& scenePath, const std::unique_ptr<Asset>& asset) {
    if (!asset) { return "";}

    return asset->readStringFromAsset(platform);
}

std::vector<Url> Importer::getResolvedImportUrls(const std::shared_ptr<Platform>& platform,
        std::shared_ptr<Scene>& scene, const Node& sceneNode, const Url& base) {

    std::vector<Url> scenePaths;

    if (const Node& import = sceneNode["import"]) {
        if (import.IsScalar()) {
            auto resolvedUrl = Url(import.Scalar()).resolved(base);
            createSceneAsset(platform, scene, resolvedUrl, import.Scalar(), base);
            scenePaths.push_back(resolvedUrl);
        } else if (import.IsSequence()) {
            for (const auto& path : import) {
                if (path.IsScalar()) {
                    auto resolvedUrl = Url(path.Scalar()).resolved(base);
                    createSceneAsset(platform, scene, resolvedUrl, path.Scalar(), base);
                    scenePaths.push_back(resolvedUrl);
                }
            }
        }
    }

    return scenePaths;
}

void Importer::importScenesRecursive(const std::shared_ptr<Platform>& platform,
        std::shared_ptr<Scene>& scene, Node& root, const Url& scenePath, std::vector<Url>& sceneStack) {

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

    auto imports = getResolvedImportUrls(platform, scene, sceneNode, scenePath);

    // Don't want to merge imports, so remove them here.
    sceneNode.remove("import");

    for (const auto& url : imports) {

        importScenesRecursive(platform, scene, root, url, sceneStack);

    }

    sceneStack.pop_back();

    mergeMapFields(root, sceneNode);

    resolveSceneUrls(platform, scene, root, scenePath);
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
