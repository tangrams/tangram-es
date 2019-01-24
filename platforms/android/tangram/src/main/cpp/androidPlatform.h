#pragma once

#include "map.h"
#include "platform.h"
#include "jniWorker.h"
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
    void shutdown() override;
    void requestRender() const override;
    void setContinuousRendering(bool _isContinuous) override;
    FontSourceHandle systemFont(const std::string& _name, const std::string& _weight, const std::string& _face) const override;
    std::vector<FontSourceHandle> systemFontFallbacksHandle() const override;
    bool startUrlRequestImpl(const Url& _url, const UrlRequestHandle _request, UrlRequestId& _id) override;
    void cancelUrlRequestImpl(const UrlRequestId _id) override;

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

    mutable JniWorker m_jniWorker;
    AsyncWorker m_fileWorker;
};

class AndroidMap : public Map {
public:
    AndroidMap(JNIEnv* _jniEnv, jobject _assetManager, jobject _tangramInstance);
    void pickFeature(float posX, float posY);
    void pickMarker(float posX, float posY);
    void pickLabel(float posX, float posY);


    jobject m_tangramInstance;
};


} // namespace Tangram
