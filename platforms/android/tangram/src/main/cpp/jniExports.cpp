#include "androidPlatform.h"
#include "data/clientGeoJsonSource.h"
#include "map.h"

#include <cassert>

using namespace Tangram;

std::vector<Tangram::SceneUpdate> unpackSceneUpdates(JNIEnv* jniEnv, jobjectArray updateStrings) {
    size_t nUpdateStrings = (updateStrings == NULL)? 0 : jniEnv->GetArrayLength(updateStrings);

    std::vector<Tangram::SceneUpdate> sceneUpdates;
    for (size_t i = 0; i < nUpdateStrings;) {
        jstring path = (jstring) (jniEnv->GetObjectArrayElement(updateStrings, i++));
        jstring value = (jstring) (jniEnv->GetObjectArrayElement(updateStrings, i++));
        sceneUpdates.emplace_back(stringFromJString(jniEnv, path), stringFromJString(jniEnv, value));
        jniEnv->DeleteLocalRef(path);
        jniEnv->DeleteLocalRef(value);
    }
    return sceneUpdates;
}

extern "C" {

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_MapController_nativeSetCameraPosition(JNIEnv* jniEnv, jobject obj, jlong mapPtr, jdouble lon, jdouble lat,
                                                                                         jfloat zoom, jfloat rotation, jfloat tilt, jfloat duration, jint ease) {
        assert(mapPtr > 0);
        auto map = reinterpret_cast<Tangram::Map*>(mapPtr);

        CameraPosition camera;
        camera.longitude = lon;
        camera.latitude = lat;
        camera.zoom = zoom;
        camera.rotation = rotation;
        camera.tilt = tilt;

        if (duration > 0) {
            map->setCameraPositionEased(camera, duration, static_cast<Tangram::EaseType>(ease));
        } else {
            map->setCameraPosition(camera);
        }
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_MapController_nativeGetCameraPosition(JNIEnv* jniEnv, jobject obj, jlong mapPtr, jdoubleArray lonLat, jfloatArray zoomRotationTilt) {
        assert(mapPtr > 0);
        auto map = reinterpret_cast<Tangram::Map*>(mapPtr);
        jdouble* pos = jniEnv->GetDoubleArrayElements(lonLat, NULL);
        jfloat* zrt = jniEnv->GetFloatArrayElements(zoomRotationTilt, NULL);

        auto camera = map->getCameraPosition();
        pos[0] = camera.longitude;
        pos[1] = camera.latitude;
        zrt[0] = camera.zoom;
        zrt[1] = camera.rotation;
        zrt[2] = camera.tilt;

        jniEnv->ReleaseDoubleArrayElements(lonLat, pos, 0);
        jniEnv->ReleaseFloatArrayElements(zoomRotationTilt, zrt, 0);
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_MapController_nativeSetPosition(JNIEnv* jniEnv, jobject obj, jlong mapPtr, jdouble lon, jdouble lat) {
        assert(mapPtr > 0);
        auto map = reinterpret_cast<Tangram::Map*>(mapPtr);
        map->setPosition(lon, lat);
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

    JNIEXPORT jfloat JNICALL Java_com_mapzen_tangram_MapController_nativeGetTilt(JNIEnv* jniEnv, jobject obj, jlong mapPtr) {
        assert(mapPtr > 0);
        auto map = reinterpret_cast<Tangram::Map*>(mapPtr);
        return map->getTilt();
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_MapController_nativeFlyTo(JNIEnv* jniEnv, jobject obj,  jlong mapPtr, jdouble lon, jdouble lat, jfloat zoom, jfloat duration, jfloat speed) {
        assert(mapPtr > 0);
        auto map = reinterpret_cast<Tangram::Map*>(mapPtr);
        map->flyTo(lon, lat, zoom, duration, speed);
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
        AndroidPlatform::setupJniEnv(jniEnv);
        auto platform = std::make_shared<Tangram::AndroidPlatform>(jniEnv, assetManager, tangramInstance);
        auto map = new Tangram::Map(platform);
        map->setSceneReadyListener([platform](Tangram::SceneID id, const Tangram::SceneError* error) {
            platform->sceneReadyCallback(id, error);
        });
        map->setCameraAnimationListener([platform](bool success) {
            platform->cameraAnimationCallback(success);
        });

        return reinterpret_cast<jlong>(map);
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_MapController_nativeDispose(JNIEnv* jniEnv, jobject obj, jlong mapPtr) {
        assert(mapPtr > 0);
        auto map = reinterpret_cast<Tangram::Map*>(mapPtr);
        // Don't dispose MapController ref before map is teared down,
        // delete map or worker threads might call back to it (e.g. requestRender)
        auto platform = map->getPlatform();

        delete map;

        static_cast<Tangram::AndroidPlatform&>(*platform).dispose(jniEnv);
    }

    JNIEXPORT jint JNICALL Java_com_mapzen_tangram_MapController_nativeLoadScene(JNIEnv* jniEnv, jobject obj, jlong mapPtr, jstring path, jobjectArray updateStrings) {
        assert(mapPtr > 0);
        auto map = reinterpret_cast<Tangram::Map*>(mapPtr);
        auto cPath = stringFromJString(jniEnv, path);

        auto sceneUpdates = unpackSceneUpdates(jniEnv, updateStrings);
        Url sceneUrl = Url(cPath).resolved("asset:///");
        jint sceneId = map->loadScene(sceneUrl.string(), false, sceneUpdates);

        return sceneId;
    }

    JNIEXPORT jint JNICALL Java_com_mapzen_tangram_MapController_nativeLoadSceneAsync(JNIEnv* jniEnv, jobject obj, jlong mapPtr, jstring path, jobjectArray updateStrings) {
        assert(mapPtr > 0);
        auto map = reinterpret_cast<Tangram::Map*>(mapPtr);
        auto cPath = stringFromJString(jniEnv, path);

        auto sceneUpdates = unpackSceneUpdates(jniEnv, updateStrings);
        Url sceneUrl = Url(cPath).resolved("asset:///");
        jint sceneId = map->loadSceneAsync(sceneUrl.string(), false, sceneUpdates);

        return sceneId;


    }

    JNIEXPORT jint JNICALL Java_com_mapzen_tangram_MapController_nativeLoadSceneYaml(JNIEnv* jniEnv, jobject obj, jlong mapPtr, jstring yaml, jstring path, jobjectArray updateStrings) {
        assert(mapPtr > 0);
        auto map = reinterpret_cast<Tangram::Map*>(mapPtr);
        auto cYaml = stringFromJString(jniEnv, yaml);
        auto cPath = stringFromJString(jniEnv, path);

        auto sceneUpdates = unpackSceneUpdates(jniEnv, updateStrings);
        Url sceneUrl = Url(cPath).resolved("asset:///");
        jint sceneId = map->loadSceneYaml(cYaml, sceneUrl.string(), false, sceneUpdates);

        return sceneId;
    }

    JNIEXPORT jint JNICALL Java_com_mapzen_tangram_MapController_nativeLoadSceneYamlAsync(JNIEnv* jniEnv, jobject obj, jlong mapPtr, jstring yaml, jstring path, jobjectArray updateStrings) {
        assert(mapPtr > 0);
        auto map = reinterpret_cast<Tangram::Map*>(mapPtr);
        auto cYaml = stringFromJString(jniEnv, yaml);
        auto cPath = stringFromJString(jniEnv, path);

        auto sceneUpdates = unpackSceneUpdates(jniEnv, updateStrings);
        Url sceneUrl = Url(cPath).resolved("asset:///");
        jint sceneId = map->loadSceneYamlAsync(cYaml, sceneUrl.string(), false, sceneUpdates);

        return sceneId;
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
        AndroidPlatform::bindJniEnvToThread(jniEnv);
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

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_MapController_nativeOnUrlComplete(JNIEnv* jniEnv, jobject obj, jlong mapPtr, jlong requestHandle, jbyteArray fetchedBytes, jstring errorString) {
        assert(mapPtr > 0);
        auto map = reinterpret_cast<Tangram::Map*>(mapPtr);
        auto platform = static_cast<AndroidPlatform*>(map->getPlatform().get());
        platform->onUrlComplete(jniEnv, requestHandle, fetchedBytes, errorString);
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_MapController_nativeSetPickRadius(JNIEnv* jniEnv, jobject obj, jlong mapPtr, jfloat radius) {
        assert(mapPtr > 0);
        auto map = reinterpret_cast<Tangram::Map*>(mapPtr);
        map->setPickRadius(radius);
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_MapController_nativePickFeature(JNIEnv* jniEnv, jobject obj, jlong mapPtr, jfloat posX, jfloat posY, jobject listener) {
        assert(mapPtr > 0);
        auto map = reinterpret_cast<Tangram::Map*>(mapPtr);
        auto object = jniEnv->NewGlobalRef(listener);
        map->pickFeatureAt(posX, posY, [object](auto pickResult) {
            Tangram::featurePickCallback(object, pickResult);
        });
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_MapController_nativePickMarker(JNIEnv* jniEnv, jobject obj, jobject tangramInstance, jlong mapPtr, jfloat posX, jfloat posY, jobject listener) {
        assert(mapPtr > 0);
        auto map = reinterpret_cast<Tangram::Map*>(mapPtr);
        auto object = jniEnv->NewGlobalRef(listener);
        auto instance = jniEnv->NewGlobalRef(tangramInstance);
        map->pickMarkerAt(posX, posY, [object, instance](auto pickMarkerResult) {
            Tangram::markerPickCallback(object, instance, pickMarkerResult);
        });
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_MapController_nativePickLabel(JNIEnv* jniEnv, jobject obj, jlong mapPtr, jfloat posX, jfloat posY, jobject listener) {
        assert(mapPtr > 0);
        auto map = reinterpret_cast<Tangram::Map*>(mapPtr);
        auto object = jniEnv->NewGlobalRef(listener);
        map->pickLabelAt(posX, posY, [object](auto pickResult) {
            Tangram::labelPickCallback(object, pickResult);
        });
    }

    // NOTE unsigned int to jlong for precision... else we can do jint return
    JNIEXPORT jlong JNICALL Java_com_mapzen_tangram_MapController_nativeMarkerAdd(JNIEnv* jniEnv, jobject obj, jlong mapPtr) {
        assert(mapPtr > 0);
        auto map = reinterpret_cast<Tangram::Map*>(mapPtr);
        auto markerID = map->markerAdd();
        return static_cast<jlong>(markerID);
    }

    JNIEXPORT bool JNICALL Java_com_mapzen_tangram_MapController_nativeMarkerRemove(JNIEnv* jniEnv, jobject obj, jlong mapPtr, jlong markerID) {
        assert(mapPtr > 0);
        auto map = reinterpret_cast<Tangram::Map*>(mapPtr);
        auto result = map->markerRemove(static_cast<unsigned int>(markerID));
        return result;
    }

    JNIEXPORT bool JNICALL Java_com_mapzen_tangram_MapController_nativeMarkerSetStylingFromString(JNIEnv* jniEnv, jobject obj, jlong mapPtr, jlong markerID, jstring styling) {
        assert(mapPtr > 0);
        auto map = reinterpret_cast<Tangram::Map*>(mapPtr);
        auto styleString = stringFromJString(jniEnv, styling);
        auto result = map->markerSetStylingFromString(static_cast<unsigned int>(markerID), styleString.c_str());
        return result;
    }

    JNIEXPORT bool JNICALL Java_com_mapzen_tangram_MapController_nativeMarkerSetStylingFromPath(JNIEnv* jniEnv, jobject obj, jlong mapPtr, jlong markerID, jstring path) {
        assert(mapPtr > 0);
        auto map = reinterpret_cast<Tangram::Map*>(mapPtr);
        auto pathString = stringFromJString(jniEnv, path);
        auto result = map->markerSetStylingFromPath(static_cast<unsigned int>(markerID), pathString.c_str());
        return result;
    }

    JNIEXPORT bool JNICALL Java_com_mapzen_tangram_MapController_nativeMarkerSetBitmap(JNIEnv* jniEnv, jobject obj, jlong mapPtr, jlong markerID, jint width, jint height, jintArray data) {
        assert(mapPtr > 0);
        auto map = reinterpret_cast<Tangram::Map*>(mapPtr);
        jint* ptr = jniEnv->GetIntArrayElements(data, NULL);
        unsigned int* imgData = reinterpret_cast<unsigned int*>(ptr);
        jniEnv->ReleaseIntArrayElements(data, ptr, JNI_ABORT);
        auto result = map->markerSetBitmap(static_cast<unsigned int>(markerID), width, height, imgData);
        return result;
    }

    JNIEXPORT bool JNICALL Java_com_mapzen_tangram_MapController_nativeMarkerSetPoint(JNIEnv* jniEnv, jobject obj, jlong mapPtr, jlong markerID, jdouble lng, jdouble lat) {
        assert(mapPtr > 0);
        auto map = reinterpret_cast<Tangram::Map*>(mapPtr);
        auto result = map->markerSetPoint(static_cast<unsigned int>(markerID), Tangram::LngLat(lng, lat));
        return result;
    }

    JNIEXPORT bool JNICALL Java_com_mapzen_tangram_MapController_nativeMarkerSetPointEased(JNIEnv* jniEnv, jobject obj, jlong mapPtr, jlong markerID, jdouble lng, jdouble lat, jfloat duration, jint ease) {
        assert(mapPtr > 0);
        auto map = reinterpret_cast<Tangram::Map*>(mapPtr);
        auto result = map->markerSetPointEased(static_cast<unsigned int>(markerID), Tangram::LngLat(lng, lat), duration, static_cast<Tangram::EaseType>(ease));
        return result;
    }

    JNIEXPORT bool JNICALL Java_com_mapzen_tangram_MapController_nativeMarkerSetPolyline(JNIEnv* jniEnv, jobject obj, jlong mapPtr, jlong markerID, jdoubleArray jcoordinates, jint count) {
        assert(mapPtr > 0);
        auto map = reinterpret_cast<Tangram::Map*>(mapPtr);
        if (!jcoordinates || count == 0) { return false; }

        auto* coordinates = jniEnv->GetDoubleArrayElements(jcoordinates, NULL);
        std::vector<Tangram::LngLat> polyline;
        polyline.reserve(count);

        for (size_t i = 0; i < count; ++i) {
            polyline.emplace_back(coordinates[2 * i], coordinates[2 * i + 1]);
        }

        auto result = map->markerSetPolyline(static_cast<unsigned int>(markerID), polyline.data(), count);
        return result;
    }

    JNIEXPORT bool JNICALL Java_com_mapzen_tangram_MapController_nativeMarkerSetPolygon(JNIEnv* jniEnv, jobject obj, jlong mapPtr, jlong markerID, jdoubleArray jcoordinates, jintArray jcounts, jint rings) {
        assert(mapPtr > 0);
        auto map = reinterpret_cast<Tangram::Map*>(mapPtr);
        if (!jcoordinates || !jcounts || rings == 0) { return false; }

        auto* coordinates = jniEnv->GetDoubleArrayElements(jcoordinates, NULL);

        auto* counts = jniEnv->GetIntArrayElements(jcounts, NULL);

        std::vector<Tangram::LngLat> polygonCoords;

        size_t coordsCount = 0;
        for (size_t i = 0; i < rings; i++) {
            size_t ringCount = *(counts+i);
            for (size_t j = 0; j < ringCount; j++) {
                polygonCoords.emplace_back(coordinates[coordsCount + 2 * j], coordinates[coordsCount + 2 * j + 1]);
            }
            coordsCount += ringCount;
        }

        auto result = map->markerSetPolygon(static_cast<unsigned int>(markerID), polygonCoords.data(), counts, rings);
        return result;
    }

    JNIEXPORT bool JNICALL Java_com_mapzen_tangram_MapController_nativeMarkerSetVisible(JNIEnv* jniEnv, jobject obj, jlong mapPtr, jlong markerID, jboolean visible) {
        assert(mapPtr > 0);
        auto map = reinterpret_cast<Tangram::Map*>(mapPtr);
        auto result = map->markerSetVisible(static_cast<unsigned int>(markerID), visible);
        return result;
    }

    JNIEXPORT bool JNICALL Java_com_mapzen_tangram_MapController_nativeMarkerSetDrawOrder(JNIEnv* jniEnv, jobject obj, jlong mapPtr, jlong markerID, jint drawOrder) {
        assert(mapPtr > 0);
        auto map = reinterpret_cast<Tangram::Map*>(mapPtr);
        auto result = map->markerSetDrawOrder(markerID, drawOrder);
        return result;
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_MapController_nativeMarkerRemoveAll(JNIEnv* jniEnv, jobject obj, jlong mapPtr) {
        assert(mapPtr > 0);
        auto map = reinterpret_cast<Tangram::Map*>(mapPtr);
        map->markerRemoveAll();
    }

    JNIEXPORT jlong JNICALL Java_com_mapzen_tangram_MapController_nativeAddTileSource(JNIEnv* jniEnv, jobject obj, jlong mapPtr, jstring name, jboolean generateCentroid) {
        assert(mapPtr > 0);
        auto map = reinterpret_cast<Tangram::Map*>(mapPtr);
        auto sourceName = stringFromJString(jniEnv, name);
        auto source = std::shared_ptr<Tangram::TileSource>(new Tangram::ClientGeoJsonSource(map->getPlatform(), sourceName, "", generateCentroid));
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
        auto map = reinterpret_cast<Tangram::Map*>(mapPtr);
        auto source = reinterpret_cast<Tangram::ClientGeoJsonSource*>(map->getPlatform(), sourcePtr);

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
        auto map = reinterpret_cast<Tangram::Map*>(mapPtr);
        auto source = reinterpret_cast<Tangram::ClientGeoJsonSource*>(map->getPlatform(), sourcePtr);
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

    JNIEXPORT jint JNICALL Java_com_mapzen_tangram_MapController_nativeUpdateScene(JNIEnv* jniEnv, jobject obj, jlong mapPtr, jobjectArray updateStrings) {
        assert(mapPtr > 0);

        auto sceneUpdates = unpackSceneUpdates(jniEnv, updateStrings);
        auto map = reinterpret_cast<Tangram::Map*>(mapPtr);

        return map->updateSceneAsync(sceneUpdates);
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_MapController_nativeOnLowMemory(JNIEnv* jnienv, jobject obj, jlong mapPtr) {
        assert(mapPtr > 0);
        auto map = reinterpret_cast<Tangram::Map*>(mapPtr);
        map->onMemoryWarning();
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_MapController_nativeSetDefaultBackgroundColor(JNIEnv* jnienv, jobject obj, jlong mapPtr, jfloat r, jfloat g, jfloat b) {
        assert(mapPtr > 0);
        auto map = reinterpret_cast<Tangram::Map*>(mapPtr);
        map->setDefaultBackgroundColor(r, g, b);
    }
}
