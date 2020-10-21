#include "AndroidMap.h"
#include "AndroidPlatform.h"
#include "JniHelpers.h"
#include "JniThreadBinding.h"
#include "data/properties.h"
#include "data/propertyItem.h"

namespace Tangram {

static jmethodID sceneReadyCallbackMID = nullptr;
static jmethodID cameraAnimationCallbackMID = nullptr;
static jmethodID featurePickCallbackMID = nullptr;
static jmethodID labelPickCallbackMID = nullptr;
static jmethodID markerPickCallbackMID = nullptr;

static jclass hashMapClass = nullptr;
static jmethodID hashMapInitMID = nullptr;
static jmethodID hashMapPutMID = nullptr;

void AndroidMap::jniOnLoad(JavaVM* javaVM, JNIEnv* jniEnv) {
    // JNI OnLoad is invoked once when the native library is loaded so this is a good place to cache
    // any method or class IDs that we'll need.
    jclass mapControllerClass = jniEnv->FindClass("com/mapzen/tangram/MapController");
    sceneReadyCallbackMID = jniEnv->GetMethodID(mapControllerClass, "sceneReadyCallback", "(IILjava/lang/String;Ljava/lang/String;)V");
    cameraAnimationCallbackMID = jniEnv->GetMethodID(mapControllerClass, "cameraAnimationCallback", "(Z)V");
    featurePickCallbackMID = jniEnv->GetMethodID(mapControllerClass, "featurePickCallback", "(Ljava/util/Map;FF)V");
    labelPickCallbackMID = jniEnv->GetMethodID(mapControllerClass, "labelPickCallback", "(Ljava/util/Map;FFIDD)V");
    markerPickCallbackMID = jniEnv->GetMethodID(mapControllerClass, "markerPickCallback", "(JFFDD)V");

    // We need a reference to the class object later to invoke the constructor. FindClass produces a
    // local reference that may not be valid later, so create a global reference to the class.
    hashMapClass = (jclass)jniEnv->NewGlobalRef(jniEnv->FindClass("java/util/HashMap"));
    hashMapInitMID = jniEnv->GetMethodID(hashMapClass, "<init>", "()V");
    hashMapPutMID = jniEnv->GetMethodID(hashMapClass, "put", "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");
}

void AndroidMap::jniOnUnload(JavaVM* javaVM, JNIEnv* jniEnv) {
    jniEnv->DeleteGlobalRef(hashMapClass);
    hashMapClass = nullptr;
}

AndroidMap::AndroidMap(JNIEnv* env, jobject mapController, jobject assetManager)
        : Map(std::make_unique<AndroidPlatform>(env, mapController, assetManager)) {

    m_mapController = env->NewWeakGlobalRef(mapController);

    setSceneReadyListener([this](SceneID sceneId, const SceneError* sceneError) {
        JniThreadBinding jniEnv(JniHelpers::getJVM());

        jint jErrorType = -1;
        jstring jUpdatePath = nullptr;
        jstring jUpdateValue = nullptr;

        if (sceneError) {
            jUpdatePath = JniHelpers::javaStringFromString(jniEnv, sceneError->update.path);
            jUpdateValue = JniHelpers::javaStringFromString(jniEnv, sceneError->update.value);
            jErrorType = (jint) sceneError->error;
        }

        jniEnv->CallVoidMethod(m_mapController, sceneReadyCallbackMID, sceneId, jErrorType,
                               jUpdatePath, jUpdateValue);
    });

    setCameraAnimationListener([this](bool finished) {
        JniThreadBinding jniEnv(JniHelpers::getJVM());

        jniEnv->CallVoidMethod(m_mapController, cameraAnimationCallbackMID, finished);
    });
}

void AndroidMap::pickFeature(float posX, float posY) {

    pickFeatureAt(posX, posY, [this](const FeaturePickResult* featurePickResult) {
        JniThreadBinding jniEnv(JniHelpers::getJVM());

        float x = 0.f, y = 0.f;
        jobject hashMap = nullptr;

        if (featurePickResult) {
            x = featurePickResult->position[0];
            y = featurePickResult->position[1];

            hashMap = jniEnv->NewObject(hashMapClass, hashMapInitMID);
            const auto& properties = featurePickResult->properties;
            for (const auto& item : properties->items()) {
                jstring jkey = JniHelpers::javaStringFromString(jniEnv, item.key);
                jstring jvalue = JniHelpers::javaStringFromString(jniEnv, properties->asString(item.value));
                jniEnv->CallObjectMethod(hashMap, hashMapPutMID, jkey, jvalue);
            }
        }
        jniEnv->CallVoidMethod(m_mapController, featurePickCallbackMID, hashMap, x, y);
    });
}

void AndroidMap::pickLabel(float posX, float posY) {

    pickLabelAt(posX, posY, [this](const LabelPickResult* labelPickResult) {
        JniThreadBinding jniEnv(JniHelpers::getJVM());

        float x = 0.f, y = 0.f;
        double lng = 0., lat = 0.;
        int type = 0;
        jobject hashmap = nullptr;

        if (labelPickResult) {
            x = labelPickResult->touchItem.position[0];
            y = labelPickResult->touchItem.position[1];
            lng = labelPickResult->coordinates.longitude;
            lat = labelPickResult->coordinates.latitude;
            type = labelPickResult->type;

            hashmap = jniEnv->NewObject(hashMapClass, hashMapInitMID);
            const auto& properties = labelPickResult->touchItem.properties;
            for (const auto& item : properties->items()) {
                jstring jkey = JniHelpers::javaStringFromString(jniEnv, item.key);
                jstring jvalue = JniHelpers::javaStringFromString(jniEnv, properties->asString(item.value));
                jniEnv->CallObjectMethod(hashmap, hashMapPutMID, jkey, jvalue);
            }
        }
        jniEnv->CallVoidMethod(m_mapController, labelPickCallbackMID, hashmap, x, y, type, lng,
                               lat);
    });
}

void AndroidMap::pickMarker(float posX, float posY) {

    pickMarkerAt(posX, posY, [this](const MarkerPickResult* markerPickResult) {
        JniThreadBinding jniEnv(JniHelpers::getJVM());

        float x = 0.f, y = 0.f;
        double lng = 0., lat = 0.;
        jlong markerID = 0;

        if (markerPickResult) {
            x = markerPickResult->position[0];
            y = markerPickResult->position[1];
            lng = markerPickResult->coordinates.longitude;
            lat = markerPickResult->coordinates.latitude;
            markerID = markerPickResult->id;
        }

        jniEnv->CallVoidMethod(m_mapController, markerPickCallbackMID, markerID, x, y, lng, lat);
    });
}

AndroidPlatform& AndroidMap::androidPlatform() {
    return static_cast<AndroidPlatform&>(getPlatform());
}

} // namespace Tangram
