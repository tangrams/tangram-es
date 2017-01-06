//#ifdef PLATFORM_ANDROID

#include "data/properties.h"
#include "data/propertyItem.h"
#include "util/url.h"
#include "tangram.h"
#include "platform.h"
#include "IUrlCallback.hpp"
#include "ITangramHelper.hpp"

#include <GLES2/gl2platform.h>

#ifndef GL_GLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES 1
#endif

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <dlfcn.h> // dlopen, dlsym

#include <android/log.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <cstdarg>

#include <libgen.h>
#include <unistd.h>
#include <sys/resource.h>
#include <fstream>
#include <algorithm>

#include <regex>

#include "log.h"

#include "ITangramHelper.hpp"
#include "PlatformTangramImpl.hpp"

std::shared_ptr<ITangramHelper> tangramHelper;

void PlatformTangramImpl::init(std::shared_ptr<ITangramHelper> const& helper)
{
	tangramHelper = helper;
}

static AAssetManager* assetManager = nullptr;

PFNGLBINDVERTEXARRAYOESPROC glBindVertexArrayOESEXT = 0;
PFNGLDELETEVERTEXARRAYSOESPROC glDeleteVertexArraysOESEXT = 0;
PFNGLGENVERTEXARRAYSOESPROC glGenVertexArraysOESEXT = 0;

// Android assets are distinguished from file paths by the "asset" scheme.
static const char* aaPrefix = "asset:///";
static const size_t aaPrefixLen = 9;

void logMsg(const char* fmt, ...) {

    va_list args;
    va_start(args, fmt);
    __android_log_vprint(ANDROID_LOG_DEBUG, "Tangram", fmt, args);
    va_end(args);
}

void requestRender() {
	return tangramHelper->requestRender();
}

std::string fontFallbackPath(int importance, int weightHint) {
	return tangramHelper->getFontFallbackFilePath(importance, weightHint);
}

std::string fontPath(const std::string& _family, const std::string& _weight, const std::string& _style) {

    std::string key = _family + "_" + _weight + "_" + _style;

	return tangramHelper->getFontFilePath(key);
}

std::vector<FontSourceHandle> systemFontFallbacksHandle() {
    std::vector<FontSourceHandle> handles;

    int importance = 0;
    int weightHint = 400;

    std::string fallbackPath = fontFallbackPath(importance, weightHint);

    while (!fallbackPath.empty()) {
        LOG("Loading font %s", fallbackPath.c_str());

        handles.emplace_back(fallbackPath);

        fallbackPath = fontFallbackPath(importance++, weightHint);
    }

    return handles;
}

unsigned char* systemFont(const std::string& _name, const std::string& _weight, const std::string& _face, size_t* _size) {
    std::string path = fontPath(_name, _weight, _face);

    if (path.empty()) { return nullptr; }

    return bytesFromFile(path.c_str(), *_size);
}

void setContinuousRendering(bool _isContinuous) {

	tangramHelper->setContinuousRendering(_isContinuous);
}

bool isContinuousRendering() {
    return tangramHelper->isContinuousRendering();
}

bool bytesFromAssetManager(const char* _path, std::function<char*(size_t)> _allocator) {

    AAsset* asset = AAssetManager_open(assetManager, _path, AASSET_MODE_UNKNOWN);
    if (asset == nullptr) {
        logMsg("Failed to open asset at path: %s\n", _path);
        return false;
    }

    size_t size = AAsset_getLength(asset);
    unsigned char* data = reinterpret_cast<unsigned char*>(_allocator(size));

    int read = AAsset_read(asset, data, size);
    if (read <= 0) {
        logMsg("Failed to read asset at path: %s\n", _path);
    }
    AAsset_close(asset);

    return read > 0;
}

bool bytesFromFileSystem(const char* _path, std::function<char*(size_t)> _allocator) {

    std::ifstream resource(_path, std::ifstream::ate | std::ifstream::binary);

    if(!resource.is_open()) {
        logMsg("Failed to read file at path: %s\n", _path);
        return false;
    }

    size_t size = resource.tellg();
    char* cdata = _allocator(size);

    resource.seekg(std::ifstream::beg);
    resource.read(cdata, size);
    resource.close();

    return true;
}

std::string stringFromFile(const char* _path) {

    std::string data;

    auto allocator = [&](size_t size) {
        data.resize(size);
        return &data[0];
    };

    if (strncmp(_path, aaPrefix, aaPrefixLen) == 0) {
        bytesFromAssetManager(_path + aaPrefixLen, allocator);
    } else {
        bytesFromFileSystem(_path, allocator);
    }
    return data;
}

unsigned char* bytesFromFile(const char* _path, size_t& _size) {

    _size = 0;
    unsigned char* data = nullptr;

    auto allocator = [&](size_t size) {
        _size = size;
        data = (unsigned char*) malloc(sizeof(char) * size);
        return reinterpret_cast<char*>(data);
    };

    if (strncmp(_path, aaPrefix, aaPrefixLen) == 0) {
        bytesFromAssetManager(_path + aaPrefixLen, allocator);
    } else {
        bytesFromFileSystem(_path, allocator);
    }
    return data;
}

std::string resolveScenePath(const char* path) {
    // If the path is an absolute URL (like a file:// or http:// URL)
    // then resolving it will return the same URL. Otherwise, we resolve
    // it against the "asset" scheme to know later that this path is in
    // the asset bundle.
    return Tangram::Url(path).resolved("asset:///").string();
}

struct CUrlCallback : public IUrlCallback {
    UrlCallback callback_;

    CUrlCallback(UrlCallback const &callback) : callback_(callback) { }

    void onUrlSuccess(std::vector<int8_t> const &response) override {
        callback_.operator()(std::vector<char>(response.begin(), response.end()));
    }
	
	void onUrlFailure() override {
		callback_.operator()(std::vector<char>());
	}
};


bool startUrlRequest(const std::string& url, UrlCallback _callback) {

	return tangramHelper->startUrlRequest(url, std::make_shared<CUrlCallback>(_callback));
}

void cancelUrlRequest(const std::string& url) {
	tangramHelper->cancelUrlRequest(url);
}

void setCurrentThreadPriority(int priority) {
    int  tid = gettid();
    setpriority(PRIO_PROCESS, tid, priority);
}

void initGLExtensions() {
    void* libhandle = dlopen("libGLESv2.so", RTLD_LAZY);

    glBindVertexArrayOESEXT = (PFNGLBINDVERTEXARRAYOESPROC) dlsym(libhandle, "glBindVertexArrayOES");
    glDeleteVertexArraysOESEXT = (PFNGLDELETEVERTEXARRAYSOESPROC) dlsym(libhandle, "glDeleteVertexArraysOES");
    glGenVertexArraysOESEXT = (PFNGLGENVERTEXARRAYSOESPROC) dlsym(libhandle, "glGenVertexArraysOES");
}

//#endif
