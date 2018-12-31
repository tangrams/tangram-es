#include "androidPlatform.h"
#include "data/clientGeoJsonSource.h"
#include "map.h"

#include <cassert>
#include <android/bitmap.h>

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

JNIEXPORT jint JNI_OnLoad(JavaVM* vm, void* reserved) {
    return AndroidPlatform::jniOnLoad(vm);
}

JNIEXPORT void JNI_OnUnload(JavaVM* vm, void* reserved) {
    AndroidPlatform::jniOnUnload(vm);
}


#define FUNC(CLASS, NAME) JNIEXPORT JNICALL Java_com_mapzen_tangram_ ## CLASS ## _native ## NAME

#define auto_map(ptr) assert(ptr); auto map = reinterpret_cast<Tangram::AndroidMap*>(mapPtr)
#define auto_source(ptr) assert(ptr); auto source = reinterpret_cast<Tangram::ClientGeoJsonSource*>(ptr)


#define MapRenderer(NAME) FUNC(MapRenderer, NAME)

jint MapRenderer(Update)(JNIEnv* jniEnv, jobject obj, jlong mapPtr, jfloat dt) {
    auto_map(mapPtr);
    auto result = map->update(dt);
    return static_cast<jint>(result.flags);
}

void MapRenderer(Render)(JNIEnv* jniEnv, jobject obj, jlong mapPtr) {
    auto_map(mapPtr);
    map->render();
}

void MapRenderer(SetupGL)(JNIEnv* jniEnv, jobject obj, jlong mapPtr) {
    AndroidPlatform::bindJniEnvToThread(jniEnv);
    auto_map(mapPtr);
    map->setupGL();
}

void MapRenderer(Resize)(JNIEnv* jniEnv, jobject obj, jlong mapPtr, jint width, jint height) {
    auto_map(mapPtr);
    map->resize(width, height);
}

void MapRenderer(CaptureSnapshot)(JNIEnv* jniEnv, jobject obj, jlong mapPtr, jintArray buffer) {
    auto_map(mapPtr);
    jint* ptr = jniEnv->GetIntArrayElements(buffer, NULL);
    unsigned int* data = reinterpret_cast<unsigned int*>(ptr);
    map->captureSnapshot(data);
    jniEnv->ReleaseIntArrayElements(buffer, ptr, JNI_ABORT);
}


#define MapController(NAME) FUNC(MapController, NAME)

void MapController(GetCameraPosition)(JNIEnv* jniEnv, jobject obj, jlong mapPtr, jdoubleArray lonLat,
                                      jfloatArray zoomRotationTilt) {
    auto_map(mapPtr);
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

void MapController(UpdateCameraPosition)(JNIEnv* jniEnv, jobject obj, jlong mapPtr,
                                         jint set, jdouble lon, jdouble lat,
                                         jfloat zoom, jfloat zoomBy,
                                         jfloat rotation, jfloat rotateBy,
                                         jfloat tilt, jfloat tiltBy,
                                         jdouble b1lon, jdouble b1lat,
                                         jdouble b2lon, jdouble b2lat,
                                         jintArray jpad, jfloat duration, jint ease) {
    auto_map(mapPtr);

    CameraUpdate update;
    update.set = set;

    update.lngLat = LngLat{lon,lat};
    update.zoom = zoom;
    update.zoomBy = zoomBy;
    update.rotation = rotation;
    update.rotationBy = rotateBy;
    update.tilt = tilt;
    update.tiltBy = tiltBy;
    update.bounds = std::array<LngLat,2>{{LngLat{b1lon, b1lat}, LngLat{b2lon, b2lat}}};
    if (jpad != NULL) {
        jint* jpadArray = jniEnv->GetIntArrayElements(jpad, NULL);
        update.padding = EdgePadding{jpadArray[0], jpadArray[1], jpadArray[2], jpadArray[3]};
        jniEnv->ReleaseIntArrayElements(jpad, jpadArray, JNI_ABORT);
    }
    map->updateCameraPosition(update, duration, static_cast<Tangram::EaseType>(ease));
}

void MapController(GetEnclosingCameraPosition)(JNIEnv* jniEnv, jobject obj, jlong mapPtr,
                                               jdouble aLng, jdouble aLat,
                                               jdouble bLng, jdouble bLat,
                                               jintArray jpad, jdoubleArray lngLatZoom) {
    auto_map(mapPtr);

    EdgePadding padding;
    if (jpad != NULL) {
        jint* jpadArray = jniEnv->GetIntArrayElements(jpad, NULL);
        padding = EdgePadding(jpadArray[0], jpadArray[1], jpadArray[2], jpadArray[3]);
        jniEnv->ReleaseIntArrayElements(jpad, jpadArray, JNI_ABORT);
    }
    CameraPosition camera = map->getEnclosingCameraPosition(LngLat{aLng,aLat}, LngLat{bLng,bLat}, padding);
    jdouble* arr = jniEnv->GetDoubleArrayElements(lngLatZoom, NULL);
    arr[0] = camera.longitude;
    arr[1] = camera.latitude;
    arr[2] = camera.zoom;
    jniEnv->ReleaseDoubleArrayElements(lngLatZoom, arr, 0);
}

void MapController(FlyTo)(JNIEnv* jniEnv, jobject obj,  jlong mapPtr, jdouble lon, jdouble lat,
                          jfloat zoom, jfloat duration, jfloat speed) {
    auto_map(mapPtr);

    CameraPosition camera = map->getCameraPosition();
    camera.longitude = lon;
    camera.latitude = lat;
    camera.zoom = zoom;
    map->flyTo(camera, duration, speed);
}

void MapController(CancelCameraAnimation)(JNIEnv* jniEnv, jobject obj,  jlong mapPtr) {
    auto_map(mapPtr);
    map->cancelCameraAnimation();
}

jboolean MapController(ScreenPositionToLngLat)(JNIEnv* jniEnv, jobject obj, jlong mapPtr,
                                               jdoubleArray coordinates) {
    auto_map(mapPtr);

    jdouble* arr = jniEnv->GetDoubleArrayElements(coordinates, NULL);
    bool result = map->screenPositionToLngLat(arr[0], arr[1], &arr[0], &arr[1]);
    jniEnv->ReleaseDoubleArrayElements(coordinates, arr, 0);
    return static_cast<jboolean>(result);
}

jboolean MapController(LngLatToScreenPosition)(JNIEnv* jniEnv, jobject obj, jlong mapPtr,
                                               jdoubleArray coordinates) {
    auto_map(mapPtr);

    jdouble* arr = jniEnv->GetDoubleArrayElements(coordinates, NULL);
    bool result = map->lngLatToScreenPosition(arr[0], arr[1], &arr[0], &arr[1]);
    jniEnv->ReleaseDoubleArrayElements(coordinates, arr, 0);
    return static_cast<jboolean>(result);
}

jlong MapController(Init)(JNIEnv* jniEnv, jobject tangramInstance, jobject assetManager) {
    auto map  = new Tangram::AndroidMap(jniEnv, assetManager, tangramInstance);
    return reinterpret_cast<jlong>(map);
}

void MapController(Dispose)(JNIEnv* jniEnv, jobject tangramInstance, jlong mapPtr) {
    auto_map(mapPtr);
    delete map;
}

void MapController(Shutdown)(JNIEnv* jniEnv, jobject tangramInstance, jlong mapPtr) {
    auto_map(mapPtr);
    map->getPlatform().shutdown();
}

jint MapController(LoadScene)(JNIEnv* jniEnv, jobject obj, jlong mapPtr, jstring path,
                              jobjectArray updateStrings) {
    auto_map(mapPtr);

    auto cPath = stringFromJString(jniEnv, path);

    auto sceneUpdates = unpackSceneUpdates(jniEnv, updateStrings);
    Url sceneUrl = Url(cPath).resolved("asset:///");
    jint sceneId = map->loadScene(sceneUrl.string(), false, sceneUpdates);

    return sceneId;
}

jint MapController(LoadSceneAsync)(JNIEnv* jniEnv, jobject obj, jlong mapPtr, jstring path,
                                   jobjectArray updateStrings) {
    auto_map(mapPtr);

    auto cPath = stringFromJString(jniEnv, path);

    auto sceneUpdates = unpackSceneUpdates(jniEnv, updateStrings);
    Url sceneUrl = Url(cPath).resolved("asset:///");
    jint sceneId = map->loadSceneAsync(sceneUrl.string(), false, sceneUpdates);

    return sceneId;


}

jint MapController(LoadSceneYaml)(JNIEnv* jniEnv, jobject obj, jlong mapPtr, jstring yaml, jstring path,
                                  jobjectArray updateStrings) {
    auto_map(mapPtr);

    auto cYaml = stringFromJString(jniEnv, yaml);
    auto cPath = stringFromJString(jniEnv, path);

    auto sceneUpdates = unpackSceneUpdates(jniEnv, updateStrings);
    Url sceneUrl = Url(cPath).resolved("asset:///");
    jint sceneId = map->loadSceneYaml(cYaml, sceneUrl.string(), false, sceneUpdates);

    return sceneId;
}

jint MapController(LoadSceneYamlAsync)(JNIEnv* jniEnv, jobject obj, jlong mapPtr, jstring yaml, jstring path,
                                       jobjectArray updateStrings) {
    auto_map(mapPtr);

    auto cYaml = stringFromJString(jniEnv, yaml);
    auto cPath = stringFromJString(jniEnv, path);

    auto sceneUpdates = unpackSceneUpdates(jniEnv, updateStrings);
    Url sceneUrl = Url(cPath).resolved("asset:///");
    jint sceneId = map->loadSceneYamlAsync(cYaml, sceneUrl.string(), false, sceneUpdates);

    return sceneId;
}

void MapController(SetPixelScale)(JNIEnv* jniEnv, jobject obj, jlong mapPtr, jfloat scale) {
    auto_map(mapPtr);
    map->setPixelScale(scale);
}

void MapController(SetCameraType)(JNIEnv* jniEnv, jobject obj, jlong mapPtr, jint type) {
    auto_map(mapPtr);
    map->setCameraType(type);
}

jint MapController(GetCameraType)(JNIEnv* jniEnv, jobject obj, jlong mapPtr) {
    auto_map(mapPtr);
    return map->getCameraType();
}

jfloat MapController(GetMinZoom)(JNIEnv* jniEnv, jobject obj, jlong mapPtr) {
    auto_map(mapPtr);
    return map->getMinZoom();
}

void MapController(SetMinZoom)(JNIEnv* jniEnv, jobject obj, jlong mapPtr, jfloat minZoom) {
    auto_map(mapPtr);
    map->setMinZoom(minZoom);
}

jfloat MapController(GetMaxZoom)(JNIEnv* jniEnv, jobject obj, jlong mapPtr) {
    auto_map(mapPtr);
    return map->getMaxZoom();
}

void MapController(SetMaxZoom)(JNIEnv* jniEnv, jobject obj, jlong mapPtr, jfloat maxZoom) {
    auto_map(mapPtr);
    map->setMaxZoom(maxZoom);
}

void MapController(HandleTapGesture)(JNIEnv* jniEnv, jobject obj, jlong mapPtr, jfloat posX, jfloat posY) {
    auto_map(mapPtr);
    map->handleTapGesture(posX, posY);
}

void MapController(HandleDoubleTapGesture)(JNIEnv* jniEnv, jobject obj, jlong mapPtr, jfloat posX, jfloat posY) {
    auto_map(mapPtr);
    map->handleDoubleTapGesture(posX, posY);
}

void MapController(HandlePanGesture)(JNIEnv* jniEnv, jobject obj, jlong mapPtr, jfloat startX, jfloat startY,
                                     jfloat endX, jfloat endY) {
    auto_map(mapPtr);
    map->handlePanGesture(startX, startY, endX, endY);
}

void MapController(HandleFlingGesture)(JNIEnv* jniEnv, jobject obj, jlong mapPtr, jfloat posX, jfloat posY,
                                       jfloat velocityX, jfloat velocityY) {
    auto_map(mapPtr);
    map->handleFlingGesture(posX, posY, velocityX, velocityY);
}

void MapController(HandlePinchGesture)(JNIEnv* jniEnv, jobject obj, jlong mapPtr, jfloat posX, jfloat posY,
                                       jfloat scale, jfloat velocity) {
    auto_map(mapPtr);
    map->handlePinchGesture(posX, posY, scale, velocity);
}

void MapController(HandleRotateGesture)(JNIEnv* jniEnv, jobject obj, jlong mapPtr, jfloat posX, jfloat posY,
                                        jfloat rotation) {
    auto_map(mapPtr);
    map->handleRotateGesture(posX, posY, rotation);
}

void MapController(HandleShoveGesture)(JNIEnv* jniEnv, jobject obj, jlong mapPtr, jfloat distance) {
    auto_map(mapPtr);
    map->handleShoveGesture(distance);
}

void MapController(OnUrlComplete)(JNIEnv* jniEnv, jobject obj, jlong mapPtr, jlong requestHandle,
                                  jbyteArray fetchedBytes, jstring errorString) {
    auto_map(mapPtr);

    auto& platform = static_cast<AndroidPlatform&>(map->getPlatform());
    platform.onUrlComplete(jniEnv, requestHandle, fetchedBytes, errorString);
}

void MapController(SetPickRadius)(JNIEnv* jniEnv, jobject obj, jlong mapPtr, jfloat radius) {
    auto_map(mapPtr);
    map->setPickRadius(radius);
}

void MapController(PickFeature)(JNIEnv* jniEnv, jobject obj, jlong mapPtr, jfloat posX, jfloat posY) {
    auto_map(mapPtr);
    map->pickFeature(posX, posY);

}

void MapController(PickMarker)(JNIEnv* jniEnv, jobject obj, jlong mapPtr, jfloat posX, jfloat posY) {
    auto_map(mapPtr);
    map->pickMarker(posX, posY);
}

void MapController(PickLabel)(JNIEnv* jniEnv, jobject obj, jlong mapPtr, jfloat posX, jfloat posY) {
    auto_map(mapPtr);
    map->pickLabel(posX, posY);
}

// NOTE unsigned int to jlong for precision... else we can do jint return
jlong MapController(MarkerAdd)(JNIEnv* jniEnv, jobject obj, jlong mapPtr) {
    auto_map(mapPtr);
    auto markerID = map->markerAdd();
    return static_cast<jlong>(markerID);
}

jboolean MapController(MarkerRemove)(JNIEnv* jniEnv, jobject obj, jlong mapPtr, jlong markerID) {
    auto_map(mapPtr);
    auto result = map->markerRemove(static_cast<unsigned int>(markerID));
    return static_cast<jboolean>(result);
}

jboolean MapController(MarkerSetStylingFromString)(JNIEnv* jniEnv, jobject obj, jlong mapPtr, jlong markerID,
                                                   jstring styling) {
    auto_map(mapPtr);

    auto styleString = stringFromJString(jniEnv, styling);
    auto result = map->markerSetStylingFromString(static_cast<unsigned int>(markerID), styleString.c_str());
    return static_cast<jboolean>(result);
}

jboolean MapController(MarkerSetStylingFromPath)(JNIEnv* jniEnv, jobject obj, jlong mapPtr, jlong markerID,
                                                 jstring path) {
    auto_map(mapPtr);

    auto pathString = stringFromJString(jniEnv, path);
    auto result = map->markerSetStylingFromPath(static_cast<unsigned int>(markerID), pathString.c_str());
    return static_cast<jboolean>(result);
}

jboolean MapController(MarkerSetBitmap)(JNIEnv* jniEnv, jobject obj, jlong mapPtr, jlong markerID,
                                        jobject jbitmap, jfloat density) {
    auto_map(mapPtr);

    AndroidBitmapInfo bitmapInfo;
    if (AndroidBitmap_getInfo(jniEnv, jbitmap, &bitmapInfo) != ANDROID_BITMAP_RESULT_SUCCESS) {
        return static_cast<jboolean>(false);
    }
    if (bitmapInfo.format != ANDROID_BITMAP_FORMAT_RGBA_8888) {
        // TODO: Add different conversion functions for other formats.
        return static_cast<jboolean>(false);
    }
    uint32_t* pixelInput;
    if (AndroidBitmap_lockPixels(jniEnv, jbitmap, (void**)&pixelInput) != ANDROID_BITMAP_RESULT_SUCCESS) {
        return static_cast<jboolean>(false);
    }
    int width = bitmapInfo.width;
    int height = bitmapInfo.height;
    uint32_t* pixelOutput = new uint32_t[height * width];
    int i = 0;
    for (int row = 0; row < height; row++) {
        // Flips image upside-down
        int flippedRow = (height - 1 - row) * width;
        for (int col = 0; col < width; col++) {
            uint32_t pixel = pixelInput[i++];
            // Undo alpha pre-multiplication.
            auto rgba = reinterpret_cast<uint8_t*>(&pixel);
            int a = rgba[3];
            if (a != 0) {
                auto alphaInv = 255.f/a;
                rgba[0] = static_cast<uint8_t>(rgba[0] * alphaInv);
                rgba[1] = static_cast<uint8_t>(rgba[1] * alphaInv);
                rgba[2] = static_cast<uint8_t>(rgba[2] * alphaInv);
            }
            pixelOutput[flippedRow + col] = pixel;
        }
    }
    AndroidBitmap_unlockPixels(jniEnv, jbitmap);
    auto result = map->markerSetBitmap(static_cast<unsigned int>(markerID), width, height, pixelOutput, density);
    delete[] pixelOutput;
    return static_cast<jboolean>(result);
}

jboolean MapController(MarkerSetPoint)(JNIEnv* jniEnv, jobject obj, jlong mapPtr, jlong markerID,
                                       jdouble lng, jdouble lat) {
    auto_map(mapPtr);

    auto result = map->markerSetPoint(static_cast<unsigned int>(markerID), Tangram::LngLat(lng, lat));
    return static_cast<jboolean>(result);
}

jboolean MapController(MarkerSetPointEased)(JNIEnv* jniEnv, jobject obj, jlong mapPtr, jlong markerID,
                                            jdouble lng, jdouble lat, jfloat duration, jint ease) {
    auto_map(mapPtr);

    auto result = map->markerSetPointEased(static_cast<unsigned int>(markerID),
                                           Tangram::LngLat(lng, lat), duration,
                                           static_cast<Tangram::EaseType>(ease));
    return static_cast<jboolean>(result);
}

jboolean MapController(MarkerSetPolyline)(JNIEnv* jniEnv, jobject obj, jlong mapPtr, jlong markerID,
                                          jdoubleArray jcoordinates, jint count) {
    auto_map(mapPtr);

    if (!jcoordinates || count == 0) { return false; }

    auto* coordinates = jniEnv->GetDoubleArrayElements(jcoordinates, NULL);
    std::vector<Tangram::LngLat> polyline;
    polyline.reserve(count);

    for (size_t i = 0; i < count; ++i) {
        polyline.emplace_back(coordinates[2 * i], coordinates[2 * i + 1]);
    }

    auto result = map->markerSetPolyline(static_cast<unsigned int>(markerID), polyline.data(), count);
    return static_cast<jboolean>(result);
}

jboolean MapController(MarkerSetPolygon)(JNIEnv* jniEnv, jobject obj, jlong mapPtr, jlong markerID,
                                         jdoubleArray jcoordinates, jintArray jcounts, jint rings) {
    auto_map(mapPtr);

    if (!jcoordinates || !jcounts || rings == 0) { return false; }

    auto* coordinates = jniEnv->GetDoubleArrayElements(jcoordinates, NULL);
    auto* counts = jniEnv->GetIntArrayElements(jcounts, NULL);

    std::vector<Tangram::LngLat> polygonCoords;

    size_t coordsCount = 0;
    for (size_t i = 0; i < rings; i++) {
        size_t ringCount = *(counts+i);
        for (size_t j = 0; j < ringCount; j++) {
            polygonCoords.emplace_back(coordinates[coordsCount + 2 * j],
                                       coordinates[coordsCount + 2 * j + 1]);
        }
        coordsCount += ringCount;
    }

    auto result = map->markerSetPolygon(static_cast<unsigned int>(markerID),
                                        polygonCoords.data(), counts, rings);

    return static_cast<jboolean>(result);
}

jboolean MapController(MarkerSetVisible)(JNIEnv* jniEnv, jobject obj, jlong mapPtr, jlong markerID,
                                         jboolean visible) {
    auto_map(mapPtr);

    auto result = map->markerSetVisible(static_cast<unsigned int>(markerID), visible);
    return static_cast<jboolean>(result);
}

jboolean MapController(MarkerSetDrawOrder)(JNIEnv* jniEnv, jobject obj, jlong mapPtr, jlong markerID,
                                           jint drawOrder) {
    auto_map(mapPtr);

    auto result = map->markerSetDrawOrder(markerID, drawOrder);
    return static_cast<jboolean>(result);
}

void MapController(MarkerRemoveAll)(JNIEnv* jniEnv, jobject obj, jlong mapPtr) {
    auto_map(mapPtr);
    map->markerRemoveAll();
}


void MapController(SetDebugFlag)(JNIEnv* jniEnv, jobject obj, jint flag, jboolean on) {
    Tangram::setDebugFlag(static_cast<Tangram::DebugFlags>(flag), on);
}

void MapController(UseCachedGlState)(JNIEnv* jniEnv, jobject obj, jlong mapPtr, jboolean use) {
    auto_map(mapPtr);
    map->useCachedGlState(use);
}

void MapController(OnLowMemory)(JNIEnv* jnienv, jobject obj, jlong mapPtr) {
    auto_map(mapPtr);
    map->onMemoryWarning();
}

void MapController(SetDefaultBackgroundColor)(JNIEnv* jnienv, jobject obj, jlong mapPtr,
                                              jfloat r, jfloat g, jfloat b) {
    auto_map(mapPtr);
    map->setDefaultBackgroundColor(r, g, b);
}

jlong MapController(AddTileSource)(JNIEnv* jniEnv, jobject obj, jlong mapPtr,
                                   jstring name, jboolean generateCentroid) {
    auto_map(mapPtr);

    auto sourceName = stringFromJString(jniEnv, name);
    auto source = std::make_shared<Tangram::ClientGeoJsonSource>(map->getPlatform(),
                                                                 sourceName, "",
                                                                 generateCentroid);
    map->addTileSource(source);
    return reinterpret_cast<jlong>(source.get());
}

void MapController(RemoveTileSource)(JNIEnv* jniEnv, jobject obj, jlong mapPtr, jlong sourcePtr) {
    auto_map(mapPtr);
    auto_source(sourcePtr);
    map->removeTileSource(*source);
}

void MapController(ClearTileSource)(JNIEnv* jniEnv, jobject obj, jlong mapPtr, jlong sourcePtr) {
    auto_map(mapPtr);
    auto_source(sourcePtr);
    map->clearTileSource(*source, true, true);
}


#define MapData(NAME) FUNC(MapData, NAME)

void MapData(AddFeature)(JNIEnv* jniEnv, jobject obj, jlong sourcePtr, jdoubleArray jcoordinates,
                         jintArray jrings, jobjectArray jproperties) {

    auto_source(sourcePtr);

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

void MapData(AddGeoJson)(JNIEnv* jniEnv, jobject obj, jlong sourcePtr, jstring geojson) {
    auto_source(sourcePtr);

    auto data = stringFromJString(jniEnv, geojson);
    source->addData(data);
}

} // extern "C"
