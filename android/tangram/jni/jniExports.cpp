#include <jni.h>
#include "tangram.h"
#include "platform.h"

extern "C" {

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_MapController_setPosition(JNIEnv* jniEnv, jobject obj, jdouble lon, jdouble lat) {
        Tangram::setPosition(lon, lat);
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_MapController_getPosition(JNIEnv* jniEnv, jobject obj, jdoubleArray lonLat) {
        jdouble* arr = jniEnv->GetDoubleArrayElements(lonLat, NULL);
        Tangram::getPosition(arr[0], arr[1]);
        jniEnv->ReleaseDoubleArrayElements(lonLat, arr, 0);
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_MapController_setZoom(JNIEnv* jniEnv, jobject obj, jfloat zoom) {
        Tangram::setZoom(zoom);
    }

    JNIEXPORT jfloat JNICALL Java_com_mapzen_tangram_MapController_getZoom(JNIEnv* jniEnv, jobject obj) {
        return Tangram::getZoom();
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_MapController_setRotation(JNIEnv* jniEnv, jobject obj, jfloat radians) {
        Tangram::setRotation(radians);
    }

    JNIEXPORT jfloat JNICALL Java_com_mapzen_tangram_MapController_getRotation(JNIEnv* jniEnv, jobject obj) {
        return Tangram::getRotation();
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_MapController_setTilt(JNIEnv* jniEnv, jobject obj, jfloat radians) {
        Tangram::setTilt(radians);
    }

    JNIEXPORT jfloat JNICALL Java_com_mapzen_tangram_MapController_getTilt(JNIEnv* jniEnv, jobject obj) {
        return Tangram::getTilt();
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_MapController_screenToWorldCoordinates(JNIEnv* jniEnv, jobject obj, jdoubleArray screenPos) {
        jdouble* arr = jniEnv->GetDoubleArrayElements(screenPos, NULL);
        Tangram::screenToWorldCoordinates(arr[0], arr[1]);
        jniEnv->ReleaseDoubleArrayElements(screenPos, arr, 0);
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_MapController_init(JNIEnv* jniEnv, jobject obj, jobject tangramInstance, jobject assetManager, jstring stylePath) {
        setupJniEnv(jniEnv, tangramInstance, assetManager);
        const char* cStylePath = jniEnv->GetStringUTFChars(stylePath, NULL);
        Tangram::initialize(cStylePath);
        jniEnv->ReleaseStringUTFChars(stylePath, cStylePath);
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

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_MapController_setPixelScale(JNIEnv* jniEnv, jobject obj, jfloat scale) {
        Tangram::setPixelScale(scale);
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

    JNIEXPORT int JNICALL Java_com_mapzen_tangram_MapData_addDataSource(JNIEnv* jniEnv, jobject obj, jstring name) {
        const char* cname = jniEnv->GetStringUTFChars(name, NULL);
        int id = Tangram::addDataSource(cname);
        jniEnv->ReleaseStringUTFChars(name, cname);
        return id;
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_MapData_clearSourceData(JNIEnv* jniEnv, jobject obj, jint id) {
        Tangram::clearSourceData(id);
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_MapData_addSourceGeoJSON(JNIEnv* jniEnv, jobject obj, jint id, jstring name) {
        const char* cstring = jniEnv->GetStringUTFChars(name, NULL);
        Tangram::addSourceGeoJSON(id, cstring);
        jniEnv->ReleaseStringUTFChars(name, NULL);
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_MapData_addSourcePoint(JNIEnv* jniEnv, jobject obj, jint id, jdoubleArray coords) {
        double* coord_ptr = jniEnv->GetDoubleArrayElements(coords, NULL);
        Tangram::addSourcePoint(id, coord_ptr);
        jniEnv->ReleaseDoubleArrayElements(coords, coord_ptr, 0);
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_MapData_addSourceLine(JNIEnv* jniEnv, jobject obj, jint id, jdoubleArray coords, jint length) {
        double* coord_ptr = jniEnv->GetDoubleArrayElements(coords, NULL);
        Tangram::addSourceLine(id, coord_ptr, length);
        jniEnv->ReleaseDoubleArrayElements(coords, coord_ptr, 0);
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_MapData_addSourcePoly(JNIEnv* jniEnv, jobject obj, jint id, jdoubleArray coords, jintArray ringLengths, jint rings) {
        double* coord_ptr = jniEnv->GetDoubleArrayElements(coords, NULL);
        int* ring_length_ptr = jniEnv->GetIntArrayElements(ringLengths, NULL);
        Tangram::addSourcePoly(id, coord_ptr, ring_length_ptr, rings);
        jniEnv->ReleaseDoubleArrayElements(coords, coord_ptr, 0);
        jniEnv->ReleaseIntArrayElements(ringLengths, ring_length_ptr, 0);
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_MapController_pickFeature(JNIEnv* jniEnv, jobject obj, jfloat posX, jfloat posY) {
        auto& items = Tangram::pickFeaturesAt(posX, posY);
        if (!items.empty()) {
            featureSelectionCallback(items);
        }
    }

}

