#pragma once

#include "map.h"

#include <jni.h>
#include <string>

namespace Tangram {

class JniHelpers {

public:

    static void jniOnLoad(JavaVM* jvm, JNIEnv* jniEnv);
    static JavaVM* getJVM() { return s_jvm; }

    static std::string stringFromJavaString(JNIEnv* jniEnv, jstring javaString);
    static jstring javaStringFromString(JNIEnv* jniEnv, const std::string& string);

    static void cameraPositionToJava(JNIEnv* env, jobject javaCamera, const CameraPosition& camera);

    static void vec2ToJava(JNIEnv* env, jobject javaPointF, float x, float y);

    static void lngLatToJava(JNIEnv* env, jobject javaLngLat, const LngLat& lngLat);

    static LngLat lngLatFromJava(JNIEnv* env, jobject javaLngLat);

    static EdgePadding edgePaddingFromJava(JNIEnv* env, jobject javaEdgePadding);

    static void edgePaddingToJava(JNIEnv* env, jobject javaEdgePadding, const EdgePadding& padding);

protected:

    static JavaVM* s_jvm;
};

} // namespace Tangram
