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

LinuxPlatform::LinuxPlatform() {
    m_urlClient = std::make_unique<UrlClient>(UrlClient::Options{});
    m_fcConfig = FcInitLoadConfigAndFonts();
}

LinuxPlatform::LinuxPlatform(UrlClient::Options urlClientOptions) :
    m_urlClient(std::make_unique<UrlClient>(urlClientOptions)) {
    m_fcConfig = FcInitLoadConfigAndFonts();
}

LinuxPlatform::~LinuxPlatform() {
    FcConfigDestroy(m_fcConfig);
}

void LinuxPlatform::shutdown() {
    // Stop all UrlWorker threads
    m_shutdown = true;
    m_urlClient.reset();
}

void LinuxPlatform::requestRender() const {
    if (m_shutdown) { return; }
    glfwPostEmptyEvent();
}

std::vector<FontSourceHandle> LinuxPlatform::systemFontFallbacksHandle() const {

    // Read system fontconfig to get list of fallback font for each
    // supported language
    auto fallbackFonts = systemFallbackFonts(m_fcConfig);

    // Create FontSourceHandle from the found list of fallback fonts
    std::vector<FontSourceHandle> handles;
    handles.reserve(fallbackFonts.size());

    std::transform(std::begin(fallbackFonts), std::end(fallbackFonts),
                   std::back_inserter(handles),
                   [](auto& path) { return FontSourceHandle(Url(path)); });

    return handles;
}

FontSourceHandle LinuxPlatform::systemFont(const std::string& _name,
                                           const std::string& _weight,
                                           const std::string& _face) const {

    auto fontFile = systemFontPath(m_fcConfig, _name, _weight, _face);

    if (fontFile.empty()) { return {}; }

    return FontSourceHandle(Url(fontFile));
}

UrlRequestHandle LinuxPlatform::startUrlRequest(Url _url, UrlCallback _callback) {
    if (m_shutdown) { return 0; }
    if (_url.hasHttpScheme()) {
        return m_urlClient->addRequest(_url.string(),
                                       [this, cb = _callback](UrlResponse&& r) {
                                           LOG(">>-------------->>");
                                           cb(std::move(r));
                                           LOG("<<--------------<<");
                                           requestRender();
                                       });
    } else {
        m_fileWorker.enqueue([path = _url.path(), _callback](){
             UrlResponse response;
             auto allocator = [&](size_t size) {
                 response.content.resize(size);
                 return response.content.data();
             };

             Platform::bytesFromFileSystem(path.c_str(), allocator);
             //LOGE("Ready file task: %s", path.c_str());

             _callback(std::move(response));
        });
        return std::numeric_limits<uint64_t>::max();
    }
}

void LinuxPlatform::cancelUrlRequest(UrlRequestHandle _request) {
    if (m_shutdown) { return; }
    if (_request == std::numeric_limits<uint64_t>::max()) { return; }

    m_urlClient->cancelRequest(_request);
}

void setCurrentThreadPriority(int priority) {
    setpriority(PRIO_PROCESS, 0, priority);
}

void initGLExtensions() {
    Tangram::Hardware::supportsMapBuffer = true;
}

} // namespace Tangram
