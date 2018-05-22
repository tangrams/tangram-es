#include "windowsPlatform.h"
#include "gl/hardware.h"
#include "log.h"
#include <stdio.h>
#include <stdarg.h>
#include <libgen.h>
#include <unistd.h>

#include <GLFW/glfw3.h>

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

WindowsPlatform::WindowsPlatform() :
    m_urlClient(UrlClient::Options{}) {}

WindowsPlatform::WindowsPlatform(UrlClient::Options urlClientOptions) :
    m_urlClient(urlClientOptions) {}

void WindowsPlatform::requestRender() const {
    glfwPostEmptyEvent();
}

std::vector<FontSourceHandle> WindowsPlatform::systemFontFallbacksHandle() const {
    std::vector<FontSourceHandle> handles;

    handles.emplace_back(Url(DEFAULT));
    handles.emplace_back(Url(FONT_AR));
    handles.emplace_back(Url(FONT_HE));
    handles.emplace_back(Url(FONT_JA));
    handles.emplace_back(Url(FALLBACK));

    return handles;
}

UrlRequestHandle WindowsPlatform::startUrlRequest(Url _url, UrlCallback _callback) {
    return m_urlClient.addRequest(_url.string(), _callback);
}

void WindowsPlatform::cancelUrlRequest(UrlRequestHandle _request) {
    m_urlClient.cancelRequest(_request);
}

WindowsPlatform::~WindowsPlatform() {}

void setCurrentThreadPriority(int priority) {
	// No-op
}

void initGLExtensions() {
    Tangram::Hardware::supportsMapBuffer = true;
}

} // namespace Tangram
