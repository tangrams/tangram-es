#include "scene/importer.h"

#include "log.h"
#include "platform.h"
#include "scene/sceneLoader.h"
#include "util/zipArchive.h"

#include <algorithm>
#include <atomic>
#include <cassert>
#include <condition_variable>
#include <mutex>

using YAML::Node;
using YAML::NodeType;

namespace Tangram {

Importer::Importer(std::shared_ptr<Scene> scene)
    : m_scene(scene) {
}

Node Importer::applySceneImports(Platform& platform) {

    Url sceneUrl = m_scene->options().url;

    Url nextUrlToImport;

    if (!m_scene->options().yaml.empty()) {
        // Load scene from yaml string.
        auto& yaml = m_scene->options().yaml;
        addSceneYaml(sceneUrl, yaml.data(), yaml.length());
    } else {
        // Load scene from yaml file.
        m_sceneQueue.push_back(sceneUrl);
    }

    std::atomic_uint activeDownloads(0);
    std::mutex sceneMutex;
    std::condition_variable condition;

    while (true) {
        {
            std::unique_lock<std::mutex> lock(sceneMutex);

            if (m_sceneQueue.empty()) {
                if (activeDownloads == 0) {
                    break;
                }
                condition.wait(lock);
            }

            if (!m_sceneQueue.empty()) {
                nextUrlToImport = m_sceneQueue.back();
                m_sceneQueue.pop_back();
            } else {
                continue;
            }

            // Mark Url as going-to-be-imported to prevent duplicate work.
            m_sceneNodes.emplace(nextUrlToImport, SceneNode{});
        }

        activeDownloads++;
        m_scene->startUrlRequest(nextUrlToImport, [&, nextUrlToImport](UrlResponse&& response) {
            std::unique_lock<std::mutex> lock(sceneMutex);
            if (response.error) {
                LOGE("Unable to retrieve '%s': %s", nextUrlToImport.string().c_str(), response.error);
            } else {
                addSceneData(nextUrlToImport, std::move(response.content));
            }
            activeDownloads--;
            condition.notify_all();
        });
    }

    Node root;

    LOGD("Processing scene import Stack:");
    std::unordered_set<Url> imported;
    importScenesRecursive(root, sceneUrl, imported);

    return root;
}

void Importer::addSceneData(const Url& sceneUrl, std::vector<char>&& sceneData) {
    LOGD("Process: '%s'", sceneUrl.string().c_str());

    if (!isZipArchiveUrl(sceneUrl)) {
        addSceneYaml(sceneUrl, sceneData.data(), sceneData.size());
        return;
    }
    // We're loading a scene from a zip archive!
    // First, create an archive from the data.
    auto zipArchive = std::make_shared<ZipArchive>();
    zipArchive->loadFromMemory(std::move(sceneData));

    // Find the "base" scene file in the archive entries.
    for (const auto& entry : zipArchive->entries()) {
        auto ext = Url::getPathExtension(entry.path);
        // The "base" scene file must have extension "yaml" or "yml" and be
        // at the root directory of the archive (i.e. no '/' in path).
        if ((ext == "yaml" || ext == "yml") && entry.path.find('/') == std::string::npos) {
            // Found the base, now extract the contents to the scene string.
            std::vector<char> yaml;
            yaml.resize(entry.uncompressedSize);

            zipArchive->decompressEntry(&entry, &yaml[0]);

            addSceneYaml(sceneUrl, yaml.data(), yaml.size());
            break;
        }
    }
    // Add the archive to the scene.
    m_scene->addZipArchive(sceneUrl, zipArchive);
}

void Importer::addSceneYaml(const Url& sceneUrl, const char* sceneYaml, size_t length) {

    auto& sceneNode = m_sceneNodes[sceneUrl];

    try {
        sceneNode.yaml = YAML::Load(sceneYaml, length);
    } catch (const YAML::ParserException& e) {
        LOGE("Parsing scene config '%s'", e.what());
        return;
    }

    if (!sceneNode.yaml.IsDefined() || !sceneNode.yaml.IsMap()) {
        LOGE("Scene is not a valid YAML map: %s", sceneUrl.string().c_str());
        return;
    }

    sceneNode.imports = getResolvedImportUrls(sceneNode.yaml, sceneUrl);

    // Remove 'import' values so they don't get merged.
    sceneNode.yaml.remove("import");

    for (const auto& url : sceneNode.imports) {
        // Check if this scene URL has been (or is going to be) imported already
        if (m_sceneNodes.find(url) == m_sceneNodes.end()) {
            m_sceneQueue.push_back(url);
        }
    }
}

std::vector<Url> Importer::getResolvedImportUrls(const Node& sceneNode, const Url& baseUrl) {

    std::vector<Url> sceneUrls;

    auto base = baseUrl;
    if (isZipArchiveUrl(baseUrl)) {
        base = getBaseUrlForZipArchive(baseUrl);
    }

    if (sceneNode.IsMap()) {
        if (const Node& import = sceneNode["import"]) {
            if (import.IsScalar()) {
                sceneUrls.push_back(Url(import.Scalar()).resolved(base));
            } else if (import.IsSequence()) {
                for (const auto &path : import) {
                    if (path.IsScalar()) {
                        sceneUrls.push_back(Url(path.Scalar()).resolved(base));
                    }
                }
            }
        }
    }

    return sceneUrls;
}

void Importer::importScenesRecursive(Node& root, const Url& sceneUrl, std::unordered_set<Url>& imported) {

    LOGD("Starting importing Scene: %s", sceneUrl.string().c_str());

    // Insert self to handle self-imports cycles
    imported.insert(sceneUrl);

    auto& sceneNode = m_sceneNodes[sceneUrl];

    // If an import URL is already in the imported set that means it is imported by a "parent" scene file to this one.
    // The parent import will assign the same values, so we can safely skip importing it here. This saves some work and
    // also prevents import cycles.
    //
    // It is important that we don't merge the same YAML node more than once. YAML node assignment is by reference, so
    // merging mutates the original input nodes.
    auto it = std::remove_if(sceneNode.imports.begin(), sceneNode.imports.end(),
                             [&](auto& i){ return imported.find(i) != imported.end(); });

    if (it != sceneNode.imports.end()) {
        LOGD("Skipping redundant import(s)");
        sceneNode.imports.erase(it, sceneNode.imports.end());
    }

    imported.insert(sceneNode.imports.begin(), sceneNode.imports.end());

    for (const auto& url : sceneNode.imports) {
        importScenesRecursive(root, url, imported);
    }

    mergeMapFields(root, sceneNode.yaml);

    resolveSceneUrls(root, sceneUrl);
}

void Importer::mergeMapFields(Node& target, const Node& import) {
    if (!target.IsMap() || !import.IsMap()) {

        if (target.IsDefined() && !target.IsNull() && (target.Type() != import.Type())) {
            LOGN("Merging different node types: \n'%s'\n<--\n'%s'",
                 Dump(target).c_str(), Dump(import).c_str());
        }

        target = import;

    } else {
        for (const auto& entry : import) {

            const auto& key = entry.first.Scalar();
            const auto& source = entry.second;
            auto dest = target[key];
            mergeMapFields(dest, source);
        }
    }
}

bool Importer::isZipArchiveUrl(const Url& url) {
    return Url::getPathExtension(url.path()) == "zip";
}

Url Importer::getBaseUrlForZipArchive(const Url& archiveUrl) {
    auto encodedSourceUrl = Url::escapeReservedCharacters(archiveUrl.string());
    auto baseUrl = Url("zip://" +  encodedSourceUrl);
    return baseUrl;
}

Url Importer::getArchiveUrlForZipEntry(const Url& zipEntryUrl) {
    auto encodedSourceUrl = zipEntryUrl.netLocation();
    auto source = Url(Url::unEscapeReservedCharacters(encodedSourceUrl));
    return source;
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

void Importer::resolveSceneUrls(Node& root, const Url& baseUrl) {

    auto base = baseUrl;
    if (isZipArchiveUrl(baseUrl)) {
        base = getBaseUrlForZipArchive(baseUrl);
    }

    // Resolve global texture URLs.

    Node textures = root["textures"];

    if (textures.IsDefined()) {
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
                    sourceUrl = Url(sourceUrl.Scalar()).resolved(base).string();
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

}
