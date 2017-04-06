#pragma once

#include "platform.h"
#include "helpers_android.h"

#include <memory>
#include <jni.h>
#include <mutex>
#include <deque>
#include <android/asset_manager.h>

JavaVM* setupJniEnv(JNIEnv* _jniEnv);

void onUrlSuccess(JNIEnv* jniEnv, jbyteArray jFetchedBytes, jlong jCallbackPtr);
void onUrlFailure(JNIEnv* jniEnv, jlong jCallbackPtr);

std::string resolveScenePath(const char* path);
std::string stringFromJString(JNIEnv* jniEnv, jstring string);

namespace Tangram {

struct LabelPickResult;
struct FeaturePickResult;
struct MarkerPickResult;
struct SceneUpdateError;

typedef std::function<void(JNIEnv*)> AndroidUITask;

class AndroidPlatform : public Platform {

public:

    AndroidPlatform(JNIEnv* _jniEnv, JavaVM* _jvm, jobject _assetManager, jobject _tangramInstance);
    void dispose(JNIEnv* _jniEnv);
    void requestRender() const override;
    std::vector<char> bytesFromFile(const char* _path) const override;
    void setContinuousRendering(bool _isContinuous) override;
    std::string stringFromFile(const char* _path) const override;
    std::vector<char> systemFont(const std::string& _name, const std::string& _weight, const std::string& _face) const override;
    std::vector<FontSourceHandle> systemFontFallbacksHandle() const override;
    bool startUrlRequest(const std::string& _url, UrlCallback _callback) override;
    void cancelUrlRequest(const std::string& _url) override;

    void queueUITask(AndroidUITask _task);
    void executeUITasks();

    void setMapPtr(jlong _mapPtr) { m_mapPtr = _mapPtr; }
    JavaVM* getJVM() const { return m_jvm; }
    void bindJniEnvToThread(JNIEnv* _jniEnv);

private:

    bool bytesFromAssetManager(const char* _path, std::function<char*(size_t)> _allocator) const;
    std::string fontPath(const std::string& _family, const std::string& _weight, const std::string& _style) const;
    std::string fontFallbackPath(int _importance, int _weightHint) const;

    jlong m_mapPtr;
    jobject m_tangramInstance;
    AAssetManager* m_assetManager;
    JavaVM* m_jvm;
    std::mutex m_UIThreadTaskMutex;
    std::deque<AndroidUITask> m_UITasks;

};

void featurePickCallback(AndroidPlatform* platform, std::shared_ptr<ScopedGlobalRef> listenerRef, const FeaturePickResult* featurePickResult);
void markerPickCallback(AndroidPlatform* platform, std::shared_ptr<ScopedGlobalRef> listenerRef, std::shared_ptr<ScopedGlobalRef> tangramRef, const MarkerPickResult* markerPickResult);
void labelPickCallback(AndroidPlatform* platform, std::shared_ptr<ScopedGlobalRef> listenerRef, const LabelPickResult* labelPickResult);
void sceneUpdateErrorCallback(AndroidPlatform* platform, std::shared_ptr<ScopedGlobalRef> updateStatusCallbackRef, const SceneUpdateError& sceneUpdateErrorStatus);

} // namespace Tangram
