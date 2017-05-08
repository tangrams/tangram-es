#pragma once

#include "platform.h"

#include <memory>
#include <jni.h>
#include <mutex>
#include <deque>
#include <android/asset_manager.h>

void bindJniEnvToThread(JNIEnv* jniEnv);
void setupJniEnv(JNIEnv* _jniEnv);
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

    AndroidPlatform(JNIEnv* _jniEnv, jobject _assetManager, jobject _tangramInstance);
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

private:

    bool bytesFromAssetManager(const char* _path, std::function<char*(size_t)> _allocator) const;
    std::string fontPath(const std::string& _family, const std::string& _weight, const std::string& _style) const;
    std::string fontFallbackPath(int _importance, int _weightHint) const;

    jlong m_mapPtr;
    jobject m_tangramInstance;
    AAssetManager* m_assetManager;
    std::mutex m_UIThreadTaskMutex;
    std::deque<AndroidUITask> m_UITasks;

};

void featurePickCallback(AndroidPlatform& platform, jobject listenerRef, const FeaturePickResult* featurePickResult);
void markerPickCallback(AndroidPlatform& platform, jobject listenerRef, jobject tangramRef, const MarkerPickResult* markerPickResult);
void labelPickCallback(AndroidPlatform& platform, jobject listenerRef, const LabelPickResult* labelPickResult);
void sceneUpdateErrorCallback(AndroidPlatform& platform, jobject updateCallbackRef, const SceneUpdateError& sceneUpdateError);

} // namespace Tangram
