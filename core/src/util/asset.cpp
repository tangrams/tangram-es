#include "util/asset.h"
#include "platform.h"
#include "log.h"

#define MINIZ_HEADER_FILE_ONLY
#include <miniz.c>

namespace Tangram {

void MZ_ZIP_ARCHIVE_DELETER::operator()(void *zip) {
    mz_free(zip);
}

Asset::Asset(std::string name, std::string path, std::shared_ptr<ZipHandle> zipHandle,
        std::vector<char> zippedData) :
        m_name(name),
        m_path(path),
        m_zipHandle(zipHandle) {

    buildZipHandle(zippedData);
}

Asset::~Asset() {
    if (m_zipHandle && m_zipHandle->archiveHandle) {
        mz_zip_reader_end(static_cast<mz_zip_archive*>(m_zipHandle->archiveHandle.get()));
    }
}

bool Asset::isBaseYaml(const std::string& filename) {
    auto extLoc = filename.find(".yaml");
    if (extLoc == std::string::npos) { return false; }

    auto slashLoc = filename.find("/");
    if (slashLoc != std::string::npos) { return false; }
    return true;
}

void Asset::buildZipHandle(const std::vector<char>& zipData) {

    if (zipData.empty()) { return; }

    m_zipHandle = std::make_shared<ZipHandle>();
    m_zipHandle->archiveHandle = ZipHandle::unique_ptr_zip_archive(new mz_zip_archive());

    mz_zip_archive* zip = static_cast<mz_zip_archive*>(m_zipHandle->archiveHandle.get());
    memset(zip, 0, sizeof(mz_zip_archive));
    if (!mz_zip_reader_init_mem(zip, zipData.data(), zipData.size(), 0)) {
        LOGE("ZippedAssetPackage: Could not open archive: %s", m_name.c_str());
        m_zipHandle.reset();
        return;
    }

    /* Instead of using mz_zip_reader_locate_file, maintaining a map of file name to index,
     * for performance reasons.
     * https://www.ncbi.nlm.nih.gov/IEB/ToolBox/CPP_DOC/lxr/source/src/util/compress/api/miniz/miniz.c
     */
    for (unsigned int i = 0; i < mz_zip_reader_get_num_files(zip); i++) {
        mz_zip_archive_file_stat st;
        if (!mz_zip_reader_file_stat(zip, i, &st)) {
            LOGE("ZippedAssetPackage: Could not read file stats: %s", st.m_filename);
            continue;
        }
        if (isBaseYaml(st.m_filename)) {
            m_path = st.m_filename;
        }
        m_zipHandle->fileIndices[st.m_filename] = i;
    }
}


std::vector<char> Asset::readBytesFromAsset(const std::shared_ptr<Platform>& platform, const std::string& filename) {

    std::vector<char> fileData;

    if (m_zipHandle) {
        if (m_zipHandle->archiveHandle) {
            mz_zip_archive* zip = static_cast<mz_zip_archive*>(m_zipHandle->archiveHandle.get());
            auto it = m_zipHandle->fileIndices.find(filename);

            if (it != m_zipHandle->fileIndices.end()) {
                std::size_t elementSize = 0;
                char* elementData = static_cast<char*>(mz_zip_reader_extract_to_heap(zip, it->second, &elementSize, 0));
                if (!elementData) {
                    LOGE("ZippedAssetPackage::loadAsset: Could not load archive asset: %s", filename.c_str());
                } else {
                    fileData.resize(elementSize);
                    fileData.assign(elementData, elementData + elementSize);
                    mz_free(elementData);
                }
                elementData = nullptr;
            }
        }
    }

    return fileData;
}

std::vector<char> Asset::readBytesFromAsset(const std::shared_ptr<Platform> &platform) {
    if (m_zipHandle) {
        return readBytesFromAsset(platform, m_path);
    }
    return platform->bytesFromFile(m_name.c_str());
}

std::string Asset::readStringFromAsset(const std::shared_ptr<Platform>& platform, const std::string& filename) {

    if (m_zipHandle) {
        auto data = readBytesFromAsset(platform, filename);
        return std::string(data.begin(), data.end());
    }

    return "";
}


std::string Asset::readStringFromAsset(const std::shared_ptr<Platform> &platform) {

    if (m_zipHandle) {
        auto data = readBytesFromAsset(platform, m_path);
        return std::string(data.begin(), data.end());
    }

    return platform->stringFromFile(m_name.c_str());
}


}
