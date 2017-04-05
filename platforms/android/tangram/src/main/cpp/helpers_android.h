#pragma once

#include <jni.h>

class JniThreadBinding {
private:
    JavaVM* jvm;
    JNIEnv *jniEnv;
    int status;
public:
    JniThreadBinding(JavaVM* _jvm) : jvm(_jvm) {
        status = jvm->GetEnv((void**)&jniEnv, JNI_VERSION_1_6);
        if (status == JNI_EDETACHED) { jvm->AttachCurrentThread(&jniEnv, NULL);}
    }

    ~JniThreadBinding() {
        if (status == JNI_EDETACHED) { jvm->DetachCurrentThread(); }
    }

    JNIEnv* operator->() const {
        return jniEnv;
    }

    operator JNIEnv*() const {
        return jniEnv;
    }
};

class ScopedGlobalRef {
private:
    JavaVM* jvm;
    jobject globalRef;
public:
    ScopedGlobalRef(JavaVM* jvm, JNIEnv* jniEnv, jobject jniObject) : jvm(jvm) {
        globalRef = jniEnv->NewGlobalRef(jniObject);
    }

    ~ScopedGlobalRef() {
        JniThreadBinding jniEnv(jvm);
        jniEnv->DeleteGlobalRef(globalRef);
    }

    template<class T>
    T get() const {
        return static_cast<T>(globalRef);
    }
};
