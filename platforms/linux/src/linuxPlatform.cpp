#include "linuxPlatform.h"
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

#if defined(TANGRAM_LINUX)
#include <GLFW/glfw3.h>
#elif defined(TANGRAM_RPI)
#include "context.h"
#endif

namespace Tangram {

void logMsg(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
}

LinuxPlatform::LinuxPlatform() :
    m_urlClient(UrlClient::Options{}) {
    m_fcConfig = FcInitLoadConfigAndFonts();
}

LinuxPlatform::LinuxPlatform(UrlClient::Options urlClientOptions) :
    m_urlClient(urlClientOptions) {}

void LinuxPlatform::requestRender() const {
    glfwPostEmptyEvent();
}

std::vector<FontSourceHandle> LinuxPlatform::systemFontFallbacksHandle() const {

    /*
     * Read system fontconfig to get list of fallback font for each supported language
     */
    auto fallbackFonts = systemFallbackFonts(m_fcConfig);

    /*
     * create FontSourceHandle from the found list of fallback fonts
     */
    std::vector<FontSourceHandle> handles;

    for (auto& path : fallbackFonts) {
        handles.emplace_back(path);
    }

    return handles;
}

FontSourceHandle LinuxPlatform::systemFont(const std::string& _name, const std::string& _weight,
        const std::string& _face) const {

    std::string fontFile = systemFontPath(m_fcConfig, _name, _weight, _face);

    if (fontFile.empty()) { return {}; }

    return FontSourceHandle(fontFile, true);
}

UrlRequestHandle LinuxPlatform::startUrlRequest(Url _url, UrlCallback _callback) {
    return m_urlClient.addRequest(_url.string(), _callback);
}

void LinuxPlatform::cancelUrlRequest(UrlRequestHandle _request) {
    m_urlClient.cancelRequest(_request);
}

LinuxPlatform::~LinuxPlatform() {}

void setCurrentThreadPriority(int priority) {
    int tid = syscall(SYS_gettid);
    setpriority(PRIO_PROCESS, tid, priority);
}

void initGLExtensions() {
    Tangram::Hardware::supportsMapBuffer = true;
}

} // namespace Tangram
