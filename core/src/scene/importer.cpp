#include "importer.h"
#include "platform.h"
#include "scene/sceneLoader.h"
#include "util/topologicalSort.h"
#include "yaml-cpp/yaml.h"

#include <regex>

using YAML::Node;
using YAML::NodeType;

namespace Tangram {

std::atomic_uint Importer::progressCounter(0);

Node Importer::applySceneImports(const std::string& scenePath) {

    std::string sceneString;
    std::string path;
    m_sceneQueue.push_back(scenePath);

    while (true) {
        {
            std::unique_lock<std::mutex> lock(sceneMutex);

            m_condition.wait(lock, [&, this]{
                    return (!m_sceneQueue.empty() || progressCounter == 0);
                });

            if (m_sceneQueue.empty() && progressCounter == 0) { break; }

            path = m_sceneQueue.back();
            m_sceneQueue.pop_back();
            if (m_scenes.find(path) != m_scenes.end()) { continue; }
        }

        std::regex r("^(http|https):/");
        std::smatch match;
        if (std::regex_search(path, match, r)) {
            if (progressCounter < MAX_SCENE_DOWNLOAD) {
                progressCounter++;
                startUrlRequest(path,
                        [&](std::vector<char>&& rawData) {
                        std::unique_lock<std::mutex> lock(sceneMutex);
                        progressCounter--;
                        processScene(path, rawData.data());
                        m_condition.notify_all();
                    });
            } else {
                // requeue this scene for importing
                std::unique_lock<std::mutex> lock(sceneMutex);
                m_sceneQueue.push_back(path);
            }
        } else {
            std::unique_lock<std::mutex> lock(sceneMutex);
            processScene(path, getSceneString(path));
        }
    }

    return importScenes(scenePath);
}

void Importer::processScene(const std::string &scenePath, const std::string &sceneString) {

    std::string path = scenePath;
    // Make sure all references from uber scene file are relative to itself, instead of being
    // absolute paths (Example: when loading a file using command line args).
    // TODO: Could be made better later.
    if (m_scenes.size() == 0 && path[0] == '/') { path= getFilename(path); }

    try {
        auto root = YAML::Load(sceneString);
        normalizeSceneImports(root, path);
        normalizeSceneTextures(root, path);
        auto imports = getScenesToImport(root);
        m_scenes[path] = root;
        for (const auto& import : imports) {
            m_sceneQueue.push_back(import);
            // notify here?
        }
    }
    catch (YAML::ParserException e) {
        LOGE("Parsing scene config '%s'", e.what());
    }
}

std::string Importer::getFilename(const std::string& filePath) {
    std::string scenePath = filePath;
    std::regex r("[^//]+$");
    std::smatch match;
    if (std::regex_search(scenePath, match, r)) { scenePath = match[0]; }
    return scenePath;
}

std::string Importer::normalizePath(const std::string &path,
                                         const std::string &parentPath) {

    //TODO: Handle network urls

    if (path[0] == '/') {
        //absolute path, return as it is
        return path;
    }

    std::regex r("[^//]+$");
    return std::regex_replace(parentPath, r, path);
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

void Importer::setNormalizedTexture(Node& texture, const std::vector<std::string>& names,
        const std::string& parentPath) {

    for (size_t index = 0; index < names.size(); index++) {
        auto& name = names[index];
        std::string normTexPath;

        // if texture url is a named texture then move on (this has been already resolved
        if (m_globalTextures.find(name) != m_globalTextures.end()) { continue; }

        //get normalized texture path
        if (m_textureNames.find(name) == m_textureNames.end()) {
            normTexPath = normalizePath(name, parentPath);
            m_textureNames[name] = normTexPath;
        } else {
            normTexPath = m_textureNames.at(name);
        }

        //set yaml node with normalized texture path
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
                setNormalizedTexture(textureUrl, {textureUrl.Scalar()}, parentPath);
                m_globalTextures.insert(texture.first.Scalar());
            }
        }
    }

    if (Node styles = root["styles"]) {

        for (auto styleNode : styles) {

            auto style = styleNode.second;
            //style->texture
            if (Node texture = style["texture"]) {
                if (texture.IsScalar()) {
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
    return stringFromFile(scenePath.c_str(), PathType::resource);
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
        const auto& name = scene.first.k;
        const auto& sceneRoot = scene.second;
        for (const auto& import : getScenesToImport(sceneRoot)) {
            dependencies.push_back( {import, name} );
        }
    }

    auto sortedScenes = topologicalSort(dependencies);

    if (sortedScenes.empty()) {
        return { baseScene };
    }

    return topologicalSort(dependencies);
}

Node Importer::importScenes(const std::string& scenePath) {

    auto importScenesSorted = getImportOrder(scenePath);

    Node root = Node();

    for (auto& import : importScenesSorted) {
        if (importScenesSorted.size() == 1 && import[0] == '/') { import = getFilename(import); }
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
