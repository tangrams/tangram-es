#include "scene/asset.h"
#include "platform.h"
#include "log.h"

// Define MINIZ_NO_ZLIB_APIS to remove all ZLIB-style compression/decompression API's.
#define MINIZ_NO_ZLIB_APIS
// Define MINIZ_NO_ZLIB_COMPATIBLE_NAME to disable zlib names, to prevent conflicts against stock zlib.
#define MINIZ_NO_ZLIB_COMPATIBLE_NAMES

#include <miniz.h>

#include <unordered_map>
#include <tuple>

namespace Tangram {

struct ZipHandle {
    ~ZipHandle();
    std::unique_ptr<mz_zip_archive> archiveHandle;
    std::unordered_map<std::string, std::pair<unsigned int, size_t>> fileInfo;

    // Path to the zip bundle (helps in resolving zip resource path)
    std::string bundlePath = "";
    std::vector<char> data;
};

ZipHandle::~ZipHandle() {
    if (archiveHandle) {
        mz_zip_reader_end(archiveHandle.get());
    }
}


/* Asset Class Implementation */
Asset::Asset(std::string name) : m_name(name) {}

std::vector<char> Asset::readBytesFromAsset(const std::shared_ptr<Platform> &platform) const {
    return platform->bytesFromFile(m_name.c_str());
}

std::string Asset::readStringFromAsset(const std::shared_ptr<Platform> &platform) const {
    return platform->stringFromFile(m_name.c_str());
}


/* ZippedAsset Class Implementation */
ZippedAsset::ZippedAsset(std::string name, std::shared_ptr<ZipHandle> zipHandle, std::vector<char> zippedData) :
        Asset(name),
        m_zipHandle(zipHandle) {

    buildZipHandle(zippedData);
}

bool ZippedAsset::isBaseSceneYaml(const std::string& filePath) const {
    auto extLoc = filePath.find(".yaml");
    if (extLoc == std::string::npos) { return false; }

    auto slashLoc = filePath.find("/");
    if (slashLoc != std::string::npos) { return false; }
    return true;
}

void ZippedAsset::buildZipHandle(std::vector<char>& zipData) {

    if (zipData.empty()) { return; }

    m_zipHandle = std::make_shared<ZipHandle>();
    m_zipHandle->archiveHandle.reset(new mz_zip_archive());
    m_zipHandle->data.swap(zipData);

    mz_zip_archive* zip = m_zipHandle->archiveHandle.get();
    memset(zip, 0, sizeof(mz_zip_archive));
    if (!mz_zip_reader_init_mem(zip, m_zipHandle->data.data(), m_zipHandle->data.size(), 0)) {
        LOGE("ZippedAssetPackage: Could not open archive: %s", m_name.c_str());
        m_zipHandle.reset();
        return;
    }

    auto lastPathSegment = m_name.rfind('/');
    m_zipHandle->bundlePath = (lastPathSegment == std::string::npos) ? "" : m_name.substr(0, lastPathSegment+1);

    /* Instead of using mz_zip_reader_locate_file, maintaining a map of file name to index,
     * for performance reasons.
     * https://www.ncbi.nlm.nih.gov/IEB/ToolBox/CPP_DOC/lxr/source/src/util/compress/api/miniz/miniz.c
     */
    const auto& numFiles = mz_zip_reader_get_num_files(zip);
    for (unsigned int i = 0; i < numFiles; i++) {
        mz_zip_archive_file_stat st;
        if (!mz_zip_reader_file_stat(zip, i, &st)) {
            LOGE("ZippedAssetPackage: Could not read file stats: %s", st.m_filename);
            continue;
        }
        if (isBaseSceneYaml(st.m_filename)) {
            m_name = m_zipHandle->bundlePath + st.m_filename;
        }
        m_zipHandle->fileInfo[st.m_filename] = std::pair<unsigned int, size_t>(i, st.m_uncomp_size);
    }
}

bool ZippedAsset::bytesFromAsset(const std::string& filePath, std::function<char*(size_t)> allocator) const{

    auto pos = filePath.find(m_zipHandle->bundlePath);
    if (pos != 0) {
        LOGE("Invalid asset path: %s", m_name.c_str());
        return false;
    }

    auto resourcePath = filePath.substr(m_zipHandle->bundlePath.size());
    if (*resourcePath.begin() == '/') { resourcePath.erase(resourcePath.begin()); }

    if (m_zipHandle->archiveHandle) {
        mz_zip_archive* zip = m_zipHandle->archiveHandle.get();
        auto it = m_zipHandle->fileInfo.find(resourcePath);

        if (it != m_zipHandle->fileInfo.end()) {
            std::size_t elementSize = it->second.second;
            char* elementData = allocator(elementSize);

            /* read file data directly to memory */
            if (mz_zip_reader_extract_to_mem(zip, it->second.first, elementData, elementSize, 0)) {
                if (!elementData) {
                    LOGE("ZippedAssetPackage::loadAsset: Could not load archive asset: %s", filePath.c_str());
                    return false;
                }
                return true;
            }
        }
    }
    return false;

}

std::vector<char> ZippedAsset::readBytesFromAsset(const std::shared_ptr<Platform>& platform,
                                                  const std::string& filePath) const {

    if (!m_zipHandle) { return {}; }

    std::vector<char> fileData;

    auto allocator = [&](size_t size) {
        fileData.resize(size);
        return fileData.data();
    };

    bytesFromAsset(filePath, allocator);

    if (fileData.empty()) {
        LOGE("Asset \"%s\" read resulted in no data read. Verify the path in the scene.", filePath.c_str());
    }

    return fileData;
}

std::vector<char> ZippedAsset::readBytesFromAsset(const std::shared_ptr<Platform> &platform) const {
    return readBytesFromAsset(platform, m_name);
}

std::string ZippedAsset::readStringFromAsset(const std::shared_ptr<Platform>& platform,
                                             const std::string& filePath) const {

    if (!m_zipHandle) { return ""; }

    std::string fileData;

    auto allocator = [&](size_t size) {
        fileData.resize(size);
        return &fileData[0];
    };

    bytesFromAsset(filePath, allocator);

    if (fileData.empty()) {
        LOGE("Asset \"%s\" read resulted in no data read. Verify the path in the scene.", m_name.c_str());
    }

    return fileData;
}


std::string ZippedAsset::readStringFromAsset(const std::shared_ptr<Platform> &platform) const {
    return readStringFromAsset(platform, m_name);
}


}
