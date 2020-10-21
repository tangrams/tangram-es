#include "AndroidPlatform.h"

#include "JniHelpers.h"
#include "JniThreadBinding.h"

#include "log.h"
#include "util/url.h"

#ifndef GL_GLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES 1
#endif

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <GLES2/gl2platform.h>
#include <android/log.h>
#include <android/asset_manager_jni.h>
#include <cstdarg>
#include <dlfcn.h> // dlopen, dlsym
#include <libgen.h>
#include <unistd.h>
#include <sys/resource.h>

#ifdef TANGRAM_MBTILES_DATASOURCE
#include "sqlite3ndk.h"
#endif

PFNGLBINDVERTEXARRAYOESPROC glBindVertexArrayOESEXT = 0;
PFNGLDELETEVERTEXARRAYSOESPROC glDeleteVertexArraysOESEXT = 0;
PFNGLGENVERTEXARRAYSOESPROC glGenVertexArraysOESEXT = 0;

namespace Tangram {

/* Followed the following document for JavaVM tips when used with native threads
 * http://android.wooyd.org/JNIExample/#NWD1sCYeT-I
 * http://developer.android.com/training/articles/perf-jni.html and
 * http://www.ibm.com/developerworks/library/j-jni/
 * http://docs.oracle.com/javase/7/docs/technotes/guides/jni/spec/invocation.html
 */

// JNI Env bound on androids render thread (our native main thread)
static jmethodID requestRenderMethodID = nullptr;
static jmethodID setRenderModeMethodID = nullptr;
static jmethodID startUrlRequestMID = nullptr;
static jmethodID cancelUrlRequestMID = nullptr;
static jmethodID getFontFilePath = nullptr;
static jmethodID getFontFallbackFilePath = nullptr;

static bool glExtensionsLoaded = false;

void AndroidPlatform::jniOnLoad(JavaVM* javaVM, JNIEnv* jniEnv) {
    // JNI OnLoad is invoked once when the native library is loaded so this is a good place to cache
    // any method or class IDs that we'll need.
    jclass tangramClass = jniEnv->FindClass("com/mapzen/tangram/MapController");
    startUrlRequestMID = jniEnv->GetMethodID(tangramClass, "startUrlRequest", "(Ljava/lang/String;J)V");
    cancelUrlRequestMID = jniEnv->GetMethodID(tangramClass, "cancelUrlRequest", "(J)V");
    getFontFilePath = jniEnv->GetMethodID(tangramClass, "getFontFilePath", "(Ljava/lang/String;)Ljava/lang/String;");
    getFontFallbackFilePath = jniEnv->GetMethodID(tangramClass, "getFontFallbackFilePath", "(II)Ljava/lang/String;");
    requestRenderMethodID = jniEnv->GetMethodID(tangramClass, "requestRender", "()V");
    setRenderModeMethodID = jniEnv->GetMethodID(tangramClass, "setRenderMode", "(I)V");
}

void logMsg(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    __android_log_vprint(ANDROID_LOG_DEBUG, "Tangram", fmt, args);
    va_end(args);
}

AndroidPlatform::AndroidPlatform(JNIEnv* jniEnv, jobject mapController, jobject assetManager)
    : m_jniWorker(JniHelpers::getJVM()) {

    m_mapController = jniEnv->NewWeakGlobalRef(mapController);

    m_assetManager = AAssetManager_fromJava(jniEnv, assetManager);

    if (m_assetManager == nullptr) {
        LOGE("Could not obtain Asset Manager reference");
        return;
    }

#ifdef TANGRAM_MBTILES_DATASOURCE
    sqlite3_ndk_init(m_assetManager);
#endif
}

void AndroidPlatform::shutdown() {
    Platform::shutdown();
    m_jniWorker.stop();
}

std::string AndroidPlatform::fontPath(const std::string& family, const std::string& weight, const std::string& style) const {

    JniThreadBinding jniEnv(JniHelpers::getJVM());

    std::string key = family + "_" + weight + "_" + style;

    jstring jkey = JniHelpers::javaStringFromString(jniEnv, key);
    jstring returnStr = (jstring) jniEnv->CallObjectMethod(m_mapController, getFontFilePath, jkey);

    auto resultStr = JniHelpers::stringFromJavaString(jniEnv, returnStr);
    jniEnv->DeleteLocalRef(returnStr);
    jniEnv->DeleteLocalRef(jkey);

    return resultStr;
}

void AndroidPlatform::requestRender() const {
    m_jniWorker.enqueue([&](JNIEnv *jniEnv) {
        jniEnv->CallVoidMethod(m_mapController, requestRenderMethodID);
    });
}

std::vector<FontSourceHandle> AndroidPlatform::systemFontFallbacksHandle() const {
    JniThreadBinding jniEnv(JniHelpers::getJVM());

    std::vector<FontSourceHandle> handles;

    int importance = 0;
    int weightHint = 400;

    auto fontFallbackPath = [&](int _importance, int _weightHint) {

        jstring returnStr = (jstring) jniEnv->CallObjectMethod(m_mapController,
                                                               getFontFallbackFilePath, _importance,
                                                               _weightHint);

        auto resultStr = JniHelpers::stringFromJavaString(jniEnv, returnStr);
        jniEnv->DeleteLocalRef(returnStr);

        return resultStr;
    };

    std::string fallbackPath = fontFallbackPath(importance, weightHint);

    while (!fallbackPath.empty()) {
        handles.emplace_back(Url(fallbackPath));

        fallbackPath = fontFallbackPath(++importance, weightHint);
    }

    return handles;
}

FontSourceHandle AndroidPlatform::systemFont(const std::string& name, const std::string& weight,
                                             const std::string& face) const {
    std::string path = fontPath(name, weight, face);

    if (path.empty()) { return {}; }

    auto data = bytesFromFile(path.c_str());

    return FontSourceHandle([data]() { return data; });
}

void AndroidPlatform::setContinuousRendering(bool isContinuous) {
    Platform::setContinuousRendering(isContinuous);

    JniThreadBinding jniEnv(JniHelpers::getJVM());

    jniEnv->CallVoidMethod(m_mapController, setRenderModeMethodID, isContinuous ? 1 : 0);
}

bool AndroidPlatform::bytesFromAssetManager(const char* path, std::function<char*(size_t)> allocator) const {

    AAsset* asset = AAssetManager_open(m_assetManager, path, AASSET_MODE_UNKNOWN);
    if (asset == nullptr) {
        LOGW("Failed to open asset at path: %s", path);
        return false;
    }

    size_t size = AAsset_getLength(asset);
    unsigned char* data = reinterpret_cast<unsigned char*>(allocator(size));

    int read = AAsset_read(asset, data, size);
    if (read <= 0) {
        LOGW("Failed to read asset at path: %s", path);
    }
    AAsset_close(asset);

    return read > 0;
}

std::vector<char> AndroidPlatform::bytesFromFile(const Url& url) const {
    std::vector<char> data;

    auto allocator = [&](size_t size) {
        data.resize(size);
        return data.data();
    };

    auto path = url.path();

    if (url.scheme() == "asset") {
        // The asset manager doesn't like paths starting with '/'.
        if (!path.empty() && path.front() == '/') {
            path = path.substr(1);
        }
        bytesFromAssetManager(path.c_str(), allocator);
    } else {
        Platform::bytesFromFileSystem(path.c_str(), allocator);
    }

    return data;
}

bool AndroidPlatform::startUrlRequestImpl(const Url& url, const UrlRequestHandle request, UrlRequestId& id) {

    // If the requested URL does not use HTTP or HTTPS, retrieve it asynchronously.
    if (!url.hasHttpScheme()) {
        m_fileWorker.enqueue([=](){
             UrlResponse response;
             response.content = bytesFromFile(url);
             onUrlResponse(request, std::move(response));
        });
        return false;
    }

    // We can use UrlRequestHandle to cancel requests. MapController handles the
    // mapping between UrlRequestHandle and request object
    id = request;

    m_jniWorker.enqueue([=](JNIEnv *jniEnv) {
        jlong jRequestHandle = static_cast<jlong>(request);

        // Make sure no one changed UrlRequestHandle from being uint64_t,
        // so that it's safe to convert to jlong and back.
        static_assert(sizeof(jlong) == sizeof(UrlRequestHandle), "Who changed UrlRequestHandle?!");
        static_assert(static_cast<jlong>(std::numeric_limits<uint64_t>::max()) ==
                      static_cast<UrlRequestHandle>(std::numeric_limits<uint64_t>::max()),
                      "Cannot convert jlong to UrlRequestHandle!");

        jstring jUrl = JniHelpers::javaStringFromString(jniEnv, url.string());

        // Call the MapController method to start the URL request.
        jniEnv->CallVoidMethod(m_mapController, startUrlRequestMID, jUrl, jRequestHandle);
        jniEnv->DeleteLocalRef(jUrl);
    });

    return true;
}

void AndroidPlatform::cancelUrlRequestImpl(const UrlRequestId id) {

    m_jniWorker.enqueue([=](JNIEnv *jniEnv) {

        jlong jRequestHandle = static_cast<jlong>(id);

        jniEnv->CallVoidMethod(m_mapController, cancelUrlRequestMID, jRequestHandle);
    });
}

void AndroidPlatform::onUrlComplete(JNIEnv* _jniEnv, jlong _jRequestHandle, jbyteArray _jBytes, jstring _jError) {
    // Start building a response object.
    UrlResponse response;

    // If the request was successful, we will receive a non-null byte array.
    if (_jBytes != nullptr) {
        size_t length = _jniEnv->GetArrayLength(_jBytes);
        response.content.resize(length);
        _jniEnv->GetByteArrayRegion(_jBytes, 0, length, reinterpret_cast<jbyte*>(response.content.data()));
        // TODO: Can we use a DirectByteBuffer to transfer data with fewer copies?
    }

    // If the request was unsuccessful, we will receive a non-null error string.
    std::string error;
    if (_jError != nullptr) {
        error = JniHelpers::stringFromJavaString(_jniEnv, _jError);
        response.error = error.c_str();
    }

    // Handle callbacks on worker thread to not block Java side.
    // (The calling thread has probably also other work to do)
    m_fileWorker.enqueue([this, _jRequestHandle, r = std::move(response)]() mutable {
        UrlRequestHandle requestHandle = static_cast<UrlRequestHandle>(_jRequestHandle);

        onUrlResponse(requestHandle, std::move(r));
    });
}

void setCurrentThreadPriority(int priority) {
    setpriority(PRIO_PROCESS, 0, priority);
}

void initGLExtensions() {
    if (glExtensionsLoaded) {
        return;
    }

    void* libhandle = dlopen("libGLESv2.so", RTLD_LAZY);

    glBindVertexArrayOESEXT = (PFNGLBINDVERTEXARRAYOESPROC) dlsym(libhandle, "glBindVertexArrayOES");
    glDeleteVertexArraysOESEXT = (PFNGLDELETEVERTEXARRAYSOESPROC) dlsym(libhandle, "glDeleteVertexArraysOES");
    glGenVertexArraysOESEXT = (PFNGLGENVERTEXARRAYSOESPROC) dlsym(libhandle, "glGenVertexArraysOES");

    glExtensionsLoaded = true;
}

} // namespace Tangram
