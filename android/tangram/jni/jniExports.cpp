#include "tangram.h"
#include "platform_android.h"

extern "C" {

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_MapController_setPosition(JNIEnv* jniEnv, jobject obj, jdouble lon, jdouble lat) {
        Tangram::setPosition(lon, lat);
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_MapController_setPositionEased(JNIEnv* jniEnv, jobject obj, jdouble lon, jdouble lat, jfloat duration, jint ease) {
        Tangram::setPosition(lon, lat, duration, static_cast<Tangram::EaseType>(ease));
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_MapController_getPosition(JNIEnv* jniEnv, jobject obj, jdoubleArray lonLat) {
        jdouble* arr = jniEnv->GetDoubleArrayElements(lonLat, NULL);
        Tangram::getPosition(arr[0], arr[1]);
        jniEnv->ReleaseDoubleArrayElements(lonLat, arr, 0);
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_MapController_setZoom(JNIEnv* jniEnv, jobject obj, jfloat zoom) {
        Tangram::setZoom(zoom);
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_MapController_setZoomEased(JNIEnv* jniEnv, jobject obj, jfloat zoom, jfloat duration, jint ease) {
        Tangram::setZoom(zoom, duration, static_cast<Tangram::EaseType>(ease));
    }

    JNIEXPORT jfloat JNICALL Java_com_mapzen_tangram_MapController_getZoom(JNIEnv* jniEnv, jobject obj) {
        return Tangram::getZoom();
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_MapController_setRotation(JNIEnv* jniEnv, jobject obj, jfloat radians) {
        Tangram::setRotation(radians);
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_MapController_setRotationEased(JNIEnv* jniEnv, jobject obj, jfloat radians, jfloat duration, jint ease) {
        Tangram::setRotation(radians, duration, static_cast<Tangram::EaseType>(ease));
    }

    JNIEXPORT jfloat JNICALL Java_com_mapzen_tangram_MapController_getRotation(JNIEnv* jniEnv, jobject obj) {
        return Tangram::getRotation();
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_MapController_setTilt(JNIEnv* jniEnv, jobject obj, jfloat radians) {
        Tangram::setTilt(radians);
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_MapController_setTiltEased(JNIEnv* jniEnv, jobject obj, jfloat radians, jfloat duration, jint ease) {
        Tangram::setTilt(radians, duration, static_cast<Tangram::EaseType>(ease));
    }

    JNIEXPORT jfloat JNICALL Java_com_mapzen_tangram_MapController_getTilt(JNIEnv* jniEnv, jobject obj) {
        return Tangram::getTilt();
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_MapController_screenToWorldCoordinates(JNIEnv* jniEnv, jobject obj, jdoubleArray screenPos) {
        jdouble* arr = jniEnv->GetDoubleArrayElements(screenPos, NULL);
        Tangram::screenToWorldCoordinates(arr[0], arr[1]);
        jniEnv->ReleaseDoubleArrayElements(screenPos, arr, 0);
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_MapController_init(JNIEnv* jniEnv, jobject obj, jobject tangramInstance, jobject assetManager, jstring stylePath, jfloat scale) {
        setupJniEnv(jniEnv, tangramInstance, assetManager);
        const char* cStylePath = jniEnv->GetStringUTFChars(stylePath, NULL);
        Tangram::initialize(cStylePath, scale);
        jniEnv->ReleaseStringUTFChars(stylePath, cStylePath);
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_MapController_loadScene(JNIEnv* jniEnv, jobject obj, jstring path) {
        const char* cPath = jniEnv->GetStringUTFChars(path, NULL);
        Tangram::loadScene(cPath);
        jniEnv->ReleaseStringUTFChars(path, cPath);
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_MapController_resize(JNIEnv* jniEnv, jobject obj, jint width, jint height) {
        Tangram::resize(width, height);
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_MapController_update(JNIEnv* jniEnv, jobject obj, jfloat dt) {
        Tangram::update(dt);
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_MapController_render(JNIEnv* jniEnv, jobject obj) {
        Tangram::render();
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_MapController_setupGL(JNIEnv* jniEnv, jobject obj) {
        Tangram::setupGL();
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_MapController_handleTapGesture(JNIEnv* jniEnv, jobject obj, jfloat posX, jfloat posY) {
        Tangram::handleTapGesture(posX, posY);
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_MapController_handleDoubleTapGesture(JNIEnv* jniEnv, jobject obj, jfloat posX, jfloat posY) {
        Tangram::handleDoubleTapGesture(posX, posY);
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_MapController_handlePanGesture(JNIEnv* jniEnv, jobject obj, jfloat startX, jfloat startY, jfloat endX, jfloat endY) {
        Tangram::handlePanGesture(startX, startY, endX, endY);
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_MapController_handleFlingGesture(JNIEnv* jniEnv, jobject obj, jfloat posX, jfloat posY, jfloat velocityX, jfloat velocityY) {
        Tangram::handleFlingGesture(posX, posY, velocityX, velocityY);
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_MapController_handlePinchGesture(JNIEnv* jniEnv, jobject obj, jfloat posX, jfloat posY, jfloat scale, jfloat velocity) {
        Tangram::handlePinchGesture(posX, posY, scale, velocity);
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_MapController_handleRotateGesture(JNIEnv* jniEnv, jobject obj, jfloat posX, jfloat posY, jfloat rotation) {
        Tangram::handleRotateGesture(posX, posY, rotation);
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_MapController_handleShoveGesture(JNIEnv* jniEnv, jobject obj, jfloat distance) {
        Tangram::handleShoveGesture(distance);
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_MapController_onUrlSuccess(JNIEnv* jniEnv, jobject obj, jbyteArray fetchedBytes, jlong callbackPtr) {
        onUrlSuccess(jniEnv, fetchedBytes, callbackPtr);
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_MapController_onUrlFailure(JNIEnv* jniEnv, jobject obj, jlong callbackPtr) {
        onUrlFailure(jniEnv, callbackPtr);
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_MapController_pickFeature(JNIEnv* jniEnv, jobject obj, jfloat posX, jfloat posY) {
        auto& items = Tangram::pickFeaturesAt(posX, posY);
        if (!items.empty()) {
            featureSelectionCallback(jniEnv, items);
        }
    }

}

