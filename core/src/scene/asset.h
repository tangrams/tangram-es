#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>

namespace Tangram {

class Platform;
struct ZipHandle;

/*
 * An Asset class representing every asset loaded via the scene file
 */
class Asset {
public:
    Asset(std::string name);

    const std::string& name() const { return m_name; }

    // Non zipped asset
    virtual std::shared_ptr<ZipHandle> zipHandle() const { return nullptr; }

    // returns the string from file
    virtual std::string readStringFromAsset(const std::shared_ptr<Platform>& platform) const;

    // returns the raw bytes from file
    virtual std::vector<char> readBytesFromAsset(const std::shared_ptr<Platform>& platform) const;

protected:
    // Name of an asset (resolved path with respect to the parent scene file to which this asset belongs)
    // Also used to read contents of the asset
    std::string m_name;
};


/*
 * A specialized asset representation for assets within a zip bundle
 */
class ZippedAsset : public Asset {
public:
    ZippedAsset(std::string name, std::string resourcePath, std::shared_ptr<ZipHandle> zipHandle = nullptr,
            std::vector<char> zippedData = {});

    std::shared_ptr<ZipHandle> zipHandle() const override { return m_zipHandle; }
    const std::string& resourcePath() const { return m_resourcePath; }

    // builds zipHandle (if not already built, from raw Data)
    void buildZipHandle(std::vector<char>& zippedData);

    // returns the string from resource at m_resourcePath in zip archive
    std::string readStringFromAsset(const std::shared_ptr<Platform>& platform) const override;

    // returns the raw bytes from resource at m_resourcePath in zip archive
    std::vector<char> readBytesFromAsset(const std::shared_ptr<Platform>& platform) const override;

    // returns the string from file bundled within the zip archive
    std::string readStringFromAsset(const std::shared_ptr<Platform>& platform, const std::string& filename) const;

    // returns the bytes from file bundled within the zip archive
    std::vector<char> readBytesFromAsset(const std::shared_ptr<Platform>& platform, const std::string& filename) const;

private:
    // path within a zip, empty for a non zipped asset
    std::string m_resourcePath;
    std::shared_ptr<ZipHandle> m_zipHandle = nullptr;

    // Check if the filePath is the base scene yaml
    bool isBaseSceneYaml(const std::string& filePath) const ;

    // read raw bytes from a bundled file
    bool bytesFromAsset(const std::string& filename, std::function<char*(size_t)> _allocator) const ;

};

}
