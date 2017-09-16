#include "rpiPlatform.h"
#include "gl/hardware.h"
#include "log.h"
#include <stdio.h>
#include <stdarg.h>
#include <libgen.h>
#include <unistd.h>
#include <sys/resource.h>
#include <sys/syscall.h>

#include "context.h"

#define DEFAULT "fonts/NotoSans-Regular.ttf"
#define FONT_AR "fonts/NotoNaskh-Regular.ttf"
#define FONT_HE "fonts/NotoSansHebrew-Regular.ttf"
#define FONT_JA "fonts/DroidSansJapanese.ttf"
#define FALLBACK "fonts/DroidSansFallback.ttf"

namespace Tangram {

void logMsg(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
}

RpiPlatform::RpiPlatform() :
    m_urlClient(UrlClient::Options{}) {}

RpiPlatform::RpiPlatform(UrlClient::Options urlClientOptions) :
    m_urlClient(urlClientOptions) {}

void RpiPlatform::requestRender() const {
    setRenderRequest(true);
}

std::vector<FontSourceHandle> RpiPlatform::systemFontFallbacksHandle() const {
    std::vector<FontSourceHandle> handles;

    handles.emplace_back(DEFAULT);
    handles.emplace_back(FONT_AR);
    handles.emplace_back(FONT_HE);
    handles.emplace_back(FONT_JA);
    handles.emplace_back(FALLBACK);

    return handles;
}

bool RpiPlatform::startUrlRequest(const std::string& _url, UrlCallback _callback) {

    return m_urlClient.addRequest(_url, _callback);
}

void RpiPlatform::cancelUrlRequest(const std::string& _url) {

    m_urlClient.cancelRequest(_url);
}

RpiPlatform::~RpiPlatform() {}

void setCurrentThreadPriority(int priority) {
    // no-op
}

void initGLExtensions() {
    // no-op
}

} // namespace Tangram
