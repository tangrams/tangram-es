#include "importer.h"
#include "platform.h"
#include "scene/sceneLoader.h"
#include "util/topologicalSort.h"
#include "yaml-cpp/yaml.h"
#include "log.h"

#include <regex>

using YAML::Node;
using YAML::NodeType;

namespace Tangram {

std::atomic_uint Importer::progressCounter(0);

bool isUrl(const std::string &path) {
    static const std::regex r("^(http|https):/");
    std::smatch match;
    return std::regex_search(path, match, r);
}

bool isBase64Data(const std::string &path) {
    return path.substr(0, 21) == "data:image/png;base64";
}

Node Importer::applySceneImports(const std::string& scenePath, const std::string& resourceRoot) {

    std::string path;
    std::string fullPath = resourceRoot + scenePath;

    m_sceneQueue.push_back(fullPath);

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

        // TODO: generic handling of uri
        if (isUrl(path)) {
            progressCounter++;
            startUrlRequest(path,
                    [&, p = path](std::vector<char>&& rawData) {

                    if (!rawData.empty()) {
                        std::unique_lock<std::mutex> lock(sceneMutex);
                        processScene(p, std::string(rawData.data(), rawData.size()));
                    }
                    progressCounter--;
                    m_condition.notify_all();
            });
        } else {
            std::unique_lock<std::mutex> lock(sceneMutex);
            processScene(path, getSceneString(path));
        }
    }

    auto root = importScenes(fullPath);

    return root;
}

void Importer::processScene(const std::string &scenePath, const std::string &sceneString) {

    LOGD("Process: '%s'", scenePath.c_str());

    try {
        auto sceneNode = YAML::Load(sceneString);

        normalizeSceneImports(sceneNode, scenePath);
        normalizeSceneDataSources(sceneNode, scenePath);
        normalizeSceneTextures(sceneNode, scenePath);

        m_scenes[scenePath] = sceneNode;

        for (const auto& import : getScenesToImport(sceneNode)) {
            m_sceneQueue.push_back(import);
            m_condition.notify_all();
        }
    }
    catch (YAML::ParserException e) {
        LOGE("Parsing scene config '%s'", e.what());
    }
}

std::string Importer::normalizePath(const std::string &_path,
                                    const std::string &_parentPath) {

    std::string path;

    // Check if absolute path or network url , return as it is
    if (_path[0] == '/' || isUrl(_path)) {
        path = _path;
    } else {
        auto r = std::regex("[^//]+$");
        path = std::regex_replace(_parentPath, r, _path);
    }

    LOGD("Normalize '%s', '%s' => '%s'", _parentPath.c_str(), _path.c_str(), path.c_str());

    return path;
}

void Importer::normalizeSceneImports(Node& root, const std::string& parentPath) {
    if (Node import = root["import"]) {
        if (import.IsScalar()) {
            import = normalizePath(import.Scalar(), parentPath);
        } else if (import.IsSequence()) {
            for (Node imp : import) {
                if (imp.IsScalar()) { imp = normalizePath(imp.Scalar(), parentPath); }
            }
        }
    }
}

void Importer::normalizeSceneDataSources(Node &root, const std::string &parentPath) {
    if (Node sources = root["sources"]) {
        for (auto source : sources) {
            if (Node sourceUrl = source.second["url"]) {
                sourceUrl = normalizePath(sourceUrl.Scalar(), parentPath);
            }
        }
    }
}

void Importer::setNormalizedTexture(Node& texture, const std::vector<std::string>& names,
                                    const std::string& parentPath) {

    for (size_t index = 0; index < names.size(); index++) {

        auto& name = names[index];
        if (isBase64Data(name)) {
            continue;
        }

        std::string normTexPath;

        // if texture url is a named texture then move on (this has been already resolved
        if (m_globalTextures.find(name) != m_globalTextures.end()) { continue; }

        // get normalized texture path
        if (m_textureNames.find(name) == m_textureNames.end()) {
            normTexPath = normalizePath(name, parentPath);
            m_textureNames[name] = normTexPath;
        } else {
            normTexPath = m_textureNames.at(name);
        }

        // set yaml node with normalized texture path
        if (names.size() > 1) {
            texture[index] = normTexPath;
        } else {
            texture = normTexPath;
        }
    }
}

void Importer::normalizeSceneTextures(Node& root, const std::string& parentPath) {

    if (Node textures = root["textures"]) {
        for (auto texture : textures) {
            if (Node textureUrl = texture.second["url"]) {
                if (!isBase64Data(textureUrl.Scalar())) {
                    setNormalizedTexture(textureUrl, {textureUrl.Scalar()}, parentPath);
                }
                m_globalTextures.insert(texture.first.Scalar());
            }
        }
    }

    if (Node styles = root["styles"]) {

        for (auto styleNode : styles) {

            auto style = styleNode.second;
            //style->texture
            if (Node texture = style["texture"]) {
                if (texture.IsScalar() && !isBase64Data(texture.Scalar())) {
                    setNormalizedTexture(texture, {texture.Scalar()}, parentPath);
                }
            }

            //style->material->texture
            if (Node material = style["material"]) {
                for (auto& prop : {"emission", "ambient", "diffuse", "specular", "normal"}) {
                    if (auto propNode = material[prop]) {
                        if (auto matTexture = material[propNode]["texture"]) {
                            if (matTexture.IsScalar()) {
                                setNormalizedTexture(matTexture, {matTexture.Scalar()}, parentPath);
                            }
                        }
                    }
                }
            }

            //style->shader->uniforms->texture
            if (Node shaders = style["shaders"]) {
                if (Node uniforms = shaders["uniforms"]) {
                    for (auto uniformNode : uniforms) {
                        StyleUniform styleUniform;
                        auto uniformValue = uniformNode.second;
                        if (SceneLoader::parseStyleUniforms(uniformValue, nullptr, styleUniform) &&
                                styleUniform.type == "sampler2D") {
                            if (styleUniform.value.is<UniformTextureArray>()) {
                                setNormalizedTexture(uniformValue,
                                             styleUniform.value.get<UniformTextureArray>().names,
                                             parentPath);
                            } else {
                                auto name = styleUniform.value.get<std::string>();
                                setNormalizedTexture(uniformValue, {name}, parentPath);
                            }
                        }
                    }
                }
            }
        }
    }
}

std::string Importer::getSceneString(const std::string &scenePath) {
    return stringFromFile(scenePath.c_str());
}

std::vector<std::string> Importer::getScenesToImport(const Node& scene) {

    std::vector<std::string> scenePaths;

    if (const Node& import = scene["import"]) {
        if (import.IsScalar()) {
            scenePaths.push_back(import.Scalar());
        }
        else if (import.IsSequence()) {
            for (const auto& imp : import) {
                if (imp.IsScalar()) { scenePaths.push_back(imp.Scalar()); }
            }
        }
    }

    return scenePaths;
}

std::vector<std::string> Importer::getImportOrder(const std::string& baseScene) {

    std::vector<std::pair<std::string, std::string>> dependencies;

    for (const auto& scene : m_scenes) {
        const auto& name = scene.first;
        const auto& sceneRoot = scene.second;
        for (const auto& import : getScenesToImport(sceneRoot)) {
            dependencies.push_back( {import, name} );
        }
    }

    auto sortedScenes = topologicalSort(dependencies);

    if (sortedScenes.empty()) {
        return { baseScene };
    }

    return sortedScenes;
}

Node Importer::importScenes(const std::string& scenePath) {

    auto importScenesSorted = getImportOrder(scenePath);

    if (importScenesSorted.size() == 1) {
        return m_scenes[importScenesSorted[0]];
    }

    Node root = Node();

    for (auto& import : importScenesSorted) {

        const auto& importRoot = m_scenes[import];
        if (!importRoot.IsMap()) { continue; }
        mergeMapFields(root, importRoot);
    }

    return root;

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
