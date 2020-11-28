#include "AndroidMap.h"
#include "AndroidPlatform.h"
#include "JniHelpers.h"

#define TANGRAM_JNI_VERSION JNI_VERSION_1_6

extern "C" JNIEXPORT jint JNI_OnLoad(JavaVM* javaVM, void*) {
    JNIEnv* jniEnv = nullptr;
    if (javaVM->GetEnv(reinterpret_cast<void**>(&jniEnv), TANGRAM_JNI_VERSION) != JNI_OK) {
        return -1;
    }

    Tangram::JniHelpers::jniOnLoad(javaVM, jniEnv);
    Tangram::AndroidPlatform::jniOnLoad(javaVM, jniEnv);
    Tangram::AndroidMap::jniOnLoad(javaVM, jniEnv);

    return TANGRAM_JNI_VERSION;
}

extern "C" JNIEXPORT void JNI_OnUnload(JavaVM* javaVM, void*) {
    JNIEnv* jniEnv = nullptr;
    if (javaVM->GetEnv(reinterpret_cast<void**>(&jniEnv), TANGRAM_JNI_VERSION) != JNI_OK) {
        return;
    }

    Tangram::AndroidMap::jniOnUnload(javaVM, jniEnv);
}
