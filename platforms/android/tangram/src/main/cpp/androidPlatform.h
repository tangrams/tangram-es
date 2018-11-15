#pragma once

#include "platform.h"
#include "util/asyncWorker.h"


#include <jni.h>
#include <android/asset_manager.h>
#include <atomic>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace Tangram {

struct LabelPickResult;
struct FeaturePickResult;
struct MarkerPickResult;
class Map;
struct SceneUpdate;
struct SceneError;
using SceneID = int32_t;

std::string stringFromJString(JNIEnv* jniEnv, jstring string);
jstring jstringFromString(JNIEnv* jniEnv, const std::string& string);

class AndroidPlatform : public Platform {

public:

    AndroidPlatform(JNIEnv* _jniEnv, jobject _assetManager, jobject _tangramInstance);
    void dispose(JNIEnv* _jniEnv);
    void requestRender() const override;
    void setContinuousRendering(bool _isContinuous) override;
    FontSourceHandle systemFont(const std::string& _name, const std::string& _weight, const std::string& _face) const override;
    std::vector<FontSourceHandle> systemFontFallbacksHandle() const override;
    UrlRequestHandle startUrlRequest(Url _url, UrlCallback _callback) override;
    void cancelUrlRequest(UrlRequestHandle _request) override;
    void sceneReadyCallback(SceneID id, const SceneError* error);
    void cameraAnimationCallback(bool finished);
    void featurePickCallback(const FeaturePickResult* featurePickResult);
    void labelPickCallback(const LabelPickResult* labelPickResult);
    void markerPickCallback(const MarkerPickResult* markerPickResult);

    void onUrlComplete(JNIEnv* jniEnv, jlong jRequestHandle, jbyteArray jBytes, jstring jError);

    static void bindJniEnvToThread(JNIEnv* jniEnv);
    static jint jniOnLoad(JavaVM* javaVM);
    static void jniOnUnload(JavaVM* javaVM);

private:

    std::vector<char> bytesFromFile(const Url& _url) const;
    bool bytesFromAssetManager(const char* _path, std::function<char*(size_t)> _allocator) const;
    std::string fontPath(const std::string& _family, const std::string& _weight, const std::string& _style) const;
    std::string fontFallbackPath(int _importance, int _weightHint) const;

    jobject m_tangramInstance;
    AAssetManager* m_assetManager;

    std::atomic_uint_fast64_t m_urlRequestCount;

    // m_callbackMutex should be locked any time m_callbacks is accessed.
    std::mutex m_callbackMutex;
    std::unordered_map<UrlRequestHandle, UrlCallback> m_callbacks;

    AsyncWorker m_fileWorker;

};

} // namespace Tangram
