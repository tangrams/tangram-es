#include "platform_linux.h"
#include "gl/hardware.h"
#include "log.h"
#include <stdio.h>
#include <stdarg.h>
#include <libgen.h>
#include <unistd.h>
#include <sys/resource.h>
#include <sys/syscall.h>

#if defined(PLATFORM_LINUX)
#include <GLFW/glfw3.h>
#elif defined(PLATFORM_RPI)
#include "context.h"
#endif

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

LinuxPlatform::LinuxPlatform() :
    m_urlClient(UrlClient::Options{}) {}

LinuxPlatform::LinuxPlatform(UrlClient::Options urlClientOptions) :
    m_urlClient(urlClientOptions) {}

void LinuxPlatform::requestRender() const {
#if defined(PLATFORM_LINUX)
    glfwPostEmptyEvent();
#elif defined(PLATFORM_RPI)
    setRenderRequest(true);
#endif
}

std::vector<FontSourceHandle> LinuxPlatform::systemFontFallbacksHandle() const {
    std::vector<FontSourceHandle> handles;

    handles.emplace_back(DEFAULT);
    handles.emplace_back(FONT_AR);
    handles.emplace_back(FONT_HE);
    handles.emplace_back(FONT_JA);
    handles.emplace_back(FALLBACK);

    return handles;
}

bool LinuxPlatform::startUrlRequest(const std::string& _url, UrlCallback _callback) {

    return m_urlClient.addRequest(_url, _callback);
}

void LinuxPlatform::cancelUrlRequest(const std::string& _url) {

    m_urlClient.cancelRequest(_url);
}

LinuxPlatform::~LinuxPlatform() {}

void setCurrentThreadPriority(int priority) {
#if defined(PLATFORM_LINUX)
    int tid = syscall(SYS_gettid);
    setpriority(PRIO_PROCESS, tid, priority);
#elif defined(PLATFORM_RPI)
    // no-op
#endif
}

void initGLExtensions() {
#if defined(PLATFORM_LINUX)
    Tangram::Hardware::supportsMapBuffer = true;
#elif defined(PLATFORM_RPI)
    // no-op
#endif
}

} // namespace Tangram
