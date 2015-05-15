#include <jni.h>
#include "tangram.h"
// Includes platform.h for setAssetManager reference
#include "platform.h"

extern "C" {
    JNIEXPORT void JNICALL Java_com_mapzen_tangram_Tangram_init(JNIEnv* jniEnv, jobject obj, jobject tangramInstance, jobject assetManager) {
        setupJniEnv(jniEnv, tangramInstance, assetManager);
        Tangram::initialize();
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_Tangram_resize(JNIEnv* jniEnv, jobject obj, jint width, jint height) {
        Tangram::resize(width, height);
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_Tangram_update(JNIEnv* jniEnv, jobject obj, jfloat dt) {
        Tangram::update(dt);
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_Tangram_render(JNIEnv* jniEnv, jobject obj) {
        Tangram::render();
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_Tangram_teardown(JNIEnv* jniEnv, jobject obj) {
        Tangram::teardown();
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_Tangram_onContextDestroyed(JNIEnv* jniEnv, jobject obj) {
        Tangram::onContextDestroyed();
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_Tangram_setPixelScale(JNIEnv* jniEnv, jobject obj, jfloat scale) {
        Tangram::setPixelScale(scale);
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_Tangram_handleTapGesture(JNIEnv* jniEnv, jobject obj, jfloat posX, jfloat posY) {
        Tangram::handleTapGesture(posX, posY);
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_Tangram_handleDoubleTapGesture(JNIEnv* jniEnv, jobject obj, jfloat posX, jfloat posY) {
        Tangram::handleDoubleTapGesture(posX, posY);
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_Tangram_handlePanGesture(JNIEnv* jniEnv, jobject obj, jfloat startX, jfloat startY, jfloat endX, jfloat endY) {
        Tangram::handlePanGesture(startX, startY, endX, endY);
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_Tangram_handlePinchGesture(JNIEnv* jniEnv, jobject obj, jfloat posX, jfloat posY, jfloat scale) {
        Tangram::handlePinchGesture(posX, posY, scale);
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_Tangram_handleRotateGesture(JNIEnv* jniEnv, jobject obj, jfloat posX, jfloat posY, jfloat rotation) {
        Tangram::handleRotateGesture(posX, posY, rotation);
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_Tangram_handleShoveGesture(JNIEnv* jniEnv, jobject obj, jfloat distance) {
        Tangram::handleShoveGesture(distance);
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_Tangram_networkDataBridge(JNIEnv* jniEnv, jobject obj, jbyteArray jFetchedBytes, jint tileIDx, jint tileIDy, jint tileIDz, jint dataSourceID) {
        networkDataBridge(jniEnv, jFetchedBytes, tileIDx, tileIDy, tileIDz, dataSourceID);
    }

}

