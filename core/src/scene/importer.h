#pragma once

#include "platform.h"

#include "yaml-cpp/yaml.h"

#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace Tangram {

class AsyncWorker;
class SceneOptions;
class ZipArchive;
class Url;

class Importer {
public:

    using Node = YAML::Node;

    Importer();
    ~Importer();

    // Loads the main scene with deep merging dependent imported scenes.
    Node loadSceneData(Platform& platform, const Url& sceneUrl, const std::string& sceneYaml = "");

    void cancelLoading(Platform& _platform);

    static bool isZipArchiveUrl(const Url& url);

    static Url getBaseUrlForZipArchive(const Url& archiveUrl);

    static Url getArchiveUrlForZipEntry(const Url& zipEntryUrl);

    // Traverses the nodes contained in the given root scene node and for all
    // nodes that represent URLs, replaces the contents with that URL resolved
    // against the given base URL.
    static void resolveSceneUrls(Node& root, const Url& base);

    // Start an asynchronous request for the scene resource at the given URL.
    // In addition to the URL types supported by the platform instance, this
    // also supports a custom ZIP URL scheme. ZIP URLs are of the form:
    //   zip://path/to/file.txt#http://host.com/some/archive.zip
    // The fragment (#http...) of the URL is the location of the archive and the
    // relative portion of the URL (path/...) is the path of the target file
    // within the archive (this allows relative path operations on URLs to work
    // as expected within zip archives). This function expects that all required
    // zip archives will be added to the scene with addZipArchive before being
    // requested.
    UrlRequestHandle readFromZip(const Url& url, UrlCallback callback);

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

    // Imported scenes must be parsed into YAML nodes to find further imports.
    // The parsed scenes are stored in a map with their URLs to be merged once
    // all imports are found and parsed.
    std::unordered_map<Url, Node> m_importedScenes = {};

    std::vector<Url> m_sceneQueue = {};

    // Container for any zip archives needed for the scene. For each entry, the
    // key is the original URL from which the zip archive was retrieved and the
    // value is a ZipArchive initialized with the compressed archive data.
    std::unique_ptr<AsyncWorker> m_zipWorker;
    std::unordered_map<Url, std::shared_ptr<ZipArchive>> m_zipArchives;

    // Keep track of UrlRequests for cancellation. NB we don't care to remove
    // handles when requests finished: Calling cancel on a finished request has
    // not much overhead and is going to be rarely used.
    std::vector<UrlRequestHandle> m_urlRequests;
    std::atomic<bool> m_canceled{false};
    std::mutex m_sceneMutex;

};

}
