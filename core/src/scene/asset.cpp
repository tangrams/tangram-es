#include "scene/asset.h"
#include "platform.h"
#include "log.h"

#define MINIZ_HEADER_FILE_ONLY
#include <miniz.c>

#include <unordered_map>
#include <tuple>

namespace Tangram {

struct ZipHandle {
    ~ZipHandle();
    std::unique_ptr<mz_zip_archive> archiveHandle;
    std::unordered_map<std::string, std::pair<unsigned int, size_t>> fileInfo;

    std::vector<char> data;
};

ZipHandle::~ZipHandle() {
    if (archiveHandle) {
        mz_zip_reader_end(archiveHandle.get());
    }
}

Asset::Asset(std::string name, std::string path, std::shared_ptr<ZipHandle> zipHandle,
        std::vector<char> zippedData) :
        m_name(name),
        m_path(path),
        m_zipHandle(zipHandle) {

    buildZipHandle(zippedData);
}

bool Asset::isBaseYaml(const std::string& filePath) {
    auto extLoc = filePath.find(".yaml");
    if (extLoc == std::string::npos) { return false; }

    auto slashLoc = filePath.find("/");
    if (slashLoc != std::string::npos) { return false; }
    return true;
}

void Asset::buildZipHandle(std::vector<char>& zipData) {

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
        if (isBaseYaml(st.m_filename)) {
            m_path = st.m_filename;
        }
        m_zipHandle->fileInfo[st.m_filename] = std::pair<unsigned int, size_t>(i, st.m_uncomp_size);
    }
}

bool Asset::bytesFromAsset(const std::string& filePath, std::function<char*(size_t)> allocator) {

    if (m_zipHandle->archiveHandle) {
        mz_zip_archive* zip = m_zipHandle->archiveHandle.get();
        auto it = m_zipHandle->fileInfo.find(filePath);

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

std::vector<char> Asset::readBytesFromAsset(const std::shared_ptr<Platform>& platform, const std::string& filePath) {

    if (!zipHandle()) { return {}; }

    std::vector<char> fileData;

    auto allocator = [&](size_t size) {
        fileData.resize(size);
        return fileData.data();
    };

    bytesFromAsset(filePath, allocator);

    if (fileData.empty()) {
        LOGE("Asset \"%s\" read resulted in no data read. Verify the path in the scene.", m_name.c_str());
    }

    return fileData;
}

std::vector<char> Asset::readBytesFromAsset(const std::shared_ptr<Platform> &platform) {
    if (m_zipHandle) {
        return readBytesFromAsset(platform, m_path);
    }
    return platform->bytesFromFile(m_name.c_str());
}

std::string Asset::readStringFromAsset(const std::shared_ptr<Platform>& platform, const std::string& filePath) {

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


std::string Asset::readStringFromAsset(const std::shared_ptr<Platform> &platform) {

    if (m_zipHandle) {
        return readStringFromAsset(platform, m_path);
    }

    return platform->stringFromFile(m_name.c_str());
}


}
