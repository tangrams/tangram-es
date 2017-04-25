#pragma once

#include "platform.h"

#include <unordered_map>

namespace Tangram {

class MockPlatform : public Platform {

public:

    void requestRender() const override;
    std::vector<FontSourceHandle> systemFontFallbacksHandle() const override;
    UrlRequestHandle startUrlRequest(Url _url, UrlCallback _callback) override;
    void cancelUrlRequest(UrlRequestHandle _request) override;

    // Put content at a URL to be retrieved by startUrlRequest.
    void putMockUrlContents(Url url, std::string contents);

    // Get the contents of a local file (not mock URL contents).
    static std::vector<char> getBytesFromFile(const char* path);

private:

    std::unordered_map<Url, std::string> m_files;

};

} // namespace Tangram
