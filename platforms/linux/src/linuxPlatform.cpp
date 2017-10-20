#include "linuxPlatform.h"
#include "gl/hardware.h"
#include "log.h"
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
    glfwPostEmptyEvent();
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
