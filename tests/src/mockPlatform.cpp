#include "mockPlatform.h"

#include <stdio.h>
#include <stdarg.h>
#include <iostream>
#include <fstream>
#include <string>

#include <libgen.h>

#define DEFAULT "fonts/NotoSans-Regular.ttf"
#define FONT_AR "fonts/NotoNaskh-Regular.ttf"
#define FONT_HE "fonts/NotoSansHebrew-Regular.ttf"
#define FONT_JA "fonts/DroidSansJapanese.ttf"
#define FALLBACK "fonts/DroidSansFallback.ttf"

#include "log.h"

namespace Tangram {

void logMsg(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
}

void MockPlatform::requestRender() const {}

std::vector<FontSourceHandle> MockPlatform::systemFontFallbacksHandle() const {
    std::vector<FontSourceHandle> handles;

    handles.emplace_back(DEFAULT);
    handles.emplace_back(FONT_AR);
    handles.emplace_back(FONT_HE);
    handles.emplace_back(FONT_JA);
    handles.emplace_back(FALLBACK);

    return handles;
}

UrlRequestHandle MockPlatform::startUrlRequest(Url _url, UrlCallback _callback) {

    UrlResponse response;

    auto it = m_files.find(_url);
    if (it != m_files.end()) {
        const auto& contents = it->second;
        response.content.assign(contents.begin(), contents.end());
    } else {
        response.error = "Url contents could not be found!";
    }

    _callback(response);

    return 0;
}

void MockPlatform::cancelUrlRequest(UrlRequestHandle _request) {}

void MockPlatform::putMockUrlContents(Url url, std::string contents) {
    m_files[url] = contents;
}

std::vector<char> MockPlatform::getBytesFromFile(const char* path) {
    std::vector<char> result;
    auto allocator = [&](size_t size) {
        result.resize(size);
        return result.data();
    };
    Platform::bytesFromFileSystem(path, allocator);
    return result;
}

void setCurrentThreadPriority(int priority) {}

void initGLExtensions() {}

} // namespace Tangram
