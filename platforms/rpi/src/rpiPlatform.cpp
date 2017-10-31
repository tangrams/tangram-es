#include "rpiPlatform.h"
#include "linuxSystemFontHelper.h"
#include "gl/hardware.h"
#include "log.h"
#include <algorithm>
#include <stdio.h>
#include <stdarg.h>
#include <libgen.h>
#include <unistd.h>
#include <sys/resource.h>
#include <sys/syscall.h>

#include "context.h"

namespace Tangram {

void logMsg(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
}

RpiPlatform::RpiPlatform() :
    m_urlClient(UrlClient::Options{}) {
    m_fcConfig = FcInitLoadConfigAndFonts();
}

RpiPlatform::RpiPlatform(UrlClient::Options urlClientOptions) :
    m_urlClient(urlClientOptions) {}

void RpiPlatform::requestRender() const {
    setRenderRequest(true);
}

std::vector<FontSourceHandle> RpiPlatform::systemFontFallbacksHandle() const {

    /*
     * Read system fontconfig to get list of fallback font for each supported language
     */
    auto fallbackFonts = systemFallbackFonts(m_fcConfig);

    /*
     * create FontSourceHandle from the found list of fallback fonts
     */
    std::vector<FontSourceHandle> handles;
    handles.reserve(fallbackFonts.size());

    for (auto& path : fallbackFonts) {
        handles.emplace_back(Url(path));
    }

    return handles;
}

FontSourceHandle RpiPlatform::systemFont(const std::string& _name, const std::string& _weight,
        const std::string& _face) const {

    std::string fontFile = systemFontPath(m_fcConfig, _name, _weight, _face);

    if (fontFile.empty()) { return {}; }

    return FontSourceHandle(Url(fontFile));
}

UrlRequestHandle RpiPlatform::startUrlRequest(Url _url, UrlCallback _callback) {

    return m_urlClient.addRequest(_url.string(), _callback);
}

void RpiPlatform::cancelUrlRequest(UrlRequestHandle _request) {
    m_urlClient.cancelRequest(_request);
}

RpiPlatform::~RpiPlatform() {}

void setCurrentThreadPriority(int priority) {
    // no-op
}

void initGLExtensions() {
    // no-op
}

} // namespace Tangram
