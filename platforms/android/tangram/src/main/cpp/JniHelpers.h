#pragma once

#include <jni.h>
#include <string>

namespace Tangram {

class JniHelpers {

public:

    static void jniOnLoad(JavaVM* jvm) { s_jvm = jvm; }
    static JavaVM* getJVM() { return s_jvm; }

    static std::string stringFromJavaString(JNIEnv* jniEnv, jstring javaString);
    static jstring javaStringFromString(JNIEnv* jniEnv, const std::string& string);

protected:

    static JavaVM* s_jvm;
};

} // namespace Tangram
