#include "zipArchive.h"

namespace Tangram {

ZipArchive::ZipArchive() {
    mz_zip_zero_struct(&minizData);
}

ZipArchive::~ZipArchive() {
    reset();
}

bool ZipArchive::loadFromMemory(std::vector<char> compressedArchiveData) {
    // Reset to an empty state.
    reset();
    // Initialize the buffer and archive with the input data.
    buffer.swap(compressedArchiveData);
    if (!mz_zip_reader_init_mem(&minizData, buffer.data(), buffer.size(), 0)) {
        return false;
    }
    // Scan the archive entries into a list.
    auto numberOfFiles = mz_zip_reader_get_num_files(&minizData);
    entryList.reserve(numberOfFiles);
    for (size_t i = 0; i < numberOfFiles; i++) {
        Entry entry;
        mz_zip_archive_file_stat stats;
        if (mz_zip_reader_file_stat(&minizData, i, &stats)) {
            entry.path = stats.m_filename;
            entry.uncompressedSize = stats.m_uncomp_size;
        }
        entryList.push_back(entry);
    }
    return true;
}

const ZipArchive::Entry* ZipArchive::findEntry(const std::string& path) const {
    for (const auto& entry : entryList) {
        if (entry.path == path) {
            return &entry;
        }
    }
    return nullptr;
}

bool ZipArchive::decompressEntry(const Entry* entry, char* output) {
    // Check that the given pointer refers to an entry in our list.
    if (entry == nullptr || entry < entryList.data() || entry >= entryList.data() + entryList.size()) {
        return false;
    }
    // Get the index of the entry (this arithmetic is only legal in an array).
    size_t index = entry - entryList.data();
    size_t size = entry->uncompressedSize;
    return mz_zip_reader_extract_to_mem(&minizData, index, output, size, 0);
}

void ZipArchive::reset() {
    // Close and free the miniz archive (if null, this is a no-op).
    mz_zip_reader_end(&minizData);
    mz_zip_zero_struct(&minizData);
    // Empty the buffer and entry list.
    buffer.clear();
    entryList.clear();
}

}
