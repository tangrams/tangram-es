#include "scene/importer.h"

#include "log.h"
#include "platform.h"
#include "util/zipArchive.h"

#include <atomic>
#include <cassert>
#include <condition_variable>
#include <mutex>

#include "yaml-cpp/proto/protobuf.h"

using YAML::Node;
using YAML::NodeType;

namespace Tangram {


Node Importer::loadSceneData(Platform& _platform, const Url& _sceneUrl, const std::string& _sceneYaml) {

    Url nextUrlToImport;

    if (!_sceneYaml.empty()) {
        // Load scene from yaml string.
        addSceneYaml(_sceneUrl, _sceneYaml.data(), _sceneYaml.length());
    } else {
        // Load scene from yaml file.
        m_sceneQueue.push_back(_sceneUrl);
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

            if (m_importedScenes.find(nextUrlToImport) != m_importedScenes.end()) {
                // This scene URL has already been imported, we're done!
                continue;
            }
        }

        if (nextUrlToImport.scheme() == "zip") {
            readFromZip(nextUrlToImport,
                [&, nextUrlToImport](UrlResponse&& response) {
                    std::unique_lock<std::mutex> lock(sceneMutex);
                    if (response.error) {
                        LOGE("Unable to retrieve '%s': %s",
                             nextUrlToImport.string().c_str(), response.error);
                    } else {
                        addSceneData(nextUrlToImport, std::move(response.content));
                    }
               });
        } else {
            activeDownloads++;
            _platform.startUrlRequest(nextUrlToImport,
                [&, nextUrlToImport](UrlResponse&& response) {
                    std::unique_lock<std::mutex> lock(sceneMutex);
                    if (response.error) {
                        LOGE("Unable to retrieve '%s': %s",
                             nextUrlToImport.string().c_str(), response.error);
                    } else {
                        addSceneData(nextUrlToImport, std::move(response.content));
                    }
                    activeDownloads--;
                    condition.notify_all();
               });
        }
    }


    LOGD("Processing scene import Stack:");
    std::vector<Url> sceneStack;
    Node root;
    importScenesRecursive(root, _sceneUrl, sceneStack);

    m_importedScenes.clear();

    return root;
}

void Importer::addSceneData(const Url& sceneUrl, std::vector<char>&& sceneData) {

    LOGD("Process: '%s'", sceneUrl.string().c_str());

    // Don't load imports twice
    if (m_importedScenes.find(sceneUrl) != m_importedScenes.end()) {
        return;
    }

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

    m_zipArchives.emplace(sceneUrl, zipArchive);
}

UrlRequestHandle Importer::readFromZip(const Url& url, UrlCallback callback) {
    UrlResponse response;
    // URL for a file in a zip archive, get the encoded source URL.
    auto source = Importer::getArchiveUrlForZipEntry(url);
    // Search for the source URL in our archive map.
    auto it = m_zipArchives.find(source);
    if (it != m_zipArchives.end()) {
        auto& archive = it->second;
        // Found the archive! Now create a response for the request.
        auto zipEntryPath = url.path().substr(1);
        auto entry = archive->findEntry(zipEntryPath);
        if (entry) {
            response.content.resize(entry->uncompressedSize);
            bool success = archive->decompressEntry(entry, response.content.data());
            if (!success) {
                response.error = "Unable to decompress zip archive file.";
            }
        } else {
            response.error = "Did not find zip archive entry.";
        }
    } else {
        response.error = "Could not find zip archive.";
    }
    callback(std::move(response));
    return 0;
}

void Importer::addSceneYaml(const Url& sceneUrl, const char* sceneYaml, size_t length) {
    LOGTO("Parsing scene config bytes: %d - '%s' ", length, sceneUrl.path().c_str());

    Node sceneNode;
    if (Url::getPathExtension(sceneUrl.path()) == "pbf") {
        sceneNode = YAML::Protobuf::Load(sceneYaml, length);

    } else {
        try {
            sceneNode = YAML::Load(sceneYaml, length);
        } catch (const YAML::ParserException& e) {
            LOGE("Parsing scene config '%s'", e.what());
            return;
        }
    }
    LOGTO("Parsing done");

    if (!sceneNode.IsDefined() || !sceneNode.IsMap()) {
        LOGE("Scene is not a valid YAML map: %s", sceneUrl.string().c_str());
        return;
    }

    m_importedScenes[sceneUrl] = sceneNode;

    for (const auto& import : getResolvedImportUrls(sceneNode, sceneUrl)) {
        m_sceneQueue.push_back(import);
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

void Importer::importScenesRecursive(Node& root, const Url& sceneUrl, std::vector<Url>& sceneStack) {

    LOGD("Starting importing Scene: %s", sceneUrl.string().c_str());

    for (const auto& s : sceneStack) {
        if (sceneUrl == s) {
            LOGE("%s will cause a cyclic import. Stopping this scene from being imported",
                    sceneUrl.string().c_str());
            return;
        }
    }

    sceneStack.push_back(sceneUrl);

    auto sceneNode = m_importedScenes[sceneUrl];

    if (!sceneNode.IsDefined() || !sceneNode.IsMap()) {
        return;
    }

    auto imports = getResolvedImportUrls(sceneNode, sceneUrl);

    // Don't want to merge imports, so remove them here.
    sceneNode.remove("import");

    for (const auto& url : imports) {
        importScenesRecursive(root, url, sceneStack);
    }

    sceneStack.pop_back();

    mergeMapFields(root, sceneNode);

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
