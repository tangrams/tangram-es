#ifdef PLATFORM_ANDROID

#include "platform.h"

#include <android/log.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <jni.h>
#include <cstdarg>

/* Followed the following document for JavaVM tips when used with native threads
 * http://android.wooyd.org/JNIExample/#NWD1sCYeT-I
 * http://developer.android.com/training/articles/perf-jni.html and
 * http://www.ibm.com/developerworks/library/j-jni/
 * http://docs.oracle.com/javase/7/docs/technotes/guides/jni/spec/invocation.html
 */

static AAssetManager* assetManager;
static JavaVM* jvm; //store the only jvm instance for android to be used to attach JNIEnv for specific threads

/* We will also cache the class instance since JavaVM is not happy when we try to do a findClass from within a native thread.
 * So we will store an instance of the class (jobject) and use it to find the class itself (jclass).
 * NOTE: this will change when we have async OKHTTP, which will only require a single instance of the OkHttpInterface
 */
const char* okHttpInterfacePath = "com/mapzen/tangram/OkHttpInterface";
static jobject okHttpInterfaceObj;

void cacheJavaVM(JNIEnv* _jniEnv) {
    _jniEnv->GetJavaVM(&jvm);
}

void cacheClassInstance(JNIEnv* _jniEnv) {
    jclass cls = _jniEnv->FindClass(okHttpInterfacePath);
    if(!cls) {
        logMsg("failed to get %s class reference.\n", okHttpInterfacePath);
        return;
    }
    jmethodID constr = _jniEnv->GetMethodID(cls, "<init>", "()V");
    if(!constr) {
        logMsg("failed to get %s constructor.\n", okHttpInterfacePath);
        return;
    }
    jobject obj = _jniEnv->NewObject(cls, constr);
    if(!obj) {
        logMsg("failed to create a %s object.\n", okHttpInterfacePath);
        return;
    }
    okHttpInterfaceObj = _jniEnv->NewGlobalRef(obj); //Make sure this is a global reference, to avoid this being garbase collected during the life cycle of the app
}

void deleteClassInstance(JNIEnv* _jniEnv) {
    _jniEnv->DeleteGlobalRef(okHttpInterfaceObj);
}

void setAssetManager(JNIEnv* _jniEnv, jobject _assetManager) {

    assetManager = AAssetManager_fromJava(_jniEnv, _assetManager);

    if (assetManager == nullptr) {
        logMsg("ERROR: Could not obtain Asset Manager reference\n");
    }

}

void logMsg(const char* fmt, ...) {

    va_list args;
    va_start(args, fmt);
    __android_log_vprint(ANDROID_LOG_DEBUG, "Tangram", fmt, args);
    va_end(args);

}

std::string stringFromResource(const char* _path) {

    std::string out;
    
    // Open asset
    AAsset* asset = AAssetManager_open(assetManager, _path, AASSET_MODE_STREAMING);
    
    if (asset == nullptr) {
        logMsg("Failed to open asset at path: %s\n", _path);
        return out;
    }

    // Allocate string
    int length = AAsset_getLength(asset);
    out.resize(length);
    
    // Read data
    int read = AAsset_read(asset, &out.front(), length);

    // Clean up
    AAsset_close(asset);

    if (read <= 0) {
        logMsg("Failed to read asset at path: %s\n", _path);
    }

    return out;

}

unsigned char* bytesFromResource(const char* _path, unsigned int* _size) {

    unsigned char* data = nullptr;

    AAsset* asset = AAssetManager_open(assetManager, _path, AASSET_MODE_UNKNOWN);

    if (asset == nullptr) {
        logMsg("Failed to open asset at path: %s\n", _path);
        *_size = 0;
        return data;
    }

    *_size = AAsset_getLength(asset);

    data = (unsigned char*) malloc(sizeof(unsigned char) * (*_size));

    int read = AAsset_read(asset, data, *_size);

    if (read <= 0) {
        logMsg("Failed to read asset at path: %s\n", _path);
    }

    AAsset_close(asset);

    return data;
}

bool streamFromHttpSync(const std::string& _url, std::stringstream& _rawData) {
    int status;
    JNIEnv *jniEnv;
    bool isAttached = false;
    jstring jUrl;
    jboolean methodResult;
    jbyteArray jfetchedBytes;
    std::string resultStr;


    /*
     * Get JNI Environment, or link new jniEnv with this native thread
     */
    status = jvm->GetEnv(reinterpret_cast<void**>(&jniEnv), JNI_VERSION_1_6);
    if(status == JNI_EVERSION) {
        logMsg("JNI_VERSION_1_6 not supported. Can not get JNI Environment.\n");
        return false;
    }
    if(status == JNI_EDETACHED) {
        logMsg("Failed to get JNI environment, assuming native thread and calling AttachCurrentThread to get JNIEnv");
        status = jvm->AttachCurrentThread(&jniEnv, NULL);
        if(status < 0) {
            logMsg("Failed to attach current thread.\n");
            return false;
        }
        isAttached = true;
    }

    /*
     * Get the interface class from the cached global object
     */
    jclass interfaceClass = jniEnv->GetObjectClass(okHttpInterfaceObj);
    if(!interfaceClass) {
        logMsg("Failed to get %s class reference from object.", okHttpInterfacePath);
        if(isAttached) {
            jvm->DetachCurrentThread();
        }
        return false;
    }

    /*
     * Get the main run methodID
     */
    jmethodID method = jniEnv->GetMethodID(interfaceClass, "run", "(Ljava/lang/String;)Z");
    if(!method) {
        logMsg("Failed to get \"run\" method ID.\n");
        if(isAttached) {
            jvm->DetachCurrentThread();
        }
        return false;
    }

    /*
     * Get the method id for the constructor to create a local object for the interfaceclass.
     */
    jmethodID constr = jniEnv->GetMethodID(interfaceClass, "<init>", "()V");
    if(!constr) {
        logMsg("failed to get %s constructor.\n", okHttpInterfacePath);
        return false;
    }
    jobject obj = jniEnv->NewObject(interfaceClass, constr);
    if(!obj) {
        logMsg("failed to create a %s object.\n", okHttpInterfacePath);
        if(isAttached) {
            jvm->DetachCurrentThread();
        }
        return false;
    }

    /* 
     * construct jstring for url
     */
    jUrl = jniEnv->NewStringUTF(_url.c_str());

    methodResult = jniEnv->CallBooleanMethod(obj, method, jUrl);

    if(!methodResult) {
        logMsg("run method returns false\n");
        if(isAttached) {
            jvm->DetachCurrentThread();
        }
        return false;
    }

    /* 
     * Get the appropriate class object field id
     */
    jfieldID fieldID = jniEnv->GetFieldID(interfaceClass, "rawDataBytes" , "[B");

    if(!fieldID) {
        logMsg("Failed getting rawDataOut fieldID\n");
        if(isAttached) {
            jvm->DetachCurrentThread();
        }
    }

    // grab byteArray field
    jfetchedBytes = (jbyteArray) jniEnv->GetObjectField(obj, fieldID);
    int length = jniEnv->GetArrayLength(jfetchedBytes);
    jbyte* const byte_array_start = jniEnv->GetByteArrayElements(jfetchedBytes, 0);
    const char* byteCVal = (const char*)byte_array_start;
    resultStr = std::string(byteCVal, length);
    jniEnv->ReleaseByteArrayElements(jfetchedBytes, byte_array_start, 0);
    _rawData << resultStr;

    if(isAttached) {
        jvm->DetachCurrentThread();
    }

    logMsg("Final Length: %d\n", resultStr.length());

    return true;
}



#endif
