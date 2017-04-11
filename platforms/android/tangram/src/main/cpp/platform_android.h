#pragma once

#include "platform.h"

#include <jni.h>
#include <android/asset_manager.h>

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace Tangram {

struct LabelPickResult;
struct FeaturePickResult;
struct MarkerPickResult;
struct SceneUpdateError;

void featurePickCallback(jobject listener, const FeaturePickResult* featurePickResult);
void markerPickCallback(jobject listener, jobject tangramInstance, const MarkerPickResult* markerPickResult);
void labelPickCallback(jobject listener, const LabelPickResult* labelPickResult);
void sceneUpdateErrorCallback(jobject updateStatusCallbackRef, const SceneUpdateError& sceneUpdateErrorStatus);

std::string stringFromJString(JNIEnv* jniEnv, jstring string);

class AndroidPlatform : public Platform {

public:

    AndroidPlatform(JNIEnv* _jniEnv, jobject _assetManager, jobject _tangramInstance);
    void dispose(JNIEnv* _jniEnv);
    void requestRender() const override;
    void setContinuousRendering(bool _isContinuous) override;
    std::vector<char> systemFont(const std::string& _name, const std::string& _weight, const std::string& _face) const override;
    std::vector<FontSourceHandle> systemFontFallbacksHandle() const override;
    UrlRequestHandle startUrlRequest(Url _url, UrlCallback _callback) override;
    void cancelUrlRequest(UrlRequestHandle _request) override;

    void onUrlComplete(JNIEnv* jniEnv, jlong jRequestHandle, jbyteArray jBytes, jstring jError);

    static void bindJniEnvToThread(JNIEnv* jniEnv);
    static void setupJniEnv(JNIEnv* _jniEnv);

private:

    std::vector<char> bytesFromFile(const Url& _url) const;
    bool bytesFromAssetManager(const char* _path, std::function<char*(size_t)> _allocator) const;
    std::string fontPath(const std::string& _family, const std::string& _weight, const std::string& _style) const;
    std::string fontFallbackPath(int _importance, int _weightHint) const;

    jobject m_tangramInstance;
    AAssetManager* m_assetManager;

    struct FileRequest {
        Url url;
        UrlCallback callback;
    };

    std::atomic_uint_fast64_t m_urlRequestCount;

    // m_fileRequestMutex should be locked any time m_fileRequests is accessed.
    std::mutex m_fileRequestMutex;
    std::vector<FileRequest> m_fileRequests;

    // This function loops while m_keepRunning is 'true', waiting on m_fileRequestCondition to get
    // items from the front of m_fileRequests and process them. m_fileRequestThread runs this function.
    void fileRequestLoop();
    std::condition_variable m_fileRequestCondition;
    std::thread m_fileRequestThread;
    bool m_keepRunning = true;

    // m_callbackMutex should be locked any time m_callbacks is accessed.
    std::mutex m_callbackMutex;
    std::unordered_map<UrlRequestHandle, UrlCallback> m_callbacks;

};

} // namespace Tangram
