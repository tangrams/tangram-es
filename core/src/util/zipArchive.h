#pragma once

#define MINIZ_NO_ZLIB_APIS // Remove all ZLIB-style compression/decompression API's.
#define MINIZ_NO_ZLIB_COMPATIBLE_NAMES // Disable zlib names, to prevent conflicts against stock zlib.
#include <miniz.h>

#include <string>
#include <vector>

namespace Tangram {

class ZipArchive {

public:

    // An Entry represents a compressed file in a ZipArchive.
    struct Entry {
        std::string path;
        size_t uncompressedSize = 0;
    };

    // Create an empty archive.
    ZipArchive();

    // Dispose an archive and any memory it owns.
    ~ZipArchive();

    // Copying would cause memory errors on the internal archive pointer.
    ZipArchive(const ZipArchive& other) = delete;

    // Load a zip archive from its compressed data in memory. This creates a
    // list of entries for the archive, but does not decompress any data. If the
    // archive is successfully loaded this returns true, otherwise returns
    // false. The data is moved out of the input vector and retained until other
    // data is loaded or the archive is destroyed.
    bool loadFromMemory(std::vector<char> compressedArchiveData);

    // Empty the archive.
    void reset();

    // Get a read-only list of the entries in the archive.
    const std::vector<Entry>& entries() const { return entryList; }

    // Return a pointer to the entry for the given path, or null if there is no
    // entry for the path.
    const Entry* findEntry(const std::string& path) const;

    // Decompress the data from the given entry into the output buffer. The
    // caller MUST ensure that the output has enough space allocated to store
    // the uncompressed size of the entry. Returns false if the entry is not
    // from this archive or it can't be decompressed, otherwise returns true.
    bool decompressEntry(const Entry* entry, char* output);

protected:
    // Buffer of compressed zip archive data.
    std::vector<char> buffer;

    // List of file entries in the archive.
    std::vector<Entry> entryList;

    // Archive data used by miniz.
    mz_zip_archive minizData;
};

} // namespace Tangram
