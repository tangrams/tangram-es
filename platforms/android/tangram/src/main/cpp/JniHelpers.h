#pragma once

#include <jni.h>
#include <string>

namespace Tangram {

struct CameraPosition;

class JniHelpers {

public:

    static void jniOnLoad(JavaVM* jvm, JNIEnv* jniEnv);
    static JavaVM* getJVM() { return s_jvm; }

    static std::string stringFromJavaString(JNIEnv* jniEnv, jstring javaString);
    static jstring javaStringFromString(JNIEnv* jniEnv, const std::string& string);

    static void cameraPositionToJava(JNIEnv* env, jobject javaCamera, const CameraPosition& camera);

protected:

    static JavaVM* s_jvm;
};

} // namespace Tangram
