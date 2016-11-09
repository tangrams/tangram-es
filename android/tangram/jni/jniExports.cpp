#include "tangram.h"
#include "platform_android.h"
#include "data/clientGeoJsonSource.h"

#include <cassert>

extern "C" {

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_MapController_nativeSetPosition(JNIEnv* jniEnv, jobject obj, jlong mapPtr, jdouble lon, jdouble lat) {
        assert(mapPtr > 0);
        auto map = reinterpret_cast<Tangram::Map*>(mapPtr);
        map->setPosition(lon, lat);
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_MapController_nativeSetPositionEased(JNIEnv* jniEnv, jobject obj, jlong mapPtr, jdouble lon, jdouble lat, jfloat duration, jint ease) {
        assert(mapPtr > 0);
        auto map = reinterpret_cast<Tangram::Map*>(mapPtr);
        map->setPositionEased(lon, lat, duration, static_cast<Tangram::EaseType>(ease));
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_MapController_nativeGetPosition(JNIEnv* jniEnv, jobject obj, jlong mapPtr, jdoubleArray lonLat) {
        assert(mapPtr > 0);
        auto map = reinterpret_cast<Tangram::Map*>(mapPtr);
        jdouble* arr = jniEnv->GetDoubleArrayElements(lonLat, NULL);
        map->getPosition(arr[0], arr[1]);
        jniEnv->ReleaseDoubleArrayElements(lonLat, arr, 0);
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_MapController_nativeSetZoom(JNIEnv* jniEnv, jobject obj, jlong mapPtr, jfloat zoom) {
        assert(mapPtr > 0);
        auto map = reinterpret_cast<Tangram::Map*>(mapPtr);
        map->setZoom(zoom);
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_MapController_nativeSetZoomEased(JNIEnv* jniEnv, jobject obj, jlong mapPtr, jfloat zoom, jfloat duration, jint ease) {
        assert(mapPtr > 0);
        auto map = reinterpret_cast<Tangram::Map*>(mapPtr);
        map->setZoomEased(zoom, duration, static_cast<Tangram::EaseType>(ease));
    }

    JNIEXPORT jfloat JNICALL Java_com_mapzen_tangram_MapController_nativeGetZoom(JNIEnv* jniEnv, jobject obj, jlong mapPtr) {
        assert(mapPtr > 0);
        auto map = reinterpret_cast<Tangram::Map*>(mapPtr);
        return map->getZoom();
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_MapController_nativeSetRotation(JNIEnv* jniEnv, jobject obj, jlong mapPtr, jfloat radians) {
        assert(mapPtr > 0);
        auto map = reinterpret_cast<Tangram::Map*>(mapPtr);
        map->setRotation(radians);
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_MapController_nativeSetRotationEased(JNIEnv* jniEnv, jobject obj, jlong mapPtr, jfloat radians, jfloat duration, jint ease) {
        assert(mapPtr > 0);
        auto map = reinterpret_cast<Tangram::Map*>(mapPtr);
        map->setRotationEased(radians, duration, static_cast<Tangram::EaseType>(ease));
    }

    JNIEXPORT jfloat JNICALL Java_com_mapzen_tangram_MapController_nativeGetRotation(JNIEnv* jniEnv, jobject obj, jlong mapPtr) {
        assert(mapPtr > 0);
        auto map = reinterpret_cast<Tangram::Map*>(mapPtr);
        return map->getRotation();
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_MapController_nativeSetTilt(JNIEnv* jniEnv, jobject obj, jlong mapPtr, jfloat radians) {
        assert(mapPtr > 0);
        auto map = reinterpret_cast<Tangram::Map*>(mapPtr);
        map->setTilt(radians);
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_MapController_nativeSetTiltEased(JNIEnv* jniEnv, jobject obj,  jlong mapPtr, jfloat radians, jfloat duration, jint ease) {
        assert(mapPtr > 0);
        auto map = reinterpret_cast<Tangram::Map*>(mapPtr);
        map->setTiltEased(radians, duration, static_cast<Tangram::EaseType>(ease));
    }

    JNIEXPORT jfloat JNICALL Java_com_mapzen_tangram_MapController_nativeGetTilt(JNIEnv* jniEnv, jobject obj, jlong mapPtr) {
        assert(mapPtr > 0);
        auto map = reinterpret_cast<Tangram::Map*>(mapPtr);
        return map->getTilt();
    }

    JNIEXPORT jboolean JNICALL Java_com_mapzen_tangram_MapController_nativeScreenPositionToLngLat(JNIEnv* jniEnv, jobject obj, jlong mapPtr, jdoubleArray coordinates) {
        assert(mapPtr > 0);
        auto map = reinterpret_cast<Tangram::Map*>(mapPtr);
        jdouble* arr = jniEnv->GetDoubleArrayElements(coordinates, NULL);
        bool ret = map->screenPositionToLngLat(arr[0], arr[1], &arr[0], &arr[1]);
        jniEnv->ReleaseDoubleArrayElements(coordinates, arr, 0);
        return ret;
    }

    JNIEXPORT jboolean JNICALL Java_com_mapzen_tangram_MapController_nativeLngLatToScreenPosition(JNIEnv* jniEnv, jobject obj, jlong mapPtr, jdoubleArray coordinates) {
        assert(mapPtr > 0);
        auto map = reinterpret_cast<Tangram::Map*>(mapPtr);
        jdouble* arr = jniEnv->GetDoubleArrayElements(coordinates, NULL);
        bool ret = map->lngLatToScreenPosition(arr[0], arr[1], &arr[0], &arr[1]);
        jniEnv->ReleaseDoubleArrayElements(coordinates, arr, 0);
        return ret;
    }

    JNIEXPORT jlong JNICALL Java_com_mapzen_tangram_MapController_nativeInit(JNIEnv* jniEnv, jobject obj, jobject tangramInstance, jobject assetManager) {
        setupJniEnv(jniEnv, tangramInstance, assetManager);
        auto map = new Tangram::Map();
        return reinterpret_cast<jlong>(map);
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_MapController_nativeDispose(JNIEnv* jniEnv, jobject obj, jlong mapPtr) {
        assert(mapPtr > 0);
        auto map = reinterpret_cast<Tangram::Map*>(mapPtr);
        delete map;
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_MapController_nativeLoadScene(JNIEnv* jniEnv, jobject obj, jlong mapPtr, jstring path) {
        assert(mapPtr > 0);
        auto map = reinterpret_cast<Tangram::Map*>(mapPtr);
        const char* cPath = jniEnv->GetStringUTFChars(path, NULL);
        map->loadScene(cPath);
        jniEnv->ReleaseStringUTFChars(path, cPath);
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_MapController_nativeResize(JNIEnv* jniEnv, jobject obj, jlong mapPtr, jint width, jint height) {
        assert(mapPtr > 0);
        auto map = reinterpret_cast<Tangram::Map*>(mapPtr);
        map->resize(width, height);
    }

    JNIEXPORT bool JNICALL Java_com_mapzen_tangram_MapController_nativeUpdate(JNIEnv* jniEnv, jobject obj, jlong mapPtr, jfloat dt) {
        assert(mapPtr > 0);
        auto map = reinterpret_cast<Tangram::Map*>(mapPtr);
        return map->update(dt);
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_MapController_nativeRender(JNIEnv* jniEnv, jobject obj, jlong mapPtr) {
        assert(mapPtr > 0);
        auto map = reinterpret_cast<Tangram::Map*>(mapPtr);
        map->render();
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_MapController_nativeSetupGL(JNIEnv* jniEnv, jobject obj, jlong mapPtr) {
        bindJniEnvToThread(jniEnv);
        assert(mapPtr > 0);
        auto map = reinterpret_cast<Tangram::Map*>(mapPtr);
        map->setupGL();
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_MapController_nativeSetPixelScale(JNIEnv* jniEnv, jobject obj, jlong mapPtr, jfloat scale) {
        assert(mapPtr > 0);
        auto map = reinterpret_cast<Tangram::Map*>(mapPtr);
        map->setPixelScale(scale);
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_MapController_nativeSetCameraType(JNIEnv* jniEnv, jobject obj, jlong mapPtr, jint type) {
        assert(mapPtr > 0);
        auto map = reinterpret_cast<Tangram::Map*>(mapPtr);
        map->setCameraType(type);
    }

    JNIEXPORT jint JNICALL Java_com_mapzen_tangram_MapController_nativeGetCameraType(JNIEnv* jniEnv, jobject obj, jlong mapPtr) {
        assert(mapPtr > 0);
        auto map = reinterpret_cast<Tangram::Map*>(mapPtr);
        return map->getCameraType();
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_MapController_nativeHandleTapGesture(JNIEnv* jniEnv, jobject obj, jlong mapPtr, jfloat posX, jfloat posY) {
        assert(mapPtr > 0);
        auto map = reinterpret_cast<Tangram::Map*>(mapPtr);
        map->handleTapGesture(posX, posY);
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_MapController_nativeHandleDoubleTapGesture(JNIEnv* jniEnv, jobject obj, jlong mapPtr, jfloat posX, jfloat posY) {
        assert(mapPtr > 0);
        auto map = reinterpret_cast<Tangram::Map*>(mapPtr);
        map->handleDoubleTapGesture(posX, posY);
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_MapController_nativeHandlePanGesture(JNIEnv* jniEnv, jobject obj, jlong mapPtr, jfloat startX, jfloat startY, jfloat endX, jfloat endY) {
        assert(mapPtr > 0);
        auto map = reinterpret_cast<Tangram::Map*>(mapPtr);
        map->handlePanGesture(startX, startY, endX, endY);
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_MapController_nativeHandleFlingGesture(JNIEnv* jniEnv, jobject obj, jlong mapPtr, jfloat posX, jfloat posY, jfloat velocityX, jfloat velocityY) {
        assert(mapPtr > 0);
        auto map = reinterpret_cast<Tangram::Map*>(mapPtr);
        map->handleFlingGesture(posX, posY, velocityX, velocityY);
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_MapController_nativeHandlePinchGesture(JNIEnv* jniEnv, jobject obj, jlong mapPtr, jfloat posX, jfloat posY, jfloat scale, jfloat velocity) {
        assert(mapPtr > 0);
        auto map = reinterpret_cast<Tangram::Map*>(mapPtr);
        map->handlePinchGesture(posX, posY, scale, velocity);
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_MapController_nativeHandleRotateGesture(JNIEnv* jniEnv, jobject obj, jlong mapPtr, jfloat posX, jfloat posY, jfloat rotation) {
        assert(mapPtr > 0);
        auto map = reinterpret_cast<Tangram::Map*>(mapPtr);
        map->handleRotateGesture(posX, posY, rotation);
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_MapController_nativeHandleShoveGesture(JNIEnv* jniEnv, jobject obj, jlong mapPtr, jfloat distance) {
        assert(mapPtr > 0);
        auto map = reinterpret_cast<Tangram::Map*>(mapPtr);
        map->handleShoveGesture(distance);
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_MapController_nativeOnUrlSuccess(JNIEnv* jniEnv, jobject obj, jbyteArray fetchedBytes, jlong callbackPtr) {
        onUrlSuccess(jniEnv, fetchedBytes, callbackPtr);
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_MapController_nativeOnUrlFailure(JNIEnv* jniEnv, jobject obj, jlong callbackPtr) {
        onUrlFailure(jniEnv, callbackPtr);
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_MapController_nativePickFeature(JNIEnv* jniEnv, jobject obj, jlong mapPtr, jfloat posX, jfloat posY, jobject listener) {
        assert(mapPtr > 0);
        auto map = reinterpret_cast<Tangram::Map*>(mapPtr);
        auto object = jniEnv->NewGlobalRef(listener);
        map->pickFeaturesAt(posX, posY, [object](const auto& items) {
            featurePickCallback(object, items);
        });
    }

    JNIEXPORT jlong JNICALL Java_com_mapzen_tangram_MapController_nativeAddTileSource(JNIEnv* jniEnv, jobject obj, jlong mapPtr, jstring name) {
        assert(mapPtr > 0);
        auto map = reinterpret_cast<Tangram::Map*>(mapPtr);
        auto sourceName = stringFromJString(jniEnv, name);
        auto source = std::shared_ptr<Tangram::TileSource>(new Tangram::ClientGeoJsonSource(sourceName, ""));
        map->addTileSource(source);
        return reinterpret_cast<jlong>(source.get());
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_MapController_nativeRemoveTileSource(JNIEnv* jniEnv, jobject obj, jlong mapPtr, jlong sourcePtr) {
        assert(mapPtr > 0);
        auto map = reinterpret_cast<Tangram::Map*>(mapPtr);
        assert(sourcePtr > 0);
        auto source = reinterpret_cast<Tangram::TileSource*>(sourcePtr);
        map->removeTileSource(*source);
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_MapController_nativeClearTileSource(JNIEnv* jniEnv, jobject obj, jlong mapPtr, jlong sourcePtr) {
        assert(mapPtr > 0);
        auto map = reinterpret_cast<Tangram::Map*>(mapPtr);
        assert(sourcePtr > 0);
        auto source = reinterpret_cast<Tangram::TileSource*>(sourcePtr);
        map->clearTileSource(*source, true, true);
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_MapController_nativeAddFeature(JNIEnv* jniEnv, jobject obj, jlong mapPtr, jlong sourcePtr,
        jdoubleArray jcoordinates, jintArray jrings, jobjectArray jproperties) {

        assert(mapPtr > 0);
        assert(sourcePtr > 0);
        auto source = reinterpret_cast<Tangram::ClientGeoJsonSource*>(sourcePtr);

        size_t n_points = jniEnv->GetArrayLength(jcoordinates) / 2;
        size_t n_rings = (jrings == NULL) ? 0 : jniEnv->GetArrayLength(jrings);
        size_t n_properties = (jproperties == NULL) ? 0 : jniEnv->GetArrayLength(jproperties) / 2;

        Tangram::Properties properties;

        for (size_t i = 0; i < n_properties; ++i) {
            jstring jkey = (jstring) (jniEnv->GetObjectArrayElement(jproperties, 2 * i));
            jstring jvalue = (jstring) (jniEnv->GetObjectArrayElement(jproperties, 2 * i + 1));
            auto key = stringFromJString(jniEnv, jkey);
            auto value = stringFromJString(jniEnv, jvalue);
            properties.set(key, value);
            jniEnv->DeleteLocalRef(jkey);
            jniEnv->DeleteLocalRef(jvalue);
        }

        auto* coordinates = jniEnv->GetDoubleArrayElements(jcoordinates, NULL);

        if (n_rings > 0) {
            // If rings are defined, this is a polygon feature.
            auto* rings = jniEnv->GetIntArrayElements(jrings, NULL);
            std::vector<std::vector<Tangram::LngLat>> polygon;
            size_t ring_start = 0, ring_end = 0;
            for (size_t i = 0; i < n_rings; ++i) {
                ring_end += rings[i];
                std::vector<Tangram::LngLat> ring;
                for (; ring_start < ring_end; ++ring_start) {
                    ring.push_back({coordinates[2 * ring_start], coordinates[2 * ring_start + 1]});
                }
                polygon.push_back(std::move(ring));
            }
            source->addPoly(properties, polygon);
            jniEnv->ReleaseIntArrayElements(jrings, rings, JNI_ABORT);
        } else if (n_points > 1) {
            // If no rings defined but multiple points, this is a polyline feature.
            std::vector<Tangram::LngLat> polyline;
            for (size_t i = 0; i < n_points; ++i) {
                polyline.push_back({coordinates[2 * i], coordinates[2 * i + 1]});
            }
            source->addLine(properties, polyline);
        } else {
            // This is a point feature.
            auto point = Tangram::LngLat(coordinates[0], coordinates[1]);
            source->addPoint(properties, point);
        }

        jniEnv->ReleaseDoubleArrayElements(jcoordinates, coordinates, JNI_ABORT);

    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_MapController_nativeAddGeoJson(JNIEnv* jniEnv, jobject obj, jlong mapPtr, jlong sourcePtr, jstring geojson) {
        assert(mapPtr > 0);
        assert(sourcePtr > 0);
        auto source = reinterpret_cast<Tangram::ClientGeoJsonSource*>(sourcePtr);
        auto data = stringFromJString(jniEnv, geojson);
        source->addData(data);
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_MapController_nativeSetDebugFlag(JNIEnv* jniEnv, jobject obj, jint flag, jboolean on) {
        Tangram::setDebugFlag(static_cast<Tangram::DebugFlags>(flag), on);
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_MapController_nativeUseCachedGlState(JNIEnv* jniEnv, jobject obj, jlong mapPtr, jboolean use) {
        assert(mapPtr > 0);
        auto map = reinterpret_cast<Tangram::Map*>(mapPtr);
        map->useCachedGlState(use);
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_MapController_nativeCaptureSnapshot(JNIEnv* jniEnv, jobject obj, jlong mapPtr, jintArray buffer) {
        assert(mapPtr > 0);
        auto map = reinterpret_cast<Tangram::Map*>(mapPtr);
        jint* ptr = jniEnv->GetIntArrayElements(buffer, NULL);
        unsigned int* data = reinterpret_cast<unsigned int*>(ptr);
        map->captureSnapshot(data);
        jniEnv->ReleaseIntArrayElements(buffer, ptr, JNI_ABORT);
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_MapController_nativeQueueSceneUpdate(JNIEnv* jnienv, jobject obj, jlong mapPtr, jstring path, jstring value) {
        assert(mapPtr > 0);
        auto map = reinterpret_cast<Tangram::Map*>(mapPtr);
        const char* cPath = jnienv->GetStringUTFChars(path, NULL);
        const char* cValue = jnienv->GetStringUTFChars(value, NULL);
        map->queueSceneUpdate(cPath, cValue);
        jnienv->ReleaseStringUTFChars(path, cPath);
        jnienv->ReleaseStringUTFChars(value, cValue);
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_MapController_nativeApplySceneUpdates(JNIEnv* jnienv, jobject obj, jlong mapPtr) {
        assert(mapPtr > 0);
        auto map = reinterpret_cast<Tangram::Map*>(mapPtr);
        map->applySceneUpdates();
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_MapController_nativeSetMBTiles(JNIEnv* jnienv, jobject obj, jlong mapPtr, jstring dataSourceName, jstring mbtilesFilePath) {
        assert(mapPtr > 0);
        auto map = reinterpret_cast<Tangram::Map*>(mapPtr);
        const char* cDataSourceName = jnienv->GetStringUTFChars(dataSourceName, NULL);
        const char* cMbtilesFilePath = jnienv->GetStringUTFChars(mbtilesFilePath, NULL);
        map->setMBTiles(cDataSourceName, cMbtilesFilePath);
    }

}
